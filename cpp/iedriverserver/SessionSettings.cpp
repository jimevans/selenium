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

#include "SessionSettings.h"

#include "../utils/WebDriverConstants.h"

#define THREAD_WAIT_TIMEOUT 1000
#define DEFAULT_SCRIPT_TIMEOUT_IN_MILLISECONDS 30000
#define DEFAULT_PAGE_LOAD_TIMEOUT_IN_MILLISECONDS 300000
#define DEFAULT_FILE_UPLOAD_DIALOG_TIMEOUT_IN_MILLISECONDS 3000
#define DEFAULT_BROWSER_REATTACH_TIMEOUT_IN_MILLISECONDS 10000

namespace webdriver {

struct SessionSettingsThreadContext {
  HWND session_settings_window_handle;
  HANDLE sync_event;
};

SessionSettings::SessionSettings() :
    browser_attach_timeout_(0),
    use_strict_file_interactability_(false),
    implicit_wait_timeout_(0),
    script_timeout_(DEFAULT_SCRIPT_TIMEOUT_IN_MILLISECONDS),
    page_load_timeout_(DEFAULT_PAGE_LOAD_TIMEOUT_IN_MILLISECONDS),
    page_load_strategy_(NORMAL_PAGE_LOAD_STRATEGY),
    unhandled_prompt_behavior_("") {
}

SessionSettings::~SessionSettings() {
}

LRESULT SessionSettings::OnGetSessionSetting(UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam,
                                             BOOL& bHandled) {
  int setting = static_cast<int>(wParam);
  switch (setting) {
    case SESSION_SETTING_IMPLICIT_WAIT_TIMEOUT: {
      long long* timeout =
          reinterpret_cast<long long*>(lParam);
      *timeout = this->implicit_wait_timeout_;
      break;
    }
    case SESSION_SETTING_PAGE_LOAD_TIMEOUT: {
      long long* timeout =
          reinterpret_cast<long long*>(lParam);
      *timeout = this->page_load_timeout_;
      break;
    }
    case SESSION_SETTING_SCRIPT_TIMEOUT: {
      long long* timeout =
          reinterpret_cast<long long*>(lParam);
      *timeout = this->script_timeout_;
      break;
    }
    case SESSION_SETTING_PAGE_LOAD_STRATEGY: {
      std::string* strategy =
          reinterpret_cast<std::string*>(lParam);
      *strategy = this->page_load_strategy_;
      break;
    }
    case SESSION_SETTING_UNHANDLED_PROMPT_BEHAVIOR: {
      std::string* behavior =
          reinterpret_cast<std::string*>(lParam);
      *behavior = this->unhandled_prompt_behavior_;
      break;
    }
    case SESSION_SETTING_STRICT_FILE_INTERACTABLILITY: {
      bool* use_strict_interactability =
          reinterpret_cast<bool*>(lParam);
      *use_strict_interactability = this->use_strict_file_interactability_;
      break;
    }
    case SESSION_SETTING_PROXY: {
      ProxySettings* proxy =
          reinterpret_cast<ProxySettings*>(lParam);
      this->proxy_settings_.CopyTo(proxy);
      break;
    }
    default: {
      break;
    }
  }
  return 0;
}

LRESULT SessionSettings::OnSetSessionSetting(UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam,
                                             BOOL& bHandled) {
  int setting = static_cast<int>(wParam);
  switch (setting) {
    case SESSION_SETTING_IMPLICIT_WAIT_TIMEOUT: {
      long long* timeout =
          reinterpret_cast<long long*>(lParam);
      this->implicit_wait_timeout_ = *timeout;
      break;
    }
    case SESSION_SETTING_PAGE_LOAD_TIMEOUT: {
      long long* timeout =
          reinterpret_cast<long long*>(lParam);
      this->page_load_timeout_ = *timeout;
      break;
    }
    case SESSION_SETTING_SCRIPT_TIMEOUT: {
      long long* timeout =
          reinterpret_cast<long long*>(lParam);
      this->script_timeout_ = *timeout;
      break;
    }
    case SESSION_SETTING_PAGE_LOAD_STRATEGY: {
      std::string* strategy =
          reinterpret_cast<std::string*>(lParam);
      this->page_load_strategy_ = *strategy;
      break;
    }
    case SESSION_SETTING_UNHANDLED_PROMPT_BEHAVIOR: {
      std::string* behavior =
          reinterpret_cast<std::string*>(lParam);
      this->unhandled_prompt_behavior_ = *behavior;
      break;
    }
    case SESSION_SETTING_STRICT_FILE_INTERACTABLILITY: {
      bool* use_strict_interactability =
          reinterpret_cast<bool*>(lParam);
      this->use_strict_file_interactability_ = *use_strict_interactability;
      break;
    }
    case SESSION_SETTING_PROXY: {
      ProxySettings* proxy =
          reinterpret_cast<ProxySettings*>(lParam);
      proxy->CopyTo(&this->proxy_settings_);
      this->proxy_settings_.is_set = true;
      break;
    }
    default: {
      break;
    }
  }
  return 0;
}

HWND SessionSettings::CreateInstance() {
  SessionSettingsThreadContext context;
  context.session_settings_window_handle = NULL;
  context.sync_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  unsigned int thread_id = 0;
  HANDLE thread_handle = reinterpret_cast<HANDLE>(
      _beginthreadex(NULL,
                     0,
                     &SessionSettings::ThreadProc,
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

  return context.session_settings_window_handle;
}

unsigned int WINAPI SessionSettings::ThreadProc(LPVOID lpParameter) {
  SessionSettingsThreadContext* thread_context =
    reinterpret_cast<SessionSettingsThreadContext*>(lpParameter);

  DWORD error = 0;
  SessionSettings session_settings;
  session_settings.Create(HWND_MESSAGE);

  MSG msg;
  ::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  // Return the HWND back through lpParameter, and signal that the
  // window is ready for messages.
  thread_context->session_settings_window_handle = session_settings.m_hWnd;

  if (thread_context->sync_event != NULL) {
    ::SetEvent(thread_context->sync_event);
  }

  // Run the message loop
  BOOL get_message_return_value;
  while ((get_message_return_value = ::GetMessage(&msg, NULL, 0, 0)) != 0) {
    if (get_message_return_value == -1) {
      break;
    }
    else {
      if (msg.message == WD_SHUTDOWN) {
        session_settings.DestroyWindow();
        break;
      } else {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }
  }

  return 0;
}

}  // namespace webdriver
