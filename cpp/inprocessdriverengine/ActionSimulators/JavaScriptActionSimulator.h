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

#ifndef WEBDRIVER_IE_JAVASCRIPTACTIONSIMULATOR_H_
#define WEBDRIVER_IE_JAVASCRIPTACTIONSIMULATOR_H_

#include "ActionSimulator.h"

namespace webdriver {

class JavaScriptActionSimulator : public ActionSimulator {
 public:
  JavaScriptActionSimulator(void);
  virtual ~JavaScriptActionSimulator(void);

  bool UseExtraInfo(void) const { return true; }

  int SimulateActions(ActionContext context,
                      std::vector<INPUT> inputs,
                      InputState* input_state);

 private:

  int SimulateKeyDown(IHTMLDocument2* browser_wrapper, INPUT input);
  int SimulatePointerMove(IHTMLDocument2* browser_wrapper, INPUT input);
  int SimulatePointerDown(IHTMLDocument2* browser_wrapper, INPUT input);
  int SimulatePointerUp(IHTMLDocument2* browser_wrapper, INPUT input);
  int SimulatePause(IHTMLDocument2* browser_wrapper, INPUT input);

  CComVariant keyboard_state_;
  CComVariant mouse_state_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_JAVASCRIPTACTIONSIMULATOR_H_
