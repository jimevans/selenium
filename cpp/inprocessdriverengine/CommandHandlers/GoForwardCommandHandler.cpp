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

#include "GoForwardCommandHandler.h"

//#include <MsHTML.h>

#include "../../utils/StringUtilities.h"
#include "../../webdriver-server/errorcodes.h"

#include "../InProcessDriver.h"

namespace webdriver {

GoForwardCommandHandler::GoForwardCommandHandler(void) {
}

GoForwardCommandHandler::~GoForwardCommandHandler(void) {
}

void GoForwardCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
  mutable_executor.set_is_navigating(true);

  CComPtr<IWebBrowser2> browser = mutable_executor.browser();
  HRESULT hr = browser->GoForward();
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

} // namespace webdriver
