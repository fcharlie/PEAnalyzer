///////////
#include "base.hpp"
#include "ui.hpp"
#include "peazres.h"
#include "charconv.hpp"
#include "version.h"
#include <shellapi.h>

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
  labels.emplace_back(s, 60, 400, 600, 420);

  // Create Controls
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  LOGFONT logFont = {0};
  GetObjectW(hFont, sizeof(logFont), &logFont);
  DeleteObject(hFont);
  hFont = nullptr;
  logFont.lfHeight = -MulDiv(14, dpiX, 96);
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
  hUri = CreateSubWindow(eex, WC_EDITW, L"", es, 60, 40, 510, 27,
                         HMENU(IDC_IMAGE_URI_EDIT));
  hClick = CreateSubWindow(bex, WC_BUTTONW, L"...", bs, 575, 40, 65, 27,
                           HMENU(IDB_IMAGE_FIND_BUTTON));

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
  dpiX = LOWORD(wParam);
  RECT *const prcNewWindow = (RECT *)lParam;

  ::SetWindowPos(m_hWnd, NULL, prcNewWindow->left, prcNewWindow->top,
                 MulDiv(prcNewWindow->right - prcNewWindow->left, dpiX, 96),
                 MulDiv(prcNewWindow->bottom - prcNewWindow->top, dpiX, 96),
                 SWP_NOZORDER | SWP_NOACTIVATE);
  LOGFONTW logFont = {0};
  GetObjectW(hFont, sizeof(logFont), &logFont);
  DeleteObject(hFont);
  hFont = nullptr;
  logFont.lfHeight = -MulDiv(14, dpiX, 96);
  logFont.lfWeight = FW_NORMAL;
  wcscpy_s(logFont.lfFaceName, L"Segoe UI");
  hFont = CreateFontIndirectW(&logFont);
  auto UpdateWindowPos = [&](HWND hWnd) {
    RECT rect;
    ::GetClientRect(hWnd, &rect);
    ::SetWindowPos(hWnd, NULL, MulDiv(rect.left, dpiX, 96),
                   MulDiv(rect.top, dpiX, 96),
                   MulDiv(rect.right - rect.left, dpiX, 96),
                   MulDiv(rect.bottom - rect.top, dpiX, 96),
                   SWP_NOZORDER | SWP_NOACTIVATE);
    ::SendMessageW(hWnd, WM_SETFONT, (WPARAM)hFont, lParam);
  };
  UpdateWindowPos(hUri);
  UpdateWindowPos(hClick);
  if (hCharacteristics != nullptr) {
    UpdateWindowPos(hCharacteristics);
  }
  if (hDepends != nullptr) {
    UpdateWindowPos(hDepends);
  }
  return S_OK;
}

LRESULT Window::OnColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam,
                            BOOL &bHandled) {
  return (INT_PTR)CreateSolidBrush(RGB(255, 255, 255));
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
  const peaz::filter_t filters[] = {
      {L"Windows  Execute File (*.exe;*.com;*.dll;*.sys)",
       L"*.exe;*.com;*.dll;*.sys"},
      {L"Windows Other File (*.scr;*.fon;*.drv)", L"*.scr;*.fon;*.drv"},
      {L"All Files (*.*)", L"*.*"}};
  auto file = peaz::PeazFilePicker(m_hWnd, L"Select PE File", filters,
                                   (uint32_t)ArrayLength(filters));
  if (!file) {
    tables.Clear();
    return S_OK;
  }
  ResolveLink(*file);
  return S_OK;
}

} // namespace ui