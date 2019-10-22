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

#ifndef WEBDRIVER_IE_ELEMENT_H_
#define WEBDRIVER_IE_ELEMENT_H_

#include <string>

#define JSON_ELEMENT_PROPERTY_NAME "element-6066-11e4-a52e-4f735466cecf"

// Forward declaration of classes.
namespace Json {
  class Value;
} // namespace Json

namespace webdriver {

class Element {
public:
  Element(IHTMLElement* element);
  virtual ~Element(void);
  Json::Value ConvertToJson(void) const;

  bool GetVisibleText(std::string* visible_text);

  bool IsAttachedToDom(void);
  bool IsContainingDocument(IHTMLDocument2* document);

  std::string element_id(void) const { return this->element_id_; }
  IHTMLElement* element(void) { return this->element_; }

private:
  bool GetContainingDocument(const bool use_dom_node, IHTMLDocument2** doc);

  std::string element_id_;
  CComPtr<IHTMLElement> element_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_ELEMENT_H_
