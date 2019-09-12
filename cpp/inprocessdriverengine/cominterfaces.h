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

//#ifndef WEBDRIVER_INPROCESSDRIVERENGINE_COMINTERFACES_H_
//#define WEBDRIVER_INPROCESSDRIVERENGINE_COMINTERFACES_H_

EXTERN_C const IID IID_IInProcessDriver;

MIDL_INTERFACE("c4edf20a-fc4d-4f04-b029-c8d338210d00")
IInProcessDriver : public IUnknown {
 public:
};

EXTERN_C const IID LIBID_InProcessDriverEngineLib;

EXTERN_C const CLSID CLSID_InProcessDriver;

class DECLSPEC_UUID("ff5186d0-7673-4ea6-8113-bc960c030891")
InProcessDriver;

EXTERN_C const IID IID_IScriptException;

EXTERN_C const CLSID CLSID_ScriptException;

MIDL_INTERFACE("cbdf5555-73f8-472d-ae74-8932d2f53748")
IScriptException : public IUnknown{
 public:
  virtual HRESULT STDMETHODCALLTYPE IsExceptionHandled(bool* is_handled) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetDescription(BSTR* description) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetSource(BSTR* source) = 0;
};


class DECLSPEC_UUID("7e2b9663-b2f0-44f0-890d-64fb08efc779")
  ScriptException;

//#endif // WEBDRIVER_INPROCESSDRIVERENGINE_COMINTERFACES_H_
