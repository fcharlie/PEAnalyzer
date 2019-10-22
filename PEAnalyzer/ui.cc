/////////////
#include "base.hpp"
#include "ui.hpp"
#include <shellapi.h>
#include "pe.hpp"
#include "resolve.hpp"

#ifndef PATHCCH_MAX_CCH
#define PATHCCH_MAX_CCH 0x8000
#endif

namespace ui {

Window::~Window() {
  // Release all resources
  Release(&wfmt);
  Release(&wfactory);
  Release(&render);
  Release(&factory);
  if (hFont != nullptr) {
    DeleteObject(hFont);
  }
}

constexpr const wchar_t *build_arch() {
#if defined(_M_ARM64)
  return L"ARM64";
#elif defined(_M_X64)
  return L"AMD64";
#elif defined(_M_ARM)
  return L"ARM";
#else
  return L"Win32";
#endif
}

//
bool Window::InitializeWindow() {
  if (CreateDeviceIndependentResources() < 0) {
    return false;
  }
  FLOAT dpiX_, dpiY_;
  factory->GetDesktopDpi(&dpiX_, &dpiY_);
  dpiX = static_cast<int>(dpiX_);
  dpiY = static_cast<int>(dpiY_);
  RECT layout = {CW_USEDEFAULT, CW_USEDEFAULT,
                 CW_USEDEFAULT + MulDiv(720, dpiX, 96),
                 CW_USEDEFAULT + MulDiv(500, dpiY, 96)};
  const auto noresizewindow =
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_MINIMIZEBOX;
  std::wstring title(L"PE \x2764 Analyzer (");
  title.append(build_arch()).append(L")");
  Create(nullptr, layout, title.c_str(), noresizewindow,
         WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
  int numArgc = 0;
  auto Argv = ::CommandLineToArgvW(GetCommandLineW(), &numArgc);
  if (Argv) {
    if (numArgc >= 2 && PathFileExistsW(Argv[1])) {
      /// ---> todo set value
      ResolveLink(Argv[1]);
    }
    LocalFree(Argv);
  }
  return true;
}

bool Window::ResolveLink(std::wstring file) {
  std::lock_guard<std::mutex> lock(mtx);
  tables.Clear();
  Destroy(&hCharacteristics);
  Destroy(&hDepends);
  ::InvalidateRect(m_hWnd, NULL, TRUE);
  if (file.empty()) {
    return false;
  }
  base::error_code ec;
  auto link = resolve::ResolveLink(file, ec);
  if (ec) {
    peaz::PeazMessageBox(m_hWnd, L"Resolve Link", ec.message.c_str(), nullptr,
                         peaz::kFatalWindow);
    return false;
  }
  ::SetWindowTextW(hUri, link ? link->c_str() : file.c_str());
  auto ret = Inquisitive();
  ::InvalidateRect(m_hWnd, NULL, TRUE);
  return ret;
}

inline std::wstring Content(HWND hWnd) {
  auto l = GetWindowTextLengthW(hWnd);
  if (l == 0 || l > PATHCCH_MAX_CCH) {
    return L"";
  }
  std::wstring s(l + 1, L'\0');
  GetWindowTextW(hWnd, &s[0], l + 1); //// Null T
  s.resize(l);
  return s;
}

inline std::wstring flatvector(const std::vector<std::wstring> &v,
                               std::wstring_view delimiter = L", ") {
  std::wstring s;
  for (auto &i : v) {
    s.append(i).append(delimiter);
  }
  if (!s.empty()) {
    s.resize(s.size() - delimiter.size());
  }
  return s;
}

bool Window::Inquisitive() {
  //
  auto path = Content(hUri);
  base::error_code ec;
  auto em = pecoff::inquisitive_pecoff(path, ec);
  if (ec) {
    peaz::PeazMessageBox(m_hWnd, L"Inquisitive PE", ec.message.c_str(), nullptr,
                         peaz::kFatalWindow);
    return false;
  }
  if (!em) {
    return false;
  }
  //::MessageBoxW(m_hWnd, em->dump().c_str(), L"Inquisitive PE", MB_OK);
  tables.Append(L"Machine:", em->machine);
  tables.Append(L"Subsystem:", em->subsystem);
  tables.Append(L"OS Version:", em->osver.strversion());
  tables.Append(L"Link Version:", em->linkver.strversion());
  if (!em->clrmsg.empty()) {
    tables.Append(L"CLR Details:", em->clrmsg);
  }
  tables.Append(L"Characteristics:", em->characteristics);
  if (!em->depends.empty()) {
    tables.Append(L"Depends:", em->depends);
  }
  auto y = 80 + 30 * tables.ats.size();
  constexpr auto es = WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                      ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY;
  constexpr auto exs = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR |
                       WS_EX_NOPARENTNOTIFY;
  hCharacteristics =
      CreateSubWindow(exs, WC_EDITW, flatvector(em->characteristics).c_str(),
                      es, 185, (int)y, 460, 55, nullptr);
  hDepends =
      CreateSubWindow(exs, WC_EDITW, flatvector(em->depends, L"\r\n").c_str(), es,
                      185, (int)y + 60, 460, 80, nullptr);
  return true;
}

} // namespace ui