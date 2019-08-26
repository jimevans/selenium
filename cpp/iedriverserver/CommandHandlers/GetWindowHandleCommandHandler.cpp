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

#include "GetWindowHandleCommandHandler.h"

#include "../../webdriver-server/errorcodes.h"

#include "../IESession.h"

namespace webdriver {

GetWindowHandleCommandHandler::GetWindowHandleCommandHandler(void) {
}

GetWindowHandleCommandHandler::~GetWindowHandleCommandHandler(void) {
}

void GetWindowHandleCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  std::string current_handle = executor.current_instance_id();
  BrowserInfo window_handles;
  int status_code = executor.GetInstance(current_handle, &window_handles);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(ERROR_NO_SUCH_WINDOW, "Window is closed");
  } else {
    response->SetSuccessResponse(current_handle);
  }
}

} // namespace webdriver
