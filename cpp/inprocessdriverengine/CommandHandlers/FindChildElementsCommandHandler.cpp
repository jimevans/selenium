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

#include "FindChildElementsCommandHandler.h"

#include <ctime>

#include "../../utils/WebDriverConstants.h"
#include "../../webdriver-server/errorcodes.h"

#include "../Element.h"
#include "../ElementFinder.h"
#include "../ElementRepository.h"
#include "../InProcessDriver.h"

namespace webdriver {

FindChildElementsCommandHandler::FindChildElementsCommandHandler(void) {
}

FindChildElementsCommandHandler::~FindChildElementsCommandHandler(void) {
}

void FindChildElementsCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  ParametersMap::const_iterator id_parameter_iterator =
      command_parameters.find("id");
  ParametersMap::const_iterator using_parameter_iterator =
      command_parameters.find("using");
  ParametersMap::const_iterator value_parameter_iterator =
      command_parameters.find("value");
  if (id_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "Missing parameter in URL: id");
    return;
  }
  if (using_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "Missing parameter: using");
    return;
  }
  if (!using_parameter_iterator->second.isString()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "using parameter must be a string");
    return;
  }
  if (value_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "Missing parameter: value");
    return;
  }
  if (!value_parameter_iterator->second.isString()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                               "value parameter must be a string");
    return;
  }

  std::string mechanism = using_parameter_iterator->second.asString();
  std::string value = value_parameter_iterator->second.asString();
  std::string element_id = id_parameter_iterator->second.asString();

  if (mechanism != "css selector" &&
      mechanism != "tag name" &&
      mechanism != "link text" &&
      mechanism != "partial link text" &&
      mechanism != "xpath") {
    response->SetErrorResponse(
        ERROR_INVALID_ARGUMENT,
        "using parameter value '" + mechanism + "' is not a valid value");
    return;
  }

  int timeout = 0;
  ::SendMessage(executor.settings_window_handle(),
                WD_GET_SESSION_SETTING,
                static_cast<WPARAM>(SESSION_SETTING_IMPLICIT_WAIT_TIMEOUT),
                reinterpret_cast<LPARAM>(&timeout));

  CComPtr<IHTMLDocument2> doc;
  int status_code = executor.GetFocusedDocument(&doc);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code,
                               "Unexpected error retrieving focused document");
    return;
  }

  clock_t end = clock() + (timeout / 1000 * CLOCKS_PER_SEC);
  if (timeout > 0 && timeout < 1000) {
    end += 1 * CLOCKS_PER_SEC;
  }

  status_code = WD_SUCCESS;
  InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
  ElementRepository* element_repository =
      mutable_executor.known_element_repository();

  ElementHandle element_wrapper;
  status_code = element_repository->GetManagedElement(element_id,
                                                      &element_wrapper);
  if (status_code != WD_SUCCESS) {
    if (status_code == EOBSOLETEELEMENT) {
      response->SetErrorResponse(
          ERROR_STALE_ELEMENT_REFERENCE,
          "Specified parent element is no longer attached to the DOM");
    } else {
      response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
                                 "Element is no longer valid");
    }
    return;
  }
  FindElementSettings settings;
  settings.mechanism = mechanism;
  settings.criteria = value;
  settings.host_document = doc;
  settings.context_element = element_wrapper->element();

  Json::Value found_elements;
  do {
    status_code = executor.element_finder()->FindElements(
        settings,
        mutable_executor.known_element_repository(),
        &found_elements);
    if (status_code == WD_SUCCESS) {
      if (found_elements.isArray() && found_elements.size() > 0) {
        response->SetSuccessResponse(found_elements);
        return;
      }
    } else if (status_code == ENOSUCHWINDOW) {
      response->SetErrorResponse(ERROR_NO_SUCH_WINDOW,
                                 "Unable to find element on closed window");
      return;
    } else {
      response->SetErrorResponse(status_code, found_elements.asString());
      return;
    }

    // Release the thread so that the browser doesn't starve.
    ::Sleep(FIND_ELEMENT_WAIT_TIME_IN_MILLISECONDS);
  } while (clock() < end);

  if (status_code == WD_SUCCESS) {
    // This code is executed when no elements where found and no errors occurred.
    response->SetSuccessResponse(found_elements);
    return;
  }

  response->SetErrorResponse(ERROR_NO_SUCH_ELEMENT,
                             "Unable to find element with " + mechanism + " == " + value);
  return;
}

} // namespace webdriver
