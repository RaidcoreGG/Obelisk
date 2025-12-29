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
using namespace Gdiplus;

#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "ResConst.h"

static bool g_canDragWindow = false;

HBITMAP CopyOffscreenToBitmap(int width, int height)
{
	// Create staging texture
	D3D11_TEXTURE2D_DESC texDesc;
	AppContext::GrOffscreenTexture->GetDesc(&texDesc);

	D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ID3D11Texture2D* stagingTex = nullptr;
	AppContext::GrDevice->CreateTexture2D(&stagingDesc, nullptr, &stagingTex);

	AppContext::GrDeviceContext->CopyResource(stagingTex, AppContext::GrOffscreenTexture);

	// Map CPU
	D3D11_MAPPED_SUBRESOURCE mapped;
	AppContext::GrDeviceContext->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mapped);

	// Copy row by row into a tightly packed buffer
	std::vector<BYTE> bmpData(width * height * 4);
	BYTE* dst = bmpData.data();
	BYTE* src = reinterpret_cast<BYTE*>(mapped.pData);
	for (UINT y = 0; y < height; y++)
	{
		memcpy(dst + y * width * 4, src + y * mapped.RowPitch, width * 4);
	}

	// Create HBITMAP using tightly packed bmpData
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC hdc = GetDC(AppContext::GrWindow);
	HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, bmpData.data(), &bmi, DIB_RGB_COLORS);
	ReleaseDC(AppContext::GrWindow, hdc);

	AppContext::GrDeviceContext->Unmap(stagingTex, 0);
	stagingTex->Release();

	return hBitmap;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	ULONG_PTR gdiToken{};
	GdiplusStartupInput gdiplusStartupInput{};
	GdiplusStartup(&gdiToken, &gdiplusStartupInput, nullptr);

	WNDCLASSEX wc{};
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = ::GetModuleHandle(NULL);
	wc.hIcon         = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_ICON));
	wc.lpszClassName = L"Raidcore_Dx_Window_Class";
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
		(desktop.right - 520) / 2,
		(desktop.bottom - 640) / 2,
		520,
		640,
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

		// Example splash content
		ImGui::Begin("Obelisk", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

		ImRect title_bar_rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false))
		{
			g_canDragWindow = true;
		}
		else
		{
			g_canDragWindow = false;
		}

		ImGui::Text("candrag: %s", g_canDragWindow ? "true" : "false");
		ImGui::Text("Rect: %.0f, %.0f, %.0f, %.0f", title_bar_rect.Min.x, title_bar_rect.Min.y, title_bar_rect.Max.x, title_bar_rect.Max.y);

		ImGui::Text("Loading...");
		ImGui::Text("Semi‑-transparent splash art here");
		ImGui::Button("button");
		if (ImGui::Button("Quit"))
		{
			::PostMessage(AppContext::GrWindow, WM_QUIT, 0, 0);
		}
		ImGui::End();

		ImGui::Render();

		static const float s_ClearColor[4] = { 0.f, 0.f, 0.f, 0.f };
		AppContext::GrDeviceContext->OMSetRenderTargets(1, &AppContext::GrOffscreenRenderTarget, NULL);
		AppContext::GrDeviceContext->ClearRenderTargetView(AppContext::GrOffscreenRenderTarget, s_ClearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Copy to HBITMAP
		HBITMAP bmp = CopyOffscreenToBitmap(520, 640);

		// Update layered window
		POINT ptZero = { 0,0 };
		SIZE size = { 520, 640 };
		BLENDFUNCTION blend{};
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;
		HDC hdcScreen = GetDC(nullptr);
		HDC hdcMem = CreateCompatibleDC(hdcScreen);
		HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, bmp);
		UpdateLayeredWindow(AppContext::GrWindow, hdcScreen, nullptr, &size, hdcMem, &ptZero, 0, &blend, ULW_ALPHA);
		SelectObject(hdcMem, oldBmp);
		DeleteDC(hdcMem);
		ReleaseDC(nullptr, hdcScreen);
		DeleteObject(bmp);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	GdiplusShutdown(gdiToken);

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
			PostQuitMessage(0);
			return 0;
		}
		case WM_NCHITTEST:
		{
			if (g_canDragWindow)
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
	texDesc.Width = 520;
	texDesc.Height = 640;
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
