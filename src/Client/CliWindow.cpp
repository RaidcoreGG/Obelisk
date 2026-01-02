///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  CliWindow.cpp
/// Description  :  Client window implementation.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#include "CliWindow.h"

#include <windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "App.h"

CClientWindow::CClientWindow()
{
	this->DraggableAreaHovered = false;
}

CClientWindow::~CClientWindow() {}

void CClientWindow::Render()
{
	/* Reset state. */
	this->DraggableAreaHovered = false;

	ImGui::Begin("Obelisk", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

	ImRect title_bar_rect = ImGui::GetCurrentWindow()->TitleBarRect();
	if (ImGui::IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false))
	{
		this->DraggableAreaHovered = true;
	}

	//ImGui::Text("candrag: %s", g_canDragWindow ? "true" : "false");
	ImGui::Text("Rect: %.0f, %.0f, %.0f, %.0f", title_bar_rect.Min.x, title_bar_rect.Min.y, title_bar_rect.Max.x, title_bar_rect.Max.y);

	ImGui::Text("Loading...");
	ImGui::Text("Semi-transparent splash art here");
	ImGui::Button("button");
	if (ImGui::Button("Quit"))
	{
		::PostMessage(AppContext::GrWindow, WM_QUIT, 0, 0);
	}

	ImGui::End();
}

bool CClientWindow::IsDraggable()
{
	return this->DraggableAreaHovered;
}
