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

Window::Window() {
  // set hdpi
  hdpi.SetAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

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
  HMONITOR hMonitor;
  POINT pt;
  UINT dpix = 0, dpiy = 0;
  HRESULT hr = E_FAIL;

  // Get the DPI for the main monitor, and set the scaling factor
  pt.x = 1;
  pt.y = 1;
  hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  if (GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy) != S_OK) {
    auto ec = base::make_system_error_code();
    peaz::PeazMessageBox(nullptr, L"GetDpiForMonitor error", ec.message.c_str(),
                         nullptr, peaz::kFatalWindow);
    return false;
  }
  hdpi.SetScale(dpix);
  RECT layout = {hdpi.Scale(100), hdpi.Scale(100), hdpi.Scale(800),
                 hdpi.Scale(600)};
  const auto noresizewindow =
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  std::wstring title(L"PE \x2764 Analyzer (");
  title.append(build_arch()).append(L")");
  Create(nullptr, layout, title.c_str(), noresizewindow,
         WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
  if (Initialize() < 0) {
    return false;
  }
  int numArgc = 0;
  auto Argv = ::CommandLineToArgvW(GetCommandLineW(), &numArgc);
  if (Argv) {
    if (numArgc >= 2 && PathFileExistsW(Argv[1])) {
      /// ---> todo set value
    }
    LocalFree(Argv);
  }
  return true;
}

bool Window::ResolveLink(std::wstring file) {
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
  return Inquisitive();
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

bool Window::Inquisitive() {
  //
  tables.Clear();
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
  peaz::PeazMessageBox(m_hWnd, L"Inquisitive PE", em->dump().c_str(), nullptr,
                       peaz::kInfoWindow);
  tables.Append(L"Machine:", em->machine);
  tables.Append(L"Subsystem:", em->subsystem);
  tables.Append(L"OS Version:", em->osver.strversion());
  tables.Append(L"Link Version:", em->linkver.strversion());
  if (!em->clrmsg.empty()) {
    tables.Append(L"CLR Version:", em->clrmsg);
  }
  tables.Append(L"Characteristics:", em->characteristics);
  if (!em->depends.empty()) {
    tables.Append(L"Depends:", em->depends);
  }
  return true;
}

} // namespace ui