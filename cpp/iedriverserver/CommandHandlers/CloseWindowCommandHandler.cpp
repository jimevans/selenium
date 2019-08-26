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

#include "CloseWindowCommandHandler.h"

#include "../../webdriver-server/errorcodes.h"

#include "../IESession.h"

namespace webdriver {

CloseWindowCommandHandler::CloseWindowCommandHandler(void) {
}

CloseWindowCommandHandler::~CloseWindowCommandHandler(void) {
}

void CloseWindowCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  // The session should end if the user sends a quit command,
  // or if the user sends a close command with exactly 1 window
  // open, per spec. Removing the window from the managed browser
  // list depends on events, which may be asynchronous, so cache
  // the window count *before* closing the current window.
  std::vector<std::string> active_instances;
  executor.GetInstanceIdList(&active_instances);
  size_t current_window_count = active_instances.size();

  // TODO: Check HRESULT values for errors.
  BrowserInfo browser_info;
  int status_code = executor.GetCurrentInstance(&browser_info);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(ERROR_NO_SUCH_WINDOW, "Unable to get browser");
    return;
  }

  HWND alert_handle = NULL;
  executor.CloseInstance(browser_info.browser_id, &alert_handle);
  if (alert_handle == NULL) {
    while (executor.IsInstance(browser_info.browser_id)) {
      ::Sleep(10);
    }
  }

  Json::Value handles(Json::arrayValue);
  if (current_window_count == 1) {
    IESession& mutable_executor = const_cast<IESession&>(executor);
    mutable_executor.set_is_valid(false);
    response->SetSuccessResponse(handles);
    return;
  }

  std::vector<std::string> remaining_windows;
  executor.GetInstanceIdList(&remaining_windows);
  std::vector<std::string>::const_iterator it = remaining_windows.begin();
  for (; it != remaining_windows.end(); ++it) {
    handles.append(*it);
  }

  response->SetSuccessResponse(handles);
}

} // namespace webdriver
