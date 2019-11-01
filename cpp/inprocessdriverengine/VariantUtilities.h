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

#ifndef WEBDRIVER_IE_VARIANTUTILITIES_H
#define WEBDRIVER_IE_VARIANTUTILITIES_H

#include <string>
#include <vector>

// Forward declaration of classes to avoid
// circular include files.
namespace Json {
  class Value;
} // namespace Json

namespace webdriver {

// Forward declaration of classes to avoid
// circular include files.
class IECommandExecutor;
class ElementRepository;

class VariantUtilities {
private:
  VariantUtilities(void);
  ~VariantUtilities(void);
public:
  static bool VariantIsEmpty(const VARIANT& value);
  static bool VariantIsString(const VARIANT& value);
  static bool VariantIsInteger(const VARIANT& value);
  static bool VariantIsBoolean(const VARIANT& value);
  static bool VariantIsDouble(const VARIANT& value);
  static bool VariantIsArray(const VARIANT& value);
  static bool VariantIsObject(const VARIANT& value);
  static bool VariantIsElement(const VARIANT& value);
  static bool VariantIsElementCollection(const VARIANT& value);
  static bool VariantIsIDispatch(const VARIANT& value);
  static int VariantAsJsonValue(const VARIANT& variant_value,
                                ElementRepository* element_resolver,
                                Json::Value* value);
  static bool GetVariantObjectPropertyValue(IDispatch* variant_object,
                                            const std::wstring& property_name,
                                            VARIANT* property_value);

private:
  static int ConvertVariantToJsonValue(const VARIANT& variant_value,
                                       ElementRepository* element_resolver,
                                       Json::Value* value);
  static bool HasSelfReferences(const VARIANT& current_object,
                                std::vector<IDispatch*>* visited);
  static std::wstring GetVariantObjectTypeName(const VARIANT& value);
  static bool ExecuteToJsonMethod(const VARIANT& object_to_serialize,
                                  VARIANT* json_object_variant);
  static int GetAllVariantObjectPropertyValues(
      const VARIANT& variant_value,
      ElementRepository* element_resolver,
      Json::Value* value);
  static int GetArrayLength(IDispatch* array_dispatch, long* length);
  static int GetArrayItem(IDispatch* array_dispatch,
                          long index,
                          VARIANT* item);
  static int GetPropertyNameList(IDispatch* object_dispatch,
                                 std::vector<std::wstring>* property_names);
};

} // namespace webdriver

#endif  // WEBDRIVER_IE_VARIANTUTILITIES_H
