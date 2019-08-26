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

#ifndef WEBDRIVER_IE_SESSIONSETTINGS_H_
#define WEBDRIVER_IE_SESSIONSETTINGS_H_

#include <string>

#include "../utils/messages.h"

#include "ProxySettings.h"

namespace webdriver {

class SessionSettings : public CWindowImpl<SessionSettings> {
public:
  SessionSettings();
  ~SessionSettings();

  BEGIN_MSG_MAP(SessionSettings)
    MESSAGE_HANDLER(WD_GET_SESSION_SETTING, OnGetSessionSetting)
    MESSAGE_HANDLER(WD_SET_SESSION_SETTING, OnSetSessionSetting)
  END_MSG_MAP()

  LRESULT OnGetSessionSetting(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam,
                              BOOL& bHandled);

  LRESULT OnSetSessionSetting(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam,
                              BOOL& bHandled);

  unsigned long long implicit_wait_timeout(void) const {
    return this->implicit_wait_timeout_;
  }
  void set_implicit_wait_timeout(const unsigned long long timeout) {
    this->implicit_wait_timeout_ = timeout;
  }

  long long  async_script_timeout(void) const {
    return this->script_timeout_;
  }
  void set_async_script_timeout(const long long timeout) {
    this->script_timeout_ = timeout;
  }

  unsigned long long page_load_timeout(void) const {
    return this->page_load_timeout_;
  }
  void set_page_load_timeout(const unsigned long long timeout) {
    this->page_load_timeout_ = timeout;
  }

  std::string unexpected_alert_behavior(void) const {
    return this->unhandled_prompt_behavior_;
  }
  void set_unexpected_alert_behavior(
    const std::string& unexpected_alert_behavior) {
    this->unhandled_prompt_behavior_ = unexpected_alert_behavior;
  }

  std::string page_load_strategy(void) const {
    return this->page_load_strategy_;
  }
  void set_page_load_strategy(const std::string& page_load_strategy) {
    this->page_load_strategy_ = page_load_strategy;
  }

  bool use_strict_file_interactability(void) const {
    return this->use_strict_file_interactability_;
  }
  void set_use_strict_file_interactability(
    const bool use_strict_file_interactability) {
    this->use_strict_file_interactability_ = use_strict_file_interactability;
  }

  static HWND CreateInstance(void);
  static unsigned int WINAPI ThreadProc(LPVOID lpParameter);

private:
  int browser_attach_timeout_;
  bool use_strict_file_interactability_;
  unsigned long long implicit_wait_timeout_;
  unsigned long long script_timeout_;
  unsigned long long page_load_timeout_;
  std::string unhandled_prompt_behavior_;
  std::string page_load_strategy_;
  ProxySettings proxy_settings_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_SESSIONSETTINGS_H_

