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

#include "GetCurrentUrlCommandHandler.h"

#include "../../utils/StringUtilities.h"

#include "../InProcessDriver.h"

namespace webdriver {

GetCurrentUrlCommandHandler::GetCurrentUrlCommandHandler(void) {
}

GetCurrentUrlCommandHandler::~GetCurrentUrlCommandHandler(void) {
}

void GetCurrentUrlCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {

  CComBSTR url;
  HRESULT hr = executor.browser()->get_LocationURL(&url);
  if (FAILED(hr)) {
    //LOGHR(WARN, hr) << "Unable to get current URL, call to IWebBrowser2::get_LocationURL failed";
    response->SetSuccessResponse("");
    return;
  }

  std::wstring converted_url = url;
  std::string current_url = StringUtilities::ToString(converted_url);

  CComPtr<IDispatch> document_dispatch;
  hr = executor.browser()->get_Document(&document_dispatch);
  if (SUCCEEDED(hr) && document_dispatch) {
    CComPtr<IHTMLDocument2> top_level_document;
    hr = document_dispatch->QueryInterface<IHTMLDocument2>(&top_level_document);
    if (SUCCEEDED(hr) && top_level_document) {
      CComBSTR document_url;
      hr = top_level_document->get_URL(&document_url);
      std::wstring converted_document_url(document_url,
                                          ::SysStringLen(document_url));


      // HACK: If the URL starts with "res://", an internal
      // resource was loaded, so don't get the document URL.
      if (converted_document_url.find_first_of(L"res://") != 0) {
        current_url = StringUtilities::ToString(converted_document_url);
      }
    }
  }

  response->SetSuccessResponse(current_url);
}

} // namespace webdriver
