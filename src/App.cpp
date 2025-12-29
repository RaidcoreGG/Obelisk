///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  App.cpp
/// Description  :  Main App component.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#include "App.h"

#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "ResConst.h"

static bool g_canDragWindow = false;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wc{};
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = ::GetModuleHandle(NULL);
	wc.hIcon         = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_ICON));
	wc.lpszClassName = L"Raidcore_Dx_Window_Class";
	::RegisterClassEx(&wc);

	DWORD dwStyle
		= WS_VISIBLE
		| WS_CLIPSIBLINGS
		| WS_MINIMIZEBOX;
#ifndef _DEBUG
	dwStyle |= WS_POPUP;
#endif

	RECT desktop{};
	GetWindowRect(GetDesktopWindow(), &desktop);

	HWND hwnd = ::CreateWindowEx(
		NULL,
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
	::SetWindowLong(
		hwnd,
		GWL_EXSTYLE,
		::GetWindowLong(hwnd, GWL_EXSTYLE)
			| WS_EX_LEFT
			| WS_EX_LTRREADING
			| WS_EX_RIGHTSCROLLBAR
			| WS_EX_LAYERED
	);
	
	if (!AppContext::GrCreateDevice(hwnd))
	{
		AppContext::GrDestroyDevice();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	::ShowWindow(hwnd, SW_SHOW);
	::UpdateWindow(hwnd);

	ImGui::CreateContext();
	ImGui_ImplWin32_EnableAlphaCompositing(hwnd);
	ImGui_ImplWin32_Init(hwnd);
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
			::PostMessage(hwnd, WM_QUIT, 0, 0);
		}
		ImGui::End();

		ImGui::Render();

		static const float s_ClearColor[4] = { 0.f, 0.f, 0.f, 0.f };
		AppContext::GrDeviceContext->OMSetRenderTargets(1, &AppContext::GrRenderTarget, NULL);
		AppContext::GrDeviceContext->ClearRenderTargetView(AppContext::GrRenderTarget, s_ClearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		AppContext::GrSwapChain->Present(1, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	AppContext::GrDestroyDevice();
	::DestroyWindow(hwnd);
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
				AppContext::GrSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
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
	DXGI_SWAP_CHAIN_DESC desc{};
	desc.BufferCount = 2;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = aWindowHandle;
	desc.SampleDesc.Count = 1;
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel{};
	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };

	if (!SUCCEEDED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION,
		&desc, &AppContext::GrSwapChain, &AppContext::GrDevice, &featureLevel,
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

	if (AppContext::GrSwapChain)
	{
		AppContext::GrSwapChain->Release();
		AppContext::GrSwapChain = nullptr;
	}

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
	ID3D11Texture2D* pBackBuffer{};
	AppContext::GrSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

	if (!pBackBuffer)
	{
		return false;
	}

	AppContext::GrDevice->CreateRenderTargetView(pBackBuffer, NULL, &AppContext::GrRenderTarget);
	pBackBuffer->Release();

	return true;
}

void AppContext::GrDestroyRenderTarget()
{
	if (AppContext::GrRenderTarget)
	{
		AppContext::GrRenderTarget->Release();
		AppContext::GrRenderTarget = nullptr;
	}
}
