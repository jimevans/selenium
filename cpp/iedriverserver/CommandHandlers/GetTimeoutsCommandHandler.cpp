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

#include "GetTimeoutsCommandHandler.h"

#include "../../utils/WebDriverConstants.h"
#include "../../webdriver-server/errorcodes.h"

#include "../IESession.h"

namespace webdriver {

GetTimeoutsCommandHandler::GetTimeoutsCommandHandler(void) {
}

GetTimeoutsCommandHandler::~GetTimeoutsCommandHandler(void) {
}

void GetTimeoutsCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  Json::Value response_value;

  unsigned long long implicit_wait_timeout = 0;
  ::SendMessage(executor.session_settings_window_handle(),
                WD_GET_SESSION_SETTING,
                SESSION_SETTING_IMPLICIT_WAIT_TIMEOUT,
                reinterpret_cast<LPARAM>(&implicit_wait_timeout));
  response_value[IMPLICIT_WAIT_TIMEOUT_NAME] = implicit_wait_timeout;

  unsigned long long page_load_timeout = 0;
  ::SendMessage(executor.session_settings_window_handle(),
                WD_GET_SESSION_SETTING,
                SESSION_SETTING_PAGE_LOAD_TIMEOUT,
                reinterpret_cast<LPARAM>(&page_load_timeout));
  response_value[PAGE_LOAD_TIMEOUT_NAME] = page_load_timeout;

  long long script_timeout = 0;
  ::SendMessage(executor.session_settings_window_handle(),
                WD_GET_SESSION_SETTING,
                SESSION_SETTING_SCRIPT_TIMEOUT,
                reinterpret_cast<LPARAM>(&script_timeout));
  if (script_timeout < 0) {
    response_value[SCRIPT_TIMEOUT_NAME] = Json::Value::null;
  } else {
    response_value[SCRIPT_TIMEOUT_NAME] = script_timeout;
  }

  response->SetSuccessResponse(response_value);
}

} // namespace webdriver
