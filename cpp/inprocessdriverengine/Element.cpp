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
  }
  catch (...) {
    //LOG(WARN) << "Found document but it's not the expected type (IHTMLDocument2)";
    return false;
  }

  return true;
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

} // namespace webdriver
