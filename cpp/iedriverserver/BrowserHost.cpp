// Licensed to the Software Freedom Conservancy (SFC) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership. The SFC licenses this file
// to you under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "BrowserHost.h"

#include <algorithm>
#include <mutex>

#include <atlsafe.h>
#include <atlwin.h>
#include <PathCch.h>
#include <MsHTML.h>
#include <ShlGuid.h>

#include "../utils/RegistryUtilities.h"
#include "../utils/StringUtilities.h"
#include "../utils/WindowUtilities.h"
#include "../webdriver-server/command.h"
#include "../webdriver-server/response.h"

#include "BrowserFactory.h"
#include "BrowserInfo.h"
#include "cominterfaces.h"

#define HTML_GETOBJECT_MSG L"WM_HTML_GETOBJECT"
#define OLEACC_LIBRARY_NAME L"OLEACC.DLL"
#define THREAD_WAIT_TIMEOUT 30000
#define IDM_STARTDIAGNOSTICSMODE 3802 
#define FAIL_IF_NOT_S_OK(hr) ATLENSURE_RETURN_HR(hr == S_OK, SUCCEEDED(hr) ? E_FAIL : hr)

namespace webdriver {

struct BrowserHostThreadContext {
  DWORD browser_process_id;
  int attach_timeout;
  HWND instance_manager_handle;
  HWND session_settings_handle;
  HANDLE sync_event;
  std::string browser_host_id;
};

struct PostSelfMessageThreadContext {
  HWND window_handle;
  UINT msg;
};

BrowserHost::BrowserHost() : top_level_window_handle_(NULL),
                             content_window_handle_(NULL),
                             tab_window_handle_(NULL),
                             notify_window_handle_(NULL),
                             settings_window_handle_(NULL),
                             is_command_aborted_(false),
                             is_explicit_close_requested_(false),
                             is_ignoring_protected_mode_(false),
                             command_(""),
                             response_(""),
                             id_(StringUtilities::CreateGuid()) {
  HWND window_handle = this->Create(HWND_MESSAGE);
  BOOL allow = ::ChangeWindowMessageFilterEx(window_handle,
                                             WM_COPYDATA,
                                             MSGFLT_ALLOW,
                                             NULL);
}

bool BrowserHost::Initialize(const DWORD process_id,
                             const HWND notify_window_handle,
                             const HWND settings_window_handle) {
  HWND document_handle = NULL;
  bool is_initialized = this->IsBrowserProcessInitialized(process_id,
                                                          &document_handle);
  if (!is_initialized) {
    return false;
  }

  std::string library_path = "";
  this->GetInProcessDriverLibraryPath(process_id,
                                      &library_path);

  CComPtr<IHTMLDocument2> document;
  HRESULT hr = S_OK;
  hr = this->GetDocumentFromWindowHandle(document_handle, &document);
  this->engine_path_ = library_path;
  hr = this->StartDiagnosticsMode(document);
  if (FAILED(hr)) {
    return false;
  }

  hr = this->GetBrowserFromDocument(document);
  if (hr != S_OK) {
    return false;
  }

  this->content_window_handle_ = document_handle;

  CComPtr<IServiceProvider> provider;
  hr = this->browser_->QueryInterface<IServiceProvider>(&provider);
  if (SUCCEEDED(hr) && provider) {
    CComPtr<IOleWindow> window;
    hr = provider->QueryService<IOleWindow>(SID_SShellBrowser, &window);
    if (SUCCEEDED(hr) && window) {
      hr = window->GetWindow(&this->tab_window_handle_);
    }
  }

  hr = this->browser_->get_HWND(
      reinterpret_cast<SHANDLE_PTR*>(&this->top_level_window_handle_));
  this->notify_window_handle_ = notify_window_handle;
  this->settings_window_handle_ = settings_window_handle;
  return true;
}

void BrowserHost::Dispose() {
  if (this->browser_ != NULL) {
    this->browser_.Release();
  }

  // This will block if an alert is present when called. After the in-proc
  // executor window is successfully destroyed, we notify the session that
  // it can remove the instance info from the list of known instances.
  ::SendMessage(this->in_proc_executor_handle_, WM_DESTROY, NULL, NULL);
  if (this->notify_window_handle_ != NULL) {
    size_t buffer_size = this->id_.size() + 1;
    char* id_buffer = new char[buffer_size];
    strcpy_s(id_buffer, buffer_size, this->id_.c_str());
    id_buffer[buffer_size - 1] = '\0';
    ::PostMessage(this->notify_window_handle_,
                  WD_NOTIFY_INSTANCE_CLOSE,
                  buffer_size,
                  reinterpret_cast<LPARAM>(id_buffer));
  }
  this->DestroyWindow();
  ::PostQuitMessage(0);
}

LRESULT BrowserHost::OnCopyData(UINT nMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& bHandled) {
  COPYDATASTRUCT* data = reinterpret_cast<COPYDATASTRUCT*>(lParam);
  std::vector<char> buffer(data->cbData);
  memcpy_s(&buffer[0], data->cbData, data->lpData, data->cbData);
  if (data->dwData == COPYDATA_RESPONSE) {
    // ASSUMPTION: Sent string data is null-terminated.
    std::string received_data(&buffer[0]);
    this->response_ = received_data;
    return 0;
  }

  if (data->dwData == COPYDATA_NEW_WINDOW_PROCESS_ID_LIST ||
      data->dwData == COPYDATA_SAME_WINDOW_PROCESS_ID_LIST) {
    this->known_process_ids_.clear();
    size_t id_count = buffer.size() / sizeof(DWORD);
    DWORD* process_id_pointer = reinterpret_cast<DWORD*>(buffer.data());
    for (unsigned int i = 0; i < id_count; ++i) {
      this->known_process_ids_.push_back(*process_id_pointer);
      ++process_id_pointer;
    }
    if (data->dwData == COPYDATA_SAME_WINDOW_PROCESS_ID_LIST) {
      this->PostMessageToSelf(WD_REACQUIRE_BROWSER);
    } else {
      this->PostMessageToSelf(WD_BROWSER_NEW_WINDOW);
    }
  }
  return 0;
}

LRESULT BrowserHost::OnQuit(UINT nMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled) {
  this->is_explicit_close_requested_ = true;
  this->Dispose();
  return 0;
}

LRESULT BrowserHost::OnSetCommand(UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  LRESULT set_command_result = 0;

  std::string serialized_command(*(reinterpret_cast<std::string*>(lParam)));
  this->command_ = serialized_command;
  this->is_command_aborted_ = false;

  return set_command_result;
}

LRESULT BrowserHost::OnExecCommand(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& bHandled) {
  std::vector<char> command_buffer(this->command_.size() + 1);
  strcpy_s(&command_buffer[0], command_buffer.size(), this->command_.c_str());
  command_buffer[command_buffer.size() - 1] = '\0';

  COPYDATASTRUCT copy_data;
  copy_data.lpData = reinterpret_cast<void*>(&command_buffer[0]);
  copy_data.cbData = static_cast<DWORD>(command_buffer.size());
  ::SendMessage(this->in_proc_executor_handle_,
                WM_COPYDATA,
                NULL,
                reinterpret_cast<LPARAM>(&copy_data));

  ::PostMessage(this->in_proc_executor_handle_,
                WD_EXEC_COMMAND,
                NULL,
                NULL);

  LRESULT response_size = ::SendMessage(this->in_proc_executor_handle_,
                                        WD_GET_RESPONSE_LENGTH,
                                        NULL,
                                        NULL);
  while (response_size == 0 && !this->is_command_aborted_) {
    ::Sleep(10);
    response_size = ::SendMessage(this->in_proc_executor_handle_,
                                  WD_GET_RESPONSE_LENGTH,
                                  NULL,
                                  NULL);
  }

  if (this->is_command_aborted_) {
    Response aborted_response;
    aborted_response.SetSuccessResponse(Json::Value::null);
    this->response_ = aborted_response.Serialize();
    return 0;
  }

  ::PostMessage(this->in_proc_executor_handle_,
                WD_GET_RESPONSE,
                reinterpret_cast<WPARAM>(this->m_hWnd),
                NULL);
  return 0;
}

LRESULT BrowserHost::OnGetResponseLength(UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         BOOL& bHandled) {
  // Not logging trace entering IECommandExecutor::OnGetResponseLength,
  // because it is polled repeatedly for a non-zero return value.
  size_t response_length = 0;
  response_length = this->response_.size();
  return response_length;
}

LRESULT BrowserHost::OnGetResponse(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& bHandled) {
  LPSTR str = reinterpret_cast<LPSTR>(lParam);
  strcpy_s(str,
           this->response_.size() + 1,
           this->response_.c_str());

  // Reset the serialized response for the next command.
  this->response_ = "";
  this->command_ = "";
  return 0;
}

LRESULT BrowserHost::OnAbortCommand(UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& bHandled) {
  this->is_command_aborted_ = true;
  return 0;
}

LRESULT BrowserHost::OnIsBrowserReady(UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam,
                                      BOOL& bHandled) {
  return 0;
}

LRESULT BrowserHost::OnReacquireBrowser(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        BOOL& bHandled) {
  if (!this->is_explicit_close_requested_) {
    if (!this->is_ignoring_protected_mode_) {
      return 0;
    }

    // TODO: implement reacquire timeout

    std::vector<DWORD> new_process_ids;
    this->GetNewBrowserProcessIds(&new_process_ids);
    if (new_process_ids.size() == 0) {
      // If no new process IDs were found yet, repost the message
      this->PostMessageToSelf(WD_REACQUIRE_BROWSER);
      return 0;
    }

    if (new_process_ids.size() > 1) {
      //LOG(WARN) << "Found more than one new iexplore.exe process. It is "
      //  << "impossible to know which is the proper one. Choosing one "
      //  << "at random.";
    }

    HWND document_handle;
    DWORD new_process_id = new_process_ids[0];
    if (!this->IsBrowserProcessInitialized(new_process_id, &document_handle)) {
      // If the browser for the new process ID is not yet ready,
      // repost the message
      this->PostMessageToSelf(WD_REACQUIRE_BROWSER);
      return 0;
    }

    this->browser_.Release();
    this->Initialize(new_process_id,
                     this->notify_window_handle_,
                     this->settings_window_handle_);
  }
  return 0;
}

LRESULT BrowserHost::OnBrowserNewWindow(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        BOOL& bHandled) {
  std::vector<DWORD> new_process_ids;
  this->GetNewBrowserProcessIds(&new_process_ids);
  if (new_process_ids.size() == 0) {
    // If no new process IDs were found yet, repost the message
    this->PostMessageToSelf(WD_BROWSER_NEW_WINDOW);
    return 0;
  }

  if (new_process_ids.size() > 1) {
    //LOG(WARN) << "Found more than one new iexplore.exe process. It is "
    //  << "impossible to know which is the proper one. Choosing one "
    //  << "at random.";
  }

  HWND document_handle;
  DWORD new_process_id = new_process_ids[0];
  if (!this->IsBrowserProcessInitialized(new_process_id, &document_handle)) {
    // If the browser for the new process ID is not yet ready,
    // repost the message
    this->PostMessageToSelf(WD_BROWSER_NEW_WINDOW);
    return 0;
  }

  BrowserHost::CreateInstance(new_process_id,
                              this->notify_window_handle_,
                              this->settings_window_handle_);

  return 0;
}

void BrowserHost::GetInProcessDriverLibraryPath(const DWORD process_id,
                                                std::string* library_path) {
  bool is_64_bit_browser_process = false;
  if (RegistryUtilities::Is64BitWindows()) {
    HANDLE browser_process_handle = ::OpenProcess(PROCESS_QUERY_INFORMATION,
                                                  FALSE,
                                                  process_id);
    BOOL is_emulated = FALSE;
    ::IsWow64Process(browser_process_handle, &is_emulated);
    is_64_bit_browser_process = is_emulated == FALSE;
    ::CloseHandle(browser_process_handle);
  }

  // TODO: Implement embedding of the in-process component as a resource;
  // extract it at run-time; and load using dynamic path here, selecting
  // the correct binary (32-bit or 64-bit) based on the bit-ness of the
  // browser's rendering process. In the meantime, simply load the binary
  // from the same directory as this executable.
  std::vector<wchar_t> file_name_buffer(MAX_PATH);
  ::GetModuleFileName(NULL, &file_name_buffer[0], MAX_PATH);
  ::PathCchRemoveFileSpec(&file_name_buffer[0], MAX_PATH);
  std::wstring wide_path(&file_name_buffer[0]);
  *library_path = StringUtilities::ToString(wide_path)
      .append("\\InProcessDriverEngine.dll");
}

HRESULT BrowserHost::GetDocumentFromWindowHandle(const HWND window_handle,
                                                 IHTMLDocument2** document) {
  HRESULT hr = S_OK;
  UINT html_getobject_msg = ::RegisterWindowMessage(HTML_GETOBJECT_MSG);

  // Explicitly load MSAA so we know if it's installed
  HMODULE oleacc_instance_handle = ::LoadLibrary(OLEACC_LIBRARY_NAME);
  if (oleacc_instance_handle == NULL) {
    return E_FAIL;
  }

  LRESULT result;

  LRESULT send_message_result =
      ::SendMessageTimeout(window_handle,
                           html_getobject_msg,
                           0L,
                           0L,
                           SMTO_ABORTIFHUNG,
                           1000,
                           reinterpret_cast<PDWORD_PTR>(&result));

  LPFNOBJECTFROMLRESULT object_pointer =
      reinterpret_cast<LPFNOBJECTFROMLRESULT>(
          ::GetProcAddress(oleacc_instance_handle, "ObjectFromLresult"));
  if (object_pointer == NULL) {
    return E_FAIL;
  }
  hr = (*object_pointer)(result,
                         IID_IHTMLDocument2,
                         0,
                         reinterpret_cast<void**>(document));
  ::FreeLibrary(oleacc_instance_handle);
  return hr;
}

HRESULT BrowserHost::GetBrowserFromDocument(IHTMLDocument2* document) {
  HRESULT hr = S_OK;
  CComPtr<IHTMLWindow2> parent_window;
  hr = document->get_parentWindow(&parent_window);
  if (FAILED(hr) || parent_window == nullptr) {
    return hr;
  }

  CComPtr<IServiceProvider> provider;
  hr = parent_window->QueryInterface<IServiceProvider>(&provider);
  if (FAILED(hr) || provider == nullptr) {
    return hr;
  }
  CComPtr<IServiceProvider> child_provider;
  hr = provider->QueryService(SID_STopLevelBrowser,
                              IID_IServiceProvider,
                              reinterpret_cast<void**>(&child_provider));
  if (FAILED(hr) || child_provider == nullptr) {
    return hr;
  }
  hr = child_provider->QueryService(SID_SWebBrowserApp,
                                    IID_IWebBrowser2,
                                    reinterpret_cast<void**>(&this->browser_));
  if (FAILED(hr) || this->browser_ == nullptr) {
    return hr;
  }
  return hr;
}

HRESULT BrowserHost::StartDiagnosticsMode(IHTMLDocument2* document) {
  HRESULT hr = S_OK;

  // Get the target from the document
  CComPtr<IOleCommandTarget> command_target;
  hr = document->QueryInterface<IOleCommandTarget>(&command_target);
  if (command_target.p == nullptr) {
    return E_INVALIDARG;
  }

  // Setup the diagnostics mode parameters
  CComBSTR guid(__uuidof(InProcessDriver));
  CComBSTR path(this->engine_path_.c_str());
  CComBSTR dummy_arg1(L"");
  CComBSTR dummy_arg2(L"");
  CComSafeArray<BSTR> safe_array(4);
  hr = safe_array.SetAt(0, ::SysAllocString(guid), FALSE);
  FAIL_IF_NOT_S_OK(hr);
  hr = safe_array.SetAt(1, ::SysAllocString(path), FALSE);
  FAIL_IF_NOT_S_OK(hr);
  hr = safe_array.SetAt(2, ::SysAllocString(L""), FALSE);
  FAIL_IF_NOT_S_OK(hr);
  hr = safe_array.SetAt(3, ::SysAllocString(L""), FALSE);
  FAIL_IF_NOT_S_OK(hr);

  // Start diagnostics mode
  CComVariant site;
  CComVariant params(safe_array);
  hr = command_target->Exec(&CGID_MSHTML,
                            IDM_STARTDIAGNOSTICSMODE,
                            OLECMDEXECOPT_DODEFAULT,
                            &params,
                            &site);
  FAIL_IF_NOT_S_OK(hr);
  ATLENSURE_RETURN_VAL(site.vt == VT_UNKNOWN, E_UNEXPECTED);
  ATLENSURE_RETURN_VAL(site.punkVal != nullptr, E_UNEXPECTED);

  CComPtr<IOleWindow> window;
  hr = site.punkVal->QueryInterface(__uuidof(window),
                                    reinterpret_cast<void**>(&window.p));
  FAIL_IF_NOT_S_OK(hr);

  hr = window->GetWindow(&this->in_proc_executor_handle_);
  FAIL_IF_NOT_S_OK(hr);
  ::SendMessage(this->in_proc_executor_handle_,
                WD_INIT,
                reinterpret_cast<WPARAM>(this->settings_window_handle_),
                reinterpret_cast<LPARAM>(this->m_hWnd));
  return hr;
}

std::string BrowserHost::CreateInstance(const DWORD process_id,
                                        const HWND notify_window_handle,
                                        const HWND session_settings_handle) {
  BrowserHostThreadContext context;
  context.browser_host_id = "";
  context.browser_process_id = process_id;
  context.instance_manager_handle = notify_window_handle;
  context.session_settings_handle = session_settings_handle;
  context.attach_timeout = 0;
  context.sync_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  unsigned int thread_id = 0;
  HANDLE thread_handle = reinterpret_cast<HANDLE>(
      _beginthreadex(NULL,
                     0,
                     &BrowserHost::ThreadProc,
                     reinterpret_cast<void*>(&context),
                     0,
                     &thread_id));
  if (context.sync_event != NULL) {
    DWORD thread_wait_status = ::WaitForSingleObject(context.sync_event,
                                                     THREAD_WAIT_TIMEOUT);
    if (thread_wait_status != WAIT_OBJECT_0) {
    }
    ::CloseHandle(context.sync_event);
  }

  if (thread_handle != NULL) {
    ::CloseHandle(thread_handle);
  }

  return context.browser_host_id;
}

void BrowserHost::GetNewBrowserProcessIds(
    std::vector<DWORD>* new_process_ids) {
  std::vector<DWORD> all_ie_process_ids;
  WindowUtilities::GetProcessesByName(L"iexplore.exe", &all_ie_process_ids);

  // Maximum size of the new process list is if all IE processes are unknown.
  std::vector<DWORD> temp_new_process_ids(all_ie_process_ids.size());
  std::sort(this->known_process_ids_.begin(),
            this->known_process_ids_.end());
  std::sort(all_ie_process_ids.begin(), all_ie_process_ids.end());
  std::vector<DWORD>::iterator end_iterator =
      std::set_difference(all_ie_process_ids.begin(),
                          all_ie_process_ids.end(),
                          this->known_process_ids_.begin(),
                          this->known_process_ids_.end(),
                          temp_new_process_ids.begin());
  temp_new_process_ids.resize(end_iterator - temp_new_process_ids.begin());
  *new_process_ids = temp_new_process_ids;
}

void BrowserHost::PostMessageToSelf(UINT msg) {
  PostSelfMessageThreadContext* thread_context =
      new PostSelfMessageThreadContext;
  thread_context->window_handle = this->m_hWnd;
  thread_context->msg = msg;
  unsigned int thread_id = 0;
  HANDLE thread_handle = reinterpret_cast<HANDLE>(
    _beginthreadex(NULL,
                   0,
                   &BrowserHost::SelfMessageThreadProc,
                   reinterpret_cast<void*>(thread_context),
                   0,
                   &thread_id));
  if (thread_handle != NULL) {
    ::CloseHandle(thread_handle);
  }
}

bool BrowserHost::IsBrowserProcessInitialized(const DWORD process_id,
                                              HWND* document_handle) {
  ProcessWindowInfo info;
  info.dwProcessId = process_id;
  info.hwndBrowser = NULL;
  info.pBrowser = NULL;
  ::EnumWindows(&BrowserFactory::FindBrowserWindow,
                reinterpret_cast<LPARAM>(&info));
  *document_handle = info.hwndBrowser;
  return (info.hwndBrowser != NULL);
}

unsigned int WINAPI BrowserHost::SelfMessageThreadProc(LPVOID lpParameter) {
  PostSelfMessageThreadContext* thread_context =
    reinterpret_cast<PostSelfMessageThreadContext*>(lpParameter);
  HWND window_handle = thread_context->window_handle;
  UINT msg = thread_context->msg;
  delete thread_context;

  ::Sleep(50);
  ::PostMessage(window_handle, msg, NULL, NULL);
  return 0;
}

unsigned int WINAPI BrowserHost::ThreadProc(LPVOID lpParameter) {
  BrowserHostThreadContext* thread_context =
      reinterpret_cast<BrowserHostThreadContext*>(lpParameter);

  DWORD error = 0;
  HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
  }

  BrowserHost wrapper;

  HWND document_handle = NULL;
  bool is_initialized = wrapper.IsBrowserProcessInitialized(
      thread_context->browser_process_id,
      &document_handle);

  clock_t end = clock() +
      (thread_context->attach_timeout / 1000 * CLOCKS_PER_SEC);
  while (!is_initialized) {
    if (thread_context->attach_timeout > 0 && (clock() > end)) {
      break;
    }
    ::Sleep(250);
    is_initialized = wrapper.IsBrowserProcessInitialized(
        thread_context->browser_process_id,
        &document_handle);
  }

  if (!is_initialized) {
    wrapper.DestroyWindow();
    if (thread_context->sync_event != NULL) {
      ::SetEvent(thread_context->sync_event);
    }
    return 0;
  }

  wrapper.Initialize(thread_context->browser_process_id,
                     thread_context->instance_manager_handle,
                     thread_context->session_settings_handle);

  BrowserInfo info;
  info.browser_host_window_handle = wrapper.m_hWnd;
  info.browser_id = wrapper.id_;
  info.content_window_handle = wrapper.content_window_handle_;
  info.instance_manager_window_handle = wrapper.notify_window_handle_;
  info.in_proc_executor_window_handle = wrapper.in_proc_executor_handle_;
  info.tab_window_handle = wrapper.tab_window_handle_;
  info.top_level_window_handle = wrapper.top_level_window_handle_;
  ::SendMessage(wrapper.notify_window_handle_,
                WD_REGISTER_INSTANCE,
                NULL,
                reinterpret_cast<LPARAM>(&info));

  MSG msg;
  ::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  // Return the ID back through lpParameter, and signal that the
  // window is ready for messages.
  thread_context->browser_host_id = info.browser_id;

  if (thread_context->sync_event != NULL) {
    ::SetEvent(thread_context->sync_event);
  }

  // Run the message loop
  BOOL get_message_return_value;
  while ((get_message_return_value = ::GetMessage(&msg, NULL, 0, 0)) != 0) {
    if (get_message_return_value == -1) {
      break;
    } else {
      if (msg.message == WD_SHUTDOWN) {
        wrapper.DestroyWindow();
        break;
      } else {
        // We need to lock this mutex here to make sure only one thread is
        // processing win32 messages at a time.
        static std::mutex messageLock;
        std::lock_guard<std::mutex> lock(messageLock);
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }
  }

  ::CoUninitialize();
  return 0;
}

}  // namespace webdriver
