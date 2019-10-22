#ifndef PEAZ_UI_HPP
#define PEAZ_UI_HPP
#pragma once

#include <atlbase.h>
#include <atlctl.h>
#include <atlwin.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <functional>
#include <string>
#include <vector>
#include <algorithm>
#include <wincodec.h>
#include <mutex>
#include "peazres.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#ifndef SYSCOMMAND_ID_HANDLER
#define SYSCOMMAND_ID_HANDLER(id, func)                                        \
  if (uMsg == WM_SYSCOMMAND && id == LOWORD(wParam)) {                         \
    bHandled = TRUE;                                                           \
    lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled);    \
    if (bHandled)                                                              \
      return TRUE;                                                             \
  }
#endif

namespace ui {
static constexpr const auto wndclassname = L"PEAZ.UI.Render";
using WindowTraits =
    CWinTraits<WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>;

struct AttributesTable {
  std::wstring name;
  std::wstring value;
};

struct AttributesMultiTable {
  std::wstring name;
  std::vector<std::wstring> values;
  const wchar_t *Name() const { return name.c_str(); }
  UINT32 NameLength() const { return static_cast<UINT32>(name.size()); }
};

struct Label {
  Label() = default;
  Label(std::wstring_view sv, LONG left, LONG top, LONG right, LONG bottom)
      : content(sv) {
    mlayout = D2D1::RectF((float)left, (float)top, (float)right, (float)bottom);
  }
  const wchar_t *data() const { return content.data(); }
  uint32_t length() const { return static_cast<uint32_t>(content.size()); }
  D2D_RECT_F layout() const { return mlayout; }
  std::wstring content;
  D2D1_RECT_F mlayout;
};

struct AttributesTables {
  std::vector<AttributesTable> ats;
  std::vector<AttributesMultiTable> amts;
  std::size_t mnlen{0};
  bool Empty() const { return ats.empty() && amts.empty(); }
  AttributesTables &Clear() {
    mnlen = 0;
    ats.clear();
    amts.clear();
    return *this;
  }
  AttributesTables &Append(std::wstring_view name, std::wstring &&value) {
    mnlen = (std::max)(mnlen, name.size());
    ats.emplace_back(AttributesTable{std::wstring(name), std::move(value)});
    return *this;
  }
  AttributesTables &Append(std::wstring_view name, std::wstring_view value) {
    mnlen = (std::max)(mnlen, name.size());
    ats.emplace_back(AttributesTable{std::wstring(name), std::wstring(value)});
    return *this;
  }
  AttributesTables &Append(std::wstring_view name,
                           const std::vector<std::wstring> &value) {
    mnlen = (std::max)(mnlen, name.size());
    AttributesMultiTable amt;
    amt.name = name;
    amts.push_back(amt);
    return *this;
  }
  bool HasDepends() const { return amts.size() == 2; }
  const AttributesMultiTable &Characteristics() { return amts[0]; }
  const AttributesMultiTable &Depends() { return amts[1]; }
};

template <class I> inline void Release(I **p) {
  if (*p != nullptr) {
    (*p)->Release();
    (*p) = nullptr;
  }
}

template <class T> inline void Destroy(T *h) {
  if (h != nullptr) {
    ::DestroyWindow(*h);
    (*h) = nullptr;
  }
}

// use raii
class AutoVisible {
public:
  AutoVisible(HWND hWnd) : hWnd_(hWnd) { EnableWindow(hWnd_, FALSE); }
  ~AutoVisible() { EnableWindow(hWnd_, TRUE); }

private:
  HWND hWnd_;
};

class Window : public CWindowImpl<Window, CWindow, WindowTraits> {
public:
  Window() = default;
  ~Window();
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  bool InitializeWindow();
  ////////////////////////////////////

  DECLARE_WND_CLASS(wndclassname)
  BEGIN_MSG_MAP(Window)
  MESSAGE_HANDLER(WM_CREATE, OnCreate)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_SIZE, OnSize)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)
  MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged);
  MESSAGE_HANDLER(WM_DROPFILES, OnDropfiles)
  MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorEdit)
  COMMAND_ID_HANDLER(IDB_IMAGE_FIND_BUTTON, OnDiscover)
  SYSCOMMAND_ID_HANDLER(IDM_COMMAND_ABOUT, OnAbout)
  END_MSG_MAP()
  LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDpiChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
  // WM_CTLCOLORSTATIC
  LRESULT OnColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
  LRESULT OnDiscover(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
  LRESULT OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);

protected:
  HWND CreateSubWindow(DWORD dwStyleEx, LPCWSTR lpClassName,
                       LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y,
                       int nWidth, int nHeight, HMENU hMenu) {
    auto hw = CreateWindowExW(
        dwStyleEx, lpClassName, lpWindowName, dwStyle, MulDiv(X, dpiX, 96),
        MulDiv(Y, dpiX, 96), MulDiv(nWidth, dpiX, 96),
        MulDiv(nHeight, dpiX, 96), m_hWnd, hMenu, HINST_THISCOMPONENT, nullptr);
    if (hw) {
      ::SendMessageW(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
	if(hw==nullptr){
		::MessageBoxW(nullptr,L"ss", L"ss", MB_OK);
	}
    return hw;
  }
  HRESULT CreateDeviceIndependentResources();
  HRESULT CreateDeviceResources();
  void DiscardDeviceResources();
  HRESULT OnRender();
  void AttributesTablesDraw();
  D2D1_SIZE_U CalculateD2DWindowSize();
  void OnResize(UINT width, UINT height);
  /// Feature
  bool ResolveLink(std::wstring file);
  bool Inquisitive();

private:
  ID2D1Factory2 *factory{nullptr};
  ID2D1HwndRenderTarget *render{nullptr};
  IDWriteFactory5 *wfactory{nullptr}; /// only u
  IDWriteTextFormat3 *wfmt{nullptr};

  /// AttributesTable text brush
  ID2D1SolidColorBrush *textbrush{nullptr};
  //// AttributesTable streak brush
  ID2D1SolidColorBrush *streaksbrush{nullptr};
  HWND hUri;
  HWND hClick;
  HWND hCharacteristics{nullptr};
  HWND hDepends{nullptr};
  HFONT hFont{nullptr};
  std::mutex mtx;
  AttributesTables tables;
  std::vector<Label> labels;
  int dpiX;
};

} // namespace ui

#endif
