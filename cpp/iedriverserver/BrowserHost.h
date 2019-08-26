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

#ifndef WEBDRIVER_IE_BROWSERHOST_H_
#define WEBDRIVER_IE_BROWSERHOST_H_

#include <string>
#include <vector>

#include <atlctl.h>

#include "../utils/messages.h"

struct IWebBrowser2;
struct IHTMLDocument2;

using namespace ATL;

namespace webdriver {

class BrowserHost : public CWindowImpl<BrowserHost> {
public:
 BrowserHost();

 BEGIN_MSG_MAP(BrowserHost)
   MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
   MESSAGE_HANDLER(WD_SET_COMMAND, OnSetCommand)
   MESSAGE_HANDLER(WD_EXEC_COMMAND, OnExecCommand)
   MESSAGE_HANDLER(WD_GET_RESPONSE_LENGTH, OnGetResponseLength)
   MESSAGE_HANDLER(WD_GET_RESPONSE, OnGetResponse)
   MESSAGE_HANDLER(WD_ABORT_COMMAND, OnAbortCommand)
   MESSAGE_HANDLER(WD_IS_BROWSER_READY, OnIsBrowserReady)
   MESSAGE_HANDLER(WD_REACQUIRE_BROWSER, OnReacquireBrowser)
   MESSAGE_HANDLER(WD_BROWSER_NEW_WINDOW, OnBrowserNewWindow)
   MESSAGE_HANDLER(WD_QUIT, OnQuit)
 END_MSG_MAP()

 LRESULT OnCopyData(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
 LRESULT OnQuit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
 LRESULT OnSetCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
 LRESULT OnExecCommand(UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam,
                       BOOL& bHandled);
 LRESULT OnGetResponseLength(UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam,
                             BOOL& bHandled);
 LRESULT OnGetResponse(UINT uMsg,
                       WPARAM wParam,
                       LPARAM lParam,
                       BOOL& bHandled);
 LRESULT OnAbortCommand(UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam,
                        BOOL& bHandled);
 LRESULT OnIsBrowserReady(UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam,
                          BOOL& bHandled);
 LRESULT OnReacquireBrowser(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);
 LRESULT OnBrowserNewWindow(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled);

 static std::string CreateInstance(const DWORD process_id,
                                   const HWND session_window_handle,
                                   const HWND session_settings_handle);

 static unsigned int WINAPI ThreadProc(LPVOID lpParameter);
 static unsigned int WINAPI SelfMessageThreadProc(LPVOID lpParameter);

 bool Initialize(const DWORD process_id,
                 const HWND session_window_handle,
                 const HWND settings_window_handle);
 void Dispose(void);

private:
  void GetInProcessDriverLibraryPath(const DWORD process_id,
                                     std::string* library_path);
  HRESULT GetDocumentFromWindowHandle(const HWND window_handle,
                                      IHTMLDocument2** document);
  HRESULT GetBrowserFromDocument(IHTMLDocument2* document);
  HRESULT StartDiagnosticsMode(IHTMLDocument2* document);
  void PostMessageToSelf(const UINT msg);
  void GetNewBrowserProcessIds(std::vector<DWORD>* new_process_ids);
  bool IsBrowserProcessInitialized(const DWORD process_id,
                                   HWND* document_handle);

  HWND in_proc_executor_handle_;
  HWND content_window_handle_;
  HWND top_level_window_handle_;
  HWND tab_window_handle_;
  HWND notify_window_handle_;
  HWND settings_window_handle_;
  bool is_command_aborted_;
  bool is_explicit_close_requested_;
  bool is_ignoring_protected_mode_;
  std::string engine_path_;
  std::string command_;
  std::string response_;
  std::string id_;
  std::vector<DWORD> known_process_ids_;
  CComPtr<IWebBrowser2> browser_;
};

}  // namespace webdriver

#endif  // WEBDRIVER_IE_BROWSERHOST_H_
