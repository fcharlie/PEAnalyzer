#include "stdafx.h"
#include <ShlObj.h>
#include <Uxtheme.h>
#include <atlbase.h>
#include <atlcoll.h>
#include <atlsimpstr.h>
#include <atlstr.h>
#include <atlwin.h>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#pragma warning(disable : 4091)
#include <Shlwapi.h>

using namespace ATL;

typedef COMDLG_FILTERSPEC FilterSpec;

const FilterSpec filterSpec[] = {
    {L"Windows  Execute File (*.exe;*.com;*.dll;*.sys)",
     L"*.exe;*.com;*.dll;*.sys"},
    {L"Windows Other File (*.scr;*.fon;*.drv)", L"*.scr;*.fon;*.drv"},
    {L"All Files (*.*)", L"*.*"}};

void ReportErrorMessage(LPCWSTR pszFunction, HRESULT hr) {
  wchar_t szBuffer[65535] = {0};
  if (SUCCEEDED(StringCchPrintf(
          szBuffer, ARRAYSIZE(szBuffer),
          L"Call: %s Failed w/hr 0x%08lx ,Please Checker Error Code!",
          pszFunction, hr))) {
    int nB = 0;
    TaskDialog(nullptr, GetModuleHandle(nullptr), L"Failed information",
               L"Call Function Failed:", szBuffer, TDCBF_OK_BUTTON,
               TD_ERROR_ICON, &nB);
  }
}
bool PEFileDiscoverWindow(HWND hParent, std::wstring &filename,
                          const wchar_t *pszWindowTitle) {
  HRESULT hr;
  bool br = false;
  CComPtr<IFileOpenDialog> pWindow;
  // Create the file-open dialog COM object.
  hr = pWindow.CoCreateInstance(__uuidof(FileOpenDialog));
  if (FAILED(hr)) {
    ReportErrorMessage(L"FileOpenWindowProvider", hr);
    return false;
  }

  // Set the dialog's caption text and the available file types.
  // NOTE: Error handling omitted here for clarity.
  hr = pWindow->SetFileTypes(sizeof(filterSpec) / sizeof(filterSpec[0]),
                             filterSpec);
  hr = pWindow->SetFileTypeIndex(1);
  pWindow->SetTitle(pszWindowTitle ? pszWindowTitle : L"Open File Provider");
  hr = pWindow->Show(hParent);
  if (SUCCEEDED(hr)) {
    CComPtr<IShellItem> pItem;
    hr = pWindow->GetResult(&pItem);
    if (SUCCEEDED(hr)) {
      PWSTR pwsz = NULL;
      hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);
      if (SUCCEEDED(hr)) {
        filename = pwsz;
        br = true;
        CoTaskMemFree(pwsz);
      }
    }
  }
  return br;
}
