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

#include "SetTimeoutsCommandHandler.h"

#include "../../utils/WebDriverConstants.h"
#include "../../webdriver-server/errorcodes.h"

#include "../IESession.h"

namespace webdriver {

SetTimeoutsCommandHandler::SetTimeoutsCommandHandler(void) {
}

SetTimeoutsCommandHandler::~SetTimeoutsCommandHandler(void) {
}

void SetTimeoutsCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  long long timeout = 0;
  ParametersMap::const_iterator timeout_parameter_iterator =
      command_parameters.begin();
  for (;
       timeout_parameter_iterator != command_parameters.end();
       ++timeout_parameter_iterator) {
    std::string timeout_type = timeout_parameter_iterator->first;
    Json::Value timeout_value = timeout_parameter_iterator->second;
    if (timeout_type == SCRIPT_TIMEOUT_NAME && timeout_value.isNull()) {
      // Special case for the script timeout, which is nullable.
      timeout = -1;
    } else {
      if (!timeout_value.isNumeric() ||
          !timeout_value.isIntegral()) {
        response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                                   "Timeout value for timeout type " + timeout_type + " must be an integer");
        return;
      }
      timeout = timeout_value.asInt64();
      if (timeout < 0 || timeout > MAX_SAFE_INTEGER) {
        response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                                   "Timeout value for timeout type " + timeout_type + " must be an integer between 0 and 2^53 - 1");
        return;
      }
    }
    int setting_type = 0;
    if (timeout_type == IMPLICIT_WAIT_TIMEOUT_NAME) {
      setting_type = SESSION_SETTING_IMPLICIT_WAIT_TIMEOUT;
    } else if (timeout_type == SCRIPT_TIMEOUT_NAME) {
      setting_type = SESSION_SETTING_SCRIPT_TIMEOUT;
    } else if (timeout_type == PAGE_LOAD_TIMEOUT_NAME) {
      setting_type = SESSION_SETTING_PAGE_LOAD_TIMEOUT;
    }
    ::SendMessage(executor.session_settings_window_handle(),
                  WD_SET_SESSION_SETTING,
                  setting_type,
                  reinterpret_cast<LPARAM>(&timeout));
  }
}

} // namespace webdriver
