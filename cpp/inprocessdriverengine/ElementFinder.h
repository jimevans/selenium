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

#ifndef WEBDRIVER_IE_ELEMENTFINDER_H_
#define WEBDRIVER_IE_ELEMENTFINDER_H_

#include <string>

#include "CustomTypes.h"

namespace Json {
  class Value;
}

namespace webdriver {

// Forward declaration of classes.
class ElementRepository;

struct FindElementSettings {
  std::string mechanism;
  std::string criteria;
  IHTMLDocument2* host_document;
  IHTMLElement* context_element;
};

class ElementFinder {
public:
  ElementFinder();
  virtual ~ElementFinder(void);
  virtual int FindElement(const FindElementSettings& settings,
                          ElementRepository* known_element_repository,
                          Json::Value* found_element);
  virtual int FindElements(const FindElementSettings& settings,
                           ElementRepository* known_element_repository,
                           Json::Value* found_elements);

private:
  int FindElementUsingJavaScriptAtom(
      const bool find_single_element,
      const FindElementSettings& settings,
      ElementRepository* known_element_repository,
      Json::Value* atom_result);
};

} // namespace webdriver

#endif // WEBDRIVER_IE_ELEMENTFINDER_H_
