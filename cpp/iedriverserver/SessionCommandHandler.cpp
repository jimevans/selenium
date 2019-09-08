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

#include "SessionCommandHandler.h"

#include "../webdriver-server/command_handler.h"
#include "../webdriver-server/errorcodes.h"
#include "../webdriver-server/logging.h"

namespace webdriver {

SessionCommandHandler::SessionCommandHandler() {
}

SessionCommandHandler::~SessionCommandHandler() {
}

void SessionCommandHandler::ExecuteInternal(
    const IESession& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  LOG(TRACE) << "Entering DriverCommandHandler::ExecuteInternal";
  response->SetErrorResponse(501, "Command not implemented");
}

} // namespace webdriver
