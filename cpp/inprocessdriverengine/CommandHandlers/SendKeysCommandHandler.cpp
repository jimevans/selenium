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

#include "SendKeysCommandHandler.h"

#include <ctime>
#include <iomanip>

#include <UIAutomation.h>

#include "../../utils/StringUtilities.h"
#include "../../utils/WindowUtilities.h"
#include "../../webdriver-server/errorcodes.h"
#include "../../webdriver-server/keycodes.h"

#include "../Element.h"
#include "../ElementRepository.h"
#include "../InProcessDriver.h"
#include "../InputManager.h"
#include "../VariantUtilities.h"

#define MAXIMUM_DIALOG_FIND_RETRIES 50
#define MAXIMUM_CONTROL_FIND_RETRIES 10

namespace webdriver {

SendKeysCommandHandler::SendKeysCommandHandler(void) {
}

SendKeysCommandHandler::~SendKeysCommandHandler(void) {
}

void SendKeysCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  ParametersMap::const_iterator id_parameter_iterator = command_parameters.find("id");
  ParametersMap::const_iterator value_parameter_iterator = command_parameters.find("text");
  if (id_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(400, "Missing parameter in URL: id");
    return;
  } else if (value_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(400, "Missing parameter: text");
    return;
  } else {
    int status_code = WD_SUCCESS;
    std::string element_id = id_parameter_iterator->second.asString();

    if (!value_parameter_iterator->second.isString()) {
      response->SetErrorResponse(ERROR_INVALID_ARGUMENT, "parameter 'text' must be a string");
      return;
    }
    std::wstring keys = StringUtilities::ToWString(value_parameter_iterator->second.asString());

    InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
    ElementRepository* element_repository =
        mutable_executor.known_element_repository();

    ElementHandle initial_element;
    status_code = element_repository->GetManagedElement(element_id,
                                                        &initial_element);
    if (status_code != WD_SUCCESS) {
      if (status_code == ENOSUCHELEMENT) {
        response->SetErrorResponse(
          ERROR_NO_SUCH_ELEMENT,
          "Invalid internal element ID requested: " + element_id);
      }
      else {
        response->SetErrorResponse(status_code, "Element is no longer valid");
      }
      return;
    }

    if (status_code == WD_SUCCESS) {
      ElementHandle element_wrapper = initial_element;
      CComPtr<IHTMLOptionElement> option;
      HRESULT hr = initial_element->element()->QueryInterface<IHTMLOptionElement>(&option);
      if (SUCCEEDED(hr) && option) {
        // If this is an <option> element, we want to operate on its parent
        // <select> element.
        CComPtr<IHTMLElement> parent_node;
        hr = initial_element->element()->get_parentElement(&parent_node);
        while (SUCCEEDED(hr) && parent_node) {
          CComPtr<IHTMLSelectElement> select;
          HRESULT select_hr = parent_node->QueryInterface<IHTMLSelectElement>(&select);
          if (SUCCEEDED(select_hr) && select) {
            element_repository->AddManagedElement(parent_node, &element_wrapper);
            break;
          }
          hr = parent_node->get_parentElement(&parent_node);
        }
      }

      // Scroll the target element into view before executing the action
      // sequence.
      LocationInfo location = {};
      std::vector<LocationInfo> frame_locations;
      element_wrapper->GetClickableLocationScroll(&location);

      if (this->IsFileUploadElement(element_wrapper)) {
        if (executor.use_strict_file_interactability()) {
          std::string upload_error_description = "";
          if (!this->IsElementInteractable(element_wrapper,
                                           &upload_error_description)) {
            response->SetErrorResponse(ERROR_ELEMENT_NOT_INTERACTABLE,
                                       upload_error_description);
            return;
          }
        }

        // TODO: Send the file parameters across
        bool allow_multiples = this->HasMultipleAttribute(element_wrapper);
        ::SendMessage(executor.notify_window_handle(),
                      WD_NOTIFY_PENDING_FILE_SELECTION,
                      NULL,
                      NULL);
        // This will block this thread until the dialog is dismissed.
        element_wrapper->element()->click();
        return;
      }

      std::string error_description = "";
      bool is_interactable = IsElementInteractable(element_wrapper,
                                                   &error_description);
      if (!is_interactable) {
        response->SetErrorResponse(ERROR_ELEMENT_NOT_INTERACTABLE,
                                   error_description);
        return;
      }

      if (!this->VerifyPageHasFocus(executor)) {
        //LOG(WARN) << "HTML rendering pane does not have the focus. Keystrokes may go to an unexpected UI element.";
      }
      if (!this->WaitUntilElementFocused(element_wrapper)) {
        error_description = "Element cannot be interacted with via the keyboard because it is not focusable";
        response->SetErrorResponse(ERROR_ELEMENT_NOT_INTERACTABLE,
                                   error_description);
        return;
      }

      Json::Value actions = this->CreateActionSequencePayload(executor, &keys);

      CComPtr<IHTMLDocument2> focused_doc;
      mutable_executor.GetFocusedDocument(&focused_doc);

      InputContext input_context;
      input_context.window_handle = executor.content_window();
      input_context.top_level_window = executor.top_level_window();
      input_context.document = focused_doc;
      std::string error_info = "";
      status_code = executor.input_manager()->PerformInputSequence(
          input_context,
          actions,
          &error_info);
      response->SetSuccessResponse(Json::Value::null);
      return;
    } else {
      response->SetErrorResponse(status_code, "Element is no longer valid");
      return;
    }
  }
}

bool SendKeysCommandHandler::IsElementInteractable(ElementHandle element_wrapper,
                                                   std::string* error_description) {
  bool displayed = element_wrapper->IsDisplayed(true);
  if (!displayed) {
    *error_description = "Element cannot be interacted with via the keyboard because it is not displayed";
    return false;
  }

  if (!element_wrapper->IsEnabled()) {
    *error_description = "Element cannot be interacted with via the keyboard because it is not enabled";
    return false;
  }

  return true;
}

Json::Value SendKeysCommandHandler::CreateActionSequencePayload(
    const InProcessDriver& executor,
    std::wstring* keys) {
  bool shift_pressed = executor.input_manager()->is_shift_pressed();
  bool control_pressed = executor.input_manager()->is_control_pressed();
  bool alt_pressed = executor.input_manager()->is_alt_pressed();

  keys->push_back(static_cast<wchar_t>(WD_KEY_NULL));
  Json::Value key_array(Json::arrayValue);
  for (size_t i = 0; i < keys->size(); ++i) {
    std::wstring character = L"";
    character.push_back(keys->at(i));
    if (IS_HIGH_SURROGATE(keys->at(i))) {
      // We've converted the key string to a wstring, which contain
      // wchar_t elements. On Windows, wchar_t is 16 bits, meaning
      // the string has been encoded to UTF-16, which implies each
      // Unicode code point will be either one wchar_t (where the
      // value <= 0xFFFF), or two wchar_ts (where the code point is
      // represented by a surrogate pair). In the latter case, we
      // test for the first part of a surrogate pair, and if  it is
      // one, we grab the next wchar_t, and use the two together to
      // represent a single Unicode "character."
      ++i;
      character.push_back(keys->at(i));
    }

    std::string single_key = StringUtilities::ToString(character);

    if (keys->at(i) == WD_KEY_SHIFT) {
      Json::Value shift_key_value;
      shift_key_value["value"] = single_key;
      if (shift_pressed) {
        shift_key_value["type"] = "keyUp";
      } else {
        shift_key_value["type"] = "keyDown";
        shift_pressed = true;
      }
      key_array.append(shift_key_value);
      continue;
    } else if (keys->at(i) == WD_KEY_CONTROL) {
      Json::Value control_key_value;
      control_key_value["value"] = single_key;
      if (control_pressed) {
        control_key_value["type"] = "keyUp";
      } else {
        control_key_value["type"] = "keyDown";
        control_pressed = true;
      }
      key_array.append(control_key_value);
      continue;
    } else if (keys->at(i) == WD_KEY_ALT) {
      Json::Value alt_key_value;
      alt_key_value["value"] = single_key;
      if (alt_pressed) {
        alt_key_value["type"] = "keyUp";
      } else {
        alt_key_value["type"] = "keyDown";
        alt_pressed = true;
      }
      key_array.append(alt_key_value);
      continue;
    }

    Json::Value key_down_value;
    key_down_value["type"] = "keyDown";
    key_down_value["value"] = single_key;
    key_array.append(key_down_value);

    Json::Value key_up_value;
    key_up_value["type"] = "keyUp";
    key_up_value["value"] = single_key;
    key_array.append(key_up_value);
  }

  Json::Value value;
  value["type"] = "key";
  value["id"] = "send keys keyboard";
  value["actions"] = key_array;

  Json::Value actions(Json::arrayValue);
  actions.append(value);
  return actions;
}

bool SendKeysCommandHandler::HasMultipleAttribute(
    ElementHandle element_wrapper) {
  bool allows_multiple = false;
  std::string multiple_value;
  bool success = element_wrapper->GetAttributeValue("multiple",
                                                     &multiple_value);
  if (success && multiple_value == "true") {
    allows_multiple = true;
  }
  return allows_multiple;
}

bool SendKeysCommandHandler::IsFileUploadElement(ElementHandle element_wrapper) {
  CComPtr<IHTMLElement> element(element_wrapper->element());

  CComPtr<IHTMLInputFileElement> file;
  element->QueryInterface<IHTMLInputFileElement>(&file);
  CComPtr<IHTMLInputElement> input;
  element->QueryInterface<IHTMLInputElement>(&input);
  CComBSTR element_type;
  if (input) {
    input->get_type(&element_type);
    HRESULT hr = element_type.ToLower();
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Failed converting type attribute of <input> element to lowercase using ToLower() method of BSTR";
    }
  }
  bool is_file_element = (file != NULL) ||
                         (input != NULL && element_type == L"file");
  return is_file_element;
}

bool SendKeysCommandHandler::VerifyPageHasFocus(
    const InProcessDriver& executor) {
  HWND browser_pane_window_handle = executor.content_window();
  DWORD proc;
  DWORD thread_id = ::GetWindowThreadProcessId(browser_pane_window_handle, &proc);
  GUITHREADINFO info;
  info.cbSize = sizeof(GUITHREADINFO);
  ::GetGUIThreadInfo(thread_id, &info);

  if (info.hwndFocus != browser_pane_window_handle) {
    //LOG(INFO) << "Focus is on a UI element other than the HTML viewer pane.";
    // The focus is on a UI element other than the HTML viewer pane (like
    // the address bar, for instance). This has implications for certain
    // keystrokes, like backspace. We need to set the focus to the HTML
    // viewer pane.
    // N.B. The SetFocus() API should *NOT* cause the IE browser window to
    // magically appear in the foreground. If that is not true, we will need
    // to find some other solution.
    // Send an explicit WM_KILLFOCUS message to free up SetFocus() to place the
    // focus on the correct window. While SetFocus() is supposed to already do
    // this, it seems to not work entirely correctly.
    ::SendMessage(info.hwndFocus, WM_KILLFOCUS, NULL, NULL);
    DWORD current_thread_id = ::GetCurrentThreadId();
    ::AttachThreadInput(current_thread_id, thread_id, TRUE);
    HWND previous_focus = ::SetFocus(browser_pane_window_handle);
    if (previous_focus == NULL) {
      //LOGERR(WARN) << "SetFocus API call failed";
    }
    ::AttachThreadInput(current_thread_id, thread_id, FALSE);
    ::GetGUIThreadInfo(thread_id, &info);
  }
  return info.hwndFocus == browser_pane_window_handle;
}

bool SendKeysCommandHandler::WaitUntilElementFocused(ElementHandle element_wrapper) {
  // Check we have focused the element.
  CComPtr<IHTMLElement> element = element_wrapper->element();
  bool has_focus = false;
  CComPtr<IDispatch> dispatch;
  element->get_document(&dispatch);
  CComPtr<IHTMLDocument2> document;
  dispatch->QueryInterface<IHTMLDocument2>(&document);

  // If the element we want is already the focused element, we're done.
  CComPtr<IHTMLElement> active_element;
  if (document->get_activeElement(&active_element) == S_OK) {
    if (active_element.IsEqualObject(element)) {
      if (this->IsContentEditable(element)) {
        this->SetElementFocus(element);
      }
      return true;
    }
  }

  this->SetElementFocus(element);

  // Hard-coded 1 second timeout here. Possible TODO is make this adjustable.
  clock_t max_wait = clock() + CLOCKS_PER_SEC;
  for (int i = clock(); i < max_wait; i = clock()) {
    WindowUtilities::Wait(1);
    CComPtr<IHTMLElement> active_wait_element;
    if (document->get_activeElement(&active_wait_element) == S_OK && active_wait_element != NULL) {
      CComPtr<IHTMLElement2> element2;
      element->QueryInterface<IHTMLElement2>(&element2);
      CComPtr<IHTMLElement2> active_wait_element2;
      active_wait_element->QueryInterface<IHTMLElement2>(&active_wait_element2);
      if (element2.IsEqualObject(active_wait_element2)) {
        this->SetInsertionPoint(element);
        has_focus = true;
        break;
      }
    }
  }

  return has_focus;
}

bool SendKeysCommandHandler::SetInsertionPoint(IHTMLElement* element) {
  CComPtr<IHTMLTxtRange> range;
  CComPtr<IHTMLInputTextElement> input_element;
  HRESULT hr = element->QueryInterface<IHTMLInputTextElement>(&input_element);
  if (SUCCEEDED(hr) && input_element) {
    input_element->createTextRange(&range);
  } else {
    CComPtr<IHTMLTextAreaElement> text_area_element;
    hr = element->QueryInterface<IHTMLTextAreaElement>(&text_area_element);
    if (SUCCEEDED(hr) && text_area_element) {
      text_area_element->createTextRange(&range);
    } else {
      bool is_content_editable = this->IsContentEditable(element);
      if (is_content_editable) {
        CComPtr<IDispatch> dispatch;
        hr = element->get_document(&dispatch);
        if (dispatch) {
          CComPtr<IHTMLDocument2> doc;
          hr = dispatch->QueryInterface<IHTMLDocument2>(&doc);
          if (doc) {
            CComPtr<IHTMLElement> body;
            hr = doc->get_body(&body);
            if (body) {
              CComPtr<IHTMLBodyElement> body_element;
              hr = body->QueryInterface<IHTMLBodyElement>(&body_element);
              if (body_element) {
                hr = body_element->createTextRange(&range);
                range->moveToElementText(element);
              }
            }
          }
        }
      }
    }
  }

  if (range) {
    range->collapse(VARIANT_FALSE);
    range->select();
    return true;
  }

  return false;
}

bool SendKeysCommandHandler::IsContentEditable(IHTMLElement* element) {
  CComPtr<IHTMLElement3> element3;
  element->QueryInterface<IHTMLElement3>(&element3);
  VARIANT_BOOL is_content_editable_variant = VARIANT_FALSE;
  if (element3) {
    element3->get_isContentEditable(&is_content_editable_variant);
  }
  return is_content_editable_variant == VARIANT_TRUE;
}

void SendKeysCommandHandler::SetElementFocus(IHTMLElement* element) {
  CComPtr<IHTMLElement2> element2;
  element->QueryInterface<IHTMLElement2>(&element2);
  element2->focus();
}

} // namespace webdriver
