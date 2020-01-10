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

#include "GetPageSourceCommandHandler.h"

#include "../../utils/StringUtilities.h"

#include "../InProcessDriver.h"

namespace webdriver {

GetPageSourceCommandHandler::GetPageSourceCommandHandler(void) {
}

GetPageSourceCommandHandler::~GetPageSourceCommandHandler(void) {
}

void GetPageSourceCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  CComPtr<IHTMLDocument2> doc;
  executor.GetFocusedDocument(&doc);
  if (!doc) {
    //LOG(WARN) << "Unable to get document object, DocumentHost::GetDocument did not return a valid IHTMLDocument2 pointer";
    return;
  }

  CComPtr<IHTMLDocument3> doc3;
  HRESULT hr = doc->QueryInterface<IHTMLDocument3>(&doc3);
  if (FAILED(hr) || !doc3) {
    //LOG(WARN) << "Unable to get document object, QueryInterface to IHTMLDocument3 failed";
    return;
  }

  CComPtr<IHTMLElement> document_element;
  hr = doc3->get_documentElement(&document_element);
  if (FAILED(hr) || !document_element) {
    //LOGHR(WARN, hr) << "Unable to get document element from page, call to IHTMLDocument3::get_documentElement failed";
    return;
  }

  CComBSTR html;
  hr = document_element->get_outerHTML(&html);
  if (FAILED(hr)) {
    //LOGHR(WARN, hr) << "Have document element but cannot read source, call to IHTMLElement::get_outerHTML failed";
    return;
  }

  std::wstring converted_html = html;
  std::string page_source = webdriver::StringUtilities::ToString(converted_html);
  response->SetSuccessResponse(page_source);
}

} // namespace webdriver
