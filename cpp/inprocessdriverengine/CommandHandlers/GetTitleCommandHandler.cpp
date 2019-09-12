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

#include "GetTitleCommandHandler.h"

#include <MsHTML.h>

#include "../../utils/StringUtilities.h"
#include "../../webdriver-server/errorcodes.h"

#include "../InProcessDriver.h"

namespace webdriver {

GetTitleCommandHandler::GetTitleCommandHandler(void) {
}

GetTitleCommandHandler::~GetTitleCommandHandler(void) {
}

void GetTitleCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  CComPtr<IDispatch> dispatch;
  HRESULT hr = executor.browser()->get_Document(&dispatch);
  if (FAILED(hr)) {
  }

  CComPtr<IHTMLDocument2> doc;
  hr = dispatch->QueryInterface(&doc);
  if (FAILED(hr)) {
  }

  CComBSTR title;
  hr = doc->get_title(&title);
  if (FAILED(hr)) {
  }

  std::wstring converted_title = title;
  response->SetSuccessResponse(StringUtilities::ToString(converted_title));
}

} // namespace webdriver
