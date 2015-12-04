//
//
//
#include "stdafx.h"
#include "PEAnalyzerUI.h"
#include <d2d1helper.h>
#include <Prsht.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Shellapi.h>
#include "Resource.h"
#include "PortableExecutableFile.h"

//#include <exception>
#pragma comment(lib,"Comctl32.lib")
#pragma comment(lib,"ComDlg32.Lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"shcore.lib")

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define WS_NORESIZEWINDOW (WS_OVERLAPPED     | \
                             WS_CAPTION        | \
                             WS_SYSMENU        | \
                             WS_MINIMIZEBOX )

template<class Interface>
inline void
SafeRelease(
Interface **ppInterfaceToRelease
)
{
	if (*ppInterfaceToRelease != NULL) {
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

int RectHeight(RECT Rect)
{
	return Rect.bottom - Rect.top;
}

int RectWidth(RECT Rect)
{
	return Rect.right - Rect.left;
}

class CDPI {
public:
	CDPI()
	{
		m_nScaleFactor = 0;
		m_nScaleFactorSDA = 0;
		m_Awareness = PROCESS_DPI_UNAWARE;
	}

	int  Scale(int x)
	{
		// DPI Unaware:  Return the input value with no scaling.
		// These apps are always virtualized to 96 DPI and scaled by the system for the DPI of the monitor where shown.
		if (m_Awareness == PROCESS_DPI_UNAWARE) {
			return x;
		}

		// System DPI Aware:  Return the input value scaled by the factor determined by the system DPI when the app was launched.
		// These apps render themselves according to the DPI of the display where they are launched, and they expect that scaling
		// to remain constant for all displays on the system.
		// These apps are scaled up or down when moved to a display with a different DPI from the system DPI.
		if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE) {
			return MulDiv(x, m_nScaleFactorSDA, 100);
		}

		// Per-Monitor DPI Aware:  Return the input value scaled by the factor for the display which contains most of the window.
		// These apps render themselves for any DPI, and re-render when the DPI changes (as indicated by the WM_DPICHANGED window message).
		return MulDiv(x, m_nScaleFactor, 100);
	}

	UINT GetScale()
	{
		if (m_Awareness == PROCESS_DPI_UNAWARE) {
			return 100;
		}

		if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE) {
			return m_nScaleFactorSDA;
		}

		return m_nScaleFactor;
	}

	void SetScale(__in UINT iDPI)
	{
		m_nScaleFactor = MulDiv(iDPI, 100, 96);
		if (m_nScaleFactorSDA == 0) {
			m_nScaleFactorSDA = m_nScaleFactor;  // Save the first scale factor, which is all that SDA apps know about
		}
		return;
	}

	PROCESS_DPI_AWARENESS GetAwareness()
	{
		HANDLE hProcess;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
		GetProcessDpiAwareness(hProcess, &m_Awareness);
		return m_Awareness;
	}

	void SetAwareness(PROCESS_DPI_AWARENESS awareness)
	{
		HRESULT hr = E_FAIL;
		hr = SetProcessDpiAwareness(awareness);
		auto l = E_INVALIDARG;
		if (hr == S_OK) {
			m_Awareness = awareness;
		} else {
			MessageBox(NULL, (LPCWSTR)L"SetProcessDpiAwareness Error", (LPCWSTR)L"Error", MB_OK);
		}
		return;
	}

	// Scale rectangle from raw pixels to relative pixels.
	void ScaleRect(__inout RECT *pRect)
	{
		pRect->left = Scale(pRect->left);
		pRect->right = Scale(pRect->right);
		pRect->top = Scale(pRect->top);
		pRect->bottom = Scale(pRect->bottom);
	}

	// Scale Point from raw pixels to relative pixels.
	void ScalePoint(__inout POINT *pPoint)
	{
		pPoint->x = Scale(pPoint->x);
		pPoint->y = Scale(pPoint->y);
	}

private:
	UINT m_nScaleFactor;
	UINT m_nScaleFactorSDA;
	PROCESS_DPI_AWARENESS m_Awareness;
};

/*
* Resources Initialize and Release
*/

MetroWindow::MetroWindow()
	:m_pFactory(nullptr),
	m_pHwndRenderTarget(nullptr),
	m_pSolidColorBrush(nullptr),
	m_PushButtonNActiveBrush(nullptr),
	m_PushButtonActiveBrush(nullptr),
	m_PushButtonClickBrush(nullptr),
	m_pBakcgroundEdgeBrush(nullptr),
	m_pWriteFactory(nullptr),
	m_pWriteTextFormat(nullptr)
{
	g_Dpi = new CDPI();
	g_Dpi->SetAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	MetroLabel ml = { { 20, 40, 120, 65 }, L"IMAGE:" };
	MetroLabel info = { { 80, 345, 480, 370 }, L"\x263B \x2665 Copyright \x0A9 2015.Force Charlie.All Rights Reserved." };
	MetroButton find({ 450, 40, 550, 65 }, L"Find",
					 std::bind(&MetroWindow::DiscoverIMAGEButtonActive, this, std::placeholders::_1));
	label_.push_back(std::move(ml));
	label_.push_back(std::move(info));
	button_.push_back(std::move(find));
}
MetroWindow::~MetroWindow()
{
	if (g_Dpi) {
		delete g_Dpi;
	}
	SafeRelease(&m_pWriteTextFormat);
	SafeRelease(&m_pWriteFactory);
	SafeRelease(&m_pSolidColorBrush);

	SafeRelease(&m_PushButtonNActiveBrush);
	SafeRelease(&m_PushButtonActiveBrush);
	SafeRelease(&m_PushButtonClickBrush);

	SafeRelease(&m_pBakcgroundEdgeBrush);

	SafeRelease(&m_pHwndRenderTarget);
	SafeRelease(&m_pFactory);
}

LRESULT MetroWindow::InitializeWindow()
{
	HMONITOR hMonitor;
	POINT    pt;
	UINT     dpix = 0, dpiy = 0;
	HRESULT  hr = E_FAIL;

	// Get the DPI for the main monitor, and set the scaling factor
	pt.x = 1;
	pt.y = 1;
	hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy);

	if (hr != S_OK) {
		::MessageBox(NULL, (LPCWSTR)L"GetDpiForMonitor failed", (LPCWSTR)L"Notification", MB_OK);
		return FALSE;
	}
	g_Dpi->SetScale(dpix);
	RECT layout = { g_Dpi->Scale(100), g_Dpi->Scale(100), g_Dpi->Scale(720), g_Dpi->Scale(540) };
	Create(nullptr, layout, L"PE File Analyzer",
		   WS_NORESIZEWINDOW,
		   WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
	return S_OK;
}


///
HRESULT MetroWindow::CreateDeviceIndependentResources()
{
	HRESULT hr = S_OK;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);
	return hr;
}
HRESULT MetroWindow::Initialize()
{
	auto hr = CreateDeviceIndependentResources();
	FLOAT dpiX, dpiY;
	m_pFactory->GetDesktopDpi(&dpiX, &dpiY);
	return hr;
}
HRESULT MetroWindow::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!m_pHwndRenderTarget) {
		RECT rc;
		::GetClientRect(m_hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
			);
		hr = m_pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hWnd, size),
			&m_pHwndRenderTarget
			);
		if (SUCCEEDED(hr)) {
			hr = m_pHwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Blue),
				&m_pBakcgroundEdgeBrush
				);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pHwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&m_pSolidColorBrush
				);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pHwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Orange),
				&m_PushButtonNActiveBrush
				);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pHwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(0xFFC300),
				&m_PushButtonActiveBrush
				);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pHwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::DarkOrange, 2.0f),
				&m_PushButtonClickBrush
				);
		}
	}
	return hr;

}
void MetroWindow::DiscardDeviceResources()
{
	SafeRelease(&m_pSolidColorBrush);
}
HRESULT MetroWindow::OnRender()
{
	auto hr = CreateDeviceResources();
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
							 __uuidof(IDWriteFactory),
							 reinterpret_cast<IUnknown**>(&m_pWriteFactory));
	if (hr != S_OK) return hr;
	hr = m_pWriteFactory->CreateTextFormat(
		L"Segoe UI",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		12.0f * 96.0f / 72.0f,
		L"zh-CN",
		&m_pWriteTextFormat);
#pragma warning(disable:4244)
#pragma warning(disable:4267)
	if (SUCCEEDED(hr)) {
		m_pHwndRenderTarget->BeginDraw();
		m_pHwndRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pHwndRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White, 1.0f));
		//m_pWriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		for (auto &label : label_) {
			if (label.text.empty())
				continue;
			m_pHwndRenderTarget->DrawTextW(label.text.c_str(),
										   label.text.size(),
										   m_pWriteTextFormat,
										   D2D1::RectF(label.layout.left, label.layout.top, label.layout.right, label.layout.bottom),
										   m_pSolidColorBrush,
										   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
		}
		for (auto &it : item_) {
			if (it.name.empty() || it.value.empty())
				continue;
			m_pHwndRenderTarget->DrawTextW(it.name.c_str(),
										   it.name.size(),
										   m_pWriteTextFormat,
										   D2D1::RectF(it.layout.left, it.layout.top, it.layout.left + 120.0f, it.layout.bottom),
										   m_pSolidColorBrush,
										   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
			m_pHwndRenderTarget->DrawTextW(it.value.c_str(),
										   it.value.size(),
										   m_pWriteTextFormat,
										   D2D1::RectF(it.layout.left + 125.0f, it.layout.top, it.layout.right, it.layout.bottom),
										   m_pSolidColorBrush,
										   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
		}
		m_pWriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		for (auto &b : button_) {
			m_pHwndRenderTarget->DrawRectangle(
				D2D1::RectF(b.layout.left, b.layout.top, b.layout.right, b.layout.bottom),
				m_pBakcgroundEdgeBrush,
				0.3f, 0);
			switch (b.status) {
				case MetroButton::kKeyDown:
				m_pHwndRenderTarget->FillRectangle(
					D2D1::RectF(b.layout.left, b.layout.top, b.layout.right, b.layout.bottom),
					m_PushButtonClickBrush);
				break;
				case MetroButton::kKeyActive:
				m_pHwndRenderTarget->FillRectangle(
					D2D1::RectF(b.layout.left, b.layout.top, b.layout.right, b.layout.bottom),
					m_PushButtonActiveBrush);
				break;
				case MetroButton::kKeyLeave:
				default:
				m_pHwndRenderTarget->FillRectangle(
					D2D1::RectF(b.layout.left, b.layout.top, b.layout.right, b.layout.bottom),
					m_PushButtonNActiveBrush);
				break;
			}
			if (!b.caption.empty()) {
				m_pHwndRenderTarget->DrawTextW(b.caption.c_str(),
											   b.caption.size(),
											   m_pWriteTextFormat,
											   D2D1::RectF(b.layout.left, b.layout.top, b.layout.right, b.layout.bottom),
											   m_pSolidColorBrush,
											   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
			}
		}
		hr = m_pHwndRenderTarget->EndDraw();
	}
#pragma warning(default:4244)
#pragma warning(default:4267)	
	if (hr == D2DERR_RECREATE_TARGET) {
		hr = S_OK;
		DiscardDeviceResources();
		::InvalidateRect(m_hWnd, nullptr, FALSE);
	}
	return hr;
}
D2D1_SIZE_U MetroWindow::CalculateD2DWindowSize()
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);

	D2D1_SIZE_U d2dWindowSize = { 0 };
	d2dWindowSize.width = rc.right;
	d2dWindowSize.height = rc.bottom;

	return d2dWindowSize;
}

void MetroWindow::OnResize(
	UINT width,
	UINT height
	)
{
	if (m_pHwndRenderTarget) {
		m_pHwndRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}

/*
*  Message Action Function
*/
LRESULT MetroWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	auto hr = Initialize();
	if (hr != S_OK) {
		::MessageBoxW(nullptr, L"Initialize() failed", L"Fatal error", MB_OK | MB_ICONSTOP);
		std::terminate();
		return S_FALSE;
	}
	HICON hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_PEANALYZER));
	SetIcon(hIcon, TRUE);
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	::DragAcceptFiles(m_hWnd, TRUE);
	DWORD dwEditEx = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | \
		WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE;
	DWORD dwEdit = WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE | \
		WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT logFont = { 0 };
	GetObject(hFont, sizeof(logFont), &logFont);
	DeleteObject(hFont);
	hFont = NULL;
	logFont.lfHeight = 19;
	logFont.lfWeight = FW_NORMAL;
	wcscpy_s(logFont.lfFaceName, L"Segoe UI");
	hFont = CreateFontIndirect(&logFont);

	HWND hEdit = CreateWindowExW(dwEditEx, WC_EDITW,
								 L"",
								 dwEdit, 80, 40, 360, 27,
								 m_hWnd,
								 HMENU(IDC_IMAGE_URI_EDIT),
								 HINST_THISCOMPONENT,
								 NULL);
	//::SetWindowFont(hEdit, hFont, TRUE);
	::SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	return S_OK;
}
LRESULT MetroWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	PostQuitMessage(0);
	return S_OK;
}
LRESULT MetroWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	::DestroyWindow(m_hWnd);
	return S_OK;
}
LRESULT MetroWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	UINT width = LOWORD(lParam);
	UINT height = HIWORD(lParam);
	OnResize(width, height);
	return S_OK;
}
LRESULT MetroWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	LRESULT hr = S_OK;
	PAINTSTRUCT ps;
	BeginPaint(&ps);
	/// if auto return OnRender(),CPU usage is too high
	hr = OnRender();
	EndPaint(&ps);
	return hr;
}

LRESULT MetroWindow::OnDropfiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	const LPCWSTR PackageSubffix[] = { L".exe", L".dll", L".com", L".sys",L"scr",L"fon",L"drv" };
	HDROP hDrop = (HDROP)wParam;
	UINT nfilecounts = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	WCHAR dropfile_name[MAX_PATH];
	std::vector<std::wstring> filelist;
	for (UINT i = 0; i < nfilecounts; i++) {
		DragQueryFileW(hDrop, i, dropfile_name, MAX_PATH);
		if (PathFindSuffixArrayW(dropfile_name, PackageSubffix, ARRAYSIZE(PackageSubffix))) {
			filelist.push_back(dropfile_name);
		}
		if (!filelist.empty()) {
			::SetWindowTextW(::GetDlgItem(m_hWnd, IDC_IMAGE_URI_EDIT), filelist[0].c_str());
			if (PortableExecutableFileRander(filelist.at(0)) != S_OK) {
				::MessageBoxW(m_hWnd, filelist.at(0).c_str(), L"Cannot analyzer this file", MB_OK | MB_ICONSTOP);
				return S_FALSE;
			}
		}
	}
	DragFinish(hDrop);
	::InvalidateRect(m_hWnd, NULL, TRUE);
	return S_OK;
}

LRESULT MetroWindow::OnLButtonUP(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	for (auto &b : button_) {
		if (pt.x >= b.layout.left
			&& pt.x <= b.layout.right
			&&pt.y >= b.layout.top
			&&pt.y <= b.layout.bottom
			) {
			b.callback(L"this is debug message");
			b.status = MetroButton::kKeyActive;
			break;
		}
		b.status = MetroButton::kKeyLeave;
	}
	::InvalidateRect(m_hWnd, nullptr, TRUE);
	return 0;
}
LRESULT MetroWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandle)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	for (auto &b : button_) {
		if (pt.x >= b.layout.left
			&& pt.x <= b.layout.right
			&&pt.y >= b.layout.top
			&&pt.y <= b.layout.bottom
			) {
			b.status = MetroButton::kKeyDown;
			break;
		}
		b.status = MetroButton::kKeyLeave;
	}
	::InvalidateRect(m_hWnd, nullptr, FALSE);
	return 0;
}

LRESULT MetroWindow::DiscoverIMAGEButtonActive(const wchar_t *debugMessage)
{
	std::wstring pefile;
	if (PEFileDiscoverWindow(m_hWnd, pefile, L"Found PE File")) {
		::SetWindowTextW(GetDlgItem(IDC_IMAGE_URI_EDIT), pefile.c_str());
		if (PortableExecutableFileRander(pefile) != S_OK) {
			::MessageBoxW(m_hWnd, pefile.c_str(), L"Cannot analyzer this file", MB_OK | MB_ICONSTOP);
			return S_FALSE;
		}
	}
	return S_OK;
}

LRESULT MetroWindow::PortableExecutableFileRander(const std::wstring &file)
{
	item_.clear();
	PortableExecutableFile portableExecuteFile(file);
	if (!portableExecuteFile.Analyzer())
		return S_FALSE;
	MetroTextItem signature = { { 80, 100, 480, 125 }, L"Signature: ", portableExecuteFile.GetSignature() };
	MetroTextItem machine = { { 80, 130, 480, 155 }, L"Machine: ", portableExecuteFile.GetMachine() };
	MetroTextItem subsystem = { { 80, 160, 480, 185 }, L"Subsystem: ", portableExecuteFile.GetSubSystem() };
	MetroTextItem characteristics = { { 80, 190, 480, 215 }, L"Characteristics: ", portableExecuteFile.GetCharacteristics() };
	MetroTextItem linkVersion = { { 80, 220, 480, 245 }, L"Linker Version: ", portableExecuteFile.GetLinkerVersion() };
	MetroTextItem osVersion = { { 80, 250, 480, 275 }, L"OS Version: ", portableExecuteFile.GetOSVersion() };
	MetroTextItem clrMessage = { { 80, 280, 480, 305 }, L"CLR Metadata: ", portableExecuteFile.GetCLRMessage() };
	item_.push_back(std::move(signature));
	item_.push_back(std::move(machine));
	item_.push_back(std::move(subsystem));
	item_.push_back(std::move(characteristics));
	item_.push_back(std::move(linkVersion));
	item_.push_back(std::move(osVersion));
	item_.push_back(std::move(clrMessage));
	::InvalidateRect(m_hWnd, nullptr, FALSE);
	//MessageBox(portableExecuteFile.GetCharacteristics().c_str(), L"This Characteristics", MB_OK);
	return S_OK;
}