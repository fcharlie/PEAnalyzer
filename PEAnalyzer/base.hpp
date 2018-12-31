#ifndef PZE_BASE_HPP
#define PZE_BASE_HPP

#pragma once

#include <SDKDDKVer.h>

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#endif
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
#include <algorithm>

/*
 * Compute the length of an array with constant length.  (Use of this method
 * with a non-array pointer will not compile.)
 *
 * Beware of the implicit trailing '\0' when using this with string constants.
 */
template <typename T, size_t N> constexpr size_t ArrayLength(T (&aArr)[N]) {
  return N;
}

template <typename T, size_t N> constexpr T *ArrayEnd(T (&aArr)[N]) {
  return aArr + ArrayLength(aArr);
}


/**
 * std::equal has subpar ergonomics.
 */

template <typename T, typename U, size_t N>
bool ArrayEqual(const T (&a)[N], const U (&b)[N]) {
  return std::equal(a, a + N, b);
}

template <typename T, typename U>
bool ArrayEqual(const T *const a, const U *const b, const size_t n) {
  return std::equal(a, a + n, b);
}

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