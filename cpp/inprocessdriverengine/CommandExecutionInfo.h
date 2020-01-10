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

// Defines a response for use in the JSON Wire Protocol. The protocol is
// defined at http://code.google.com/p/selenium/wiki/JsonWireProtocol.

#ifndef WEBDRIVER_COMMANDEXECUTIONINFO_H_
#define WEBDRIVER_COMMANDEXECUTIONINFO_H_

#include <string>

namespace webdriver {

class Command;
class Response;

class CommandExecutionInfo {
public:
  CommandExecutionInfo(void);
  virtual ~CommandExecutionInfo(void);

  void Initialize(const std::string& serialized_command);
  std::string Finalize(void);
  
  void Reset(void);

  std::string command_id(void) const { return this->command_id_; }

  int command_status(void) const { return this->command_status_; }
  void set_command_status(const int status) { this->command_status_ = status; }

  Command* command(void) { return this->command_; }
  Response* response(void) { return this->response_; }

private:
  Command* command_;
  Response* response_;
  std::string command_id_;
  int command_status_;
};

}  // namespace webdriver

#endif  // WEBDRIVER_COMMANDEXECUTIONINFO_H_
