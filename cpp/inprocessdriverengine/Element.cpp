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

#include "Element.h"

#include <algorithm>

#include "../utils/StringUtilities.h"
#include "../webdriver-server/errorcodes.h"
#include "json.h"

#include "Generated/atoms.h"
#include "Script.h"
#include "VariantUtilities.h"

namespace webdriver {

Element::Element(IHTMLElement* element) {
  this->element_id_ = StringUtilities::CreateGuid();
  this->element_ = element;
}

Element::~Element(void) {
}

Json::Value Element::ConvertToJson() const {
  Json::Value json_element;
  json_element[JSON_ELEMENT_PROPERTY_NAME] = this->element_id_;
  return json_element;
}

bool Element::IsDisplayed(const bool ignore_opacity) {
  std::wstring script_source(L"return ");
  script_source.append(atoms::asString(atoms::IS_DISPLAYED));

  CComPtr<IHTMLDocument2> doc;
  bool document_success = this->GetContainingDocument(false, &doc);
  if (!document_success) {
    return false;
  }

  CComVariant element_variant(this->element_);
  CComVariant ignore_opacity_variant(ignore_opacity);
  std::vector<CComVariant> args;
  args.push_back(element_variant);
  args.push_back(ignore_opacity_variant);

  Script is_selected_script(script_source, doc);
  int status_code = is_selected_script.Execute(args);
  if (status_code != WD_SUCCESS) {
    return false;
  }

  CComVariant script_result = is_selected_script.result();
  if (script_result.vt != VT_BOOL) {
    return false;
  }

  return script_result.boolVal == VARIANT_TRUE;
}

bool Element::IsEnabled() {
  std::wstring script_source(L"return ");
  script_source.append(atoms::asString(atoms::IS_ENABLED));

  CComPtr<IHTMLDocument2> doc;
  bool document_success = this->GetContainingDocument(false, &doc);
  if (!document_success) {
    return false;
  }

  if (this->IsXmlDocument(doc)) {
    return false;
  }

  CComVariant element_variant(this->element_);
  std::vector<CComVariant> args;
  args.push_back(element_variant);

  Script is_enabled_script(script_source, doc);
  int status_code = is_enabled_script.Execute(args);
  if (status_code != WD_SUCCESS) {
    return false;
  }

  CComVariant script_result = is_enabled_script.result();
  if (script_result.vt != VT_BOOL) {
    return false;
  }

  return script_result.boolVal == VARIANT_TRUE;
}

bool Element::IsSelected() {
  std::wstring script_source(L"return ");
  script_source.append(atoms::asString(atoms::IS_SELECTED));

  CComPtr<IHTMLDocument2> doc;
  bool document_success = this->GetContainingDocument(false, &doc);
  if (!document_success) {
    return false;
  }

  CComVariant element_variant(this->element_);
  std::vector<CComVariant> args;
  args.push_back(element_variant);

  Script is_selected_script(script_source, doc);
  int status_code = is_selected_script.Execute(args);
  if (status_code != WD_SUCCESS) {
    return false;
  }

  CComVariant script_result = is_selected_script.result();
  if (script_result.vt != VT_BOOL) {
    return false;
  }

  return script_result.boolVal == VARIANT_TRUE;
}

bool Element::GetVisibleText(std::string* visible_text) {
  std::wstring script_source(L"return ");
  script_source.append(atoms::asString(atoms::GET_TEXT));

  CComPtr<IHTMLDocument2> doc;
  bool document_success = this->GetContainingDocument(false, &doc);
  if (!document_success) {
    return false;
  }

  CComVariant element_variant(this->element_);
  std::vector<CComVariant> args;
  args.push_back(element_variant);

  Script get_text_script(script_source, doc);
  int status_code = get_text_script.Execute(args);
  if (status_code != WD_SUCCESS) {
    return false;
  }

  CComVariant script_result = get_text_script.result();
  if (script_result.vt != VT_BSTR) {
    return false;
  }

  std::wstring text(script_result.bstrVal);
  *visible_text = StringUtilities::ToString(text);

  return true;
}

bool Element::GetAttributeValue(const std::string& attribute_name,
  std::string* attribute_value) {
  HRESULT hr = S_OK;
  CComBSTR attribute_name_bstr(attribute_name.c_str());
  CComPtr<IHTMLElement5> has_attribute_element;
  hr = this->element_->QueryInterface<IHTMLElement5>(&has_attribute_element);

  VARIANT_BOOL has_attribute = VARIANT_FALSE;
  has_attribute_element->hasAttribute(attribute_name_bstr, &has_attribute);
  if (has_attribute != VARIANT_TRUE) {
    return false;
  }

  CComPtr<IHTMLElement4> attribute_element;
  hr = this->element_->QueryInterface<IHTMLElement4>(&attribute_element);

  CComPtr<IHTMLDOMAttribute> attribute_node;
  hr = attribute_element->getAttributeNode(attribute_name_bstr,
                                           &attribute_node);
  CComVariant value_variant;
  hr = attribute_node->get_nodeValue(&value_variant);

  if (attribute_name == "allowfullscreen" ||
      attribute_name == "allowpaymentrequest" ||
      attribute_name == "allowusermedia" ||
      attribute_name == "async" ||
      attribute_name == "autofocus" ||
      attribute_name == "autoplay" ||
      attribute_name == "checked" ||
      attribute_name == "compact" ||
      attribute_name == "complete" ||
      attribute_name == "controls" ||
      attribute_name == "declare" ||
      attribute_name == "default" ||
      attribute_name == "defaultchecked" ||
      attribute_name == "defaultselected" ||
      attribute_name == "defer" ||
      attribute_name == "disabled" ||
      attribute_name == "ended" ||
      attribute_name == "formnovalidate" ||
      attribute_name == "hidden" ||
      attribute_name == "indeterminate" ||
      attribute_name == "iscontenteditable" ||
      attribute_name == "ismap" ||
      attribute_name == "itemscope" ||
      attribute_name == "loop" ||
      attribute_name == "multiple" ||
      attribute_name == "muted" ||
      attribute_name == "nohref" ||
      attribute_name == "nomodule" ||
      attribute_name == "noresize" ||
      attribute_name == "noshade" ||
      attribute_name == "novalidate" ||
      attribute_name == "nowrap" ||
      attribute_name == "open" ||
      attribute_name == "paused" ||
      attribute_name == "playsinline" ||
      attribute_name == "pubdate" ||
      attribute_name == "readonly" ||
      attribute_name == "required" ||
      attribute_name == "reversed" ||
      attribute_name == "scoped" ||
      attribute_name == "seamless" ||
      attribute_name == "seeking" ||
      attribute_name == "selected" ||
      attribute_name == "truespeed" ||
      attribute_name == "typemustmatch" ||
      attribute_name == "willvalidate") {
    if (value_variant.vt != VT_BSTR) {
      return false;
    }
    *attribute_value = "true";
    return true;
  }

  std::wstring raw_value = L"";
  if (value_variant.vt == VT_BSTR) {
    raw_value.assign(value_variant.bstrVal);
  } else if (value_variant.vt == VT_I4 || value_variant.vt == VT_I8) {
    long int_value = value_variant.lVal;
    raw_value = std::to_wstring(int_value);
  } else if (value_variant.vt == VT_R4 || value_variant.vt == VT_R8) {
    double dbl_value = value_variant.dblVal;
    raw_value = std::to_wstring(dbl_value);
  } else if (value_variant.vt == VT_BOOL) {
    if (value_variant.boolVal == VARIANT_TRUE) {
      raw_value = L"true";
    } else {
      raw_value = L"false";
    }
  }
  *attribute_value = StringUtilities::ToString(raw_value);
  return true;
}

bool Element::GetPropertyValue(const std::string& property_name,
  VARIANT* property_value) {
  std::wstring wide_property_name = StringUtilities::ToWString(property_name);

  LPOLESTR property_name_pointer =
      const_cast<wchar_t*>(wide_property_name.c_str());
  DISPID dispid_property;
  HRESULT hr = this->element_->GetIDsOfNames(IID_NULL,
                                             &property_name_pointer,
                                             1,
                                             LOCALE_USER_DEFAULT,
                                             &dispid_property);
  if (FAILED(hr)) {
    property_value->vt = VT_EMPTY;
    return true;
  }

  DISPPARAMS no_args_dispatch_parameters = { 0 };
  hr = this->element_->Invoke(dispid_property,
                              IID_NULL,
                              LOCALE_USER_DEFAULT,
                              DISPATCH_PROPERTYGET,
                              &no_args_dispatch_parameters,
                              property_value,
                              NULL,
                              NULL);
  if (FAILED(hr)) {
    property_value->vt = VT_EMPTY;
    return true;
  }

  return true;
}

bool Element::GetCssPropertyValue(const std::string& property_name,
  std::string* property_value) {
  int status_code = WD_SUCCESS;
  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  if (this->IsXmlDocument(doc)) {
    *property_value = "";
    return status_code;
  }

  // The atom is just the definition of an anonymous
  // function: "function() {...}"; Wrap it in another function so we can
  // invoke it with our arguments without polluting the current namespace.
  std::wstring script_source = L"return ";
  script_source.append(atoms::asString(atoms::GET_EFFECTIVE_STYLE));

  CComVariant element_variant(this->element_);
  CComVariant property_name_variant(property_name.c_str());
  std::vector<CComVariant> args;
  args.push_back(element_variant);
  args.push_back(property_name_variant);

  Script script_wrapper(script_source, doc);
  status_code = script_wrapper.Execute(args);

  if (status_code == WD_SUCCESS) {
    CComVariant result = script_wrapper.result();
    std::wstring raw_value = L"";
    if (result.vt == VT_BSTR) {
      raw_value.assign(script_wrapper.result().bstrVal);
    } else if (result.vt == VT_I4 || result.vt == VT_I8) {
      long int_value = script_wrapper.result().lVal;
      raw_value = std::to_wstring(int_value);
    } else if (result.vt == VT_R4 || result.vt == VT_R8) {
      double dbl_value = script_wrapper.result().dblVal;
      raw_value = std::to_wstring(dbl_value);
    } else if (result.vt == VT_BOOL) {
      if (script_wrapper.result().boolVal == VARIANT_TRUE) {
        raw_value = L"true";
      } else {
        raw_value = L"false";
      }
    }
    std::string value = StringUtilities::ToString(raw_value);
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   tolower);
    *property_value = value;
  }

  return status_code == WD_SUCCESS;
}

bool Element::GetTagName(std::string* tag_name) {
  CComBSTR tag_name_bstr;
  HRESULT hr = this->element_->get_tagName(&tag_name_bstr);
  if (FAILED(hr)) {
    return false;
  }
  std::wstring converted_tag_name = tag_name_bstr;
  std::string result = StringUtilities::ToString(converted_tag_name);
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  *tag_name = result;
  return true;
}

bool Element::GetRect(FloatingPointLocationInfo* element_rect) {
  int status_code = WD_SUCCESS;
  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);

  // The atom is just the definition of an anonymous
  // function: "function() {...}"; Wrap it in another function so we can
  // invoke it with our arguments without polluting the current namespace.
  std::wstring script_source = L"return ";
  script_source.append(atoms::asString(atoms::GET_ELEMENT_RECT));

  CComVariant element_variant(this->element_);
  std::vector<CComVariant> args;
  args.push_back(element_variant);

  Script script_wrapper(script_source, doc);
  status_code = script_wrapper.Execute(args);
  if (status_code != WD_SUCCESS) {
    return false;
  }

  CComVariant width_variant;
  VariantUtilities::GetVariantObjectPropertyValue(
      script_wrapper.result().pdispVal,
      L"width",
      &width_variant);
  element_rect->width = width_variant.dblVal;

  CComVariant height_variant;
  VariantUtilities::GetVariantObjectPropertyValue(
      script_wrapper.result().pdispVal,
      L"height",
      &height_variant);
  element_rect->height = height_variant.dblVal;

  CComVariant x_variant;
  VariantUtilities::GetVariantObjectPropertyValue(
      script_wrapper.result().pdispVal,
      L"x",
      &x_variant);
  element_rect->x = x_variant.dblVal;

  CComVariant y_variant;
  VariantUtilities::GetVariantObjectPropertyValue(
      script_wrapper.result().pdispVal,
      L"y",
      &y_variant);
  element_rect->y = y_variant.dblVal;

  return true;
}

bool Element::GetClickableLocationScroll(LocationInfo* click_location) {
  return this->GetClickableLocation(false, click_location);
}

bool Element::GetClickableLocationNoScroll(LocationInfo* click_location) {
  return this->GetClickableLocation(true, click_location);
}

bool Element::GetClickableLocation(const bool scroll_if_needed, LocationInfo* click_location) {
  LocationInfo element_location = {};
  bool location_result = this->GetLocationInDocument(&element_location);

  std::string overflow_state = "";
  bool overflow_result = this->GetOverflowState(&overflow_state);
  if (!overflow_result) {
    // Cannot get overflow state
    return false;
  }

  if (overflow_state == "hidden") {
    // Element cannot be scrolled into view, ever.
    return false;
  }

  bool is_calculated = this->CalculateClickPoint(element_location, click_location);
  if (!scroll_if_needed) {
    return is_calculated;
  }
  
  bool is_location_in_view_port = this->IsLocationInViewPort(*click_location);

  if (!location_result ||
      overflow_state == "scroll" ||
      !is_location_in_view_port) {
    CComVariant scroll_behavior(false);
    HRESULT hr = this->element_->scrollIntoView(scroll_behavior);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Cannot scroll element into view, IHTMLElement::scrollIntoView failed";
      //return EOBSOLETEELEMENT;
      return false;
    }

    std::vector<LocationInfo> scrolled_frame_locations;
    bool result = this->GetLocationInDocument(&element_location);
    //if (result != WD_SUCCESS) {
    //  LOG(WARN) << "Unable to get location of scrolled to element";
    //  return result;
    //}
    if (!result) {
      return result;
    }

    this->CalculateClickPoint(element_location, click_location);
    if (!this->IsLocationInViewPort(*click_location)) {
      //LOG(WARN) << "Scrolled element is not in view";
      //status_code = EELEMENTCLICKPOINTNOTSCROLLED;
      return false;
    }

    // TODO: Handle the case where the element's click point is in
    // the view port but hidden by the overflow of a parent element.
    // That could would look something like the following:
    // if (this->IsHiddenByOverflow(element_location, click_location)) {
    //   if (!this->IsEntirelyHiddenByOverflow()) {
    //     this->ScrollWithinOverflow(element_location);
    //   }
    //   status_code = EELEMENTCLICKPOINTNOTSCROLLED;
    // }
  }

  //LOG(DEBUG) << "(x, y, w, h): "
  //           << element_location.x << ", "
  //           << element_location.y << ", "
  //           << element_location.width << ", "
  //           << element_location.height;
  
  // At this point, we know the element is displayed according to its
  // style attributes, and we've made a best effort at scrolling it so
  // that it's completely within the viewport. We will always return
  // the coordinates of the element, even if the scrolling is unsuccessful.
  // However, we will still return the "element not displayed" status code
  // if the click point has not been scrolled to the viewport.
  //location->x = element_location.x;
  //location->y = element_location.y;
  //location->width = element_location.width;
  //location->height = element_location.height;

  return true;
}

bool Element::IsObscured(LocationInfo* click_location,
                         long* obscuring_element_index,
                         std::string* obscuring_element_description) {
  CComPtr<ISVGElement> svg_element;
  HRESULT hr = this->element_->QueryInterface<ISVGElement>(&svg_element);
  if (SUCCEEDED(hr) && svg_element != NULL) {
    // SVG elements can have complex paths making them non-hierarchical
    // when drawn. We'll just assume the user knows what they're doing
    // and bail on this test here.
    return false;
  }

  // If an element has a style value where pointer-events is set to 'none',
  // the element is "obscured" by definition, since any mouse interaction
  // will not be handled by the element.
  CComPtr<IHTMLCSSStyleDeclaration> computed_style;
  if (this->GetComputedStyle(&computed_style)) {
    CComBSTR pointer_events_value = L"";
    hr = computed_style->get_pointerEvents(&pointer_events_value);
    if (SUCCEEDED(hr) && pointer_events_value == L"none") {
      return true;
    }
  }

  // The element being obscured only makes sense within the context
  // of its own document, even if it's not in the top-level document.
  LocationInfo element_location = {};
  int status_code = this->GetLocationInDocument(&element_location);
  this->CalculateClickPoint(element_location, click_location);
  long x = click_location->x;
  long y = click_location->y;

  bool is_inline = this->IsInline();

  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  CComPtr<IHTMLElement> element_hit;
  hr = doc->elementFromPoint(x, y, &element_hit);
  if (SUCCEEDED(hr) && element_hit) {
    if (element_.IsEqualObject(element_hit)) {
      // Short circuit the use of elementsFromPoint if we don't
      // have to use it.
      return false;
    }
    else {
      // Short circuit in the case where this element is specifically
      // an "inline" element (<label>, <span>, <a>, at present),
      // and the top-most element as determined by elementFromPoint is
      // a direct child of this element. This is to work around IE's bug
      // in elementsFromPoint that does not return inline elements in the
      // list of elements hit.
      // N.B., this is a hack of the highest order, and there's every
      // likelihood that some page somewhere will fail this check.
      if (is_inline) {
        CComPtr<IHTMLElement> element_hit_parent;
        hr = element_hit->get_parentElement(&element_hit_parent);
        CComBSTR element_hit_parent_tag;
        element_hit_parent->get_tagName(&element_hit_parent_tag);
        if (SUCCEEDED(hr) && element_hit_parent) {
          if (this->element_.IsEqualObject(element_hit_parent)) {
            return false;
          }
        }
      }
    }
  }

  bool has_shadow_root = this->HasShadowRoot();
  CComPtr<IHTMLElement> shadow_root_parent;
  if (has_shadow_root) {
    // TODO: Walk up the DOM tree until we receive an ancestor that
    // does not have a shadow root.
    hr = this->element()->get_parentElement(&shadow_root_parent);
    if (FAILED(hr)) {
      // LOGHR(WARN, hr) << "Element has shadow root, but cannot get parent";
    }
  }

  CComPtr<IHTMLDocument8> elements_doc;
  hr = doc.QueryInterface<IHTMLDocument8>(&elements_doc);
  if (FAILED(hr)) {
    // If we failed to QI for IHTMLDocument8, we can't easily determine if
    // the element is obscured or not. We will assume we are not obscured
    // and bail, even though that may not be the case.
    // LOGHR(WARN, hr) << "QueryInterface for IHTMLDocument8 failed";
    return false;
  }

  bool is_obscured = false;
  CComPtr<IHTMLDOMChildrenCollection> elements_hit;
  hr = elements_doc->elementsFromPoint(static_cast<float>(x),
                                       static_cast<float>(y),
                                       &elements_hit);
  if (SUCCEEDED(hr) && elements_hit != NULL) {
    std::vector<std::string> element_descriptions;
    long element_count;
    elements_hit->get_length(&element_count);
    for (long index = 0; index < element_count; ++index) {
      CComPtr<IDispatch> dispatch_in_list;
      elements_hit->item(index, &dispatch_in_list);

      CComPtr<IHTMLElement> element_in_list;
      hr = dispatch_in_list->QueryInterface<IHTMLElement>(&element_in_list);
      bool are_equal = element_in_list.IsEqualObject(this->element_);
      if (are_equal) {
        break;
      }

      Element list_element_wrapper(element_in_list);
      bool is_list_element_displayed = list_element_wrapper.IsDisplayed(false);
      if (is_list_element_displayed) {
        if (has_shadow_root && shadow_root_parent) {
          // Shadow DOM is problematic. Shadow DOM is only available in IE as a
          // polyfill. If the element is part of a Shadow DOM (using a
          // polyfill), elementsFromPoint will show the component elements, not
          // necessarily the Web Component root element itself. If the direct
          // parent of the Web Component host element is in this list, then it
          // counts as a direct descendent, and won't be obscured.
          bool is_shadow_root_parent =
              element_in_list.IsEqualObject(shadow_root_parent);
          if (is_shadow_root_parent) {
            break;
          }
        }

        VARIANT_BOOL is_child;
        hr = this->element_->contains(element_in_list, &is_child);
        VARIANT_BOOL is_ancestor;
        hr = element_in_list->contains(this->element_, &is_ancestor);
        bool found_element_not_in_tree = is_child != VARIANT_TRUE &&
                                         is_ancestor != VARIANT_TRUE;
        if (found_element_not_in_tree) {
          CComPtr<IHTMLFrameBase> frame_element;
          hr = element_in_list->QueryInterface<IHTMLFrameBase>(&frame_element);
          if (SUCCEEDED(hr) && frame_element) {
            // Candidate element is a <frame> or <iframe>, meaning it must
            // be a different document tree, which implies that it cannot
            // be obscuring the element we are attempting to click on.
            continue;
          }

          CComPtr<IHTMLCSSStyleDeclaration> list_element_computed_style;
          if (list_element_wrapper.GetComputedStyle(
              &list_element_computed_style)) {
            CComBSTR list_element_pointer_events_value = L"";
            hr = list_element_computed_style->get_pointerEvents(
                &list_element_pointer_events_value);
            if (SUCCEEDED(hr) &&
                list_element_pointer_events_value != L"none") {
              // If the element has a pointer-events value set to 'none', it
              // may be technically obscuring this element, but manipulating
              // it with the pointer device has no effect, so it is effectively
              // not obscuring this element.
              is_obscured = true;
            }
          } else {
            // We were unable to retrieve the computed style, so we must assume
            // the other element is obscuring this one.
            is_obscured = true;
          }
        }
        else {
          // Repeating the immediate-child-of-inline-element hack from above
          // for elements found in the list.
          if (is_inline) {
            CComPtr<IHTMLElement> list_element_parent;
            hr = element_in_list->get_parentElement(&list_element_parent);
            if (SUCCEEDED(hr) && list_element_parent) {
              if (this->element_.IsEqualObject(list_element_parent)) {
                break;
              }
            }
          }
        }
        if (is_obscured) {
          // Return the top-most element in the event we find an obscuring
          // element in the tree between this element and the top-most one.
          // Note that since it's the top-most element, it will have no
          // descendants, so its outerHTML property will contain only itself.
          std::string outer_html = this->GetElementHtmlDescription(
              element_in_list);
          *obscuring_element_index = index;
          *obscuring_element_description = outer_html;
          break;
        }
      }
    }
  }

  return is_obscured;
}

std::string Element::GetElementHtmlDescription(IHTMLElement* element) {
  CComBSTR outer_html_bstr;
  HRESULT hr = element->get_outerHTML(&outer_html_bstr);
  std::wstring outer_html = outer_html_bstr;
  size_t bracket_pos = outer_html.find(L'>');
  if (bracket_pos != std::wstring::npos) {
    outer_html = outer_html.substr(0, bracket_pos + 1);
  }
  return StringUtilities::ToString(outer_html);
}

bool Element::HasShadowRoot() {
  std::wstring script_source = L"return function() { ";
  script_source.append(L"if (arguments[0].shadowRoot && arguments[0].shadowRoot !== null) { ");
  script_source.append(L"return true; ");
  script_source.append(L"} return false; }");

  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  Script script_wrapper(script_source, doc);
  CComVariant element_arg(this->element_);
  std::vector<CComVariant> args;
  args.push_back(element_arg);
  int status_code = script_wrapper.Execute(args);
  if (status_code == WD_SUCCESS) {
    if (script_wrapper.result().vt == VT_BOOL) {
      return script_wrapper.result().boolVal == VARIANT_TRUE;
    }
  }
  return false;
}

bool Element::GetComputedStyle(IHTMLCSSStyleDeclaration** computed_style) {
  HRESULT hr = S_OK;
  CComPtr<IHTMLDocument2> doc;
  int status_code = this->GetContainingDocument(false, &doc);
  if (status_code == WD_SUCCESS) {
    CComPtr<IHTMLWindow2> window;
    hr = doc->get_parentWindow(&window);
    if (SUCCEEDED(hr) && window) {
      CComPtr<IHTMLWindow7> style_window;
      hr = window->QueryInterface<IHTMLWindow7>(&style_window);
      if (SUCCEEDED(hr) && style_window) {
        CComPtr<IHTMLDOMNode> node;
        hr = this->element_->QueryInterface<IHTMLDOMNode>(&node);
        if (SUCCEEDED(hr) && node) {
          hr = style_window->getComputedStyle(node, NULL, computed_style);
          if (SUCCEEDED(hr) && computed_style) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool Element::GetContainingDocument(const bool use_dom_node,
                                    IHTMLDocument2** doc) {
  HRESULT hr = S_OK;
  CComPtr<IDispatch> dispatch_doc;

  if (use_dom_node) {
    CComPtr<IHTMLDOMNode2> node;
    hr = this->element_->QueryInterface(&node);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Unable to cast element to IHTMLDomNode2";
      return false;
    }

    hr = node->get_ownerDocument(&dispatch_doc);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Unable to locate owning document, call to IHTMLDOMNode2::get_ownerDocument failed";
      return false;
    }
  } else {
    hr = this->element_->get_document(&dispatch_doc);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Unable to locate document property, call to IHTMLELement::get_document failed";
      return false;
    }
  }

  try {
    hr = dispatch_doc.QueryInterface<IHTMLDocument2>(doc);
    if (FAILED(hr)) {
      //LOGHR(WARN, hr) << "Found document but it's not the expected type (IHTMLDocument2)";
      return false;
    }
  } catch (...) {
    //LOG(WARN) << "Found document but it's not the expected type (IHTMLDocument2)";
    return false;
  }

  return true;
}

bool Element::GetViewPortInfo(LocationInfo* view_port_info) {
  HRESULT hr = S_OK;
  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  CComPtr<IHTMLWindow2> window;
  hr = doc->get_parentWindow(&window);
  CComPtr<IHTMLWindow2> top_level_window;
  hr = window->get_top(&top_level_window);
  CComPtr<IHTMLDocument2> top_level_doc;
  if (top_level_window.IsEqualObject(window)) {
    top_level_doc = doc;
  } else {
    this->GetDocumentFromWindow(window, &top_level_doc);
  }
  CComPtr<IHTMLDocument3> document_element_doc;
  CComPtr<IHTMLElement> document_element;
  hr = top_level_doc->QueryInterface<IHTMLDocument3>(&document_element_doc);
  hr = document_element_doc->get_documentElement(&document_element);
  CComPtr<IHTMLElement2> size_element;
  hr = document_element->QueryInterface<IHTMLElement2>(&size_element);
  size_element->get_clientHeight(&view_port_info->height);
  size_element->get_clientWidth(&view_port_info->width);
  return true;
}

bool Element::GetFrameElement(IHTMLDocument2* parent_doc,
                              IHTMLDocument2* target_doc,
                              IHTMLElement** frame_element) {
  HRESULT hr = S_OK;
  CComPtr<IHTMLFramesCollection2> frames;
  hr = parent_doc->get_frames(&frames);

  long frame_count(0);
  hr = frames->get_length(&frame_count);
  CComVariant index;
  index.vt = VT_I4;
  for (long i = 0; i < frame_count; ++i) {
    // See if the document in each frame is this element's 
    // owner document.
    index.lVal = i;
    CComVariant result;
    hr = frames->item(&index, &result);
    CComPtr<IHTMLWindow2> frame_window;
    result.pdispVal->QueryInterface<IHTMLWindow2>(&frame_window);
    if (!frame_window) {
      // Frame is not an HTML frame.
      continue;
    }

    CComPtr<IHTMLDocument2> frame_doc;
    this->GetDocumentFromWindow(frame_window, &frame_doc);
    if (frame_doc.IsEqualObject(target_doc)) {
      // The document in this frame *is* this element's owner
      // document. Get the frameElement property of the document's
      // containing window (which is itself an HTML element, either
      // a frame or an iframe). Then get the x and y coordinates of
      // that frame element.
      // N.B. We must use JavaScript here, as directly using
      // IHTMLWindow4.get_frameElement() returns E_NOINTERFACE under
      // some circumstances.
      std::string script_source = "return function() {\n";
      script_source.append("return arguments[0].frameElement");
      script_source.append("\n}");

      std::vector<CComVariant> args;
      CComVariant window_variant(frame_window);

      Script script_wrapper(script_source, frame_doc);
      args.push_back(window_variant);
      int status_code = script_wrapper.Execute(args);

      CComPtr<IHTMLFrameBase> frame_base;
      if (status_code == WD_SUCCESS) {
        hr = script_wrapper.result().pdispVal->QueryInterface<IHTMLFrameBase>(
          &frame_base);
        //if (FAILED(hr)) {
        //  LOG(WARN) << "Found the frame element, but could not QueryInterface "
        //            << "to IHTMLFrameBase.";
        //}
      } else {
        // Can't get the frameElement property, likely because the frames are
        // from different domains. So start at the parent document, and use
        // getElementsByTagName to retrieve all of the iframe elements (if
        // there are no iframe elements, get the frame elements)
        // **** BIG HUGE ASSUMPTION!!! ****
        // The index of the frame from the document.frames collection will
        // correspond to the index into the collection of iframe/frame elements
        // returned by getElementsByTagName.
        //LOG(WARN) << "Attempting to get frameElement via JavaScript failed. "
        //          << "This usually means the frame is in a different domain "
        //          << "than the parent frame. Browser security against "
        //          << "cross-site scripting attacks will not allow this. "
        //          << "Attempting alternative method.";
        long collection_count = 0;
        CComPtr<IDispatch> element_dispatch;
        CComPtr<IHTMLDocument3> doc;
        parent_doc->QueryInterface<IHTMLDocument3>(&doc);
        if (doc) {
          //LOG(DEBUG) << "Looking for <iframe> elements in parent document.";
          CComBSTR iframe_tag_name = L"iframe";
          CComPtr<IHTMLElementCollection> iframe_collection;
          hr = doc->getElementsByTagName(iframe_tag_name, &iframe_collection);
          hr = iframe_collection->get_length(&collection_count);
          if (collection_count != 0) {
            if (collection_count > index.lVal) {
              //LOG(DEBUG) << "Found <iframe> elements in parent document, "
              //           << "retrieving element" << index.lVal << ".";
              hr = iframe_collection->item(index, index, &element_dispatch);
              hr = element_dispatch->QueryInterface<IHTMLFrameBase>(&frame_base);
            }
          }
          else {
            //LOG(DEBUG) << "No <iframe> elements, looking for <frame> elements "
            //           << "in parent document.";
            CComBSTR frame_tag_name = L"frame";
            CComPtr<IHTMLElementCollection> frame_collection;
            hr = doc->getElementsByTagName(frame_tag_name, &frame_collection);
            hr = frame_collection->get_length(&collection_count);
            if (collection_count > index.lVal) {
              //LOG(DEBUG) << "Found <frame> elements in parent document, "
              //           << "retrieving element" << index.lVal << ".";
              hr = frame_collection->item(index, index, &element_dispatch);
              hr = element_dispatch->QueryInterface<IHTMLFrameBase>(&frame_base);
            }
          }
        }
      }
      frame_base->QueryInterface<IHTMLElement>(frame_element);
      return true;
    }
  }
  return false;
}

bool Element::IsLocationInViewPort(const LocationInfo& location) {
  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  return this->IsLocationInViewPort(location, doc);
}

bool Element::IsLocationInViewPort(const LocationInfo& location,
                                   IHTMLDocument2* current_doc) {
  bool result = true;
  HRESULT hr = S_OK;

  CComPtr<IHTMLWindow2> current_window;
  hr = current_doc->get_parentWindow(&current_window);
  CComPtr<IHTMLWindow2> parent_window;
  hr = current_window->get_parent(&parent_window);
  if (parent_window && current_window.IsEqualObject(parent_window)) {
    LocationInfo viewport;
    this->GetViewPortInfo(&viewport);
    return location.x >= 0 &&
           location.x < viewport.width &&
           location.y >= 0 &&
           location.y < viewport.height;
  }

  CComPtr<IHTMLDocument2> parent_doc;
  this->GetDocumentFromWindow(parent_window, &parent_doc);

  CComPtr<IHTMLElement> frame_element;
  this->GetFrameElement(parent_doc, current_doc, &frame_element);

  LocationInfo frame_location = {};
  Element element_wrapper(frame_element);
  element_wrapper.GetLocationInDocument(&frame_location);

  RECT frame_element_rect = frame_location.AsRect();
  CComPtr<IDisplayServices> display_services;
  parent_doc->QueryInterface<IDisplayServices>(&display_services);
  display_services->TransformRect(&frame_element_rect,
                                  COORD_SYSTEM_FRAME,
                                  COORD_SYSTEM_GLOBAL,
                                  nullptr);
  result = (location.x >= frame_element_rect.left &&
            location.x <= frame_element_rect.right &&
            location.y >= frame_element_rect.top &&
            location.y <= frame_element_rect.bottom);
  return result && IsLocationInViewPort(location, parent_doc);
}

bool Element::CalculateClickPoint(const LocationInfo& location,
                                  LocationInfo* click_location) {
  bool is_location_in_view_port = true;
  RECT element_rect = location.AsRect();
  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  CComPtr<IDisplayServices> display_services;
  HRESULT hr = doc->QueryInterface<IDisplayServices>(&display_services);
  hr = display_services->TransformRect(&element_rect,
                                       COORD_SYSTEM_FRAME,
                                       COORD_SYSTEM_GLOBAL,
                                       nullptr);

  long corrected_width = element_rect.right - element_rect.left;
  long corrected_height = element_rect.bottom - element_rect.top;
  long corrected_x = element_rect.left;
  long corrected_y = element_rect.top;

  LocationInfo clickable_viewport = {};
  bool result = this->GetViewPortInfo(&clickable_viewport);

  if (result) {
    // TODO: Handle the case where the center of the target element
    // is already in the view port. The code would look something like
    // the following:
    // If the center of the target element is already in the view port,
    // we don't need to adjust to find the "in view center point."
    // Technically, this is a deliberate violation of the spec.
    //long element_center_x = location.x + static_cast<long>(floor(location.width / 2.0));
    //long element_center_y = location.y + static_cast<long>(floor(location.height / 2.0));
    //if (element_center_x < 0 ||
    //    element_center_x >= clickable_viewport.width ||
    //    element_center_y < 0 ||
    //    element_center_y >= clickable_viewport.height) {

    RECT viewport_rect = clickable_viewport.AsRect();

    RECT intersect_rect;
    BOOL is_intersecting = ::IntersectRect(&intersect_rect,
                                           &element_rect,
                                           &viewport_rect);
    if (is_intersecting) {
      corrected_width = intersect_rect.right - intersect_rect.left;
      corrected_height = intersect_rect.bottom - intersect_rect.top;
      // If the x or y coordinate is greater than or equal to zero, the
      // initial location will already be correct, and not need to be
      // adjusted.
      if (location.x < 0) {
        corrected_x = 0;
      }
      if (location.y < 0) {
        corrected_y = 0;
      }
    }
  }

  click_location->x = corrected_x +
                      static_cast<long>(floor(corrected_width / 2.0));
  click_location->y = corrected_y +
                      static_cast<long>(floor(corrected_height / 2.0));
  return is_location_in_view_port;
}

bool Element::GetDocumentFromWindow(IHTMLWindow2* parent_window,
                                    IHTMLDocument2** parent_doc) { 
  HRESULT hr = parent_window->get_document(parent_doc);
  if (FAILED(hr)) {
    if (hr == E_ACCESSDENIED) {
      // Cross-domain documents may throw Access Denied. If so,
      // get the document through the IWebBrowser2 interface.
      CComPtr<IServiceProvider> service_provider;
      hr = parent_window->QueryInterface<IServiceProvider>(&service_provider);
      if (FAILED(hr)) {
        //LOGHR(WARN, hr) << "Unable to get browser, call to "
        //                << "IHTMLWindow2::QueryInterface failed for "
        //                << "IServiceProvider";
        //return ENOSUCHDOCUMENT;
        return false;
      }
      CComPtr<IWebBrowser2> window_browser;
      hr = service_provider->QueryService(IID_IWebBrowserApp, &window_browser);
      if (FAILED(hr)) {
        //LOGHR(WARN, hr) << "Unable to get browser, call to "
        //                << "IServiceProvider::QueryService failed for "
        //                << "IID_IWebBrowserApp";
        //return ENOSUCHDOCUMENT;
        return false;
      }
      CComPtr<IDispatch> parent_doc_dispatch;
      hr = window_browser->get_Document(&parent_doc_dispatch);
      if (FAILED(hr)) {
        //LOGHR(WARN, hr) << "Unable to get document, call to "
        //                << "IWebBrowser2::get_Document failed";
        //return ENOSUCHDOCUMENT;
        return false;
      }
      try {
        hr = parent_doc_dispatch->QueryInterface<IHTMLDocument2>(parent_doc);
        if (FAILED(hr)) {
          //LOGHR(WARN, hr) << "Unable to get document, QueryInterface for "
          //                << "IHTMLDocument2 failed";
          //return ENOSUCHDOCUMENT;
          return false;
        }
      } catch (...) {
        //LOG(WARN) << "Unable to get document, exception thrown attempting to "
        //          << "QueryInterface for IHTMLDocument2";
        //return ENOSUCHDOCUMENT;
        return false;
      }
    } else {
      //LOGHR(WARN, hr) << "Unable to get document, IHTMLWindow2::get_document "
      //                << "failed with error code other than E_ACCESSDENIED";
      //return ENOSUCHDOCUMENT;
      return false;
    }
  }
  //return WD_SUCCESS;
  return true;
}

bool Element::GetLocationInDocument(LocationInfo* current_location) {
  bool has_absolute_position_ready_to_return = false;

  CComPtr<IHTMLElement2> bounding_rect_element;
  HRESULT hr = this->element_->QueryInterface(&bounding_rect_element);
  if (FAILED(hr)) {
    return false;
  }

  long top = 0, bottom = 0, left = 0, right = 0;
  LocationInfo map_location = { 0, 0, 0, 0 };
  if (this->IsImageMap(&map_location)) {
    left = map_location.x;
    top = map_location.y;
    right = map_location.x + map_location.width;
    bottom = map_location.y + map_location.height;
  } else {
    // If this element is inline, we need to check whether we should 
    // use getBoundingClientRect() or the first non-zero-sized rect returned
    // by getClientRects(). If the element is not inline, we can use
    // getBoundingClientRect() directly.
    CComPtr<IHTMLRect> rect;
    if (this->IsInline()) {
      CComPtr<IHTMLRectCollection> rects;
      hr = bounding_rect_element->getClientRects(&rects);
      long rect_count;
      rects->get_length(&rect_count);
      if (rect_count > 1) {
        for (long i = 0; i < rect_count; ++i) {
          CComVariant index(i);
          CComVariant rect_variant;
          hr = rects->item(&index, &rect_variant);
          if (SUCCEEDED(hr) && rect_variant.pdispVal) {
            CComPtr<IHTMLRect> qi_rect;
            rect_variant.pdispVal->QueryInterface<IHTMLRect>(&qi_rect);
            if (qi_rect) {
              rect = qi_rect;
              if (RectHasNonZeroDimensions(rect)) {
                // IE returns absolute positions in the page, rather than
                // frame- and scroll-bound positions, for clientRects
                // (as opposed to boundingClientRects).
                has_absolute_position_ready_to_return = true;
                break;
              }
            }
          }
        }
      } else {
        hr = bounding_rect_element->getBoundingClientRect(&rect);
      }
    } else {
      hr = bounding_rect_element->getBoundingClientRect(&rect);
      if (this->HasFirstChildTextNodeOfMultipleChildren()) {
        // Note that since subsequent statements in this method use the HTMLRect
        // object, we will update that object with the values of the text node.
        LocationInfo text_node_location;
        this->GetTextBoundaries(&text_node_location);
        rect->put_left(text_node_location.x);
        rect->put_top(text_node_location.y);
        rect->put_right(text_node_location.x + text_node_location.width);
        rect->put_bottom(text_node_location.y + text_node_location.height);
      }
    }
    if (FAILED(hr)) {
      //return EUNHANDLEDERROR;
      return false;
    }

    // If the rect of the element has zero width and height, check its
    // children to see if any of them have width and height, in which
    // case, this element will be visible.
    if (!RectHasNonZeroDimensions(rect)) {
      CComPtr<IHTMLDOMNode> node;
      bounding_rect_element->QueryInterface(&node);
      CComPtr<IDispatch> children_dispatch;
      node->get_childNodes(&children_dispatch);
      CComPtr<IHTMLDOMChildrenCollection> children;
      children_dispatch->QueryInterface<IHTMLDOMChildrenCollection>(&children);
      if (!!children) {
        long children_count = 0;
        children->get_length(&children_count);
        for (long i = 0; i < children_count; ++i) {
          CComPtr<IDispatch> child_dispatch;
          children->item(i, &child_dispatch);
          CComPtr<IHTMLElement> child;
          child_dispatch->QueryInterface(&child);
          if (child != NULL) {
            int result = WD_SUCCESS;
            Element child_element(child);
            result = child_element.GetLocationInDocument(current_location);
            if (result == WD_SUCCESS) {
              return result;
            }
          }
        }
      }
    }

    rect->get_top(&top);
    rect->get_left(&left);
    rect->get_bottom(&bottom);
    rect->get_right(&right);
  }

  long w = right - left;
  long h = bottom - top;

  if (!has_absolute_position_ready_to_return) {
    long scroll_left, scroll_top = 0;
    bounding_rect_element->get_scrollLeft(&scroll_left);
    bounding_rect_element->get_scrollTop(&scroll_top);
    left += scroll_left;
    top += scroll_top;
  }

  current_location->x = left;
  current_location->y = top;
  current_location->width = w;
  current_location->height = h;

  // return WD_SUCCESS;
  return true;
}

bool Element::IsImageMap(LocationInfo* current_location) {
  CComPtr<IHTMLElement> map_element;
  CComPtr<IHTMLAreaElement> area_element;
  CComPtr<IHTMLMapElement> map_element_candidate;
  this->element_->QueryInterface<IHTMLMapElement>(&map_element_candidate);
  if (map_element_candidate == NULL) {
    this->element_->QueryInterface<IHTMLAreaElement>(&area_element);
    if (area_element) {
      this->element_->get_parentElement(&map_element);
      if (map_element) {
        map_element->QueryInterface<IHTMLMapElement>(&map_element_candidate);
      }
    }
  }

  if (map_element_candidate && map_element) {
    CComBSTR name_bstr;
    map_element_candidate->get_name(&name_bstr);
    CComBSTR img_selector = L"*[usemap='#";
    img_selector.Append(name_bstr);
    img_selector.Append(L"']");

    CComPtr<IDispatch> doc_dispatch;
    map_element->get_document(&doc_dispatch);

    CComPtr<IDocumentSelector> doc;
    doc_dispatch->QueryInterface<IDocumentSelector>(&doc);
    if (doc) {
      CComPtr<IHTMLElement> img_element;
      doc->querySelector(img_selector, &img_element);
      if (img_element) {
        CComPtr<IHTMLElement2> rect_element;
        img_element->QueryInterface<IHTMLElement2>(&rect_element);
        if (rect_element) {
          CComPtr<IHTMLRect> rect;
          rect_element->getBoundingClientRect(&rect);
          CComBSTR shape;
          area_element->get_shape(&shape);
          shape.ToLower();
          if (shape == L"default") {
            current_location->CopyFromHtmlRect(rect);
            return true;
          }

          RECT img_rect;
          rect->get_left(&img_rect.left);
          rect->get_top(&img_rect.top);
          rect->get_right(&img_rect.right);
          rect->get_bottom(&img_rect.bottom);

          CComBSTR coords_bstr;
          area_element->get_coords(&coords_bstr);
          std::wstring coords(coords_bstr);
          std::vector<std::wstring> individual;
          StringUtilities::Split(coords, L",", &individual);
          RECT area_rect = { 0, 0, 0, 0 };
          if (shape == L"rect" && individual.size() == 4) {
            area_rect.left = std::stol(individual.at(0).c_str(), 0, 10);
            area_rect.top = std::stol(individual.at(1).c_str(), 0, 10);
            area_rect.right = std::stol(individual.at(2).c_str(), 0, 10);
            area_rect.bottom = std::stol(individual.at(3).c_str(), 0, 10);
          } else if ((shape == L"circle" || shape == "circ") &&
                     individual.size() == 3) {
            long center_x = std::stol(individual.at(0), 0, 10);
            long center_y = std::stol(individual.at(1), 0, 10);
            long radius = std::stol(individual.at(2), 0, 10);
            area_rect.left = center_x - radius;
            area_rect.top = center_y - radius;
            area_rect.right = center_x + radius;
            area_rect.bottom = center_y + radius;
          } else if ((shape == L"poly" || shape == L"polygon") &&
                     individual.size() > 2) {
            long min_x = std::stol(individual.at(0), 0, 10);
            long min_y = std::stol(individual.at(1), 0, 10);
            long max_x = min_x;
            long max_y = min_y;
            for (size_t i = 2; i + 1 < individual.size(); i += 2) {
              long next_x = std::stol(individual.at(i), 0, 10);
              long next_y = std::stol(individual.at(i + 1), 0, 10);
              min_x = min(min_x, next_x);
              max_x = max(max_x, next_x);
              min_y = min(min_y, next_y);
              max_y = max(max_y, next_y);
            }
            area_rect.left = min_x;
            area_rect.bottom = min_y;
            area_rect.right = max_x;
            area_rect.bottom = max_y;
          } else {
            // Invalid shape value or coordinate values. Not modifying location.
            return false;
          }

          long img_width = img_rect.right - img_rect.left;
          long img_height = img_rect.bottom - img_rect.top;
          long area_width = area_rect.right - area_rect.left;
          long area_height = area_rect.bottom - area_rect.top;
          current_location->x = img_rect.left + min(max(area_rect.left, 0),
                                                    img_width);
          current_location->y = img_rect.top + min(max(area_rect.top, 0),
                                                   img_height);
          current_location->width = min(area_width,
                                        img_width - current_location->x);
          current_location->height = min(area_height,
                                         img_height - current_location->y);
          return true;
        }
      }
    }
  }
  return false;
}

bool Element::IsInline() {
  // TODO(jimevans): Clean up this extreme lameness.
  // We should be checking styles here for whether the
  // element is inline or not.
  CComPtr<IHTMLAnchorElement> anchor;
  HRESULT hr = this->element_->QueryInterface(&anchor);
  if (anchor) {
    return true;
  }

  CComPtr<IHTMLSpanElement> span;
  hr = this->element_->QueryInterface(&span);
  if (span) {
    return true;
  }

  CComPtr<IHTMLLabelElement> label;
  hr = this->element_->QueryInterface(&label);
  if (label) {
    return true;
  }

  return false;
}

bool Element::RectHasNonZeroDimensions(IHTMLRect* rect) {
  long top = 0, bottom = 0, left = 0, right = 0;

  rect->get_top(&top);
  rect->get_left(&left);
  rect->get_bottom(&bottom);
  rect->get_right(&right);

  long w = right - left;
  long h = bottom - top;

  return w > 0 && h > 0;
}

bool Element::GetOverflowState(std::string* overflow_state) {
  std::wstring script_source(L"return ");
  script_source.append(atoms::asString(atoms::IS_ELEMENT_IN_PARENT_OVERFLOW));

  std::vector<CComVariant> args;
  CComVariant element_variant(this->element_);
  args.push_back(element_variant);

  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  Script script_wrapper(script_source, doc);
  int status_code = script_wrapper.Execute(args);

  if (status_code == WD_SUCCESS) {
    std::wstring raw_overflow_state(script_wrapper.result().bstrVal);
    *overflow_state = StringUtilities::ToString(raw_overflow_state);
    return true;
  }
  return false;
}

bool Element::IsXmlDocument(IHTMLDocument2* document) {
  // If the document has an xmlVersion property, it can be either an XML
  // document or an XHTML document. Otherwise, it's an HTML document.
  CComPtr<IHTMLDocument7> xml_version_document;
  HRESULT hr = document->QueryInterface<IHTMLDocument7>(&xml_version_document);
  if (SUCCEEDED(hr) && xml_version_document) {
    CComBSTR xml_version = "";
    hr = xml_version_document->get_xmlVersion(&xml_version);
    if (SUCCEEDED(hr) && xml_version && xml_version != L"") {
      // The document is either XML or XHTML, so to differentiate between
      // the two cases, check for a doctype of "html". If we can't find
      // a doctype property, or the doctype is anything other than "html",
      // the document is an XML document.
      CComPtr<IHTMLDocument5> doc_type_document;
      hr = document->QueryInterface<IHTMLDocument5>(&doc_type_document);
      if (SUCCEEDED(hr) && doc_type_document) {
        CComPtr<IHTMLDOMNode> doc_type_dom_node;
        hr = doc_type_document->get_doctype(&doc_type_dom_node);
        if (SUCCEEDED(hr) && doc_type_dom_node) {
          CComPtr<IDOMDocumentType> doc_type;
          hr = doc_type_dom_node->QueryInterface<IDOMDocumentType>(&doc_type);
          if (SUCCEEDED(hr) && doc_type) {
            CComBSTR type_name_bstr = L"";
            hr = doc_type->get_name(&type_name_bstr);
            type_name_bstr.ToLower();
            std::wstring type_name(type_name_bstr);
            if (SUCCEEDED(hr) && type_name != L"html") {
              return true;
            }
          }
        } else {
          return true;
        }
      }
    }
  }
  return false;
}

bool Element::IsContainingDocument(IHTMLDocument2* document) {
  CComPtr<IDispatch> parent_doc_dispatch;
  HRESULT hr = this->element_->get_document(&parent_doc_dispatch);
  if (FAILED(hr)) {
  }

  if (parent_doc_dispatch.IsEqualObject(document)) {
    return true;
  }
  return false;
}

bool Element::IsAttachedToDom() {
  // Verify that the element is still valid by getting the document
  // element and calling IHTMLElement::contains() to see if the document
  // contains this element.
  if (this->element_) {
    CComPtr<IHTMLDOMNode2> node;
    HRESULT hr = this->element_->QueryInterface<IHTMLDOMNode2>(&node);
    if (FAILED(hr)) {
      return false;
    }

    CComPtr<IDispatch> dispatch_doc;
    hr = node->get_ownerDocument(&dispatch_doc);
    if (FAILED(hr)) {
      return false;
    }

    if (dispatch_doc) {
      CComPtr<IHTMLDocument3> doc;
      hr = dispatch_doc.QueryInterface<IHTMLDocument3>(&doc);
      if (FAILED(hr)) {
        return false;
      }

      CComPtr<IHTMLElement> document_element;
      hr = doc->get_documentElement(&document_element);
      if (FAILED(hr)) {
        return false;
      }

      if (document_element) {
        VARIANT_BOOL contains(VARIANT_FALSE);
        hr = document_element->contains(this->element_, &contains);
        if (FAILED(hr)) {
          return false;
        }

        return contains == VARIANT_TRUE;
      }
    }
  }
  return false;
}

bool Element::HasFirstChildTextNodeOfMultipleChildren() {
  CComPtr<IHTMLDOMNode> element_node;
  HRESULT hr = this->element_.QueryInterface<IHTMLDOMNode>(&element_node);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "QueryInterface for IHTMLDOMNode on element failed.";
    return false;
  }

  CComPtr<IDispatch> child_nodes_dispatch;
  hr = element_node->get_childNodes(&child_nodes_dispatch);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_childNodes on element failed.";
    return false;
  }

  CComPtr<IHTMLDOMChildrenCollection> child_nodes;
  hr = child_nodes_dispatch.QueryInterface<IHTMLDOMChildrenCollection>(
      &child_nodes);

  long length = 0;
  hr = child_nodes->get_length(&length);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_length on child nodes collection failed.";
    return false;
  }

  // If the element has no children, then it has no single text node child.
  // If the element has only one child, then the element itself should be seen
  // as the correct size by the caller. Only in the case where we have multiple
  // children, and the first is a text element containing non-whitespace text
  // should we have to worry about using the text node as the focal point.
  if (length > 1) {
    CComPtr<IDispatch> child_dispatch;
    hr = child_nodes->item(0, &child_dispatch);
    if (FAILED(hr)) {
      // LOGHR(WARN, hr) << "Call to item(0) on child nodes collection failed.";
      return false;
    }

    CComPtr<IHTMLDOMNode> child_node;
    hr = child_dispatch.QueryInterface<IHTMLDOMNode>(&child_node);
    if (FAILED(hr)) {
      // LOGHR(WARN, hr) << "QueryInterface for IHTMLDOMNode on child node failed.";
      return false;
    }

    long node_type = 0;
    hr = child_node->get_nodeType(&node_type);
    if (FAILED(hr)) {
      // LOGHR(WARN, hr) << "Call to get_nodeType on child node failed.";
      return false;
    }

    if (node_type == 3) {
      CComVariant node_value;
      hr = child_node->get_nodeValue(&node_value);
      if (FAILED(hr)) {
        // LOGHR(WARN, hr) << "Call to get_nodeValue on child node failed.";
        return false;
      }

      if (node_value.vt != VT_BSTR) {
        // nodeValue is not a string.
        return false;
      }

      CComBSTR bstr = node_value.bstrVal;
      std::wstring node_text = node_value.bstrVal;
      if (StringUtilities::Trim(node_text) != L"") {
        // This element has a text node only if the text node
        // contains actual text other than whitespace.
        return true;
      }
    }
  }
  return false;
}

bool Element::GetTextBoundaries(LocationInfo* text_info) {
  CComPtr<IHTMLDocument2> doc;
  this->GetContainingDocument(false, &doc);
  CComPtr<IHTMLElement> body_element;
  HRESULT hr = doc->get_body(&body_element);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_body on document failed.";
    return false;
  }

  CComPtr<IHTMLBodyElement> body;
  hr = body_element.QueryInterface<IHTMLBodyElement>(&body);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "QueryInterface for IHTMLBodyElement on body element failed.";
    return false;
  }

  CComPtr<IHTMLTxtRange> range;
  hr = body->createTextRange(&range);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to createTextRange on body failed.";
    return false;
  }

  hr = range->moveToElementText(this->element_);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to moveToElementText on range failed.";
    return false;
  }

  CComPtr<IHTMLTextRangeMetrics> range_metrics;
  hr = range.QueryInterface<IHTMLTextRangeMetrics>(&range_metrics);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "QueryInterface for IHTMLTextRangeMetrics on range failed.";
    return false;
  }

  long height = 0;
  hr = range_metrics->get_boundingHeight(&height);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_boundingHeight on range metrics failed.";
    return false;
  }

  long width = 0;
  hr = range_metrics->get_boundingWidth(&width);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_boundingWidth on range metrics failed.";
    return false;
  }

  long top = 0;
  hr = range_metrics->get_offsetTop(&top);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_offsetTop on range metrics failed.";
    return false;
  }

  long left = 0;
  hr = range_metrics->get_offsetLeft(&left);
  if (FAILED(hr)) {
    // LOGHR(WARN, hr) << "Call to get_offsetLeft on range metrics failed.";
    return false;
  }

  text_info->x = left;
  text_info->y = top;
  text_info->height = height;
  text_info->width = width;
  return true;
}

} // namespace webdriver
