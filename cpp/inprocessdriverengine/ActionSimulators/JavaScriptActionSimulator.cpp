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

#include "JavaScriptActionSimulator.h"

#include "../../webdriver-server/errorcodes.h"

#include "../Generated/atoms.h"
#include "../Script.h"

#define WAIT_TIME_IN_MILLISECONDS_PER_INPUT_EVENT 100

namespace webdriver {


JavaScriptActionSimulator::JavaScriptActionSimulator() {
  CComVariant keyboard_state;
  keyboard_state.vt = VT_NULL;
  this->keyboard_state_ = keyboard_state;

  CComVariant mouse_state;
  mouse_state.vt = VT_NULL;
  this->mouse_state_ = mouse_state;
}

JavaScriptActionSimulator::~JavaScriptActionSimulator() {
}

int JavaScriptActionSimulator::SimulateActions(ActionContext context,
                                               std::vector<INPUT> inputs,
                                               InputState* input_state) {
  //LOG(TRACE) << "Entering JavaScriptActionSimulator::SimulateActions";
  for (size_t i = 0; i < inputs.size(); ++i) {
    INPUT current_input = inputs[i];
    this->UpdateInputState(current_input, input_state);
    if (current_input.type == INPUT_MOUSE) {
      if (current_input.mi.dwFlags & MOUSEEVENTF_MOVE) {
        this->SimulatePointerMove(context.document, current_input);
      } else if (current_input.mi.dwFlags & MOUSEEVENTF_LEFTDOWN) {
        this->SimulatePointerDown(context.document, current_input);
      } else if (current_input.mi.dwFlags & MOUSEEVENTF_LEFTUP) {
        this->SimulatePointerUp(context.document, current_input);
      } else if (current_input.mi.dwFlags & MOUSEEVENTF_RIGHTDOWN) {
        this->SimulatePointerDown(context.document, current_input);
      } else if (current_input.mi.dwFlags & MOUSEEVENTF_RIGHTUP) {
        this->SimulatePointerUp(context.document, current_input);
      }
    } else if (current_input.type == INPUT_KEYBOARD) {
      this->SimulateKeyDown(context.document, current_input);
    } else if (current_input.type == INPUT_HARDWARE) {
      ::Sleep(current_input.hi.uMsg);
    }
  }
  return WD_SUCCESS;
}

int JavaScriptActionSimulator::SimulateKeyDown(IHTMLDocument2* document,
                                               INPUT input) {
  KeyboardExtraInfo* extra_info = reinterpret_cast<KeyboardExtraInfo*>(input.ki.dwExtraInfo);
  std::wstring key = extra_info->character;
  delete extra_info;
  // LOG(DEBUG) << "Using synthetic events for sending keys";
  std::wstring script_source = L"(function() { return function(){" +
                               atoms::asString(atoms::INPUTS) +
                               L"; return webdriver.atoms.inputs.sendKeys(" +
                               L"arguments[0], arguments[1], arguments[2], arguments[3]);" +
                               L"};})();";
  Script script_wrapper(script_source, document);

  std::vector<CComVariant> args;
  CComVariant empty_arg;
  CComVariant key_arg(key.c_str());
  CComVariant state_arg(this->keyboard_state_);
  CComVariant persist_modifiers_arg(true);

  args.push_back(empty_arg);
  args.push_back(key_arg);
  args.push_back(state_arg);
  args.push_back(persist_modifiers_arg);
  int status_code = script_wrapper.Execute(args);
  if (status_code == WD_SUCCESS) {
    this->keyboard_state_ = script_wrapper.result();
  }
  return status_code;
}

int JavaScriptActionSimulator::SimulatePointerMove(IHTMLDocument2* document,
                                                   INPUT input) {
  // LOG(DEBUG) << "Using synthetic events for mouse move";
  MouseExtraInfo* extra_info =
      reinterpret_cast<MouseExtraInfo*>(input.mi.dwExtraInfo);
  int x_offset = extra_info->offset_x;
  int y_offset = extra_info->offset_y;
  bool is_offset_specified = extra_info->offset_specified;
  CComPtr<IHTMLElement> target_element = extra_info->element;
  delete extra_info;

  std::wstring script_source = L"(function() { return function(){" +
                               atoms::asString(atoms::INPUTS) +
                               L"; return webdriver.atoms.inputs.mouseMove(arguments[0], arguments[1], arguments[2], arguments[3]);" +
                               L"};})();";

  Script script_wrapper(script_source, document);

  CComVariant element_arg;
  if (target_element != NULL) {
    element_arg = target_element;
  }

  CComVariant x_offset_arg;
  CComVariant y_offset_arg;
  if (is_offset_specified) {
    x_offset_arg = x_offset;
    y_offset_arg = y_offset;
  }

  CComVariant state_arg(this->mouse_state_);
  std::vector<CComVariant> args;
  args.push_back(element_arg);
  args.push_back(x_offset_arg);
  args.push_back(y_offset_arg);
  args.push_back(state_arg);
  int status_code = script_wrapper.Execute(args);
  if (status_code == WD_SUCCESS) {
    this->mouse_state_ = script_wrapper.result();
  }
  return status_code;
}

int JavaScriptActionSimulator::SimulatePointerDown(IHTMLDocument2* document,
                                                   INPUT input) {
  // LOG(DEBUG) << "Using synthetic events for mouse button down";
  std::wstring script_source = L"(function() { return function(){" +
                               atoms::asString(atoms::INPUTS) +
                               L"; return webdriver.atoms.inputs.mouseButtonDown(arguments[0]);" +
                               L"};})();";

  Script script_wrapper(script_source, document);
  CComVariant state_arg(this->mouse_state_);
  std::vector<CComVariant> args;
  args.push_back(state_arg);

  int status_code = script_wrapper.Execute(args);
  if (status_code == WD_SUCCESS) {
    this->mouse_state_ = script_wrapper.result();
  }
  return status_code;
}

int JavaScriptActionSimulator::SimulatePointerUp(IHTMLDocument2* document,
                                                 INPUT input) {
  //LOG(DEBUG) << "Using synthetic events for mouse button up";
  std::wstring script_source = L"(function() { return function(){" +
                               atoms::asString(atoms::INPUTS) +
                               L"; return webdriver.atoms.inputs.mouseButtonUp(arguments[0]);" +
                               L"};})();";

  Script script_wrapper(script_source, document);
  CComVariant state_arg(this->mouse_state_);
  std::vector<CComVariant> args;
  args.push_back(state_arg);

  int status_code = script_wrapper.Execute(args);
  if (status_code == WD_SUCCESS) {
    this->mouse_state_ = script_wrapper.result();
  }
  return status_code;
}

int JavaScriptActionSimulator::SimulatePause(IHTMLDocument2* document,
                                             INPUT input) {
  ::Sleep(input.hi.uMsg);
  return WD_SUCCESS;
}

} // namespace webdriver
