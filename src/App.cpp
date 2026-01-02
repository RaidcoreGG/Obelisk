///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  App.cpp
/// Description  :  Main App component.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#include "App.h"

#include <vector>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"

#include "ResConst.h"

static bool g_canDragWindow = false;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	ULONG_PTR gdiToken{};
	Gdiplus::GdiplusStartupInput gdiplusStartupInput{};
	Gdiplus::GdiplusStartup(&gdiToken, &gdiplusStartupInput, nullptr);

	WNDCLASSEX wc{};
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = ::GetModuleHandle(NULL);
	wc.hIcon         = ::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(APP_ICON));
	wc.lpszClassName = L"Raidcore_Gr_Splash_Class";
	::RegisterClassEx(&wc);

	DWORD dwStyle
		= WS_POPUP
		| WS_VISIBLE
		| WS_CLIPSIBLINGS
		| WS_MINIMIZEBOX;

	DWORD dwExStyle
		= WS_EX_LEFT
		| WS_EX_LTRREADING
		| WS_EX_RIGHTSCROLLBAR
		| WS_EX_LAYERED;

	RECT desktop{};
	::GetWindowRect(::GetDesktopWindow(), &desktop);

	AppContext::GrWindow = ::CreateWindowEx(
		dwExStyle,
		wc.lpszClassName,
		L"Obelisk",
		dwStyle,
		(desktop.right - AppContext::GrDimensions.Width) / 2,
		(desktop.bottom - AppContext::GrDimensions.Height) / 2,
		AppContext::GrDimensions.Width,
		AppContext::GrDimensions.Height,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	if (!AppContext::GrCreateDevice(AppContext::GrWindow))
	{
		AppContext::GrDestroyDevice();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	::ShowWindow(AppContext::GrWindow, SW_SHOW);
	::UpdateWindow(AppContext::GrWindow);

	ImGui::CreateContext();
	ImGui_ImplWin32_Init(AppContext::GrWindow);
	ImGui_ImplDX11_Init(AppContext::GrDevice, AppContext::GrDeviceContext);

	// Main loop
	MSG msg{};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowMetricsWindow();
		ImGui::ShowStyleEditor();

		AppContext::CliWindow.Render();

#ifdef _DEBUG
		ImDrawList* dlBg = ImGui::GetBackgroundDrawList();
		dlBg->AddRect(ImVec2(0, 0), ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImColor(1.f, 0.f, 0.f));
#endif

		ImGui::Render();

		static const float s_ClearColor[4] = { 0.f, 0.f, 0.f, 0.f };
		AppContext::GrDeviceContext->OMSetRenderTargets(1, &AppContext::GrOffscreenRenderTarget, NULL);
		AppContext::GrDeviceContext->ClearRenderTargetView(AppContext::GrOffscreenRenderTarget, s_ClearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Copy to HBITMAP
		HBITMAP bmp = AppContext::GrCopyRTVToBitmap();

		// Update layered window
		POINT ptZero = { 0,0 };
		SIZE size = { static_cast<LONG>(AppContext::GrDimensions.Width), static_cast<LONG>(AppContext::GrDimensions.Height) };
		BLENDFUNCTION blend{};
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;
		HDC hdcScreen = ::GetDC(nullptr);
		HDC hdcMem = ::CreateCompatibleDC(hdcScreen);
		HBITMAP oldBmp = static_cast<HBITMAP>(::SelectObject(hdcMem, bmp));
		::UpdateLayeredWindow(AppContext::GrWindow, hdcScreen, nullptr, &size, hdcMem, &ptZero, 0, &blend, ULW_ALPHA);
		::SelectObject(hdcMem, oldBmp);
		::DeleteDC(hdcMem);
		::ReleaseDC(nullptr, hdcScreen);
		::DeleteObject(bmp);

		constexpr float k_MinDeltaTime = 1.0f / 60.0f;
		ImGuiIO& io = ImGui::GetIO();

		if (io.DeltaTime < k_MinDeltaTime)
		{
			::Sleep(static_cast<uint32_t>((k_MinDeltaTime - io.DeltaTime) * 1000.0f));
		}
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	Gdiplus::GdiplusShutdown(gdiToken);

	AppContext::GrDestroyDevice();
	::DestroyWindow(AppContext::GrWindow);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return true;
	}

	switch (uMsg)
	{
		case WM_SIZE:
		{
			if (AppContext::GrDevice != nullptr && wParam != SIZE_MINIMIZED)
			{
				AppContext::GrDestroyRenderTarget();
				AppContext::GrCreateRenderTarget();
			}
			return 0;
		}
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			return 0;
		}
		case WM_NCHITTEST:
		{
			if (AppContext::CliWindow.IsDraggable())
			{
				return HTCAPTION;
			}
			else
			{
				break;
			}
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HBITMAP AppContext::GrCopyRTVToBitmap()
{
	// Create staging texture
	D3D11_TEXTURE2D_DESC texDesc{};
	AppContext::GrOffscreenTexture->GetDesc(&texDesc);

	D3D11_TEXTURE2D_DESC stagingDesc{ texDesc };
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ID3D11Texture2D* stagingTex = nullptr;
	AppContext::GrDevice->CreateTexture2D(&stagingDesc, nullptr, &stagingTex);

	AppContext::GrDeviceContext->CopyResource(stagingTex, AppContext::GrOffscreenTexture);

	// Map CPU
	D3D11_MAPPED_SUBRESOURCE mapped{};
	AppContext::GrDeviceContext->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mapped);

	// Copy row by row into a tightly packed buffer
	std::vector<BYTE> bmpData(texDesc.Width * texDesc.Height * 4);
	BYTE* dst = bmpData.data();
	BYTE* src = reinterpret_cast<BYTE*>(mapped.pData);
	for (UINT y = 0; y < texDesc.Height; y++)
	{
		memcpy(dst + y * texDesc.Width * 4, src + y * mapped.RowPitch, texDesc.Width * 4);
	}

	// Create HBITMAP using tightly packed bmpData
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = static_cast<int32_t>(texDesc.Width);
	bmi.bmiHeader.biHeight = -static_cast<int32_t>(texDesc.Height); // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC hdc = ::GetDC(AppContext::GrWindow);
	HBITMAP hBitmap = ::CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, bmpData.data(), &bmi, DIB_RGB_COLORS);
	::ReleaseDC(AppContext::GrWindow, hdc);

	AppContext::GrDeviceContext->Unmap(stagingTex, 0);
	stagingTex->Release();

	return hBitmap;
}

void AppContext::GrResizeClient(uint32_t aWidth, uint32_t aHeight)
{
	RECT desktop{};
	::GetWindowRect(::GetDesktopWindow(), &desktop);

	::SetWindowPos(
		AppContext::GrWindow,
		nullptr,
		(desktop.right - aWidth) / 2,
		(desktop.bottom - aHeight) / 2,
		aWidth,
		aHeight,
		SWP_NOZORDER | SWP_NOACTIVATE
	);

	AppContext::GrDimensions.Width = aWidth;
	AppContext::GrDimensions.Height = aHeight;
}

bool AppContext::GrCreateDevice(HWND aWindowHandle)
{
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel{};
	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };

	if (!SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION,
		&AppContext::GrDevice, &featureLevel,
		&AppContext::GrDeviceContext)))
	{
		return false;
	}

	AppContext::GrCreateRenderTarget();
	return true;
}

void AppContext::GrDestroyDevice()
{
	AppContext::GrDestroyRenderTarget();

	if (AppContext::GrDeviceContext)
	{
		AppContext::GrDeviceContext->Release();
		AppContext::GrDeviceContext = nullptr;
	}

	if (AppContext::GrDevice)
	{
		AppContext::GrDevice->Release();
		AppContext::GrDevice = nullptr;
	}
}

bool AppContext::GrCreateRenderTarget()
{
	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = AppContext::GrDimensions.Width;
	texDesc.Height = AppContext::GrDimensions.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	AppContext::GrDevice->CreateTexture2D(&texDesc, nullptr, &AppContext::GrOffscreenTexture);

	if (!AppContext::GrOffscreenTexture)
	{
		return false;
	}

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	AppContext::GrDevice->CreateRenderTargetView(AppContext::GrOffscreenTexture, NULL, &AppContext::GrOffscreenRenderTarget);

	return true;
}

void AppContext::GrDestroyRenderTarget()
{
	if (AppContext::GrOffscreenRenderTarget)
	{
		AppContext::GrOffscreenRenderTarget->Release();
		AppContext::GrOffscreenRenderTarget = nullptr;
	}

	if (AppContext::GrOffscreenTexture)
	{
		AppContext::GrOffscreenTexture->Release();
		AppContext::GrOffscreenTexture = nullptr;
	}
}
