///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  App.h
/// Description  :  Main App component.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "imgui/backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

///----------------------------------------------------------------------------------------------------
/// WinMain:
/// 	Application entry point.
///----------------------------------------------------------------------------------------------------
int APIENTRY WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
);

///----------------------------------------------------------------------------------------------------
/// WndProc:
/// 	Application window procedure.
///----------------------------------------------------------------------------------------------------
LRESULT WINAPI WndProc(
	HWND   hWnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
);

///----------------------------------------------------------------------------------------------------
/// AppContext Class
///----------------------------------------------------------------------------------------------------
class AppContext
{
	public:
	inline static HWND                    GrWindow{};
	inline static ID3D11Device*           GrDevice{};
	inline static ID3D11DeviceContext*    GrDeviceContext{};
	inline static ID3D11Texture2D*        GrOffscreenTexture{};
	inline static ID3D11RenderTargetView* GrOffscreenRenderTarget{};

	///----------------------------------------------------------------------------------------------------
	/// GrCreateDevice:
	/// 	Helper to create graphics device for the window.
	///----------------------------------------------------------------------------------------------------
	static bool GrCreateDevice(HWND aWindowHandle);

	///----------------------------------------------------------------------------------------------------
	/// GrDestroyDevice:
	/// 	Helper to destroy graphics device for the window.
	///----------------------------------------------------------------------------------------------------
	static void GrDestroyDevice();

	///----------------------------------------------------------------------------------------------------
	/// GrCreateRenderTarget:
	/// 	Helper to create render target for the window.
	///----------------------------------------------------------------------------------------------------
	static bool GrCreateRenderTarget();

	///----------------------------------------------------------------------------------------------------
	/// GrDestroyRenderTarget:
	/// 	Helper to destroy render target for the window.
	///----------------------------------------------------------------------------------------------------
	static void GrDestroyRenderTarget();
};
