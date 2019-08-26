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

#include "GoToUrlCommandHandler.h"

#include <MsHTML.h>

#include "../../utils/StringUtilities.h"
#include "../../webdriver-server/errorcodes.h"

#include "../InProcessDriver.h"

namespace webdriver {

GoToUrlCommandHandler::GoToUrlCommandHandler(void) {
}

GoToUrlCommandHandler::~GoToUrlCommandHandler(void) {
}

void GoToUrlCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  ParametersMap::const_iterator url_parameter_iterator =
      command_parameters.find("url");
  if (url_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "Missing parameter: url");
    return;
  } else {
    // TODO: PathIsURL isn't quite the right thing. We need to
    // find the correct API that will parse the URL to tell 
    // us whether the URL is valid according to the URL spec.
    std::string url = url_parameter_iterator->second.asString();
    std::wstring wide_url = StringUtilities::ToWString(url);

    // TODO: Validate URL for completeness, and for navigation to
    // spec-compatible locations.
    // TODO: Handle cross-zone navigation for Protected Mode.

    InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
    mutable_executor.set_is_navigating(true);

    CComPtr<IWebBrowser2> browser = mutable_executor.browser();
    CComVariant dummy;
    CComVariant url_variant(url.c_str());
    HRESULT hr = browser->Navigate2(&url_variant,
      &dummy,
      &dummy,
      &dummy,
      &dummy);
    if (FAILED(hr)) {
      mutable_executor.set_is_navigating(false);
      _com_error error(hr);
      std::wstring formatted_message = 
          StringUtilities::Format(L"Received error: 0x%08x ['%s']",
                                  hr,
                                  error.ErrorMessage());
      response->SetErrorResponse(ERROR_UNKNOWN_ERROR,
                                 StringUtilities::ToString(formatted_message));
      return;
    }

    response->SetSuccessResponse(Json::Value::null);
  }
}

} // namespace webdriver
