///////// UI D2D1 Render code
#include "ui.hpp"

namespace ui {
//// -->
HRESULT Window::CreateDeviceIndependentResources() {
  D2D1_FACTORY_OPTIONS options;
  ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
  HRESULT hr = S_OK;
  if ((hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory)) <
      0) {
    return hr;
  }
  if ((hr = DWriteCreateFactory(
           DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5),
           reinterpret_cast<IUnknown **>(&wfactory))) < 0) {
    return hr;
  }
  return wfactory->CreateTextFormat(
      L"Segeo UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, 12.0f * 96.0f / 72.0f, L"zh-CN",
      (IDWriteTextFormat **)&wfmt);
}
HRESULT Window::Initialize() {
  auto hr = CreateDeviceIndependentResources();
  if (!SUCCEEDED(hr)) {
    /// --
  }
  return S_OK;
}
HRESULT Window::CreateDeviceResources() {
  if (render != nullptr) {
    return S_OK;
  }
  RECT rect;
  ::GetClientRect(m_hWnd, &rect);
  auto size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);
  HRESULT hr = S_OK;
  if ((hr = factory->CreateHwndRenderTarget(
           D2D1::RenderTargetProperties(),
           D2D1::HwndRenderTargetProperties(m_hWnd, size), &render)) < 0) {
    return hr;
  }
  if ((hr = render->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                          &textbrush)) < 0) {
    return hr;
  }
  if ((hr = render->CreateSolidColorBrush(D2D1::ColorF(0xFFC300),
                                          &streaksbrush)) < 0) {
    return hr;
  }
  return S_OK;
}
void Window::DiscardDeviceResources() {
  //
  Release(&textbrush);
  Release(&streaksbrush);
}

D2D1_SIZE_U Window::CalculateD2DWindowSize() {
  RECT rc;
  ::GetClientRect(m_hWnd, &rc);

  D2D1_SIZE_U d2dWindowSize = {0};
  d2dWindowSize.width = rc.right;
  d2dWindowSize.height = rc.bottom;

  return d2dWindowSize;
}
void Window::OnResize(UINT width, UINT height) {
  if (render != nullptr) {
    render->Resize(D2D1::SizeU(width, height));
  }
}

HRESULT Window::OnRender() {
  auto hr = CreateDeviceResources();
  if (SUCCEEDED(hr)) {
    return hr;
  }
  auto dsz = render->GetSize();
  render->BeginDraw();
  render->SetTransform(D2D1::Matrix3x2F::Identity());
  render->Clear(D2D1::ColorF(D2D1::ColorF::White, 1.0f));
  AttributesTablesDraw(); ////
  for (const auto &l : labels) {
    render->DrawTextW(l.data(), l.length(), wfmt, l.layout(), textbrush,
                      D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                      DWRITE_MEASURING_MODE_NATURAL);
  }
  render->EndDraw();
  return S_OK;
}

void Window::AttributesTablesDraw() {
  // Draw tables
  if (tables.Empty()) {
    return; ///
  }
}

} // namespace ui