#ifndef PEAZ_UI_HPP
#define PEAZ_UI_HPP
#pragma once

#include <atlbase.h>
#include <atlctl.h>
#include <atlwin.h>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <functional>
#include <string>
#include <vector>
#include <wincodec.h>
#include "hdpi.hpp"

namespace ui {
static constexpr const auto wndclassname = L"PEAZ.UI.Render";
using WindowTraits =
    CWinTraits<WS_OVERLAPPEDWINDOW, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>;

struct AttributesTable {
  std::wstring name;
  std::wstring value;
};

struct AttributesTableEx {
  std::wstring name;
  std::vector<std::wstring> values;
};

struct D2DButton {
  enum : int {
    MouseLeave, //
    MouseNear,
    KeyDown,
    KeyUp
  };
  using invoke_t = std::function<void(void *)>;
  std::wstring text;
  RECT layout;
  D2D1::ColorF color;
  ID2D1HwndRenderTarget *render{nullptr};
  void Draw() {
    if (render == nullptr) {
      return;
    }
    // render->DrawRectangle(D2D1::RectF);
  }
  int status{0};
};

class Window : public CWindowImpl<Window, CWindow, WindowTraits> {
public:
  Window();
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
  MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonClick)
  MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
  END_MSG_MAP()
  LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDpiChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  LRESULT OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
  LRESULT OnLButtonClick(UINT nMsg, WPARAM wParam, LPARAM lParam,
                         BOOL &bHandle);
  LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle);
  bool ResolveLink();
  bool Inquisitive(std::wstring path);

private:
  ID2D1Factory2 *factory{nullptr};
  ID2D1RenderTarget *render{nullptr};
  ID2D1Device *device{nullptr};
  ID2D1DeviceContext *devctx{nullptr};
  HWND hUri;
  HFONT hFont{nullptr};
  HDPI hdpi;
};

} // namespace ui

#endif
