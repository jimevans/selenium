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

#include "InProcessCommandRepository.h"

#include "../webdriver-server/command_types.h"

#include "CommandHandlers/ClickElementCommandHandler.h"
#include "CommandHandlers/ExecuteScriptCommandHandler.h"
#include "CommandHandlers/FindChildElementCommandHandler.h"
#include "CommandHandlers/FindChildElementsCommandHandler.h"
#include "CommandHandlers/FindElementCommandHandler.h"
#include "CommandHandlers/FindElementsCommandHandler.h"
#include "CommandHandlers/GetCurrentUrlCommandHandler.h"
#include "CommandHandlers/GetElementTextCommandHandler.h"
#include "CommandHandlers/GetTitleCommandHandler.h"
#include "CommandHandlers/GoBackCommandHandler.h"
#include "CommandHandlers/GoForwardCommandHandler.h"
#include "CommandHandlers/GoToUrlCommandHandler.h"
#include "CommandHandlers/IsElementSelectedCommandHandler.h"
#include "CommandHandlers/SwitchToFrameCommandHandler.h"
#include "CommandHandlers/SwitchToParentFrameCommandHandler.h"
#include "InProcessCommandHandler.h"

namespace webdriver {

InProcessCommandRepository::InProcessCommandRepository(void) {
  this->PopulateCommandHandlers();
}

InProcessCommandRepository::~InProcessCommandRepository(void) {
}

bool InProcessCommandRepository::IsValidCommand(
    const std::string& command_name) {
  std::map<std::string, CommandHandlerHandle>::const_iterator found_iterator =
    this->command_handlers_.find(command_name);

  return found_iterator != this->command_handlers_.end();
}

CommandHandlerHandle InProcessCommandRepository::GetCommandHandler(
    const std::string& command_name) {
  std::map<std::string, CommandHandlerHandle>::const_iterator found_iterator =
    this->command_handlers_.find(command_name);

  if (found_iterator == this->command_handlers_.end()) {
    return nullptr;
  }

  return found_iterator->second;
}

void InProcessCommandRepository::PopulateCommandHandlers() {
  this->command_handlers_[webdriver::CommandType::NoCommand] = CommandHandlerHandle(new InProcessCommandHandler);
  this->command_handlers_[webdriver::CommandType::Get] = CommandHandlerHandle(new GoToUrlCommandHandler);
  this->command_handlers_[webdriver::CommandType::GoBack] = CommandHandlerHandle(new GoBackCommandHandler);
  this->command_handlers_[webdriver::CommandType::GoForward] = CommandHandlerHandle(new GoForwardCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetTitle] = CommandHandlerHandle(new GetTitleCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetCurrentUrl] = CommandHandlerHandle(new GetCurrentUrlCommandHandler);
  this->command_handlers_[webdriver::CommandType::ExecuteScript] = CommandHandlerHandle(new ExecuteScriptCommandHandler);
  this->command_handlers_[webdriver::CommandType::FindElement] = CommandHandlerHandle(new FindElementCommandHandler);
  this->command_handlers_[webdriver::CommandType::FindElements] = CommandHandlerHandle(new FindElementsCommandHandler);
  this->command_handlers_[webdriver::CommandType::FindChildElement] = CommandHandlerHandle(new FindChildElementCommandHandler);
  this->command_handlers_[webdriver::CommandType::FindChildElements] = CommandHandlerHandle(new FindChildElementsCommandHandler);
  this->command_handlers_[webdriver::CommandType::ClickElement] = CommandHandlerHandle(new ClickElementCommandHandler);
  this->command_handlers_[webdriver::CommandType::IsElementSelected] = CommandHandlerHandle(new IsElementSelectedCommandHandler);
  this->command_handlers_[webdriver::CommandType::GetElementText] = CommandHandlerHandle(new GetElementTextCommandHandler);
  this->command_handlers_[webdriver::CommandType::SwitchToFrame] = CommandHandlerHandle(new SwitchToFrameCommandHandler);
  this->command_handlers_[webdriver::CommandType::SwitchToParentFrame] = CommandHandlerHandle(new SwitchToParentFrameCommandHandler);
}

} // namespace webdriver
