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

#include "ElementFinder.h"

#include <vector>

#include "json.h"
#include "../webdriver-server/errorcodes.h"

#include "Element.h"
#include "Generated/atoms.h"
#include "Script.h"
#include "VariantUtilities.h"

namespace webdriver {

ElementFinder::ElementFinder() {
}

ElementFinder::~ElementFinder() {
}

int ElementFinder::FindElement(const FindElementSettings& settings,
                               ElementRepository* known_element_repository,
                               Json::Value* found_element) {
  int status_code = WD_SUCCESS;
  Json::Value atom_result;
  status_code = this->FindElementUsingJavaScriptAtom(true,
                                                     settings,
                                                     known_element_repository,
                                                     &atom_result);
  if (status_code == WD_SUCCESS) {
    int atom_status_code = atom_result["status"].asInt();
    Json::Value atom_value = atom_result["value"];
    status_code = atom_status_code;
    *found_element = atom_result["value"];
  }
  return status_code;
}

int ElementFinder::FindElements(const FindElementSettings& settings,
                                ElementRepository* known_element_repository,
                                Json::Value* found_elements) {
  int status_code = WD_SUCCESS;
  Json::Value atom_result;
  status_code = this->FindElementUsingJavaScriptAtom(false,
                                                     settings,
                                                     known_element_repository,
                                                     &atom_result);
  if (status_code == WD_SUCCESS) {
    int atom_status_code = atom_result["status"].asInt();
    Json::Value atom_value = atom_result["value"];
    status_code = atom_status_code;
    *found_elements = atom_result["value"];
  }
  return status_code;
}

int ElementFinder::FindElementUsingJavaScriptAtom(
    const bool find_single_element,
    const FindElementSettings& settings,
    ElementRepository* known_element_repository,
    Json::Value* atom_result) {
  int status_code = WD_SUCCESS;

  std::wstring script_source(L"return ");
  if (find_single_element) {
    script_source.append(atoms::asString(atoms::FIND_ELEMENT));
  } else {
    script_source.append(atoms::asString(atoms::FIND_ELEMENTS));
  }

  Script find_element_script(script_source,
                             settings.host_document);
  CComVariant mechanism(settings.mechanism.c_str());
  CComVariant criteria(settings.criteria.c_str());
  std::vector<CComVariant> args;
  args.push_back(mechanism);
  args.push_back(criteria);
  if (settings.context_element != nullptr) {
    CComVariant context_element(settings.context_element);
    args.push_back(context_element);
  } else {
    CComVariant empty_arg;
    empty_arg.vt = VT_EMPTY;
    args.push_back(empty_arg);
  }

  status_code = find_element_script.Execute(args);
  if (status_code != WD_SUCCESS) {
    // Hitting a JavaScript error with the atom is an unrecoverable
    // error. The most common case of this for IE is when there is a
    // page refresh, navigation, or similar, and the driver is polling
    // for element presence. The calling code can't do anything about
    // it, so we might as well just log and return. In the common case,
    // this means that the error will be transitory, and will sort itself
    // out once the DOM returns to normal after the page transition is
    // completed. Note carefully that this is an extreme hack, and has
    // the potential to be papering over a very serious problem in the
    // driver.
    // LOG(WARN) << "A JavaScript error was encountered executing the findElement atom.";
    if (find_single_element) {
      // If we are finding a single element, return an error status.
      return ENOSUCHELEMENT;
    } else {
      // If we are finding multiple elements, return success and an
      // empty array.
      *atom_result = Json::Value(Json::arrayValue);
      return WD_SUCCESS;
    }
  }

  status_code = VariantUtilities::VariantAsJsonValue(
      find_element_script.result(),
      known_element_repository,
      atom_result);
  return status_code;
}

} // namespace webdriver
