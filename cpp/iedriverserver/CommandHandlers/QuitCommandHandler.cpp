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

#include "QuitCommandHandler.h"

#include "../../webdriver-server/errorcodes.h"

#include "../BrowserHost.h"
#include "../IESession.h"

namespace webdriver {

QuitCommandHandler::QuitCommandHandler(void) {
}

QuitCommandHandler::~QuitCommandHandler(void) {
}

void QuitCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  std::vector<std::string> managed_browser_handles;
  executor.GetInstanceIdList(&managed_browser_handles);

  std::vector<std::string>::iterator it = managed_browser_handles.begin();
  for (; it != managed_browser_handles.end(); ++it) {
    BrowserInfo window_handles;
    executor.GetInstance(*it, &window_handles);
    HWND browser_host_handle = window_handles.browser_host_window_handle;

    HWND alert_handle = NULL;
    executor.CloseInstance(*it, &alert_handle);
    if (alert_handle != NULL) {
      std::string alert_text = "";
      executor.HandleUnexpectedAlert(alert_handle, true, &alert_text);
      
      while (executor.IsInstance(window_handles.browser_id)) {
        ::Sleep(10);
      }
    }
  }

  // Calling quit will always result in an invalid session.
  IESession& mutable_executor = const_cast<IESession&>(executor);
  mutable_executor.set_is_valid(false);
  response->SetSuccessResponse(Json::Value::null);
}

} // namespace webdriver
