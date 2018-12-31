// PEAnalyzer.cpp : Defines the entry point for the application.
//

#include "base.hpp"
#include <CommCtrl.h>
#include <Objbase.h>
#include <VersionHelpers.h>
#include <commdlg.h>
#include "errorcode.hpp"
///
#include "peazres.h"
#include "ui.hpp"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "ComDlg32.Lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "shcore.lib")

class COMInitializer {
public:
  COMInitializer() { CoInitialize(nullptr); }
  ~COMInitializer() { CoUninitialize(); }
};

int WindowMessageLoop() {
  INITCOMMONCONTROLSEX info = {sizeof(INITCOMMONCONTROLSEX),
                               ICC_TREEVIEW_CLASSES | ICC_COOL_CLASSES |
                                   ICC_LISTVIEW_CLASSES};
  InitCommonControlsEx(&info);
  ui::Window window;
  if (!window.InitializeWindow()) {
    auto ec = base::make_system_error_code();
    MessageBoxW(nullptr, L"InitializeWindow.", ec.message.c_str(),
                MB_OK | MB_ICONERROR);
    return 1;
  }

  window.ShowWindow(SW_SHOW);
  window.UpdateWindow();
  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  if (!IsWindows10OrGreater()) {
    MessageBoxW(nullptr, L"You need at least Windows 10.",
                L"Version Not Supported", MB_OK | MB_ICONERROR);
    return -1;
  }
  COMInitializer comer; /// RAII com init
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
  return WindowMessageLoop();
}