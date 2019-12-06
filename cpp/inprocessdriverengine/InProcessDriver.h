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

#ifndef WEBDRIVER_INPROCESSDRIVERENGINE_H_
#define WEBDRIVER_INPROCESSDRIVERENGINE_H_

#include <string>
#include <vector>

#include <ExDispid.h>
#include <mshtmldiagnostics.h>

#include "../utils/messages.h"

#include "cominterfaces.h"
#include "InputState.h"

#define BROWSER_EVENTS_ID   250
#define UNINITIALIZED 0
#define READY 1
#define EXECUTING 2
#define COMPLETE 3
#define ABORTED 4

namespace webdriver {
  class InProcessCommandRepository;
  class InputManager;
  class ElementRepository;
  class ElementFinder;
}

class InProcessDriver : public CComObjectRootEx<CComSingleThreadModel>,
                        public CComCoClass<InProcessDriver, &CLSID_InProcessDriver>,
                        public CWindowImpl<InProcessDriver>,
                        public IObjectWithSiteImpl<InProcessDriver>,
                        public IDispEventImpl<BROWSER_EVENTS_ID, InProcessDriver, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw, 1, 1>,
                        public IOleWindow,
                        public IDiagnosticsScriptEngineSite,
                        public IInProcessDriver {

public:
  DECLARE_NO_REGISTRY()
  DECLARE_NOT_AGGREGATABLE(InProcessDriver)

  BEGIN_COM_MAP(InProcessDriver)
    COM_INTERFACE_ENTRY(IInProcessDriver)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IOleWindow)
    COM_INTERFACE_ENTRY(IDiagnosticsScriptEngineSite)
  END_COM_MAP()

  BEGIN_SINK_MAP(InProcessDriver)
    SINK_ENTRY_EX(BROWSER_EVENTS_ID, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, &InProcessDriver::OnBeforeNavigate2)
    SINK_ENTRY_EX(BROWSER_EVENTS_ID, DIID_DWebBrowserEvents2, DISPID_NAVIGATECOMPLETE2, &InProcessDriver::OnNavigateComplete2)
    SINK_ENTRY_EX(BROWSER_EVENTS_ID, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, &InProcessDriver::OnDocumentComplete)
    SINK_ENTRY_EX(BROWSER_EVENTS_ID, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW3, &InProcessDriver::OnNewWindow)
    SINK_ENTRY_EX(BROWSER_EVENTS_ID, DIID_DWebBrowserEvents2, DISPID_NEWPROCESS, &InProcessDriver::OnNewProcess)
    SINK_ENTRY_EX(BROWSER_EVENTS_ID, DIID_DWebBrowserEvents2, DISPID_ONQUIT, &InProcessDriver::OnQuit)
  END_SINK_MAP()

  BEGIN_MSG_MAP(InProcessDriver)
    MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WD_INIT, OnInit)
    MESSAGE_HANDLER(WD_EXEC_COMMAND, OnExecuteCommand)
    MESSAGE_HANDLER(WD_ABORT_COMMAND, OnAbortCommand)
    MESSAGE_HANDLER(WD_GET_RESPONSE_LENGTH, OnGetResponseLength)
    MESSAGE_HANDLER(WD_GET_RESPONSE, OnGetResponse)
    MESSAGE_HANDLER(WD_WAIT, OnWait)
  END_MSG_MAP()

  // IObjectWithSite
  STDMETHOD(SetSite)(_In_opt_ IUnknown* object_site);

  // IOleWindow
  STDMETHOD(GetWindow)(__RPC__deref_out_opt HWND* window_handle);
  STDMETHOD(ContextSensitiveHelp)(BOOL enter_mode) { return E_NOTIMPL; }

  // IDiagnosticsScriptEngineSite
  STDMETHOD(OnMessage)(LPCWSTR* data, ULONG data_length);
  STDMETHOD(OnScriptError)(IActiveScriptError* script_error);

  // DWebBrowserEvents2
  STDMETHOD_(void, OnBeforeNavigate2)(LPDISPATCH pObject,
                                      VARIANT* pvarUrl,
                                      VARIANT* pvarFlags,
                                      VARIANT* pvarTargetFrame,
                                      VARIANT* pvarData,
                                      VARIANT* pvarHeaders,
                                      VARIANT_BOOL* pbCancel);
  STDMETHOD_(void, OnNavigateComplete2)(LPDISPATCH pDisp, VARIANT* URL);
  STDMETHOD_(void, OnDocumentComplete)(LPDISPATCH pDisp, VARIANT* URL);
  STDMETHOD_(void, OnNewWindow)(LPDISPATCH ppDisp,
                                VARIANT_BOOL* pbCancel,
                                DWORD dwFlags,
                                BSTR bstrUrlContext,
                                BSTR bstrUrl);
  STDMETHOD_(void, OnNewProcess)(DWORD lCauseFlag,
                                 IDispatch* pWB2,
                                 VARIANT_BOOL* pbCancel);
  STDMETHOD_(void, OnQuit)();

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct() {
    return S_OK;
  }

  void FinalRelease() {
  }

  InProcessDriver();
  ~InProcessDriver();

  LRESULT OnCopyData(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnInit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnExecuteCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnAbortCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetCommandStatus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetResponseLength(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnGetResponse(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWait(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  static unsigned int WINAPI WaitThreadProc(LPVOID lpParameter);
  static BOOL CALLBACK FindChildContentWindow(HWND hwnd, LPARAM arg);

  int GetFocusedDocument(IHTMLDocument2** document) const;
  int SetFocusedFrameByIndex(const int frame_index);
  int SetFocusedFrameByElement(IHTMLElement* frame_element);
  void SetFocusedFrameToParent(void);

  HWND settings_window_handle(void) const { return this->settings_window_; }

  IWebBrowser2* browser(void) const { return this->browser_; }

  webdriver::ElementRepository* known_element_repository(void) {
    return this->known_element_repository_;
  }

  webdriver::ElementFinder* element_finder(void) const {
    return this->element_finder_;
  }

  webdriver::InputManager* input_manager(void) const {
    return this->input_manager_;
  }

  void set_is_navigating(const bool is_navigating) {
    this->is_navigating_ = is_navigating;
  }

  HWND top_level_window(void) const { return this->top_level_window_; }
  HWND tab_window(void) const { return this->tab_window_; }
  HWND content_window(void) const { return this->content_window_; }

private:
  void WriteDebug(const std::string& message);
  void CreateWaitThread(const std::string& command_id);
  bool IsDocumentReady(void);

  HWND notify_window_;
  HWND settings_window_;
  HWND top_level_window_;
  HWND tab_window_;
  HWND content_window_;
  bool is_navigating_;
  int command_status_;
  std::string command_id_;
  std::string serialized_command_;
  std::string serialized_response_;
  webdriver::InProcessCommandRepository* command_handlers_;
  webdriver::ElementRepository* known_element_repository_;
  webdriver::ElementFinder* element_finder_;
  webdriver::InputManager* input_manager_;

  CComPtr<IWebBrowser2> browser_;
  CComPtr<IHTMLWindow2> focused_frame_;
  CComPtr<IHTMLDocument> script_host_document_;
  CComPtr<IDiagnosticsScriptEngine> script_engine_;
};

OBJECT_ENTRY_AUTO(__uuidof(InProcessDriver), InProcessDriver)

#endif  // WEBDRIVER_INPROCESSDRIVERENGINE_H_
