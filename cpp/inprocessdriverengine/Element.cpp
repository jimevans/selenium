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

#include "../utils/StringUtilities.h"
#include "json.h"

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

bool Element::IsContainingDocument(IHTMLDocument2* document) {
  CComPtr<IDispatch> parent_doc_dispatch;
  this->element_->get_document(&parent_doc_dispatch);

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

} // namespace webdriver
