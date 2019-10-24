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
  renderTarget->SetDpi(static_cast<float>(dpiX), static_cast<float>(dpiX));
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
  Release(&render);
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
  if (!SUCCEEDED(hr)) {
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
  hr = render->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    hr = S_OK;
    DiscardDeviceResources();
  }
  return hr;
}

void Window::AttributesTablesDraw() {
  // Draw tables
  if (tables.Empty()) {
    return; ///
  }
  ////////// -->
  float offset = 80;
  float w1 = 30;
  float w2 = 60;
  float keyoff = 180;
  float xoff = 60;
  for (const auto &e : tables.ats) {
    render->DrawTextW(e.name.c_str(), (UINT32)e.name.size(), wfmt,
                      D2D1::RectF(xoff, offset, keyoff, offset + w1), textbrush,
                      D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                      DWRITE_MEASURING_MODE_NATURAL);
    render->DrawTextW(
        e.value.c_str(), (UINT32)e.value.size(), wfmt,
        D2D1::RectF(keyoff + 10, offset, keyoff + 400, offset + w1), textbrush,
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
        DWRITE_MEASURING_MODE_NATURAL);
    offset += w1;
  }
  if (!tables.HasDepends()) {
    return;
  }
  render->DrawTextW(tables.Characteristics().Name(),
                    (UINT32)tables.Characteristics().NameLength(), wfmt,
                    D2D1::RectF(xoff, offset, keyoff, offset + w2), textbrush,
                    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                    DWRITE_MEASURING_MODE_NATURAL);
  render->DrawTextW(
      tables.Depends().Name(), (UINT32)tables.Depends().NameLength(), wfmt,
      D2D1::RectF(xoff, offset + w2, keyoff, offset + w2 + w2), textbrush,
      D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT, DWRITE_MEASURING_MODE_NATURAL);
}

} // namespace ui
