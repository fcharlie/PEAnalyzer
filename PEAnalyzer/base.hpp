#ifndef PZE_BASE_HPP
#define PZE_BASE_HPP

#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>

// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Specific header required by the program
#include <shellscalingapi.h>

#include <Shlwapi.h>
#include <string>
#include <optional>

namespace peaz {

template <class T> class comptr {
public:
  comptr() { ptr = NULL; }
  comptr(T *p) {
    ptr = p;
    if (ptr != NULL)
      ptr->AddRef();
  }
  comptr(const comptr<T> &sptr) {
    ptr = sptr.ptr;
    if (ptr != NULL)
      ptr->AddRef();
  }
  T **operator&() { return &ptr; }
  T *operator->() { return ptr; }
  T *operator=(T *p) {
    if (*this != p) {
      ptr = p;
      if (ptr != NULL)
        ptr->AddRef();
    }
    return *this;
  }
  operator T *() const { return ptr; }
  template <class I> HRESULT QueryInterface(REFCLSID rclsid, I **pp) {
    if (pp != NULL) {
      return ptr->QueryInterface(rclsid, (void **)pp);
    } else {
      return E_FAIL;
    }
  }
  HRESULT CoCreateInstance(REFCLSID clsid, IUnknown *pUnknown,
                           REFIID interfaceId,
                           DWORD dwClsContext = CLSCTX_ALL) {
    HRESULT hr = ::CoCreateInstance(clsid, pUnknown, dwClsContext, interfaceId,
                                    (void **)&ptr);
    return hr;
  }
  ~comptr() {
    if (ptr != NULL)
      ptr->Release();
  }

private:
  T *ptr;
};

enum windowtype_t {
  // window type
  kInfoWindow,
  kWarnWindow,
  kFatalWindow,
  kAboutWindow
};

HRESULT PeazMessageBox(HWND hWnd, LPCWSTR pszWindowTitle, LPCWSTR pszContent,
                       LPCWSTR pszExpandedInfo, windowtype_t type);
using filter_t = COMDLG_FILTERSPEC;
std::optional<std::wstring> PeazFilePicker(HWND hWnd, const wchar_t *title,
                                           const filter_t *filter,
                                           uint32_t flen);
std::optional<std::wstring> PeazFolderPicker(HWND hWnd, const wchar_t *title);
} // namespace peaz

#endif