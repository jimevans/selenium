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

#include "GetElementTextCommandHandler.h"

#include <vector>

#include "json.h"
#include "../../utils/WebDriverConstants.h"
#include "../../webdriver-server/errorcodes.h"

#include "../CustomTypes.h"
#include "../Element.h"
#include "../ElementRepository.h"
#include "../Generated/atoms.h"
#include "../InProcessDriver.h"
#include "../Script.h"
#include "../VariantUtilities.h"

namespace webdriver {

GetElementTextCommandHandler::GetElementTextCommandHandler(void) {
}

GetElementTextCommandHandler::~GetElementTextCommandHandler(void) {
}

void GetElementTextCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  ParametersMap::const_iterator id_parameter_iterator =
      command_parameters.find("id");
  if (id_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "Missing parameter in URL: id");
    return;
  }

  std::string element_id = id_parameter_iterator->second.asString();

  CComPtr<IHTMLDocument2> doc;
  int status_code = executor.GetFocusedDocument(&doc);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code,
                               "Unexpected error retrieving focused document");
    return;
  }

  InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
  ElementRepository* element_repository =
      mutable_executor.known_element_repository();

  ElementHandle element_wrapper;
  status_code = element_repository->GetManagedElement(element_id,
                                                      &element_wrapper);
  if (status_code == ENOSUCHELEMENT) {
      response->SetErrorResponse(
          ERROR_NO_SUCH_ELEMENT,
          "Invalid internal element ID requested: " + element_id);
      return;
  } else if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(ERROR_STALE_ELEMENT_REFERENCE,
                               "Element is no longer valid");
    return;
  }

  std::wstring script_source(L"return ");
  script_source.append(atoms::asString(atoms::GET_TEXT));

  Script get_text_script(script_source, doc, element_repository);
  CComVariant element(element_wrapper->element());
  std::vector<CComVariant> args;
  args.push_back(element);
  status_code = get_text_script.Execute(args);

  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code,
                               "Unable to get element text");
    return;
  }

  Json::Value text_value;
  status_code = VariantUtilities::VariantAsJsonValue(get_text_script.result(),
                                                     element_repository,
                                                     &text_value);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code,
                               "Unable to get element text");
    return;
  }
  if (!text_value.isString()) {
    // This really should never happen, since we're executing an atom
    // over which we have complete control. Nevertheless, check for
    // the error here, just in case.
    response->SetErrorResponse(
        ERROR_JAVASCRIPT_ERROR,
        "Atom retrieving text was executed, but did not return a string");
    return;
  }
  response->SetSuccessResponse(text_value.asString());
}

} // namespace webdriver
