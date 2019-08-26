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

#include "SwitchToWindowCommandHandler.h"

#include "../../webdriver-server/errorcodes.h"

#include "../IESession.h"

namespace webdriver {

SwitchToWindowCommandHandler::SwitchToWindowCommandHandler(void) {
}

SwitchToWindowCommandHandler::~SwitchToWindowCommandHandler(void) {
}

void SwitchToWindowCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  ParametersMap::const_iterator name_parameter_iterator = command_parameters.find("handle");
  if (name_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT, "Missing parameter: name");
    return;
  }
  else {
    std::string found_browser_handle = "";
    std::string desired_name = name_parameter_iterator->second.asString();

    unsigned int limit = 10;
    unsigned int tries = 0;
    do {
      tries++;

      std::vector<std::string> handle_list;
      executor.GetInstanceIdList(&handle_list);

      std::vector<std::string>::const_iterator it = handle_list.begin();
      for (; it != handle_list.end(); ++it) {
        if (*it == desired_name) {
          BrowserInfo window_handles;
          int status_code = executor.GetInstance(*it, &window_handles);
          if (status_code == WD_SUCCESS) {
            found_browser_handle = *it;
            break;
          }
        }
      }

      // Wait until new window name becomes available
      ::Sleep(100);
    } while (tries < limit && found_browser_handle.size() == 0);

    if (found_browser_handle == "") {
      response->SetErrorResponse(ERROR_NO_SUCH_WINDOW, "No window found");
      return;
    } else {
      // TODO: Reset the path to the focused frame before switching window context.

      IESession& mutable_executor = const_cast<IESession&>(executor);
      mutable_executor.set_current_instance_id(found_browser_handle);
      response->SetSuccessResponse(Json::Value::null);
    }
  }
}

} // namespace webdriver
