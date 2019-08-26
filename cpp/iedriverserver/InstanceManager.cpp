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

#include "InstanceManager.h"

#include <mutex>
#include <vector>

#include "BrowserInfo.h"

#define THREAD_WAIT_TIMEOUT 1000

namespace webdriver {

struct InstanceManagerThreadContext {
  HWND instance_manager_window_handle;
  HANDLE sync_event;
};

InstanceManager::InstanceManager() {
}

InstanceManager::~InstanceManager() {
}

LRESULT InstanceManager::OnInit(UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& bHandled) {
  return 0;
}

LRESULT InstanceManager::OnRegisterInstance(UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam,
                                            BOOL& bHandled) {
  BrowserInfo* to_register = reinterpret_cast<BrowserInfo*>(lParam);
  BrowserInfo info;
  this->CopyBrowserInfo(to_register, &info);
  this->instances_[info.browser_id] = info;
  return 0;
}

LRESULT InstanceManager::OnIsValidInstance(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  std::string instance_id(*reinterpret_cast<std::string*>(lParam));
  std::map<std::string, BrowserInfo>::const_iterator finder =
      this->instances_.find(instance_id);
  if (finder == this->instances_.end()) {
    return 0;
  }

  return 1;
}

LRESULT InstanceManager::OnGetInstanceInfo(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  BrowserInfo* info = reinterpret_cast<BrowserInfo*>(lParam);
  std::string instance_id = info->browser_id;
  std::map<std::string, BrowserInfo>::const_iterator finder =
      this->instances_.find(instance_id);
  if (finder == this->instances_.end()) {
    return 0;
  }
  this->CopyBrowserInfo(&finder->second, info);
  return 1;
}

LRESULT InstanceManager::OnGetInstanceList(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  std::vector<std::string>* instance_id_list =
      reinterpret_cast<std::vector<std::string>*>(lParam);
  std::map<std::string, BrowserInfo>::const_iterator it =
      this->instances_.begin();
  for (; it != this->instances_.end(); ++it) {
    instance_id_list->push_back(it->first);
  }
  return 0;
}

LRESULT InstanceManager::OnNotifyInstanceClose(UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam,
                                               BOOL& bHandled) {
  char* buffer_content = reinterpret_cast<char*>(lParam);
  std::vector<char> buffer(wParam);
  strcpy_s(&buffer[0], buffer.size(), buffer_content);
  delete[] buffer_content;
  std::string instance_id(&buffer[0]);
  std::map<std::string, BrowserInfo>::const_iterator finder =
      this->instances_.find(instance_id);
  if (finder != this->instances_.end()) {
    this->instances_.erase(instance_id);
  }
  return 0;
}

void InstanceManager::CopyBrowserInfo(const BrowserInfo* src,
                                      BrowserInfo* dest) {
  dest->browser_host_window_handle = src->browser_host_window_handle;
  dest->browser_id = src->browser_id;
  dest->content_window_handle = src->content_window_handle;
  dest->in_proc_executor_window_handle = src->in_proc_executor_window_handle;
  dest->tab_window_handle = src->tab_window_handle;
  dest->top_level_window_handle = src->top_level_window_handle;
}

HWND InstanceManager::CreateManager() {
  InstanceManagerThreadContext context;
  context.instance_manager_window_handle = NULL;
  context.sync_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  unsigned int thread_id = 0;
  HANDLE thread_handle = reinterpret_cast<HANDLE>(
      _beginthreadex(NULL,
                     0,
                     &InstanceManager::ThreadProc,
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

  return context.instance_manager_window_handle;
}

unsigned int WINAPI InstanceManager::ThreadProc(LPVOID lpParameter) {
  InstanceManagerThreadContext* thread_context =
      reinterpret_cast<InstanceManagerThreadContext*>(lpParameter);

  DWORD error = 0;
  InstanceManager manager;
  manager.Create(HWND_MESSAGE);

  MSG msg;
  ::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  // Return the HWND back through lpParameter, and signal that the
  // window is ready for messages.
  thread_context->instance_manager_window_handle = manager.m_hWnd;

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
        manager.DestroyWindow();
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

  return 0;
}

}  // namespace webdriver
