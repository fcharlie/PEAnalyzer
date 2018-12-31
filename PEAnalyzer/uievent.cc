///////////
#include "base.hpp"
#include "ui.hpp"
#include "peazres.h"
#include "charconv.hpp"
#include "version.h"
#include <shellapi.h>

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
  labels.emplace_back(L"PE:", 20, 40, 100, 65); /// Copyrigth
  std::wstring s(L"\xD83D\xDE0B \x2764 Copyright \x0A9 ");
  base::Integer_append_chars(Year(), 10, s);
  s.append(L". Force Charlie. All Rights Reserved.");
  labels.emplace_back(s, 60, 380, 600, 400);

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
  hUri = ::CreateWindowExW(eex, WC_EDITW, L"", es, hdpi.Scale(60),
                           hdpi.Scale(40), hdpi.Scale(510), hdpi.Scale(27),
                           m_hWnd, HMENU(IDC_IMAGE_URI_EDIT),
                           HINST_THISCOMPONENT, nullptr);
  hClick = ::CreateWindowExW(bex, WC_BUTTONW, L"...", bs, hdpi.Scale(575),
                             hdpi.Scale(40), hdpi.Scale(60), hdpi.Scale(27),
                             m_hWnd, HMENU(IDB_IMAGE_FIND_BUTTON),
                             HINST_THISCOMPONENT, nullptr);
  ::SendMessageW(hUri, WM_SETFONT, (WPARAM)hFont, TRUE);
  ::SendMessageW(hClick, WM_SETFONT, (WPARAM)hFont, TRUE);
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
  HMONITOR hMonitor;
  POINT pt;
  UINT dpix = 0, dpiy = 0;
  HRESULT hr = E_FAIL;

  // Get the DPI for the main monitor, and set the scaling factor
  pt.x = 1;
  pt.y = 1;
  hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy);

  if (hr != S_OK) {
    ::MessageBox(NULL, (LPCWSTR)L"GetDpiForMonitor failed",
                 (LPCWSTR)L"Notification", MB_OK);
    return FALSE;
  }
  hdpi.SetScale(dpix);
  RECT *const prcNewWindow = (RECT *)lParam;
  ::SetWindowPos(m_hWnd, NULL, prcNewWindow->left, prcNewWindow->top,
                 hdpi.Scale(prcNewWindow->right - prcNewWindow->left),
                 hdpi.Scale(prcNewWindow->bottom - prcNewWindow->top),
                 SWP_NOZORDER | SWP_NOACTIVATE);
  LOGFONTW logFont = {0};
  GetObjectW(hFont, sizeof(logFont), &logFont);
  DeleteObject(hFont);
  hFont = nullptr;
  logFont.lfHeight = hdpi.Scale(19);
  logFont.lfWeight = FW_NORMAL;
  wcscpy_s(logFont.lfFaceName, L"Segoe UI");
  hFont = CreateFontIndirectW(&logFont);
  auto adjustui = [&](HWND hElement) {
    RECT rect;
    ::GetClientRect(hElement, &rect);
    ::SetWindowPos(hElement, NULL, hdpi.Scale(rect.left), hdpi.Scale(rect.top),
                   hdpi.Scale(rect.right - rect.left),
                   hdpi.Scale(rect.bottom - rect.top),
                   SWP_NOZORDER | SWP_NOACTIVATE);
    ::SendMessageW(hElement, WM_SETFONT, (WPARAM)hFont, lParam);
  };
  adjustui(hUri);
  adjustui(hClick);
  return S_OK;
}

LRESULT Window::OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam,
                            BOOL &bHandled) {

  HDROP hDrop = (HDROP)wParam;
  UINT nfilecounts = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
  wchar_t buffer[8192];
  std::vector<std::wstring> files;
  for (UINT i = 0; i < nfilecounts; i++) {
    DragQueryFileW(hDrop, i, buffer, (UINT)ArrayLength(buffer));
    files.push_back(buffer);
  }
  DragFinish(hDrop);
  if (files.empty()) {
    return S_OK;
  }
  ResolveLink(files[0]);
  ::InvalidateRect(m_hWnd, NULL, TRUE);
  return S_OK;
}

LRESULT Window::OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL &bHandled) {
  constexpr const wchar_t *appurl =
      L"For more information about this tool. \nVisit: <a "
      L"href=\"https://github.com/charlie/PEAnalyzer\">PEAnalyzer</a>\nVisit: "
      L"<a href=\"https://forcemz.net/\">forcemz.net</a>";
  peaz::PeazMessageBox(
      m_hWnd, L"About PE \x2764  Analyzer",
      L"Prerelease"
      L":" PEAZ_STRING_VERSION L"\n\x2764  Copyright \xA9 2019, Force "
      L"Charlie. All Rights Reserved.",
      appurl, peaz::kAboutWindow);
  return S_OK;
}

LRESULT Window::OnDiscover(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                           BOOL &bHandled) {
  //
  return S_OK;
}

} // namespace ui