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

#include "InProcessDriver.h"

#include <ShlGuid.h>

#include "../utils/StringUtilities.h"
#include "../utils/WebDriverConstants.h"
#include "../utils/WindowUtilities.h"
#include "../webdriver-server/command.h"
#include "../webdriver-server/command_handler.h"
#include "../webdriver-server/command_types.h"
#include "../webdriver-server/errorcodes.h"
#include "../webdriver-server/response.h"

#include "ElementFinder.h"
#include "ElementRepository.h"
#include "InProcessCommandHandler.h"
#include "InProcessCommandRepository.h"
#include "InputManager.h"

#define INTERACTIVE_READY_STATE L"interactive"
#define COMPLETE_READY_STATE L"complete"
#define IE_PROCESS_NAME L"iexplore.exe"
#define IE_SERVER_CHILD_WINDOW_CLASS "Internet Explorer_Server"

struct WaitThreadContext {
  HWND window_handle;
  int command_id_size;
  LPSTR command_id;
};

struct ProcessIdThreadContext {
  HWND notify_window;
  HWND sending_window;
  DWORD notification_type;
  DWORD process_count;
  DWORD* process_id_list;
};

InProcessDriver::InProcessDriver() : notify_window_(NULL),
                                     top_level_window_(NULL),
                                     tab_window_(NULL),
                                     content_window_(NULL),
                                     command_id_(""),
                                     command_status_(UNINITIALIZED),
                                     is_navigating_(false),
                                     use_strict_file_interactability_(false),
                                     page_load_timeout_(0),
                                     script_timeout_(0),
                                     implicit_wait_timeout_(0),
                                     page_load_strategy_(""),
                                     serialized_command_(""),
                                     serialized_response_(""),
                                     focused_frame_(nullptr) {
  this->command_handlers_ = new webdriver::InProcessCommandRepository();
  this->known_element_repository_ = new webdriver::ElementRepository();
  this->element_finder_ = new webdriver::ElementFinder();
  this->input_manager_ = new webdriver::InputManager();
  this->Create(HWND_MESSAGE);
}

InProcessDriver::~InProcessDriver() {
  this->known_element_repository_->Clear();
  delete this->known_element_repository_;
  delete this->element_finder_;
  delete this->input_manager_;
  delete this->command_handlers_;
}

STDMETHODIMP_(HRESULT) InProcessDriver::SetSite(IUnknown* pUnkSite) {
  HRESULT hr = S_OK;
  if (pUnkSite == nullptr) {
    this->DispEventUnadvise(this->browser_);
    this->DestroyWindow();
    if (this->notify_window_ != NULL) {
      ::SendMessage(this->notify_window_, WD_NOTIFY_PENDING_REACQUIRE, NULL, NULL);
    }
    ::PostQuitMessage(0);
    return hr;
  }

  CComPtr<IWebBrowser2> browser;
  pUnkSite->QueryInterface<IWebBrowser2>(&browser);
  if (browser != nullptr) {
    CComPtr<IDispatch> document_dispatch;
    hr = browser->get_Document(&document_dispatch);
    if (document_dispatch.p == nullptr) {
      return E_FAIL;
    }

    CComPtr<IHTMLDocument> document;
    hr = document_dispatch->QueryInterface<IHTMLDocument>(&document);

    CComPtr<IServiceProvider> service_provider;
    hr = document_dispatch->QueryInterface<IServiceProvider>(
        &service_provider);
    if (service_provider == nullptr) {
      return E_NOINTERFACE;
    }
    CComPtr<IDiagnosticsScriptEngineProvider> engine_provider;
    hr = service_provider->QueryService(IID_IDiagnosticsScriptEngineProvider,
                                        &engine_provider);
    hr = engine_provider->CreateDiagnosticsScriptEngine(this,
                                                        FALSE,
                                                        0,
                                                        &this->script_engine_);

    CComPtr<IServiceProvider> browser_service_provider;
    hr = browser->QueryInterface<IServiceProvider>(&browser_service_provider);
    if (browser_service_provider == nullptr) {
      return E_FAIL;
    }
    CComPtr<IOleWindow> window;
    hr = browser_service_provider->QueryService<IOleWindow>(SID_SShellBrowser,
                                                            &window);
    hr = window->GetWindow(&this->tab_window_);
    hr = browser->get_HWND(reinterpret_cast<SHANDLE_PTR*>(&this->top_level_window_));
    ::EnumChildWindows(this->tab_window_,
                       &FindChildContentWindow,
                       reinterpret_cast<LPARAM>(&this->content_window_));

    this->browser_ = browser;
    this->script_host_document_ = document;
    hr = this->script_engine_->EvaluateScript(L"browser.addEventListener('consoleMessage', function(e) { external.sendMessage('consoleMessage', JSON.stringify(e)); });", L"");
    this->DispEventAdvise(this->browser_);
  }
  return hr;
}

STDMETHODIMP_(HRESULT) InProcessDriver::GetWindow(HWND* pHwnd) {
  *pHwnd = this->m_hWnd;
  return S_OK;
}

STDMETHODIMP_(HRESULT) InProcessDriver::OnMessage(LPCWSTR* pszData,
                                                  ULONG ulDataCount) {
  std::vector<std::wstring> message;
  for (ULONG i = 0; i < ulDataCount; ++i) {
    std::wstring data(*pszData);
    message.push_back(data);
    ++pszData;
  }
  return S_OK;
}

STDMETHODIMP_(HRESULT) InProcessDriver::OnScriptError(
    IActiveScriptError* pScriptError) {
  return S_OK;
}

STDMETHODIMP_(void) InProcessDriver::OnBeforeNavigate2(
    LPDISPATCH pDisp,
    VARIANT* pvarUrl,
    VARIANT* pvarFlags,
    VARIANT* pvarTargetFrame,
    VARIANT* pvarData,
    VARIANT* pvarHeaders,
    VARIANT_BOOL* pbCancel) {
  std::string command_id = this->command_id_;
  if (this->browser_.IsEqualObject(pDisp)) {
    if (command_id.size() > 0) {
      this->CreateWaitThread(command_id);
    }
    this->SetFocusedFrameByElement(nullptr);
  }
}

STDMETHODIMP_(void) InProcessDriver::OnNavigateComplete2(LPDISPATCH pDisp,
                                                         VARIANT* URL) {
  if (this->browser_.IsEqualObject(pDisp)) {
    this->is_navigating_ = false;
  }
}

STDMETHODIMP_(void) InProcessDriver::OnDocumentComplete(LPDISPATCH pDisp,
                                                        VARIANT* URL) {
}

STDMETHODIMP_(void) InProcessDriver::OnNewWindow(LPDISPATCH ppDisp,
                                                 VARIANT_BOOL* pbCancel,
                                                 DWORD dwFlags,
                                                 BSTR bstrUrlContext,
                                                 BSTR bstrUrl) {

  if (this->notify_window_ != NULL) {
    ::SendMessage(this->notify_window_, WD_NOTIFY_PENDING_NEW_WINDOW, NULL, NULL);
  }
}

STDMETHODIMP_(void) InProcessDriver::OnNewProcess(DWORD lCauseFlag,
                                                  IDispatch* pWB2,
                                                  VARIANT_BOOL* pbCancel) {
}

STDMETHODIMP_(void) InProcessDriver::OnQuit() {
}


LRESULT InProcessDriver::OnCopyData(UINT nMsg,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    BOOL& handled) {
  COPYDATASTRUCT* data = reinterpret_cast<COPYDATASTRUCT*>(lParam);
  std::vector<char> buffer(data->cbData + 1);
  memcpy_s(&buffer[0], data->cbData, data->lpData, data->cbData);
  buffer[buffer.size() - 1] = '\0';
  std::string received_data(&buffer[0]);
  if (data->dwData == COPYDATA_UPDATE_SETTINGS) {
    this->UpdateSettings(received_data);
    return 0;
  }

  this->command_id_ = webdriver::StringUtilities::CreateGuid();
  this->serialized_command_ = received_data;
  this->command_status_ = READY;
  return 0;
}

LRESULT InProcessDriver::OnDestroy(UINT nMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& handled) {
  if (this->script_engine_ != nullptr) {
    this->script_engine_.Release();
  }
  if (this->script_host_document_ != nullptr) {
    this->script_host_document_.Release();
  }
  if (this->browser_ != nullptr) {
    this->DispEventUnadvise(browser_);
    this->browser_->Quit();
    this->browser_.Release();
  }
  return 0;
}

LRESULT InProcessDriver::OnInit(UINT nMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& handled) {
  this->notify_window_ = reinterpret_cast<HWND>(lParam);
  int action_simulator_type = static_cast<int>(wParam);
  
  webdriver::InputManagerSettings settings;
  settings.element_repository = this->known_element_repository_;
  settings.action_simulator_type = action_simulator_type;
  input_manager_->Initialize(settings);
  return 0;
}

LRESULT InProcessDriver::OnExecuteCommand(UINT nMsg,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          BOOL& handled) {
  webdriver::Command command;
  command.Deserialize(this->serialized_command_);

  webdriver::CommandHandlerHandle command_handler =
      this->command_handlers_->GetCommandHandler(command.command_type());

  std::string executing_command_id = this->command_id_;
  webdriver::Response response;
  if (command_handler == nullptr) {
    response.SetErrorResponse(ERROR_UNKNOWN_COMMAND,
                              "No handler for " + command.command_type());
  } else {
    this->command_status_ = EXECUTING;
    command_handler->Execute(*this, command, &response);
  }
  if (this->is_navigating_) {
    return 0;
  }

  // There is a chance that a command execution will block (e.g., via
  // an alert). In this case, it's possible that the current command
  // might return after the alert is handled by the out-of-process
  // component, and after the next command has been submitted here
  // in-process. When we find ourselves in this situation, we need to
  // not return the response for the abandoned command.
  // CONSIDER: We could probably make this more robust by assigning
  // an ID to each command and keeping track of all in-process commands.
  // This is a potential future enhancement, if it's found necessary.
  if (executing_command_id == this->command_id_ &&
      this->command_status_ != ABORTED) {
    this->serialized_response_ = response.Serialize();
    this->command_status_ = COMPLETE;
  }
  return 0;
}

LRESULT InProcessDriver::OnAbortCommand(UINT nMsg,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        BOOL& handled) {
  this->command_status_ = ABORTED;
  this->command_id_ = "";
  return 0;
}

LRESULT InProcessDriver::OnGetCommandStatus(UINT nMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            BOOL& handled) {
  return this->command_status_;
}

LRESULT InProcessDriver::OnGetResponseLength(UINT nMsg,
                                             WPARAM wParam,
                                             LPARAM lParam,
                                             BOOL& handled) {
  return this->serialized_response_.size();
}

LRESULT InProcessDriver::OnGetResponse(UINT nMsg,
                                       WPARAM wParam,
                                       LPARAM lParam,
                                       BOOL& handled) {
  HWND return_window = reinterpret_cast<HWND>(wParam);
  std::string serialized_response = this->serialized_response_;
  std::vector<char> response_buffer(serialized_response.size() + 1);
  strcpy_s(&response_buffer[0],
           response_buffer.size(),
           serialized_response.c_str());
  response_buffer[response_buffer.size() - 1] = '\0';

  COPYDATASTRUCT copy_data;
  copy_data.dwData = COPYDATA_RESPONSE;
  copy_data.lpData = reinterpret_cast<void*>(&response_buffer[0]);
  copy_data.cbData = static_cast<DWORD>(response_buffer.size());
  ::SendMessage(return_window,
                WM_COPYDATA,
                reinterpret_cast<WPARAM>(this->m_hWnd),
                reinterpret_cast<LPARAM>(&copy_data));
  this->serialized_command_ = "";
  this->serialized_response_ = "";
  this->command_status_ = UNINITIALIZED;
  this->command_id_ = "";
  return 0;
}

LRESULT InProcessDriver::OnWait(UINT nMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& handled) {
  WaitThreadContext* context = reinterpret_cast<WaitThreadContext*>(lParam);
  std::vector<char> command_id_buffer(context->command_id_size);
  strcpy_s(&command_id_buffer[0],
           context->command_id_size,
           context->command_id);
  std::string command_id(&command_id_buffer[0]);
  delete[] context->command_id;
  delete context;

  if (command_id.size() != 0 && command_id == this->command_id_ &&
      this->command_status_ != ABORTED) {
    if (this->IsDocumentReady()) {
      webdriver::Response response;
      response.SetSuccessResponse(Json::Value::null);
      this->serialized_response_ = response.Serialize();
    } else {
      this->CreateWaitThread(command_id);
    }
  }
  return 0;
}

int InProcessDriver::GetFocusedDocument(IHTMLDocument2** document) const {
  HRESULT hr = S_OK;
  CComPtr<IHTMLWindow2> window;
  if (this->focused_frame_ == nullptr) {
    // N.B. IWebBrowser2::get_Document and IHTMLWindow2::get_document
    // return two very different objects. We want the latter in all
    // cases.
    CComPtr<IDispatch> dispatch;
    hr = this->browser_->get_Document(&dispatch);
    if (FAILED(hr)) {
      // TODO: log the error and return error code
    }

    CComPtr<IHTMLDocument2> dispatch_document;
    hr = dispatch->QueryInterface(&dispatch_document);
    if (FAILED(hr)) {
      // TODO: log the error and return error code
    }

    hr = dispatch_document->get_parentWindow(&window);
    if (FAILED(hr)) {
      // TODO: log the error
    }
  } else {
    window = this->focused_frame_;
  }

  if (window) {
    hr = window->get_document(document);
    if (FAILED(hr)) {
      // TODO: log the error and return error code
    }
  } else {
    // TODO: log the error and return error code
  }
  return WD_SUCCESS;
}

int InProcessDriver::SetFocusedFrameByElement(IHTMLElement* frame_element) {
  HRESULT hr = S_OK;
  if (!frame_element) {
    this->focused_frame_ = nullptr;
    return WD_SUCCESS;
  }

  CComPtr<IHTMLWindow2> interim_result;
  CComPtr<IHTMLObjectElement4> object_element;
  hr = frame_element->QueryInterface<IHTMLObjectElement4>(&object_element);
  if (SUCCEEDED(hr) && object_element) {
    CComPtr<IDispatch> object_disp;
    object_element->get_contentDocument(&object_disp);
    if (!object_disp) {
      //LOG(WARN) << "Cannot get IDispatch interface from IHTMLObjectElement4 element";
      return ENOSUCHFRAME;
    }

    CComPtr<IHTMLDocument2> object_doc;
    object_disp->QueryInterface<IHTMLDocument2>(&object_doc);
    if (!object_doc) {
      //LOG(WARN) << "Cannot get IHTMLDocument2 document from IDispatch reference";
      return ENOSUCHFRAME;
    }

    hr = object_doc->get_parentWindow(&interim_result);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Cannot get parentWindow from IHTMLDocument2, call to IHTMLDocument2::get_parentWindow failed";
      return ENOSUCHFRAME;
    }
  }
  else {
    CComPtr<IHTMLFrameBase2> frame_base;
    frame_element->QueryInterface<IHTMLFrameBase2>(&frame_base);
    if (!frame_base) {
      //LOG(WARN) << "IHTMLElement is not a FRAME or IFRAME element";
      return ENOSUCHFRAME;
    }

    hr = frame_base->get_contentWindow(&interim_result);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Cannot get contentWindow from IHTMLFrameBase2, call to IHTMLFrameBase2::get_contentWindow failed";
      return ENOSUCHFRAME;
    }
  }

  this->focused_frame_ = interim_result;
  return WD_SUCCESS;
}

int InProcessDriver::SetFocusedFrameByIndex(const int frame_index) {
  CComPtr<IHTMLDocument2> doc;
  this->GetFocusedDocument(&doc);

  CComPtr<IHTMLFramesCollection2> frames;
  HRESULT hr = doc->get_frames(&frames);

  if (!frames) {
    //LOG(WARN) << "No frames in document are set, IHTMLDocument2::get_frames returned NULL";
    return ENOSUCHFRAME;
  }

  long length = 0;
  frames->get_length(&length);
  if (!length) {
    //LOG(WARN) << "No frames in document are found IHTMLFramesCollection2::get_length returned 0";
    return ENOSUCHFRAME;
  }

  // Find the frame
  CComVariant frame_identifier;
  frame_identifier.vt = VT_I4;
  frame_identifier.lVal = frame_index;
  CComVariant frame_holder;
  hr = frames->item(&frame_identifier, &frame_holder);

  if (FAILED(hr)) {
    //LOGHR(WARN, hr) << "Error retrieving frame holder, call to IHTMLFramesCollection2::item failed";
    return ENOSUCHFRAME;
  }

  CComPtr<IHTMLWindow2> interim_result;
  frame_holder.pdispVal->QueryInterface<IHTMLWindow2>(&interim_result);
  if (!interim_result) {
    //LOG(WARN) << "Error retrieving frame, IDispatch cannot be cast to IHTMLWindow2";
    return ENOSUCHFRAME;
  }

  this->focused_frame_ = interim_result;
  return WD_SUCCESS;
}

void InProcessDriver::SetFocusedFrameToParent() {
  // Three possible outcomes.
  // Outcome 1: Already at top-level browsing context. No-op.
  if (this->focused_frame_ != NULL) {
    CComPtr<IHTMLWindow2> parent_window;
    HRESULT hr = this->focused_frame_->get_parent(&parent_window);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "IHTMLWindow2::get_parent call failed.";
    }
    CComPtr<IHTMLWindow2> top_window;
    hr = this->focused_frame_->get_top(&top_window);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "IHTMLWindow2::get_top call failed.";
    }
    if (top_window.IsEqualObject(parent_window)) {
      // Outcome 2: Focus is on a frame one level deep, making the
      // parent the top-level browsing context. Set focused frame
      // pointer to NULL.
      this->focused_frame_ = NULL;
    } else {
      // Outcome 3: Focus is on a frame more than one level deep.
      // Set focused frame pointer to parent frame.
      this->focused_frame_ = parent_window;
    }
  }
}

bool InProcessDriver::IsDocumentReady() {
  if (this->is_navigating_) {
    return false;
  }

  if (this->page_load_strategy_ == NONE_PAGE_LOAD_STRATEGY) {
    return true;
  }

  HRESULT hr = S_OK;
  CComPtr<IDispatch> document_dispatch;
  hr = this->browser_->get_Document(&document_dispatch);
  if (FAILED(hr) || document_dispatch == nullptr) {
    return false;
  }

  CComPtr<IHTMLDocument2> doc;
  hr = document_dispatch->QueryInterface<IHTMLDocument2>(&doc);
  if (FAILED(hr) || doc == nullptr) {
    return false;
  }

  CComBSTR ready_state_bstr;
  hr = doc->get_readyState(&ready_state_bstr);
  if (FAILED(hr)) {
    return false;
  }

  if (this->page_load_strategy_ == EAGER_PAGE_LOAD_STRATEGY) {
    return ready_state_bstr == INTERACTIVE_READY_STATE;
  }

  return ready_state_bstr == COMPLETE_READY_STATE;
}

void InProcessDriver::UpdateSettings(const std::string& settings_json) {
  Json::Value settings;
  std::string parse_errors;
  std::stringstream json_stream;
  json_stream.str(settings_json);
  Json::parseFromStream(Json::CharReaderBuilder(),
                        json_stream,
                        &settings,
                        &parse_errors);
  if (settings.isMember(PAGE_LOAD_STRATEGY_CAPABILITY)) {
    this->page_load_strategy_ =
        settings[PAGE_LOAD_STRATEGY_CAPABILITY].asString();
  }

  if (settings.isMember(STRICT_FILE_INTERACTABILITY_CAPABILITY)) {
    this->use_strict_file_interactability_ =
        settings[STRICT_FILE_INTERACTABILITY_CAPABILITY].asBool();
  }

  if (settings.isMember(TIMEOUTS_CAPABILITY)) {
    Json::Value timeouts = settings[TIMEOUTS_CAPABILITY];
    if (timeouts.isMember(IMPLICIT_WAIT_TIMEOUT_NAME)) {
      this->implicit_wait_timeout_ =
          timeouts[IMPLICIT_WAIT_TIMEOUT_NAME].asUInt64();
    }
    if (timeouts.isMember(PAGE_LOAD_TIMEOUT_NAME)) {
      this->page_load_timeout_ = timeouts[PAGE_LOAD_TIMEOUT_NAME].asUInt64();
    }
    if (timeouts.isMember(SCRIPT_TIMEOUT_NAME)) {
      this->script_timeout_ = timeouts[SCRIPT_TIMEOUT_NAME].asUInt64();
    }
  }
}

void InProcessDriver::CreateWaitThread(const std::string& command_id) {
  // If we are still waiting, we need to wait a bit then post a message to
  // ourselves to run the wait again. However, we can't wait using Sleep()
  // on this thread. This call happens in a message loop, and we would be
  // unable to process the COM events in the browser if we put this thread
  // to sleep.
  WaitThreadContext* thread_context = new WaitThreadContext;
  thread_context->window_handle = this->m_hWnd;
  thread_context->command_id_size = command_id.size() + 1;
  thread_context->command_id = new char[thread_context->command_id_size];
  strcpy_s(thread_context->command_id,
           thread_context->command_id_size,
           command_id.c_str());
  unsigned int thread_id = 0;
  HANDLE thread_handle = reinterpret_cast<HANDLE>(
      _beginthreadex(NULL,
                     0,
                     &InProcessDriver::WaitThreadProc,
                     reinterpret_cast<void*>(thread_context),
                     0,
                     &thread_id));
  if (thread_handle != NULL) {
    ::CloseHandle(thread_handle);
  }
}

void InProcessDriver::WriteDebug(const std::string& message) {
  std::string actual = "******** " + message + "\n";
  ::OutputDebugStringA(actual.c_str());
}

BOOL CALLBACK InProcessDriver::FindChildContentWindow(HWND hwnd, LPARAM arg) {
  HWND* content_window = reinterpret_cast<HWND*>(arg);

  // Could this be an Internet Explorer Server window?
  // 25 == "Internet Explorer_Server\0"
  char name[25];
  if (::GetClassNameA(hwnd, name, 25) == 0) {
    // No match found. Skip
    return TRUE;
  }

  if (strcmp(IE_SERVER_CHILD_WINDOW_CLASS, name) != 0) {
    return TRUE;
  } else {
    *content_window = hwnd;
    return FALSE;
  }

  return TRUE;
}

unsigned int WINAPI InProcessDriver::WaitThreadProc(LPVOID lpParameter) {
  WaitThreadContext* thread_context =
      reinterpret_cast<WaitThreadContext*>(lpParameter);
  HWND window_handle = thread_context->window_handle;

  ::Sleep(50);
  ::PostMessage(window_handle,
                WD_WAIT,
                NULL,
                reinterpret_cast<LPARAM>(lpParameter));
  return 0;
}
