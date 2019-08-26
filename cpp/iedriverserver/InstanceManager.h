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

#ifndef WEBDRIVER_IE_INSTANCEMANAGER_H_
#define WEBDRIVER_IE_INSTANCEMANAGER_H_

#include <map>
#include <string>

#include "../utils/messages.h"

namespace webdriver {

struct BrowserInfo;
class BrowserFactory;

class InstanceManager : public CWindowImpl<InstanceManager> {
public:
  InstanceManager();
  ~InstanceManager();

  BEGIN_MSG_MAP(InstanceManager)
    MESSAGE_HANDLER(WD_INIT, OnInit)
    MESSAGE_HANDLER(WD_REGISTER_INSTANCE, OnRegisterInstance)
    MESSAGE_HANDLER(WD_IS_VALID_INSTANCE, OnIsValidInstance)
    MESSAGE_HANDLER(WD_GET_INSTANCE_INFO, OnGetInstanceInfo)
    MESSAGE_HANDLER(WD_GET_INSTANCE_LIST, OnGetInstanceList)
    MESSAGE_HANDLER(WD_NOTIFY_INSTANCE_CLOSE, OnNotifyInstanceClose)
  END_MSG_MAP()

  LRESULT OnInit(UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam,
                 BOOL& bHandled);
  LRESULT OnRegisterInstance(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& bHandled);
  LRESULT OnIsValidInstance(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);
  LRESULT OnGetInstanceInfo(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);
  LRESULT OnGetInstanceList(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);
  LRESULT OnNotifyInstanceClose(UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& bHandled);

static HWND CreateManager(void);
static unsigned int WINAPI ThreadProc(LPVOID lpParameter);

private:
  void CopyBrowserInfo(const BrowserInfo* source, BrowserInfo* destination);

  int browser_attach_timeout_;
  std::map<std::string, BrowserInfo> instances_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_INSTANCEMANAGER_H_
