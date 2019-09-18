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

#include "Script.h"

#include "json.h"
#include "../utils/StringUtilities.h"
#include "../webdriver-server/errorcodes.h"

#include "Element.h"
#include "ElementRepository.h"
#include "ScriptException.h"

#define JAVASCRIPT_OBJECT "Object"
#define JAVASCRIPT_ARRAY "Array"
#define JAVASCRIPT_FUNCTION "Function"

namespace webdriver {

Script::Script(const std::string& script_source,
               IHTMLDocument2* document,
               ElementRepository* element_resolver) {
  std::wstring wide_script = StringUtilities::ToWString(script_source);
  this->Initialize(wide_script, document, element_resolver);
}

Script::Script(const std::wstring& script_source,
               IHTMLDocument2* document,
               ElementRepository* element_resolver) {
  this->Initialize(script_source, document, element_resolver);
}

Script::~Script(void) {
}

void Script::Initialize(const std::wstring& script_source,
                        IHTMLDocument2* document,
                        ElementRepository* element_resolver) {
  this->script_engine_host_ = document;
  this->element_resolver_ = element_resolver;
  this->source_code_ = script_source;
}

int Script::Execute(const Json::Value& args) {
  if (!args.isArray()) {
    return EINVALIDARGUMENT;
  }

  std::vector<CComVariant> variant_args;
  for (Json::ArrayIndex index = 0; index < args.size(); ++index) {
    Json::Value arg = args[index];
    CComVariant variant_arg;
    this->JsonToVariant(arg, &variant_arg);
    variant_args.push_back(variant_arg);
  }
  return this->Execute(variant_args);
}

int Script::Execute(const std::vector<CComVariant>& args) {
  int status_code = WD_SUCCESS;
  CComVariant function_object;
  status_code = this->CreateAnonymousFunction(&function_object);
  if (status_code != WD_SUCCESS) {
    // LogError("Failed to create anonymous function");
    return status_code;
  }
  CComVariant result;
  status_code = this->InvokeAnonymousFunction(function_object, args, &result);
  this->result_.Copy(&result);
  return status_code;
}

int Script::CreateAnonymousFunction(CComVariant* function_object) {
  HRESULT hr = S_OK;
  CComPtr<IDispatch> script_dispatch;
  hr = this->script_engine_host_->get_Script(&script_dispatch);
  if (FAILED(hr)) {
    // LogError("Failed to get script IDispatch from document pointer", hr);
    return EUNEXPECTEDJSERROR;
  }

  CComPtr<IDispatchEx> script_engine;
  hr = script_dispatch->QueryInterface<IDispatchEx>(&script_engine);
  if (FAILED(hr)) {
    // LogError("Failed to get script IDispatch from script IDispatch", hr);
    return EUNEXPECTEDJSERROR;
  }

  CComVariant script_source_variant(this->source_code_.c_str());

  std::vector<CComVariant> argument_array;
  argument_array.push_back(script_source_variant);

  DISPPARAMS ctor_parameters = { 0 };
  memset(&ctor_parameters, 0, sizeof ctor_parameters);
  ctor_parameters.cArgs = static_cast<unsigned int>(argument_array.size());
  ctor_parameters.rgvarg = &argument_array[0];

  // Find the JavaScript Function object using the IDispatchEx of the
  // script engine
  DISPID dispatch_id;
  hr = script_engine->GetDispID(CComBSTR(JAVASCRIPT_FUNCTION),
                                0,
                                &dispatch_id);
  if (FAILED(hr)) {
    // LogError("Failed to get DispID for Function constructor", hr);
    return EUNEXPECTEDJSERROR;
  }

  CComVariant function_creator;
  // Create the JavaScript function creator function by calling
  // its constructor.
  hr = script_engine->InvokeEx(dispatch_id,
                               LOCALE_USER_DEFAULT,
                               DISPATCH_CONSTRUCT,
                               &ctor_parameters,
                               &function_creator,
                               nullptr,
                               nullptr);

  // We now have a function that returns a function. Execute it
  // to return the function to execute the user's JavaScript code.
  std::vector<CComVariant> empty_args;
  hr = this->InvokeAnonymousFunction(function_creator,
                                     empty_args,
                                     function_object);
  return WD_SUCCESS;
}

int Script::InvokeAnonymousFunction(const CComVariant& function_object,
                                    const std::vector<CComVariant>& args,
                                    CComVariant* result) {
  HRESULT hr = S_OK;
  CComPtr<IDispatchEx> function_dispatch;
  hr = function_object.pdispVal->QueryInterface<IDispatchEx>(
      &function_dispatch);
  if (FAILED(hr)) {
    // LogError("Failed to get IDispatch from function object", hr);
    return EUNEXPECTEDJSERROR;
  }

  // Grab the "call" method out of the returned function
  DISPID call_member_id;
  CComBSTR call_member_name = L"call";
  hr = function_dispatch->GetDispID(call_member_name, 0, &call_member_id);
  if (FAILED(hr)) {
    // LogError("Failed to get dispatch ID of call method", hr);
    return EUNEXPECTEDJSERROR;
  }

  // Get the document window to serve as the 'this' object.
  CComPtr<IHTMLWindow2> win;
  hr = this->script_engine_host_->get_parentWindow(&win);
  if (FAILED(hr)) {
    // LogError("Failed to get document's window object", hr);
    return EUNEXPECTEDJSERROR;
  }

  // IDispatch::Invoke() expects the arguments to be passed into it
  // in reverse order. To accomplish this, we create a new variant
  // array of size n + 1 where n is the number of arguments we have.
  // we copy each element of arguments_array_ into the new array in
  // reverse order, and add an extra argument, the window object,
  // to the end of the array to use as the "this" parameter for the
  // function invocation.
  std::vector<CComVariant> argument_array(args.size() + 1);
  for (size_t index = 0; index < args.size(); ++index) {
    argument_array[args.size() - 1 - index].Copy(&args[index]);
  }

  CComVariant window_variant(win);
  argument_array[argument_array.size() - 1].Copy(&window_variant);

  DISPPARAMS call_parameters = { 0 };
  memset(&call_parameters, 0, sizeof call_parameters);
  call_parameters.cArgs = static_cast<unsigned int>(argument_array.size());
  call_parameters.rgvarg = &argument_array[0];

  // Setup custom exception handling object for script.
  EXCEPINFO exception;
  memset(&exception, 0, sizeof exception);
  CComPtr<IServiceProvider> custom_exception_service_provider;
  hr = ScriptException::CreateInstance<IServiceProvider>(
      &custom_exception_service_provider);
  if (FAILED(hr)) {
    // LogError("Failed to create instance of script exception handler", hr);
    return EUNEXPECTEDJSERROR;
  }
  CComPtr<IScriptException> custom_exception;
  hr = custom_exception_service_provider.QueryInterface<IScriptException>(
      &custom_exception);
  if (FAILED(hr)) {
    // LogError("Failed to get IScriptException interface", hr);
    return EUNEXPECTEDJSERROR;
  }

  hr = function_dispatch->InvokeEx(call_member_id,
                                   LOCALE_USER_DEFAULT,
                                   DISPATCH_METHOD,
                                   &call_parameters,
                                   result,
                                   &exception,
                                   custom_exception_service_provider);

  if (FAILED(hr)) {
    CComBSTR error_description = L"EUNEXPECTEDJSERROR";
    if (DISP_E_EXCEPTION == hr) {
      if (exception.bstrDescription) {
        error_description = exception.bstrDescription;
      }
      // CComBSTR error_source(exception.bstrSource ? exception.bstrSource : L"EUNEXPECTEDJSERROR");
      // LOG(INFO) << "Exception message was: '" << error_description << "'";
      // LOG(INFO) << "Exception source was: '" << error_source << "'";
    } else {
      bool is_handled = false;
      hr = custom_exception->IsExceptionHandled(&is_handled);
      if (is_handled) {
        error_description = "Error from JavaScript: ";
        CComBSTR script_message = L"";
        custom_exception->GetDescription(&script_message);
        error_description.Append(script_message);
        // LOG(DEBUG) << script_message;
      } else {
        // LOGHR(DEBUG, hr) << "Failed to execute anonymous function, no exception information retrieved";
      }
    }

    result->Clear();
    result->vt = VT_BSTR;
    result->bstrVal = error_description;
    return EUNEXPECTEDJSERROR;
  }

  return WD_SUCCESS;
}

int Script::JsonToVariant(const Json::Value& json_arg,
                          CComVariant* variant_arg) {
  int status_code = WD_SUCCESS;
  if (json_arg.isString()) {
    std::string value = json_arg.asString();
    *variant_arg = StringUtilities::ToWString(value).c_str();
  } else if (json_arg.isInt()) {
    int int_number = json_arg.asInt();
    *variant_arg = int_number;
  } else if (json_arg.isDouble()) {
    double dbl_number = json_arg.asDouble();
    *variant_arg = dbl_number;
  } else if (json_arg.isBool()) {
    bool bool_arg = json_arg.asBool();
    *variant_arg = bool_arg;
  } else if (json_arg.isNull()) {
    variant_arg->vt = VT_NULL;
  } else if (json_arg.isArray()) {
    std::map<std::string, CComVariant> array_values;
    for (Json::ArrayIndex index = 0; index < json_arg.size(); ++index) {
      Json::Value array_value = json_arg[index];
      CComVariant array_value_variant;
      this->JsonToVariant(array_value,
                          &array_value_variant);
      std::string index_string = std::to_string(index);
      array_values[index_string] = array_value_variant;
    }
    status_code = this->CreateJavaScriptObject(JAVASCRIPT_ARRAY,
                                               array_values,
                                               variant_arg);
  } else if (json_arg.isObject()) {
    if (json_arg.isMember(JSON_ELEMENT_PROPERTY_NAME)) {
      std::string element_id = json_arg[JSON_ELEMENT_PROPERTY_NAME].asString();
      ElementHandle wrapped_element;
      status_code = this->element_resolver_->GetManagedElement(
          element_id, &wrapped_element);
      if (status_code == WD_SUCCESS) {
        bool is_element_valid = wrapped_element->IsAttachedToDom();
        if (is_element_valid) {
          is_element_valid = wrapped_element->IsContainingDocument(
              this->script_engine_host_);
        } else {
          this->element_resolver_->RemoveManagedElement(element_id);
        }

        if (is_element_valid) {
          *variant_arg = wrapped_element->element();
        } else {
          status_code = EOBSOLETEELEMENT;
        }
      }
    } else {
      std::map<std::string, CComVariant> object_properties;
      std::vector<std::string> property_names = json_arg.getMemberNames();
      std::vector<std::string>::const_iterator it = property_names.begin();
      for (; it != property_names.end(); ++it) {
        Json::Value property_value = json_arg[*it];
        CComVariant property_value_variant;
        this->JsonToVariant(property_value,
                            &property_value_variant);
        object_properties[*it] = property_value_variant;
      }
      status_code = this->CreateJavaScriptObject(JAVASCRIPT_OBJECT,
                                                 object_properties,
                                                 variant_arg);
    }
  }
  return status_code;
}

int Script::CreateJavaScriptObject(
    const std::string& object_type,
    const std::map<std::string, CComVariant> properties,
    CComVariant* javascript_object) {
  HRESULT hr = S_OK;
  CComPtr<IDispatch> script_dispatch;
  hr = this->script_engine_host_->get_Script(&script_dispatch);
  if (FAILED(hr)) {
    //LogError("Failed to get script IDispatch from document pointer", hr);
    return EUNEXPECTEDJSERROR;
  }

  CComPtr<IDispatchEx> script_engine;
  hr = script_dispatch->QueryInterface<IDispatchEx>(&script_engine);
  if (FAILED(hr)) {
    //LogError("Failed to get script IDispatchEx from script IDispatch", hr);
    return EUNEXPECTEDJSERROR;
  }

  // Find the javascript object prototype using the IDispatchEx of the
  // script engine
  DISPID dispatch_id;
  hr = script_engine->GetDispID(CComBSTR(object_type.c_str()),
                                0,
                                &dispatch_id);
  if (FAILED(hr)) {
    //LogError("Failed to get DispID for Object constructor", hr);
    return EUNEXPECTEDJSERROR;
  }

  // Create the variables we need
  DISPPARAMS no_arguments_dispatch_params = { nullptr, nullptr, 0, 0 };
  CComVariant created_javascript_object;

  // Create the jscript object by calling its constructor
  hr = script_engine->InvokeEx(dispatch_id,
                               LOCALE_USER_DEFAULT,
                               DISPATCH_CONSTRUCT,
                               &no_arguments_dispatch_params,
                               &created_javascript_object,
                               nullptr,
                               nullptr);
  if (FAILED(hr)) {
    //LogError("Failed to call InvokeEx on Object constructor", hr);
    return EUNEXPECTEDJSERROR;
  }

  // Add the property values to the newly created object.
  std::map<std::string, CComVariant>::const_iterator it = properties.begin();
  for (; it != properties.end(); ++it) {
    int status = this->AddPropertyToObject(it->first,
                                           it->second,
                                           &created_javascript_object);
    if (status != WD_SUCCESS) {
      //LogError("Failed to add property " + it->first + " to object", hr);
      return status;
    }
  }

  javascript_object->Copy(&created_javascript_object);
  return WD_SUCCESS;
}

int Script::AddPropertyToObject(const std::string& property_name,
                                const CComVariant& property_value,
                                CComVariant* object_variant) {
  std::wstring name = StringUtilities::ToWString(property_name);

  HRESULT hr = S_OK;
  CComPtr<IDispatch> object_dispatch = object_variant->pdispVal;
  CComPtr<IDispatchEx> object_dispatch_ex;
  hr = object_dispatch->QueryInterface<IDispatchEx>(&object_dispatch_ex);
  if (FAILED(hr)) {
    //LogError("Failed to get object IDispatchEx from script IDispatch", hr);
    return EUNEXPECTEDJSERROR;
  }

  DISPID dispatch_id;
  hr = object_dispatch_ex->GetDispID(CComBSTR(name.c_str()),
                                     fdexNameEnsure,
                                     &dispatch_id);
  if (FAILED(hr)) {
    //LogError("Failed to get dispatch ID for property name", hr);
    return EUNEXPECTEDJSERROR;
  }

  std::vector<VARIANTARG> params;
  params.push_back(property_value);

  DISPID named_arguments[] = { DISPID_PROPERTYPUT };
  DISPPARAMS parameters = { &params[0], named_arguments, 1, 1 };
  hr = object_dispatch_ex->InvokeEx(dispatch_id,
                                    LOCALE_USER_DEFAULT,
                                    DISPATCH_PROPERTYPUTREF,
                                    &parameters,
                                    nullptr,
                                    nullptr,
                                    nullptr);
  if (FAILED(hr)) {
    //LogError("Failed to get invoke setting of property to value", hr);
    return EUNEXPECTEDJSERROR;
  }

  return WD_SUCCESS;
}


} // namespace webdriver
