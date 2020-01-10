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

#ifndef WEBDRIVER_FILE_SELECTION_DIALOG_H_
#define WEBDRIVER_FILE_SELECTION_DIALOG_H_

#include <string>
#include <vector>

struct IUIAutomation;
struct IUIAutomationElement;
struct IUIAutomationElementArray;

namespace webdriver {

struct FileNameData {
  HWND main;
  HWND hwnd;
  DWORD ieProcId;
  DWORD dialogTimeout;
  bool useLegacyDialogHandling;
  const wchar_t* text;
};

class Response;

class FileSelectionDialog {
private:
  unsigned int WINAPI SetFileValue(void* file_data);
  bool SendFileNameKeys(FileNameData* file_data);
  bool GetFileSelectionDialogCandidates(
      std::vector<HWND> parent_window_handles,
      IUIAutomation* ui_automation,
      IUIAutomationElementArray** dialog_candidates);
  bool FillFileName(const wchar_t* file_name,
                           IUIAutomation* ui_automation,
                           IUIAutomationElement* file_selection_dialog);
  bool AcceptFileSelection(IUIAutomation* ui_automation,
                                  IUIAutomationElement* file_selection_dialog);
  bool WaitForFileSelectionDialogClose(
      const int timeout,
      IUIAutomationElement* file_selection_dialog);
  bool FindFileSelectionErrorDialog(
      IUIAutomation* ui_automation,
      IUIAutomationElement* file_selection_dialog,
      IUIAutomationElement** error_dialog);
  bool DismissFileSelectionErrorDialog(
      IUIAutomation* ui_automation,
      IUIAutomationElement* error_dialog);
  bool DismissFileSelectionDialog(
      IUIAutomation* ui_automation,
      IUIAutomationElement* file_selection_dialog);

  bool LegacySelectFile(FileNameData* file_data);
  bool LegacySendKeysToFileUploadAlert(HWND dialog_window_handle,
                                              const wchar_t* value);

  static BOOL CALLBACK FindWindowWithClassNameAndProcess(HWND hwnd,
                                                         LPARAM arg);

  static std::vector<HWND> FindWindowCandidates(FileNameData* file_data);

  void UploadFile(const std::wstring& file_name,
                  const bool allows_multiple,
                  Response* response);

  std::wstring error_text_;
};

} // namespace webdriver

#endif // WEBDRIVER_FILE_SELECTION_DIALOG_H_
