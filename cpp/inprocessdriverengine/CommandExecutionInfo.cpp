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

#include "CommandExecutionInfo.h"

#include "../utils/StringUtilities.h"
#include "../webdriver-server/command.h"
#include "../webdriver-server/response.h"

namespace webdriver {

CommandExecutionInfo::CommandExecutionInfo() : command_id_(""),
                                               command_status_(UNINITIALIZED),
                                               command_(nullptr),
                                               response_(nullptr) {
}

CommandExecutionInfo::~CommandExecutionInfo() {
}

void CommandExecutionInfo::Initialize(const std::string& serialized_command) {
  this->command_id_ = StringUtilities::CreateGuid();
  this->command_ = new Command();
  this->response_ = new Response();
  this->command_->Deserialize(serialized_command);
  this->command_status_ = READY;
}

std::string CommandExecutionInfo::Finalize() {
  std::string serialized_response = this->response_->Serialize();
  this->command_status_ = COMPLETE;
  this->command_id_ = "";
  delete this->command_;
  delete this->response_;
  return serialized_response;
}

}  // namespace webdriver
