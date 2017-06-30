//
//
//
//

#ifndef PEANALYZER_UI_H
#define PEANALYZER_UI_H

#include <atlbase.h>
#include <atlctl.h>
#include <atlwin.h>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <wincodec.h>
//#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <wincodec.h>

#define IDC_IMAGE_URI_EDIT 1010
#define IDC_IMAGE_FIND_BUTTON 1011

bool OpenFileWindow(HWND hParent, std::wstring &filename,
                          const wchar_t *pszWindowTitle);

#define PEANALYZER_UI_MAINWINDOW _T("PEAnalyzer.Render.UI.Window")
typedef CWinTraits<WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>
    CMetroWindowTraits;

struct MetroTitle {
  RECT layout;
  std::wstring title;
};

struct MetroLabel {
  RECT layout;
  std::wstring text;
};

struct MetroTextItem {
  RECT layout;
  std::wstring name;
  std::wstring value;
};

struct MetroTextArea {
  RECT layout;
  D2D1::ColorF backend;
  std::wstring text;
};

struct MetroButton {
  enum { kKeyLeave = 0, kKeyActive = 1, kKeyDown };
  MetroButton(RECT layout_, const wchar_t *caption,
              std::function<LRESULT(const wchar_t *)> c)
      : layout(layout_), caption(caption), callback(c) {
    status = kKeyLeave;
  }
  std::function<LRESULT(const wchar_t *)> callback;
  int status;
  RECT layout;
  std::wstring caption;
};
class CDPI;
class MetroWindow
    : public CWindowImpl<MetroWindow, CWindow, CMetroWindowTraits> {
private:
  CDPI *g_Dpi;
  ID2D1Factory2 *m_d2dFactory;
  ID2D1HwndRenderTarget *m_pHwndRenderTarget;
  ID2D1Device *m_d2dDevice;
  ID2D1DeviceContext *m_d2dContext;
  ID2D1SolidColorBrush *m_pSolidColorBrush;
  //// Button Solid Color Brush
  ID2D1SolidColorBrush *m_PushButtonBackgoundBrush;
  ID2D1SolidColorBrush *m_PushButtonForegroundBrush;
  ID2D1SolidColorBrush *m_PushButtonClickBrush;

  ID2D1SolidColorBrush *m_pBakcgroundEdgeBrush;
  IDWriteTextFormat *m_pWriteTextFormat;
  IDWriteFactory *m_pWriteFactory;

  HRESULT CreateDeviceIndependentResources();
  HRESULT Initialize();
  HRESULT CreateDeviceResources();
  void DiscardDeviceResources();
  HRESULT OnRender();
  D2D1_SIZE_U CalculateD2DWindowSize();
  void OnResize(UINT width, UINT height);
  HWND hEdit;
  std::vector<MetroLabel> label_;
  std::vector<MetroButton> button_;
  std::vector<MetroTextItem> item_;

public:
  MetroWindow();
  ~MetroWindow();
  LRESULT InitializeWindow();
  DECLARE_WND_CLASS(PEANALYZER_UI_MAINWINDOW)
  BEGIN_MSG_MAP(MetroWindow)
  MESSAGE_HANDLER(WM_CREATE, OnCreate)
  MESSAGE_HANDLER(WM_CLOSE, OnClose)
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_SIZE, OnSize)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)
  MESSAGE_HANDLER(WM_DROPFILES, OnDropfiles)
  MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonClick)
  MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
  END_MSG_MAP()
  LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
  LRESULT OnLButtonClick(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT DiscoverIMAGEButtonActive(const wchar_t *debugMessage);
  ////
  LRESULT PortableExecutableFileRander(const std::wstring &file);
};
#endif
