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

#include "GetWindowHandlesCommandHandler.h"

#include "../IESession.h"

namespace webdriver {

GetWindowHandlesCommandHandler::GetWindowHandlesCommandHandler(void) {
}

GetWindowHandlesCommandHandler::~GetWindowHandlesCommandHandler(void) {
}

void GetWindowHandlesCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  std::vector<std::string> handle_list;
  executor.GetInstanceIdList(&handle_list);

  Json::Value handles(Json::arrayValue);
  std::vector<std::string>::const_iterator it = handle_list.begin();
  for (; it != handle_list.end(); ++it) {
    handles.append(*it);
  }

  response->SetSuccessResponse(handles);
}

} // namespace webdriver
