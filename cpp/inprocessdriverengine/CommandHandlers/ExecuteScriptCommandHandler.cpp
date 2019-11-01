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

#include "ExecuteScriptCommandHandler.h"

#include <string>

#include "../../webdriver-server/errorcodes.h"
#include "../ElementRepository.h"
#include "../InProcessDriver.h"
#include "../Script.h"
#include "../VariantUtilities.h"

namespace webdriver {

ExecuteScriptCommandHandler::ExecuteScriptCommandHandler(void) {
}

ExecuteScriptCommandHandler::~ExecuteScriptCommandHandler(void) {
}

void ExecuteScriptCommandHandler::ExecuteInternal(
    const InProcessDriver& executor,
    const ParametersMap& command_parameters,
    Response* response) {
  ParametersMap::const_iterator script_parameter_iterator =
    command_parameters.find("script");
  ParametersMap::const_iterator args_parameter_iterator =
    command_parameters.find("args");
  if (script_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
      "Missing parameter: script");
    return;
  }

  if (!script_parameter_iterator->second.isString()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
      "script parameter must be a string");
    return;
  }

  if (args_parameter_iterator == command_parameters.end()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
      "Missing parameter: args");
    return;
  }

  if (!args_parameter_iterator->second.isArray()) {
    response->SetErrorResponse(ERROR_INVALID_ARGUMENT,
      "args parameter must be an array");
    return;
  }

  std::string script_body = script_parameter_iterator->second.asString();
  std::string script_source = "return function() {\n";
  script_source.append(script_body);
  script_source.append("\n}");

  Json::Value json_args(args_parameter_iterator->second);

  CComPtr<IHTMLDocument2> doc;
  int status_code = executor.GetFocusedDocument(&doc);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code,
                               "Unexpected error retrieving focused document");
    return;
  }

  InProcessDriver& mutable_executor = const_cast<InProcessDriver&>(executor);

  Script script_wrapper(script_source, doc);

  status_code = script_wrapper.Execute(
      json_args,
      mutable_executor.known_element_repository());

  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(status_code, "JavaScript error");
    return;
  }

  Json::Value result_value;
  status_code = VariantUtilities::VariantAsJsonValue(
      script_wrapper.result(),
      mutable_executor.known_element_repository(),
      &result_value);
  if (status_code != WD_SUCCESS) {
    response->SetErrorResponse(
        status_code,
        "Error encountered converting script return value to JSON");
  }
  response->SetSuccessResponse(result_value);
}

} // namespace webdriver
