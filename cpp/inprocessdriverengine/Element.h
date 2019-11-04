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
#include <vector>

#include "LocationInfo.h"

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

  bool GetClickableLocationScroll(LocationInfo* click_location);
  bool GetClickableLocationNoScroll(LocationInfo* click_location);

  bool GetVisibleText(std::string* visible_text);
  bool GetAttributeValue(const std::string& attribute_name,
                         std::string* attribute_value);
  bool GetPropertyValue(const std::string& property_name,
                        VARIANT* property_value);
  bool GetCssPropertyValue(const std::string& property_name,
                           std::string* property_value);
  bool GetTagName(std::string* tag_name);
  bool GetRect(FloatingPointLocationInfo* element_rect);
  bool IsDisplayed(const bool ignore_opacity);
  bool IsEnabled(void);
  bool IsSelected(void);
  bool IsObscured(LocationInfo* click_location,
                  long* obscuring_element_index,
                  std::string* obscuring_element_description);

  bool IsAttachedToDom(void);
  bool IsContainingDocument(IHTMLDocument2* document);

  std::string element_id(void) const { return this->element_id_; }
  IHTMLElement* element(void) { return this->element_; }

private:
  static bool RectHasNonZeroDimensions(IHTMLRect* rect);

  bool GetClickableLocation(const bool scroll_if_needed, LocationInfo* click_location);
  bool CalculateClickPoint(const LocationInfo& location,
                           LocationInfo* click_location);
  bool GetComputedStyle(IHTMLCSSStyleDeclaration** computed_style);
  bool GetContainingDocument(const bool use_dom_node, IHTMLDocument2** doc);
  bool GetDocumentFromWindow(IHTMLWindow2* parent_window,
                             IHTMLDocument2** parent_doc);
  std::string GetElementHtmlDescription(IHTMLElement* element);
  bool GetLocationInDocument(LocationInfo* current_location);
  bool GetTextBoundaries(LocationInfo* text_info);
  bool GetOverflowState(std::string* overflow_state);
  bool GetViewPortInfo(LocationInfo* view_port_info);
  bool HasFirstChildTextNodeOfMultipleChildren(void);
  bool HasShadowRoot(void);
  bool IsImageMap(LocationInfo* current_location);
  bool IsInline(void);
  bool IsXmlDocument(IHTMLDocument2* document);

  bool IsLocationInViewPort(const LocationInfo& location);
  bool IsLocationInViewPort(const LocationInfo& location,
                            IHTMLDocument2* owner_doc);
  bool GetFrameElement(IHTMLDocument2* parent_doc,
                       IHTMLDocument2* target_doc,
                       IHTMLElement** frame_element);

  std::string element_id_;
  CComPtr<IHTMLElement> element_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_ELEMENT_H_
