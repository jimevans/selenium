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

#include "dllmain.h"
#include "resource.h"

using namespace ATL;

CInProcessDriverEngineModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance,
                               DWORD dwReason,
                               LPVOID lpReserved) {
  hInstance;
  return _AtlModule.DllMain(dwReason, lpReserved);
}

// Used to determine whether the DLL can be unloaded by OLE.
_Use_decl_annotations_
STDAPI DllCanUnloadNow(void) {
  return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type.
_Use_decl_annotations_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid,
                         _In_ REFIID riid,
                         _Outptr_ LPVOID* ppv) {
  return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer - Adds entries to the system registry.
_Use_decl_annotations_
STDAPI DllRegisterServer(void) {
  // registers object, typelib and all interfaces in typelib
  HRESULT hr = _AtlModule.DllRegisterServer();
  return hr;
}

// DllUnregisterServer - Removes entries from the system registry.
_Use_decl_annotations_
STDAPI DllUnregisterServer(void) {
  HRESULT hr = _AtlModule.DllUnregisterServer();
  return hr;
}

// DllInstall - Adds/Removes entries to the system registry per user per machine.
STDAPI DllInstall(BOOL bInstall, _In_opt_  LPCWSTR pszCmdLine) {
  HRESULT hr = E_FAIL;
  static const wchar_t szUserSwitch[] = L"user";

  if (pszCmdLine != nullptr) {
    if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0) {
      ATL::AtlSetPerUserRegistration(true);
    }
  }

  if (bInstall) {
    hr = DllRegisterServer();
    if (FAILED(hr)) {
      DllUnregisterServer();
    }
  } else {
    hr = DllUnregisterServer();
  }

  return hr;
}


