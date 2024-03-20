#pragma once

#include <d3d11.h>

inline ID3D11Device* g_pd3dDevice = nullptr;

class gui
{
	bool m_show_demo_window = false;

public:
	static constexpr auto window_title      = "Immediate Mod Manager";
	static constexpr auto window_title_wide = L"Immediate Mod Manager";

	static constexpr auto dockspace_title = "DockSpace";

	static constexpr auto installed_mods_title = "Installed Mods";
	static constexpr auto available_mods_title = "Available Mods";

	void render_docking_layout();
	void render_installed_mods_panel();
	void render_available_mods_panel();
	void render();
	void render_main_menu_bar();
};
