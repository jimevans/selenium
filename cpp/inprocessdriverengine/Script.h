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

#ifndef WEBDRIVER_IE_SCRIPT_H_
#define WEBDRIVER_IE_SCRIPT_H_

#include <map>
#include <string>
#include <vector>

#define ANONYMOUS_FUNCTION_START L"(function() { "
#define ANONYMOUS_FUNCTION_END L" })();"

// Forward declaration of classes.
namespace Json {
  class Value;
} // namespace Json

namespace webdriver {

class ElementRepository;

class Script {
public:
  Script(const std::string& script_source,
         IHTMLDocument2* document,
         ElementRepository* element_resolver);
  Script(const std::wstring& script_source,
         IHTMLDocument2* document,
         ElementRepository* element_resolver);
  ~Script(void);

  std::wstring source_code() const { return this->source_code_; }
  VARIANT result() { return this->result_; }
  void set_result(VARIANT value) {
    this->result_.Copy(&value);
  }

  int Execute(const Json::Value& args);
  int Execute(const std::vector<CComVariant>& args);

private:
  void Initialize(const std::wstring& script_source,
                  IHTMLDocument2* document,
                  ElementRepository* element_resolver);

  int JsonToVariant(const Json::Value& json_arg,
                    CComVariant* variant_arg);

  int CreateAnonymousFunction(CComVariant* function_object);
  int InvokeAnonymousFunction(const CComVariant& function_object,
                              const std::vector<CComVariant>& args,
                              CComVariant* result);

  int CreateJavaScriptObject(
    const std::string& object_type,
    const std::map<std::string, CComVariant> properties,
    CComVariant* javascript_object);

  int AddPropertyToObject(const std::string& property_name,
                          const CComVariant& property_value,
                          CComVariant* object_variant);

  CComPtr<IHTMLDocument2> script_engine_host_;
  ElementRepository* element_resolver_;
  std::wstring source_code_;
  CComVariant result_;
};

} // namespace webdriver

#endif // WEBDRIVER_IE_SCRIPT_H_
