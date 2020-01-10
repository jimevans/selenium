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

#ifndef WEBDRIVER_IE_IESESSION_H_
#define WEBDRIVER_IE_IESESSION_H_

#include <ctime>
#include <map>
#include <string>
#include <vector>

#include "../utils/messages.h"
#include "../webdriver-server/session.h"

#include "BrowserInfo.h"

struct IHTMLDocument2;

namespace webdriver {

// Structure to be used for storing session initialization parameters
struct SessionParameters {
  int port;
};

class BrowserFactory;
class BrowserHost;
class SessionCommandRepository;

class IESession : public Session {
 public:
  IESession();
  virtual ~IESession(void);

  void Initialize(void* init_params);
  void ShutDown(void);
  bool ExecuteCommand(const std::string& command_name,
                      const std::string& url_parameters,
                      const std::string& parameters,
                      std::string* serialized_response);

  int CreateNewBrowser(std::string* error_message);
  void GetInstanceIdList(std::vector<std::string>* instance_id_list) const;
  int GetInstance(const std::string& instance_id,
                  BrowserInfo* window_handles) const;
  int GetCurrentInstance(BrowserInfo* window_handles) const;
  bool IsInstance(const std::string& instance_id) const;
  void CloseInstance(const std::string& instance_id,
                     HWND* alert_handle) const;
  void UpdateInstanceSettings(void) const;

  bool IsAlertActive(const HWND content_window_handle,
                     HWND* alert_handle) const;
  bool HandleUnexpectedAlert(const HWND alert_handle,
                             const bool force_use_dismiss,
                             std::string* alert_text) const;

  std::string session_id(void) const { return this->session_id_; }
  int port(void) const { return this->port_; }
  HWND session_settings_window_handle(void) const {
    return this->session_settings_window_handle_;
  }

  std::string current_instance_id(void) const {
     return this->current_instance_id_;
  }
  void set_current_instance_id(const std::string& instance_id) {
    this->current_instance_id_ = instance_id;
  }

  bool is_valid(void) const { return this->is_valid_; }
  void set_is_valid(const bool is_valid) { this->is_valid_ = is_valid; }

  BrowserFactory* browser_factory(void) const { return this->factory_; }
private:
  void InitializeLocalCommandNames(void);
  bool IsLocalCommand(const std::string& command_name);
  bool IsNavigationCommand(const std::string& command_name);
  bool IsScriptCommand(const std::string& command_name);
  bool DispatchInProcessCommand(const std::string& serialized_command,
                                std::string* serialized_response);
  void PrepareInProcessCommand(HWND host_window_handle,
                               const std::string& serialized_command);
  bool WaitForInProcessCommandComplete(HWND host_window_handle,
                                       HWND content_window_handle, 
                                       HWND* alert_handle);
  void GetInProcessCommandResult(HWND host_window_handle,
                                 std::string* serialized_response);
  int GetCommandTimeout(const int timeout_type);

  int port_;
  int browser_attach_timeout_;
  bool is_valid_;
  bool is_pending_file_selection_;
  clock_t command_timeout_;
  std::string session_id_;
  std::string current_instance_id_;
  BrowserFactory* factory_;
  SessionCommandRepository* command_handlers_;
  std::vector<std::string> local_command_names_;
  std::vector<std::string> navigation_command_names_;
  std::vector<std::string> script_command_names_;

  HWND instance_manager_window_handle_;
  HWND session_settings_window_handle_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_IESESSION_H_

