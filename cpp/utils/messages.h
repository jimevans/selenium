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

#define WD_INIT WM_APP + 1
#define WD_SET_COMMAND WM_APP + 2
#define WD_EXEC_COMMAND WM_APP + 3
#define WD_GET_RESPONSE_LENGTH WM_APP + 4
#define WD_GET_RESPONSE WM_APP + 5
#define WD_WAIT WM_APP + 6
#define WD_ABORT_COMMAND WM_APP + 7
#define WD_BROWSER_NEW_WINDOW WM_APP + 8
#define WD_BROWSER_QUIT WM_APP + 9
#define WD_NEW_HTML_DIALOG WM_APP + 10
#define WD_GET_QUIT_STATUS WM_APP + 11
#define WD_QUIT WM_APP + 12
#define WD_SHUTDOWN WM_APP + 13

#define WD_IS_BROWSER_READY WM_APP + 100
#define WD_REGISTER_INSTANCE WM_APP + 200
#define WD_IS_VALID_INSTANCE WM_APP + 201
#define WD_GET_INSTANCE_INFO WM_APP + 202
#define WD_GET_INSTANCE_LIST WM_APP + 203
#define WD_NOTIFY_INSTANCE_CLOSE WM_APP + 204
#define WD_REACQUIRE_BROWSER WM_APP + 205

#define WD_GET_SESSION_SETTING WM_APP + 300
#define WD_SET_SESSION_SETTING WM_APP + 301

#define SESSION_SETTING_IMPLICIT_WAIT_TIMEOUT 0
#define SESSION_SETTING_SCRIPT_TIMEOUT 1
#define SESSION_SETTING_PAGE_LOAD_TIMEOUT 2
#define SESSION_SETTING_UNHANDLED_PROMPT_BEHAVIOR 3
#define SESSION_SETTING_PAGE_LOAD_STRATEGY 4
#define SESSION_SETTING_STRICT_FILE_INTERACTABLILITY 5
#define SESSION_SETTING_PROXY 6
#define SESSION_SETTING_ACTION_SIMULATOR_TYPE 7

#define COPYDATA_RESPONSE 0
#define COPYDATA_SAME_WINDOW_PROCESS_ID_LIST 1
#define COPYDATA_NEW_WINDOW_PROCESS_ID_LIST 2

#define SEND_MESSAGE_ACTION_SIMULATOR 0
#define SEND_INPUT_ACTION_SIMULATOR 1
#define JAVASCRIPT_ACTION_SIMULATOR 2
