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

#include "RegistryUtilities.h"

#include <vector>
#include "StringUtilities.h"

#define OPEN_KEY_ERROR_TEMPLATE L"RegOpenKeyEx failed with error code %d attempting to open subkey %s in hive %s"
#define QUERY_VALUE_BUFFER_SIZE_ERROR_TEMPLATE L"RegQueryValueEx failed with error code %  retrieving required buffer size for value with name %s in subkey %s in hive %s"
#define INVALID_VALUE_TYPE_ERROR_TEMPLATE L"Unexpected value type of %d for RegQueryValueEx was found for value with name %s in subkey %s in hive %s"
#define QUERY_VALUE_ERROR_TEMPLATE L"RegQueryValueEx failed with error code %d retreiving value with name %s in subkey %s in hive %s"

namespace webdriver {

RegistryUtilities::RegistryUtilities(void) {
}

RegistryUtilities::~RegistryUtilities(void) {
}

bool RegistryUtilities::GetRegistryValue(const HKEY root_key,
                                         const std::wstring& subkey,
                                         const std::wstring& value_name,
                                         std::wstring *value) {
  return GetRegistryValue(root_key, subkey, value_name, false, value);
}

bool RegistryUtilities::GetRegistryValue(const HKEY root_key,
                                         const std::wstring& subkey,
                                         const std::wstring& value_name,
                                         const bool bypass_registry_redirection,
                                         std::wstring *value) {
  std::string root_key_description = "HKEY_CURRENT_USER";
  if (root_key == HKEY_CLASSES_ROOT) {
    root_key_description = "HKEY_CLASSES_ROOT";
  } else if (root_key == HKEY_LOCAL_MACHINE) {
    root_key_description = "HKEY_LOCAL_MACHINE";
  }
  bool value_retrieved = false;
  DWORD required_buffer_size;
  DWORD value_type;
  HKEY key_handle;
  REGSAM desired_security_mask = KEY_QUERY_VALUE;
  if (Is64BitWindows() && bypass_registry_redirection) {
    desired_security_mask |= KEY_WOW64_64KEY;
  }
  long registry_call_result = ::RegOpenKeyEx(root_key,
                                             subkey.c_str(),
                                             0,
                                             desired_security_mask,
                                             &key_handle);
  if (ERROR_SUCCESS != registry_call_result) {
    *value = StringUtilities::Format(OPEN_KEY_ERROR_TEMPLATE,
                                     registry_call_result,
                                     subkey,
                                     root_key_description);
    return false;
  }
  registry_call_result = ::RegQueryValueEx(key_handle,
                                           value_name.c_str(),
                                           NULL,
                                           &value_type,
                                           NULL,
                                           &required_buffer_size);
  if (ERROR_SUCCESS != registry_call_result) {
    *value = StringUtilities::Format(QUERY_VALUE_BUFFER_SIZE_ERROR_TEMPLATE,
                                     registry_call_result,
                                     subkey,
                                     root_key_description);
    return false;
  }
  if (value_type == REG_SZ || value_type == REG_EXPAND_SZ || value_type == REG_MULTI_SZ) {
    std::vector<wchar_t> value_buffer(required_buffer_size);
    registry_call_result = ::RegQueryValueEx(key_handle,
                                             value_name.c_str(),
                                             NULL,
                                             &value_type,
                                             reinterpret_cast<LPBYTE>(&value_buffer[0]),
                                             &required_buffer_size);
    if (ERROR_SUCCESS == registry_call_result) {
      *value = &value_buffer[0];
      value_retrieved = true;
    }
  } else if (value_type == REG_DWORD) {
    DWORD numeric_value = 0;
    registry_call_result = ::RegQueryValueEx(key_handle,
                                             value_name.c_str(),
                                             NULL,
                                             &value_type,
                                             reinterpret_cast<LPBYTE>(&numeric_value),
                                             &required_buffer_size);
    if (ERROR_SUCCESS == registry_call_result) {
      // Coerce the numeric value to a string to return back.
      // Assume 10 characters will be enough to hold the size
      // of the value.
      std::vector<wchar_t> numeric_value_buffer(10);
      _ltow_s(numeric_value, &numeric_value_buffer[0], numeric_value_buffer.size(), 10);
      *value = &numeric_value_buffer[0];
      value_retrieved = true;
    }
  } else {
    *value = StringUtilities::Format(INVALID_VALUE_TYPE_ERROR_TEMPLATE,
                                     value_type,
                                     value_name,
                                     subkey,
                                     root_key_description);
    return false;
  }
  if (ERROR_SUCCESS != registry_call_result) {
    *value = StringUtilities::Format(QUERY_VALUE_ERROR_TEMPLATE,
                                     value_type,
                                     value_name,
                                     subkey,
                                     root_key_description);
  }
  
  ::RegCloseKey(key_handle);
  return value_retrieved;
}

bool RegistryUtilities::RegistryKeyExists(HKEY root_key,
                                          const std::wstring& subkey) {
  HKEY key_handle;
  long registry_call_result = ::RegOpenKeyEx(root_key,
                                             subkey.c_str(),
                                             0,
                                             KEY_QUERY_VALUE,
                                             &key_handle);
  bool result = (ERROR_SUCCESS == registry_call_result);
  if (result) {
    ::RegCloseKey(key_handle);
  }
  return result;
}

bool RegistryUtilities::Is64BitWindows() {
  SYSTEM_INFO system_info;
  ::GetNativeSystemInfo(&system_info);
  if (system_info.wProcessorArchitecture == 0) {
    // wProcessorArchitecture == 0 means processor architecture
    // is "x86", and therefore 32-bit. Note that we don't check
    // for specific processor flavors because we don't support
    // the driver running on any architecture other than x86 or
    // x64 (AMD or Intel).
    return false;
  }
  return true;
}

} // namespace webdriver
