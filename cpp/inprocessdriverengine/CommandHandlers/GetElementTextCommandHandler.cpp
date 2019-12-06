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

#include <string>
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

  InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
  ElementRepository* element_repository =
      mutable_executor.known_element_repository();

  ElementHandle element_wrapper;
  int status_code = element_repository->GetManagedElement(element_id,
                                                          &element_wrapper);
  if (status_code != WD_SUCCESS) {
    if (status_code == ENOSUCHELEMENT) {
      response->SetErrorResponse(
          ERROR_NO_SUCH_ELEMENT,
          "Invalid internal element ID requested: " + element_id);
    } else {
      response->SetErrorResponse(status_code,
                                 "Element is no longer valid");
    }
    return;
  }

  std::string visible_text;
  bool success = element_wrapper->GetVisibleText(&visible_text);
  if (!success) {
    response->SetErrorResponse(ERROR_UNKNOWN_ERROR,
                               "Unable to get element text");
    return;
  }
  response->SetSuccessResponse(visible_text);
}

} // namespace webdriver
