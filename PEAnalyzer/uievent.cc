///////////
#include "ui.hpp"
#include "peazres.h"
#include "charconv.hpp"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

namespace ui {

static inline int Year() {
  SYSTEMTIME stime;
  GetSystemTime(&stime);
  return stime.wYear;
}
LRESULT Window::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam,
                         BOOL &bHandle) {
  HICON hIcon =
      LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_PEANALYZER));
  SetIcon(hIcon, TRUE);
  // Enable DragFile
  ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
  ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
  ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
  ::DragAcceptFiles(m_hWnd, TRUE);

  // Initialize Labels
  labels.emplace_back(L"PE:", 20, 40, 120, 65); /// Copyrigth
  std::wstring s(L"\xD83D\xDE0B \x2764 Copyright \x0A9 ");
  base::Integer_append_chars(Year(), 10, s);
  s.append(L". Force Charlie. All Rights Reserved.");
  labels.emplace_back(s, 80, 345, 540, 370);

  // Create Controls
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  LOGFONT logFont = {0};
  GetObjectW(hFont, sizeof(logFont), &logFont);
  DeleteObject(hFont);
  hFont = nullptr;
  logFont.lfHeight = hdpi.Scale(19);
  logFont.lfWeight = FW_NORMAL;
  wcscpy_s(logFont.lfFaceName, L"Segoe UI");
  hFont = CreateFontIndirectW(&logFont);
  HMENU hSystemMenu = ::GetSystemMenu(m_hWnd, FALSE);
  InsertMenuW(hSystemMenu, SC_CLOSE, MF_ENABLED, IDM_COMMAND_ABOUT,
              L"About PE Analyzer\tAlt+F1");
  constexpr const auto eex = WS_EX_LEFT | WS_EX_LTRREADING |
                             WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY |
                             WS_EX_CLIENTEDGE;
  constexpr const auto es = WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE |
                            WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
  constexpr const auto bex = WS_EX_LEFT | WS_EX_LTRREADING |
                             WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY;
  constexpr const auto bs =
      BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE;
  hUri = ::CreateWindowExW(eex, WC_EDITW, L"", es, hdpi.Scale(80),
                           hdpi.Scale(40), hdpi.Scale(360), hdpi.Scale(27),
                           m_hWnd, HMENU(IDC_IMAGE_URI_EDIT),
                           HINST_THISCOMPONENT, nullptr);
  hClick = ::CreateWindowExW(bex, WC_BUTTONW, L"...", bs, hdpi.Scale(200),
                             hdpi.Scale(40), hdpi.Scale(40), hdpi.Scale(27),
                             m_hWnd, HMENU(IDB_IMAGE_FIND_BUTTON),
                             HINST_THISCOMPONENT, nullptr);
  return S_OK;
}
LRESULT Window::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam,
                          BOOL &bHandle) {
  PostQuitMessage(0);
  return S_OK;
}
LRESULT Window::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam,
                        BOOL &bHandle) {
  ::DestroyWindow(m_hWnd);
  return S_OK;
}
LRESULT Window::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle) {
  UINT width = LOWORD(lParam);
  UINT height = HIWORD(lParam);
  OnResize(width, height);
  return S_OK;
}
LRESULT Window::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam,
                        BOOL &bHandle) {
  LRESULT hr = S_OK;
  PAINTSTRUCT ps;
  BeginPaint(&ps);
  /// if auto return OnRender(),CPU usage is too high
  hr = OnRender();
  EndPaint(&ps);
  return hr;
}
LRESULT Window::OnDpiChanged(UINT nMsg, WPARAM wParam, LPARAM lParam,
                             BOOL &bHandle) {
  return S_OK;
}
LRESULT Window::OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam,
                            BOOL &bHandled) {
  return S_OK;
}

LRESULT Window::OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL &bHandled) {

  return S_OK;
}

LRESULT Window::OnDiscover(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                           BOOL &bHandled) {
  //
  return S_OK;
}

} // namespace ui