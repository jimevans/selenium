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

#ifndef WEBDRIVER_IE_SENDKEYSCOMMANDHANDLER_H_
#define WEBDRIVER_IE_SENDKEYSCOMMANDHANDLER_H_

#include "../InProcessCommandHandler.h"

#include "../CustomTypes.h"

struct IUIAutomation;
struct IUIAutomationElement;
struct IUIAutomationElementArray;

namespace webdriver {

class SendKeysCommandHandler : public InProcessCommandHandler {
 public:
  struct FileNameData {
    HWND main;
    HWND hwnd;
    DWORD ieProcId;
    DWORD dialogTimeout;
    bool useLegacyDialogHandling;
    const wchar_t* text;
  };

  SendKeysCommandHandler(void);
  virtual ~SendKeysCommandHandler(void);

 protected:
  void ExecuteInternal(const InProcessDriver& executor,
                       const ParametersMap& command_parameters,
                       Response* response);
 private:
  bool IsFileUploadElement(ElementHandle element_wrapper);
  bool HasMultipleAttribute(ElementHandle element_wrapper);
  bool IsElementInteractable(ElementHandle element_wrapper,
                             std::string* error_description);
  bool WaitUntilElementFocused(ElementHandle element_wrapper);
  bool VerifyPageHasFocus(const InProcessDriver& executor);
  bool SetInsertionPoint(IHTMLElement* element);
  bool IsContentEditable(IHTMLElement* element);
  void SetElementFocus(IHTMLElement* element);
  Json::Value CreateActionSequencePayload(const InProcessDriver& executor,
                                          std::wstring* keys);
};

} // namespace webdriver

#endif // WEBDRIVER_IE_SENDKEYSCOMMANDHANDLER_H_
