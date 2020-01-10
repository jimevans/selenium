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

#include "SessionCommandRepository.h"

#include "../webdriver-server/command_types.h"

#include "CommandHandlers/AcceptAlertCommandHandler.h"
#include "CommandHandlers/CloseWindowCommandHandler.h"
#include "CommandHandlers/DismissAlertCommandHandler.h"
#include "CommandHandlers/GetAlertTextCommandHandler.h"
#include "CommandHandlers/GetTimeoutsCommandHandler.h"
#include "CommandHandlers/GetWindowHandleCommandHandler.h"
#include "CommandHandlers/GetWindowHandlesCommandHandler.h"
#include "CommandHandlers/NewSessionCommandHandler.h"
#include "CommandHandlers/QuitCommandHandler.h"
#include "CommandHandlers/SendAlertTextCommandHandler.h"
#include "CommandHandlers/SetTimeoutsCommandHandler.h"
#include "CommandHandlers/SwitchToWindowCommandHandler.h"
#include "SessionCommandHandler.h"

namespace webdriver {

SessionCommandRepository::SessionCommandRepository(void) {
  this->PopulateCommandHandlers();
}

SessionCommandRepository::~SessionCommandRepository(void) {
}

bool SessionCommandRepository::IsValidCommand(
    const std::string& command_name) {
  std::map<std::string, CommandHandlerHandle>::const_iterator found_iterator =
      this->command_handlers_.find(command_name);

  return found_iterator != this->command_handlers_.end();
}

CommandHandlerHandle SessionCommandRepository::GetCommandHandler(
    const std::string& command_name) {
  std::map<std::string, CommandHandlerHandle>::const_iterator found_iterator =
      this->command_handlers_.find(command_name);

  if (found_iterator == this->command_handlers_.end()) {
    return nullptr;
  }

  return found_iterator->second;
}

void SessionCommandRepository::PopulateCommandHandlers() {
  this->command_handlers_[webdriver::CommandType::NoCommand] = CommandHandlerHandle(new SessionCommandHandler);
  this->command_handlers_[webdriver::CommandType::NewSession] = CommandHandlerHandle(new NewSessionCommandHandler);
  this->command_handlers_[webdriver::CommandType::Quit] = CommandHandlerHandle(new QuitCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetCurrentWindowHandle] = CommandHandlerHandle(new GetWindowHandleCommandHandler);
  this->command_handlers_[webdriver::CommandType::CloseWindow] = CommandHandlerHandle(new CloseWindowCommandHandler);
  this->command_handlers_[webdriver::CommandType::SwitchToWindow] = CommandHandlerHandle(new SwitchToWindowCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetWindowHandles] = CommandHandlerHandle(new GetWindowHandlesCommandHandler);
  this->command_handlers_[webdriver::CommandType::AcceptAlert] = CommandHandlerHandle(new AcceptAlertCommandHandler);
  this->command_handlers_[webdriver::CommandType::DismissAlert] = CommandHandlerHandle(new DismissAlertCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetAlertText] = CommandHandlerHandle(new GetAlertTextCommandHandler);
  this->command_handlers_[webdriver::CommandType::SendKeysToAlert] = CommandHandlerHandle(new SendAlertTextCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetTimeouts] = CommandHandlerHandle(new GetTimeoutsCommandHandler);
  this->command_handlers_[webdriver::CommandType::SetTimeouts] = CommandHandlerHandle(new SetTimeoutsCommandHandler);
}

} // namespace webdriver
