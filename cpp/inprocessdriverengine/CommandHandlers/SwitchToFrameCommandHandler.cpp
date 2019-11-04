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

#include "SwitchToFrameCommandHandler.h"

#include "../../webdriver-server/errorcodes.h"

#include "../CustomTypes.h"
#include "../Element.h"
#include "../ElementRepository.h"

namespace webdriver {

SwitchToFrameCommandHandler::SwitchToFrameCommandHandler(void) {
}

SwitchToFrameCommandHandler::~SwitchToFrameCommandHandler(void) {
}

void SwitchToFrameCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  Json::Value frame_id = Json::Value::null;
  ParametersMap::const_iterator it = command_parameters.find("id");
  if (it != command_parameters.end()) {
    frame_id = it->second;
  } else {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT, "Missing parameter: id");
    return;
  }

  int status_code = WD_SUCCESS;
  InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);
  std::string error_message = "No frame found";
  if (frame_id.isNull()) {
    mutable_executor.SetFocusedFrameByElement(NULL);
  } else if (frame_id.isObject()) {
    Json::Value element_id = frame_id.get(JSON_ELEMENT_PROPERTY_NAME, Json::Value::null);

    if (element_id.isNull()) {
      status_code = EINVALIDARGUMENT;
      error_message = "Frame identifier was an object, but not a web element reference";
    } else {
      std::string frame_element_id = element_id.asString();

      ElementHandle frame_element_wrapper;
      status_code = mutable_executor.known_element_repository()->
          GetManagedElement(frame_element_id, &frame_element_wrapper);
      if (status_code == WD_SUCCESS) {
        status_code = mutable_executor.SetFocusedFrameByElement(
            frame_element_wrapper->element());
      }
    }
  } else if(frame_id.isIntegral()) {
    int frame_index = frame_id.asInt();
    if (frame_index < 0 || frame_index > 65535) {
      status_code = EINVALIDARGUMENT;
      error_message = "Frame identifier was an integer, but must be between 0 and 65535 inclusive";
    } else {
      status_code = mutable_executor.SetFocusedFrameByIndex(frame_index);
    }
  } else {
    status_code = EINVALIDARGUMENT;
    error_message = "Frame identifier argument must be null, an integer, or a web element reference";
  }

  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code, error_message);
  } else {
    response->SetSuccessResponse(Json::Value::null);
  }
}

} // namespace webdriver
