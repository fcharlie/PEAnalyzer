///////////
#include "ui.hpp"
#include "peazres.h"

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

namespace ui {
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
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  LOGFONT logFont = {0};
  GetObjectW(hFont, sizeof(logFont), &logFont);
  DeleteObject(hFont);
  hFont = nullptr;
  logFont.lfHeight = hdpi.Scale(19);
  logFont.lfWeight = FW_NORMAL;
  wcscpy_s(logFont.lfFaceName, L"Segoe UI");
  hFont = CreateFontIndirectW(&logFont);

  constexpr const auto editstyleex = WS_EX_LEFT | WS_EX_LTRREADING |
                                     WS_EX_RIGHTSCROLLBAR |
                                     WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE;
  constexpr const auto editstyle = WS_CHILDWINDOW | WS_CLIPSIBLINGS |
                                   WS_VISIBLE | WS_TABSTOP | ES_LEFT |
                                   ES_AUTOHSCROLL;
  hUri = ::CreateWindowExW(editstyleex, WC_EDITW, L"", editstyle,
                           hdpi.Scale(80), hdpi.Scale(40), hdpi.Scale(360),
                           hdpi.Scale(27), m_hWnd, HMENU(IDC_IMAGE_URI_EDIT),
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
  return S_OK;
}
LRESULT Window::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam,
                        BOOL &bHandle) {
  return S_OK;
}
LRESULT Window::OnDpiChanged(UINT nMsg, WPARAM wParam, LPARAM lParam,
                             BOOL &bHandle) {
  return S_OK;
}
LRESULT Window::OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam,
                            BOOL &bHandled) {
  return S_OK;
}
LRESULT Window::OnLButtonClick(UINT nMsg, WPARAM wParam, LPARAM lParam,
                               BOOL &bHandle) {
  return S_OK;
}
LRESULT Window::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam,
                              BOOL &bHandle) {
  return S_OK;
}
} // namespace ui