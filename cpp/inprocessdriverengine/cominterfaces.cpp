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

#include "cominterfaces.h"

EXTERN_C const IID IID_IInProcessDriver = { 0xc4edf20a, 0xfc4d, 0x4f04, { 0xb0, 0x29, 0xc8, 0xd3, 0x38, 0x21, 0x0d, 0x00 } };

EXTERN_C const IID LIBID_InProcessDriverEngineLib = { 0xd2dd85a7, 0x1c28, 0x4312, { 0x87, 0xbf, 0x17, 0x35, 0x8a, 0xa4, 0xd5, 0x23 } };

EXTERN_C const CLSID CLSID_InProcessDriver = { 0xff5186d0, 0x7673, 0x4ea6, { 0x81, 0x13, 0xbc, 0x96, 0x0c, 0x03, 0x08, 0x91 } };

EXTERN_C const IID IID_IScriptException = { 0xcbdf5555, 0x73f8, 0x472d, { 0xae, 0x74, 0x89, 0x32, 0xd2, 0xf5, 0x37, 0x48 } };

EXTERN_C const CLSID CLSID_ScriptException = { 0x7e2b9663, 0xb2f0, 0x44f0, { 0x89, 0x0d, 0x64, 0xfb, 0x08, 0xef, 0xc7, 0x79 } };
