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

#include "IESession.h"

#include <PathCch.h>

#include "../utils/messages.h"
#include "../utils/RegistryUtilities.h"
#include "../utils/StringUtilities.h"
#include "../utils/WebDriverConstants.h"
#include "../webdriver-server/command.h"
#include "../webdriver-server/errorcodes.h"
#include "../webdriver-server/command_handler.h"
#include "../webdriver-server/command_types.h"
#include "../webdriver-server/response.h"

#include "Alert.h"
#include "BrowserFactory.h"
#include "BrowserHost.h"
#include "InstanceManager.h"
#include "SessionCommandHandler.h"
#include "SessionCommandRepository.h"
#include "SessionSettings.h"

namespace webdriver {

IESession::IESession() {
}

IESession::~IESession(void) {
}

void IESession::Initialize(void* init_params) {
  SessionParameters* params = 
      reinterpret_cast<SessionParameters*>(init_params);

  this->port_ = params->port;

  std::string session_id = StringUtilities::CreateGuid();
  this->session_id_ = session_id;
  this->is_valid_ = true;
  this->command_timeout_ = 0;

  this->current_instance_id_ = "";

  this->InitializeLocalCommandNames();
  this->factory_ = new BrowserFactory();
  this->command_handlers_ = new SessionCommandRepository();
  this->instance_manager_window_handle_ = InstanceManager::CreateManager();
  this->session_settings_window_handle_ = SessionSettings::CreateInstance();
}

void IESession::ShutDown(void) {
  ::SendMessage(this->instance_manager_window_handle_, WM_DESTROY, NULL, NULL);
  ::SendMessage(this->session_settings_window_handle_, WM_DESTROY, NULL, NULL);
}

bool IESession::ExecuteCommand(const std::string& command_name,
                               const std::string& url_parameters,
                               const std::string& parameters,
                               std::string* serialized_response) {
  std::string serialized_command = "{ \"name\": \"";
  serialized_command.append(command_name);
  serialized_command.append("\"");
  serialized_command.append(", \"locator\": ");
  serialized_command.append(url_parameters);
  serialized_command.append(", \"parameters\": ");
  serialized_command.append(parameters);
  serialized_command.append(" }");

  if (!this->IsLocalCommand(command_name)) {
    if (this->IsNavigationCommand(command_name)) {
      int page_load_timeout = this->GetCommandTimeout(
          SESSION_SETTING_PAGE_LOAD_TIMEOUT);
      if (page_load_timeout >= 0) {
        this->command_timeout_ =
            clock() + (page_load_timeout / 1000 * CLOCKS_PER_SEC);
      }
    }

    if (this->IsScriptCommand(command_name)) {
      int script_timeout = this->GetCommandTimeout(
          SESSION_SETTING_SCRIPT_TIMEOUT);
      if (script_timeout >= 0) {
        this->command_timeout_ =
            clock() + (script_timeout / 1000 * CLOCKS_PER_SEC);
      }
    }

    this->DispatchInProcessCommand(serialized_command, serialized_response);
    this->command_timeout_ = 0;
    return true;
  }

  CommandHandlerHandle command_handler =
      this->command_handlers_->GetCommandHandler(command_name);
  Command command;
  command.Deserialize(serialized_command);
  Response response;
  command_handler->Execute(*this, command, &response);
  *serialized_response = response.Serialize();
  return this->is_valid_;
}

int IESession::CreateNewBrowser(std::string* error_message) {
  DWORD process_id = this->factory_->LaunchBrowserProcess(error_message);
  if (process_id == NULL) {
    *error_message = "Could not retrieve ID of browser process";
    return ENOSUCHDRIVER;
  }

  std::string browser_id = BrowserHost::CreateInstance(
      process_id,
      this->instance_manager_window_handle_,
      this->session_settings_window_handle_);
  if (browser_id == "") {
    *error_message = "Could not attach to browser instance";
    return ENOSUCHDRIVER;
  }

  if (this->current_instance_id_.size() == 0) {
    this->current_instance_id_ = browser_id;
  }
  return WD_SUCCESS;
}

void IESession::GetInstanceIdList(
    std::vector<std::string>* instance_id_list) const {
  ::SendMessage(this->instance_manager_window_handle_,
                WD_GET_INSTANCE_LIST,
                NULL,
                reinterpret_cast<LPARAM>(instance_id_list));
}

int IESession::GetInstance(const std::string& instance_id,
                           BrowserInfo* window_handles) const {
  if (!this->is_valid_) {
    return ENOSUCHDRIVER;
  }

  if (instance_id == "") {
    return ENOSUCHWINDOW;
  }

  LRESULT is_instance = ::SendMessage(this->instance_manager_window_handle_,
                                      WD_IS_VALID_INSTANCE,
                                      NULL,
                                      reinterpret_cast<LPARAM>(&instance_id));
  if (is_instance == 0) {
    return ENOSUCHWINDOW;
  }

  window_handles->browser_id = instance_id;
  ::SendMessage(this->instance_manager_window_handle_,
                WD_GET_INSTANCE_INFO,
                NULL,
                reinterpret_cast<LPARAM>(window_handles));
  return WD_SUCCESS;
}

int IESession::GetCurrentInstance(BrowserInfo* instance) const {
  return this->GetInstance(this->current_instance_id_, instance);
}

void IESession::CloseInstance(const std::string& instance_id,
                              HWND* alert_handle) const {
  *alert_handle = NULL;
  BrowserInfo window_handles;
  this->GetInstance(instance_id, &window_handles);
  HWND browser_host_handle = window_handles.browser_host_window_handle;
  ::PostMessage(browser_host_handle,
                WD_QUIT,
                reinterpret_cast<WPARAM>(this->instance_manager_window_handle_),
                NULL);
  while (this->IsInstance(instance_id)) {
    ::Sleep(10);
    if (this->IsAlertActive(window_handles.content_window_handle,
                            alert_handle)) {
      break;
    }
  }
}

bool IESession::IsInstance(const std::string& instance_id) const {
  LRESULT is_instance = ::SendMessage(this->instance_manager_window_handle_,
                                      WD_IS_VALID_INSTANCE,
                                      NULL,
                                      reinterpret_cast<LPARAM>(&instance_id));
  return is_instance != 0;
}

bool IESession::IsAlertActive(const HWND content_window_handle,
                              HWND* alert_handle) const {
  DWORD process_id = 0;
  ::GetWindowThreadProcessId(content_window_handle, &process_id);
  if (process_id == 0) {
    return false;
  }

  HWND active_dialog_handle = NULL;
  ProcessWindowInfo process_win_info;
  process_win_info.dwProcessId = process_id;
  process_win_info.hwndBrowser = NULL;
  ::EnumWindows(&BrowserFactory::FindDialogWindowForProcess,
                reinterpret_cast<LPARAM>(&process_win_info));
  if (process_win_info.hwndBrowser != NULL) {
    active_dialog_handle = process_win_info.hwndBrowser;
  }
  if (active_dialog_handle != NULL) {
    // Found a window handle, make sure it's an actual alert,
    // and not a showModalDialog() window.
    std::vector<char> window_class_name(34);
    ::GetClassNameA(active_dialog_handle, &window_class_name[0], 34);
    if (strcmp(ALERT_WINDOW_CLASS, &window_class_name[0]) == 0) {
      *alert_handle = active_dialog_handle;
      return true;
    } else if (strcmp(SECURITY_DIALOG_WINDOW_CLASS,
                      &window_class_name[0]) == 0) {
      *alert_handle = active_dialog_handle;
      return true;
    } else {
      // LOG(WARN) << "Found alert handle does not have a window class consistent with an alert";
    }
  }

  return false;
}

bool IESession::HandleUnexpectedAlert(HWND alert_handle,
                                      bool force_use_dismiss,
                                      std::string* alert_text) const {
  clock_t end = clock() + 5 * CLOCKS_PER_SEC;
  bool is_visible = (::IsWindowVisible(alert_handle) == TRUE);
  while (!is_visible && clock() < end) {
    ::Sleep(50);
    is_visible = (::IsWindowVisible(alert_handle) == TRUE);
  }
  Alert dialog(alert_handle);
  *alert_text = dialog.GetText();
  if (!dialog.is_standard_alert()) {
    // The dialog was non-standard. The most common case of this is
    // an onBeforeUnload dialog, which must be accepted to continue.
    dialog.Accept();
    return false;
  }

  // CONSIDER: Should this be cached? It is set once for the session,
  // is never changed, and does not need to be accessed across the
  // process boundary. Not doing so here, because it is likely a
  // microoptimization.
  std::string unhandled_prompt_behavior = "";
  ::SendMessage(this->session_settings_window_handle_,
                WD_GET_SESSION_SETTING,
                SESSION_SETTING_UNHANDLED_PROMPT_BEHAVIOR,
                reinterpret_cast<LPARAM>(&unhandled_prompt_behavior));
  if (unhandled_prompt_behavior == ACCEPT_UNEXPECTED_ALERTS ||
      unhandled_prompt_behavior == ACCEPT_AND_NOTIFY_UNEXPECTED_ALERTS) {
    dialog.Accept();
  } else if (
      unhandled_prompt_behavior.size() == 0 ||
      unhandled_prompt_behavior == DISMISS_UNEXPECTED_ALERTS ||
      unhandled_prompt_behavior == DISMISS_AND_NOTIFY_UNEXPECTED_ALERTS ||
      force_use_dismiss) {
    // If a quit command was issued, we should not ignore an unhandled
    // alert, even if the alert behavior is set to "ignore".
    if (dialog.is_standard_alert() || dialog.is_security_alert()) {
      dialog.Dismiss();
    } else {
      // The dialog was non-standard. The most common case of this is
      // an onBeforeUnload dialog, which must be accepted to continue.
      dialog.Accept();
    }
  }

  bool is_notify_unexpected_alert =
      unhandled_prompt_behavior.size() == 0 ||
      unhandled_prompt_behavior == IGNORE_UNEXPECTED_ALERTS ||
      unhandled_prompt_behavior == DISMISS_AND_NOTIFY_UNEXPECTED_ALERTS ||
      unhandled_prompt_behavior == ACCEPT_AND_NOTIFY_UNEXPECTED_ALERTS;
  is_notify_unexpected_alert = is_notify_unexpected_alert &&
                               dialog.is_standard_alert();
  return is_notify_unexpected_alert;
}

bool IESession::DispatchInProcessCommand(const std::string& serialized_command,
                                         std::string* serialized_response) {
  // Sending a command consists of four actions:
  // 1. Setting the command to be executed
  // 2. Executing the command
  // 3. Waiting for the response to be populated
  // 4. Retrieving the response
  // We rely on a poll-and-pull model rather than implementing a push
  // model from the in-process component because of COM threading.
  BrowserInfo info;
  int status_code = this->GetCurrentInstance(&info);
  if (status_code != WD_SUCCESS) {
    return false;
  }
  HWND host_window_handle = info.browser_host_window_handle;
  HWND content_window_handle = info.content_window_handle;
  HWND alert_handle = NULL;
  if (this->IsAlertActive(content_window_handle, &alert_handle)) {
    std::string alert_text = "";
    if (this->HandleUnexpectedAlert(alert_handle, false, &alert_text)) {
      Response alert_response;
      alert_response.SetErrorResponse(
          EUNEXPECTEDALERTOPEN,
          "Modal dialog present with text: " + alert_text);
      alert_response.AddAdditionalData("text", alert_text);
      *serialized_response = alert_response.Serialize();
    }
  }
  this->PrepareInProcessCommand(host_window_handle, serialized_command);
  ::PostMessage(host_window_handle, WD_EXEC_COMMAND, NULL, NULL);
  bool is_command_complete =
      this->WaitForInProcessCommandComplete(host_window_handle,
                                            content_window_handle,
                                            &alert_handle);
  if (is_command_complete) {
    this->GetInProcessCommandResult(host_window_handle, serialized_response);
  } else {
    Response timeout_response;
    timeout_response.SetErrorResponse(ETIMEOUT, "Timed out executing command");
    *serialized_response = timeout_response.Serialize();
    return false;
  }
  return true;
}

void IESession::PrepareInProcessCommand(
    HWND host_window_handle,
    const std::string& serialized_command) {
  ::SendMessage(host_window_handle,
                WD_SET_COMMAND,
                NULL,
                reinterpret_cast<LPARAM>(&serialized_command));
}

bool IESession::WaitForInProcessCommandComplete(HWND host_window_handle,
                                                HWND content_window_handle,
                                                HWND* alert_handle) {
  size_t response_length =
      static_cast<size_t>(::SendMessage(host_window_handle,
                                        WD_GET_RESPONSE_LENGTH,
                                        NULL,
                                        NULL));
  int counter = 0;
  bool is_timed_out = this->command_timeout_ > 0 &&
                      this->command_timeout_ < clock();
  while (response_length == 0 && !is_timed_out) {
    ::Sleep(10);
    response_length = static_cast<size_t>(::SendMessage(host_window_handle,
                                                        WD_GET_RESPONSE_LENGTH,
                                                        NULL,
                                                        NULL));
    // Check for active alerts on a much slower cadence than every
    // hundredth of a second. Reset the counter at each interval.
    if (counter % 50 == 0) {
      counter = 0;
      if (this->IsAlertActive(content_window_handle, alert_handle)) {
        ::SendMessage(host_window_handle, WD_ABORT_COMMAND, NULL, NULL);
      }
    }
    counter++;
    is_timed_out = this->command_timeout_ > 0 &&
                   this->command_timeout_ < clock();
  }

  if (is_timed_out) {
    ::SendMessage(host_window_handle, WD_ABORT_COMMAND, NULL, NULL);
    return false;
  }
  return true;
}

void IESession::GetInProcessCommandResult(HWND host_window_handle,
                                          std::string* serialized_response) {
  size_t response_length =
      static_cast<size_t>(::SendMessage(host_window_handle,
                                        WD_GET_RESPONSE_LENGTH,
                                        NULL,
                                        NULL));
  std::vector<char> buffer(response_length + 1);
  ::SendMessage(host_window_handle,
                WD_GET_RESPONSE,
                NULL,
                reinterpret_cast<LPARAM>(&buffer[0]));
  std::string response(&buffer[0]);
  *serialized_response = response;
}

int IESession::GetCommandTimeout(const int timeout_type) {
  int timeout = 0;
  ::SendMessage(this->session_settings_window_handle_,
                WD_GET_SESSION_SETTING,
                static_cast<WPARAM>(timeout_type),
                reinterpret_cast<LPARAM>(&timeout));
  return timeout;
}

bool IESession::IsLocalCommand(const std::string& command_name) {
  std::vector<std::string>::iterator it =
      std::find(this->local_command_names_.begin(),
                this->local_command_names_.end(),
                command_name);
  return it != this->local_command_names_.end();
}

bool IESession::IsNavigationCommand(const std::string& command_name) {
  std::vector<std::string>::iterator it =
      std::find(this->navigation_command_names_.begin(),
                this->navigation_command_names_.end(),
                command_name);
  return it != this->navigation_command_names_.end();
}

bool IESession::IsScriptCommand(const std::string& command_name) {
  std::vector<std::string>::iterator it =
      std::find(this->script_command_names_.begin(),
                this->script_command_names_.end(),
                command_name);
  return it != this->script_command_names_.end();
}

void IESession::InitializeLocalCommandNames() {
  this->local_command_names_.push_back(CommandType::NewSession);
  this->local_command_names_.push_back(CommandType::Quit);
  this->local_command_names_.push_back(CommandType::CloseWindow);
  this->local_command_names_.push_back(CommandType::GetCurrentWindowHandle);
  this->local_command_names_.push_back(CommandType::GetWindowHandles);
  this->local_command_names_.push_back(CommandType::SwitchToWindow);
  this->local_command_names_.push_back(CommandType::AcceptAlert);
  this->local_command_names_.push_back(CommandType::DismissAlert);
  this->local_command_names_.push_back(CommandType::GetAlertText);
  this->local_command_names_.push_back(CommandType::SendKeysToAlert);
  this->local_command_names_.push_back(CommandType::GetTimeouts);
  this->local_command_names_.push_back(CommandType::SetTimeouts);
  this->local_command_names_.push_back(CommandType::Screenshot);
  this->local_command_names_.push_back(CommandType::ElementScreenshot);

  this->navigation_command_names_.push_back(CommandType::Get);
  this->navigation_command_names_.push_back(CommandType::GoBack);
  this->navigation_command_names_.push_back(CommandType::GoForward);
  this->navigation_command_names_.push_back(CommandType::Refresh);

  this->script_command_names_.push_back(CommandType::ExecuteScript);
  this->script_command_names_.push_back(CommandType::ExecuteAsyncScript);
}

} // namespace webdriver
