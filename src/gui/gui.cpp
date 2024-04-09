#include "gui.hpp"

#include "logger.hpp"

#include <codecvt>
#include <cpr/cpr.h>
#include <d3d11.h>
#include <fcntl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_toggle/imgui_toggle.h>
#include <io.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <queue>
#include <semver.hpp>
#include <shellapi.h>
#include <string/string.hpp>
#include <thunderstore/v1/manifest.hpp>
#include <thunderstore/v1/package.hpp>
#include <tlhelp32.h>
#include <vector>
#include <zip/zip.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static std::filesystem::path _root_cache_folder = "";

std::filesystem::path get_root_cache_folder()
{
	if (_root_cache_folder == "")
	{
		const auto appdata_folder = _wgetenv(L"appdata");
		if (appdata_folder)
		{
			_root_cache_folder  = appdata_folder;
			_root_cache_folder /= "ImmediateModManager";
			_root_cache_folder /= "cache";
		}
		else
		{
			SPDLOG_LOGGER_INFO(logger, "No appdata env, making cache folder relative to current_path");

			_root_cache_folder  = "./ImmediateModManager";
			_root_cache_folder /= "cache";
		}

		SPDLOG_LOGGER_INFO(logger, L"root_cache_folder: {}", _root_cache_folder.wstring());
	}

	return _root_cache_folder;
}

static ImU32 DEPRECATED_COLOR    = IM_COL32(235, 125, 52, 255);
static ImU32 DEPRECATED_COLOR_BG = IM_COL32(255, 50, 25, 125);

struct package_enabled_state
{
	bool is_enabled = true;
	std::string full_name;
	std::string version;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(package_enabled_state, is_enabled, full_name, version)
};

struct profile
{
	std::string name = "default";

	std::vector<package_enabled_state> package_enabled_states;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(profile, name, package_enabled_states)
};

static std::filesystem::path app_cache_path;

struct app_cache
{
	std::string game_folder_path_utf8{};
	std::wstring game_folder_path{};

	std::string rom_folder_path_utf8{};

	std::wstring game_exe_path{};

	std::vector<std::shared_ptr<profile>> profiles{};
	std::string active_profile_name = "default";

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(app_cache, game_folder_path, game_exe_path, profiles, active_profile_name)

	profile* active_profile{};

	void save()
	{
		std::ofstream app_cache_file_stream(app_cache_path);
		nlohmann::json j = *this;
		app_cache_file_stream << j << std::endl;
	}
};

static app_cache s_app_cache;

void gui::render_docking_layout()
{
	//ImGuiWindowFlags window_flags = ImGuiWindowFlags_NyoDocking; //ImGuiWindowFlags_MenuBar
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar; //ImGuiWindowFlags_MenuBar
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport;
	viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.65f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

	bool dockingopen = true;
	if (ImGui::Begin(dockspace_title, &dockingopen, window_flags))
	{
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		if (ImGui::DockBuilderGetNode(ImGui::GetID(dockspace_title)) == NULL)
		{
			// Default docking setup.
			ImGuiID dockspace_id  = ImGui::GetID(dockspace_title);
			ImGuiID dock_id_left  = ImGui::GetID(available_mods_title);
			ImGuiID dock_id_right = ImGui::GetID(installed_mods_title);
			ImGui::DockBuilderRemoveNode(dockspace_id);                            // Clear out existing layout
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // ImGuiDockNodeFlags_Dockspace); // Add empty node
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

			// This variable will track the document node, however we are not using it here as we aren't docking anything into it.
			ImGuiID dock_main_id = dockspace_id;
			ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.5f, &dock_id_left, &dock_id_right);

			ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_main_id);
			//node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

			ImGui::DockBuilderDockWindow(available_mods_title, dock_id_left);
			ImGui::DockBuilderDockWindow(installed_mods_title, dock_id_right);

			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGuiID dockspace_id = ImGui::GetID(dockspace_title);

		ImGuiStyle& style        = ImGui::GetStyle();
		ImVec2 vOldWindowMinSize = style.WindowMinSize;
		style.WindowMinSize.x    = 150.0f;

		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

		style.WindowMinSize = vOldWindowMinSize;

		ImGui::End();
	}
}

void gui::render_main_menu_bar()
{
	// ImGui::Begin(dockspace_title);

	// ImGui::End();

	// if (ImGui::BeginMainMenuBar())
	// {
	// if (ImGui::Button("Crash it"))
	// {
	// 	*(int*)0xDE'AD = 0;
	// }

	// 	ImGui::EndMainMenuBar();
	// }
}

// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile(FILE* f, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load from disk into a raw RGBA buffer
	int image_width           = 0;
	int image_height          = 0;
	unsigned char* image_data = stbi_load_from_file(f, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
	{
		return false;
	}

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width            = image_width;
	desc.Height           = image_height;
	desc.MipLevels        = 1;
	desc.ArraySize        = 1;
	desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage            = D3D11_USAGE_DEFAULT;
	desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags   = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem          = image_data;
	subResource.SysMemPitch      = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels       = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width  = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}

static auto available_packages_ready = false;
static std::mutex packages_mutex;
static std::vector<std::unique_ptr<ts::v1::package>> packages;

static auto has_valid_game_folder_path = false;

struct installed_package
{
	ts::v1::package* pkg;
	size_t pkg_version_index;
	bool is_enabled = true;
	bool is_local   = false;
	std::filesystem::path folder;
};

static std::mutex installed_packages_mutex;
static std::vector<installed_package> installed_packages;
static std::vector<ts::v1::package> packages_json;
static std::mutex t_queue_mutex;
static std::queue<std::function<void()>> t_queue;

static void init_available_packages()
{
	packages.clear();
	for (auto& package : packages_json)
	{
		package.full_name_lower = imm::string::to_lower(package.full_name);
		if (package.full_name_lower.contains("immediatemodman"))
		{
			continue;
		}
		for (auto& pkg_version : package.versions)
		{
			pkg_version.full_name_lower = imm::string::to_lower(pkg_version.full_name);
		}
		packages.push_back(std::make_unique<ts::v1::package>(package));
	}
}

static void on_game_folder_found()
{
	if (installed_packages.size())
	{
		std::unique_lock installed_packages_lock(installed_packages_mutex);
		installed_packages.clear();
		std::unordered_map<std::string, ID3D11ShaderResourceView*> full_name_to_tex;
		std::unique_lock packages_lock(packages_mutex);
		for (const auto& pkg : packages)
		{
			for (const auto& pkg_version : pkg->versions)
			{
				full_name_to_tex[pkg_version.full_name] = pkg_version.icon_texture;
			}
		}
		init_available_packages();
		for (const auto& pkg : packages)
		{
			for (auto& pkg_version : pkg->versions)
			{
				pkg_version.icon_texture = full_name_to_tex[pkg_version.full_name];
			}
		}
	}

	if (!s_app_cache.active_profile)
	{
		if (!s_app_cache.profiles.size())
		{
			s_app_cache.profiles.push_back(std::make_shared<profile>());
		}

		for (const auto& prof : s_app_cache.profiles)
		{
			if (prof->name == s_app_cache.active_profile_name)
			{
				s_app_cache.active_profile = prof.get();
				break;
			}
		}
	}

	const auto rom_folder = std::filesystem::path(s_app_cache.game_folder_path) / "ReturnOfModding";

	const auto config_folder = rom_folder / "config";
	if (!std::filesystem::exists(config_folder))
	{
		std::filesystem::create_directories(config_folder);
	}

	const auto plugins_data_folder = rom_folder / "plugins_data";
	if (!std::filesystem::exists(plugins_data_folder))
	{
		std::filesystem::create_directories(plugins_data_folder);
	}

	const auto plugins_folder = rom_folder / "plugins";
	if (!std::filesystem::exists(plugins_folder))
	{
		std::filesystem::create_directories(plugins_folder);
	}
	for (const auto& entry : std::filesystem::recursive_directory_iterator(plugins_folder, std::filesystem::directory_options::skip_permission_denied))
	{
		if (entry.is_directory())
		{
			continue;
		}

		if (entry.path().filename() != "manifest.json" && entry.path().filename() != "manifest_disabled.json")
		{
			continue;
		}

		std::ifstream f(entry.path());
		ts::v1::manifest m = nlohmann::json::parse(f, nullptr, false, true);

		const auto pkg_folder = entry.path().parent_path();

		m.author_name = (char*)pkg_folder.filename().u8string().c_str();
		if (m.author_name.contains('-'))
		{
			m.author_name = imm::string::split(m.author_name, '-')[0];
		}

		const auto full_name_package = m.author_name + '-' + m.name;

		bool is_enabled        = true;
		bool has_enabled_entry = false;
		for (auto& enabled_state : s_app_cache.active_profile->package_enabled_states)
		{
			if (enabled_state.full_name == full_name_package)
			{
				is_enabled = enabled_state.is_enabled;

				if (is_enabled && entry.path().filename() == "manifest_disabled.json")
				{
					// inconsistency between profile state and manifest filename
					// the filename has priority

					is_enabled               = false;
					enabled_state.is_enabled = false;
				}

				has_enabled_entry = true;
				break;
			}
		}

		std::unique_lock t_queue_lock(t_queue_mutex);
		t_queue.push(
		    [full_name_package, m, pkg_folder, is_enabled, has_enabled_entry]
		    {
			    auto find_installed_pkg_from_available_packages = [&](bool is_local)
			    {
				    std::unique_lock packages_lock(packages_mutex);
				    for (auto& package : packages)
				    {
					    if (package->full_name == full_name_package)
					    {
						    size_t i = 0;
						    for (auto& pkg_ver : package->versions)
						    {
							    if (pkg_ver.version_number == m.version_number)
							    {
								    std::unique_lock installed_packages_lock(installed_packages_mutex);
								    installed_packages.push_back({.pkg = package.get(), .pkg_version_index = i, .is_enabled = is_enabled, .is_local = is_local, .folder = pkg_folder});

								    if (!has_enabled_entry)
								    {
									    s_app_cache.active_profile->package_enabled_states.push_back({.is_enabled = is_enabled, .full_name = full_name_package, .version = m.version_number});
								    }

								    package->is_installed             = true;
								    package->installed_version_number = pkg_ver.version_number;

								    return true;
							    }

							    i++;
						    }
					    }
				    }

				    return false;
			    };

			    if (find_installed_pkg_from_available_packages(false))
			    {
				    return;
			    }

			    // Reaching here means it's just a local package

			    auto local_pkg                      = std::make_unique<ts::v1::package>();
			    local_pkg->name                     = m.name;
			    local_pkg->full_name                = full_name_package;
			    local_pkg->full_name_lower          = imm::string::to_lower(full_name_package);
			    local_pkg->owner                    = m.author_name;
			    local_pkg->is_local                 = true;
			    local_pkg->is_installed             = true;
			    local_pkg->installed_version_number = m.version_number;

			    auto full_name_version = full_name_package + '-' + m.version_number;
			    local_pkg->versions.push_back({.name = m.name, .full_name = full_name_version, .description = m.description, .version_number = m.version_number, .dependencies = m.dependencies, .is_installed = true, .full_name_lower = imm::string::to_lower(full_name_version)});
			    packages.push_back(std::move(local_pkg));

			    find_installed_pkg_from_available_packages(true);
		    });
	}

	auto rom_path = std::filesystem::path(s_app_cache.game_folder_path) / "version.dll";
	if (std::filesystem::exists(rom_path))
	{
		DWORD verHandle = 0;
		UINT size       = 0;
		LPBYTE lpBuffer = NULL;
		DWORD verSize   = GetFileVersionInfoSize(rom_path.c_str(), &verHandle);

		if (verSize != NULL)
		{
			std::vector<char> verData;
			verData.reserve(verSize);

			if (GetFileVersionInfo(rom_path.c_str(), verHandle, verSize, verData.data()))
			{
				if (VerQueryValue(verData.data(), L"\\", (VOID FAR * FAR*)&lpBuffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
						if (verInfo->dwSignature == 0xfe'ef'04'bd)
						{
							const int major = (verInfo->dwFileVersionMS >> 16) & 0xff'ff;
							const int minor = (verInfo->dwFileVersionMS >> 0) & 0xff'ff;
							const int patch = (verInfo->dwFileVersionLS >> 16) & 0xff'ff;

							const auto version_number = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
							std::string full_name_package = "ReturnOfModding-ReturnOfModding";

							std::unique_lock t_queue_lock(t_queue_mutex);
							t_queue.push(
							    [full_name_package, version_number]()
							    {
								    std::unique_lock packages_lock(packages_mutex);
								    for (auto& package : packages)
								    {
									    if (package->full_name == full_name_package)
									    {
										    size_t i = 0;
										    for (auto& pkg_ver : package->versions)
										    {
											    if (pkg_ver.version_number == version_number)
											    {
												    std::unique_lock installed_packages_lock(installed_packages_mutex);
												    installed_packages.push_back({.pkg               = package.get(),
												                                  .pkg_version_index = i,
												                                  .is_enabled        = true,
												                                  .is_local          = false,
												                                  .folder = s_app_cache.game_folder_path});

												    bool has_enabled_entry = false;
												    for (auto& enabled_state : s_app_cache.active_profile->package_enabled_states)
												    {
													    if (enabled_state.full_name == full_name_package)
													    {
														    has_enabled_entry = true;
														    break;
													    }
												    }

												    if (!has_enabled_entry)
												    {
													    s_app_cache.active_profile->package_enabled_states.push_back({.is_enabled = true, .full_name = full_name_package, .version = version_number});
												    }

												    package->is_installed             = true;
												    package->installed_version_number = version_number;
											    }

											    i++;
										    }
									    }
								    }
							    });
						}
					}
				}
			}
		}
	}

	s_app_cache.save();
}

static void uninstall(ts::v1::package* package)
{
	const auto rom_plugins_plugin_folder = std::filesystem::path(s_app_cache.rom_folder_path_utf8) / "plugins" / package->full_name;

	SPDLOG_LOGGER_INFO(logger, "uninstalling {}", package->full_name);
	SPDLOG_LOGGER_INFO(logger, L"with path {}", rom_plugins_plugin_folder.wstring());

	if (std::filesystem::exists(rom_plugins_plugin_folder))
	{
		std::filesystem::remove_all(rom_plugins_plugin_folder);
	}
	else
	{
		SPDLOG_LOGGER_INFO(logger, "path did not exist");
	}

	std::thread(
	    []()
	    {
		    on_game_folder_found();
	    })
	    .detach();
}

void gui::render_available_mods_panel()
{
	ImGui::Begin(available_mods_title);

	static auto need_to_init = true;
	if (need_to_init)
	{
		need_to_init = false;

		std::thread(
		    []
		    {
			    const auto r = cpr::Get(cpr::Url{"https://thunderstore.io/c/risk-of-rain-returns/api/v1/package/"}, cpr::Header{{"accept", "application/json"}});
			    // const auto r = cpr::Get(cpr::Url{"https://thunderstore.io/c/riskofrain2/api/v1/package/"}, cpr::Header{{"accept", "application/json"}});
			    auto session = cpr::Session();
			    if (r.status_code == 200)
			    {
				    packages_json = nlohmann::json::parse(r.text, nullptr, false, true);
				    init_available_packages();

				    std::unique_lock packages_lock(packages_mutex);
				    for (auto& el : packages)
				    {
					    std::filesystem::path icon_path  = get_root_cache_folder();
					    icon_path                       /= "icons";

					    if (!std::filesystem::exists(icon_path))
					    {
						    std::filesystem::create_directories(icon_path);
					    }

					    icon_path /= el->versions[0].full_name;
					    icon_path += ".png";

					    if (!std::filesystem::exists(icon_path))
					    {
						    auto ofstream = std::ofstream(icon_path, std::ios::app | std::ios::binary);
						    session.SetUrl(cpr::Url{el->versions[0].icon});
						    auto response = session.Download(ofstream);
					    }

					    HANDLE file_handle = CreateFileW(icon_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					    if (file_handle == INVALID_HANDLE_VALUE)
					    {
						    continue;
					    }
					    int nHandle = _open_osfhandle((intptr_t)file_handle, _O_RDONLY);
					    if (nHandle == -1)
					    {
						    ::CloseHandle(file_handle);
						    continue;
					    }
					    FILE* f = _fdopen(nHandle, "rb");
					    if (!f)
					    {
						    ::CloseHandle(file_handle);
						    continue;
					    }

					    int my_image_width;
					    int my_image_height;
					    LoadTextureFromFile(f, &el->versions[0].icon_texture, &my_image_width, &my_image_height);
				    }

				    available_packages_ready = true;
			    }
		    })
		    .detach();
	}

	if (available_packages_ready)
	{
		ImGui::SeparatorText("Search & Sort");

		static std::string search_text_input;
		if (ImGui::InputText("Search##available_mods", &search_text_input))
		{
			search_text_input = imm::string::to_lower(search_text_input);
		}

		if (ImGui::Button("A to Z"))
		{
			std::sort(packages.begin(),
			          packages.end(),
			          [](std::unique_ptr<ts::v1::package>& a, std::unique_ptr<ts::v1::package>& b)
			          {
				          return a->full_name < b->full_name;
			          });
		}
		ImGui::SameLine();
		if (ImGui::Button("Z to A"))
		{
			std::sort(packages.begin(),
			          packages.end(),
			          [](std::unique_ptr<ts::v1::package>& a, std::unique_ptr<ts::v1::package>& b)
			          {
				          return a->full_name > b->full_name;
			          });
		}
		ImGui::SameLine();
		if (ImGui::Button("Last Updated"))
		{
			std::sort(packages.begin(),
			          packages.end(),
			          [](std::unique_ptr<ts::v1::package>& a, std::unique_ptr<ts::v1::package>& b)
			          {
				          return a->date_updated > b->date_updated;
			          });
		}

		static bool show_modpacks      = false;
		static bool show_only_modpacks = false;
		ImGui::Checkbox("Show Modpacks", &show_modpacks);
		ImGui::SameLine();
		static bool show_deprecated = true;
		ImGui::Checkbox("Show Deprecated", &show_deprecated);
		if (show_modpacks)
		{
			ImGui::Checkbox("Show Only Modpacks", &show_only_modpacks);
		}

		ImGui::SeparatorText(std::format("Available Mods ({})", packages.size()).c_str());

		// ImGui::BeginChild("Available Mods", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle);
		ImGui::BeginChild("Available Mods");
		std::unique_lock packages_lock(packages_mutex);
		for (const auto& package : packages)
		{
			bool is_modpack = false;
			for (const auto& categ : package->categories)
			{
				if (categ == "Modpacks")
				{
					is_modpack = true;
					break;
				}
			}

			if (!show_modpacks)
			{
				if (is_modpack)
				{
					continue;
				}
			}
			else if (show_only_modpacks && !is_modpack)
			{
				continue;
			}

			if (!show_deprecated)
			{
				if (package->is_deprecated)
				{
					continue;
				}
			}

			if (strlen(search_text_input.data()))
			{
				if (!package->full_name_lower.contains(search_text_input))
				{
					continue;
				}
			}

			bool pushed_color_this_frame = false;
			if (package->is_deprecated)
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, DEPRECATED_COLOR_BG);
				pushed_color_this_frame = true;
			}
			else if (package->is_installed)
			{
				// ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 1.0f, 0.0f, 0.25f));
				// pushed_color_this_frame = true;
			}

			ImGui::BeginChild(package->name.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle);

			ImGui::Image((void*)package->versions[0].icon_texture, ImVec2(256 / 2, 256 / 2));
			ImGui::SameLine();
			if (package->is_local)
			{
				ImGui::TextWrapped("Author: %s\n\nName: %s\n\nDescription: %s\n\nVersion: %s",
				                   package->owner.c_str(),
				                   package->name.c_str(),
				                   package->versions[0].description.c_str(),
				                   package->versions[0].version_number.c_str());
			}
			else
			{
				ImGui::TextWrapped("Author: %s\n\nName: %s\n\nDescription: %s\n\nLatest Version: %s",
				                   package->owner.c_str(),
				                   package->name.c_str(),
				                   package->versions[0].description.c_str(),
				                   package->versions[0].version_number.c_str());
			}
			if (package->is_deprecated)
			{
				ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(DEPRECATED_COLOR), "Deprecated");
			}

			if (package->is_installed)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.75f));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.75f));
			}

			if (ImGui::Button(package->is_installed ? std::format("Uninstall {}", package->installed_version_number).c_str() :
			                                          std::format("Install {}", package->versions[0].version_number).c_str(),
			                  ImVec2(200, 0)))
			{
				package->is_installed = !package->is_installed;

				if (package->is_installed)
				{
					std::thread(
					    [&]
					    {
						    auto session = cpr::Session();

						    auto download_package_version = [&](const ts::v1::package_version& pkg_version) -> std::filesystem::path
						    {
							    std::filesystem::path zip_path  = get_root_cache_folder();
							    zip_path                       /= "zips";

							    if (!std::filesystem::exists(zip_path))
							    {
								    std::filesystem::create_directories(zip_path);
							    }

							    zip_path /= pkg_version.full_name;
							    zip_path += ".zip";

							    if (!std::filesystem::exists(zip_path))
							    {
								    auto ofstream = std::ofstream(zip_path, std::ios::app | std::ios::binary);
								    session.SetUrl(cpr::Url{pkg_version.download_url});
								    auto response = session.Download(ofstream);
							    }

							    return zip_path;
						    };

						    auto download_dep = [&](std::string dep) -> std::filesystem::path
						    {
							    std::unique_lock packages_lock(packages_mutex);
							    for (const auto& other_pkg : packages)
							    {
								    /*for (const auto& other_pkg_version : other_pkg->versions)
								    {
									    if (other_pkg_version.full_name == dep)
									    {
										    return download_package_version(other_pkg_version);
									    }
								    }*/

								    for (const auto& other_pkg_version : other_pkg->versions)
								    {
									    if (other_pkg_version.full_name == dep)
									    {
										    // Download latest.
										    return download_package_version(other_pkg->versions[0]);
									    }
								    }
							    }

							    return "";
						    };

						    auto handle_zip = [](std::filesystem::path zip_path)
						    {
							    const auto extracted_zip_folder_path = zip_path.parent_path() / zip_path.stem();
							    if (std::filesystem::exists(zip_path))
							    {
								    if (zip_extract((char*)zip_path.u8string().c_str(),
								                    (char*)extracted_zip_folder_path.u8string().c_str(),
								                    nullptr,
								                    nullptr)
								        == 0)
								    {
									    const auto output_package_folder_name_splitted =
									        imm::string::split((char*)zip_path.stem().u8string().c_str(), '-');
									    const auto output_package_folder_name = output_package_folder_name_splitted[0] + '-' + output_package_folder_name_splitted[1];

									    if (output_package_folder_name == "ReturnOfModding-ReturnOfModding")
									    {
										    for (const auto& entry : std::filesystem::recursive_directory_iterator(extracted_zip_folder_path, std::filesystem::directory_options::skip_permission_denied))
										    {
											    if (entry.path().filename() == "version.dll")
											    {
												    std::filesystem::copy(entry, std::filesystem::path(s_app_cache.game_folder_path) / "version.dll", std::filesystem::copy_options::overwrite_existing);
											    }
										    }
									    }
									    else
									    {
										    const auto rom_plugins_plugin_folder = std::filesystem::path(s_app_cache.rom_folder_path_utf8) / "plugins" / output_package_folder_name;
										    if (!std::filesystem::exists(rom_plugins_plugin_folder))
										    {
											    std::filesystem::create_directories(rom_plugins_plugin_folder);
										    }

										    const auto rom_plugins_data_plugin_folder =
										        std::filesystem::path(s_app_cache.rom_folder_path_utf8) / "plugins_data" / output_package_folder_name;
										    if (!std::filesystem::exists(rom_plugins_data_plugin_folder))
										    {
											    std::filesystem::create_directories(rom_plugins_data_plugin_folder);
										    }

										    const auto rom_config_plugin_folder = std::filesystem::path(s_app_cache.rom_folder_path_utf8) / "config" / output_package_folder_name;
										    if (!std::filesystem::exists(rom_config_plugin_folder))
										    {
											    std::filesystem::create_directories(rom_config_plugin_folder);
										    }

										    std::vector<std::filesystem::path> already_copied_directories;
										    for (const auto& entry : std::filesystem::recursive_directory_iterator(extracted_zip_folder_path, std::filesystem::directory_options::skip_permission_denied))
										    {
											    SPDLOG_LOGGER_INFO(logger, entry.path().wstring());

											    if (entry.path().parent_path() == extracted_zip_folder_path && !entry.is_directory())
											    {
												    std::filesystem::copy(entry.path(),
												                          rom_plugins_plugin_folder / entry.path().filename(),
												                          std::filesystem::copy_options::overwrite_existing);
											    }
											    else if (entry.is_directory())
											    {
												    auto is_subpath = [](const std::filesystem::path& path, const std::filesystem::path& base) -> bool
												    {
													    auto rel = std::filesystem::relative(path, base);
													    return !rel.empty() && rel.native()[0] != '.';
												    };

												    bool is_already_copied = false;
												    for (const auto& already_copied_dir : already_copied_directories)
												    {
													    if (is_subpath(entry.path(), already_copied_dir))
													    {
														    is_already_copied = true;
													    }
												    }
												    if (is_already_copied)
												    {
													    continue;
												    }
												    already_copied_directories.push_back(entry.path());

												    if (entry.path().filename() == "plugins")
												    {
													    std::filesystem::copy(entry.path(),
													                          rom_plugins_plugin_folder / entry.path().filename(),
													                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
												    }
												    else if (entry.path().filename() == "plugins_data")
												    {
													    std::filesystem::copy(entry.path(),
													                          rom_plugins_data_plugin_folder
													                              / entry.path().filename(),
													                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
												    }
												    else if (entry.path().filename() == "config")
												    {
													    std::filesystem::copy(entry.path(),
													                          rom_config_plugin_folder / entry.path().filename(),
													                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
												    }
												    else
												    {
													    std::filesystem::copy(entry.path(),
													                          rom_plugins_plugin_folder / entry.path().filename(),
													                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
												    }
											    }
										    }
									    }
								    }
							    }
						    };

						    for (const auto& dep : package->versions[0].dependencies)
						    {
							    const auto zip_path = download_dep(dep);

							    std::unique_lock installed_packages_lock(installed_packages_mutex);
							    for (const auto& installed_package : installed_packages)
							    {
								    const auto dep_split   = imm::string::split(dep, '-');
								    const auto dep_version = dep_split[2];
								    semver::version installed_vers(
								        installed_package.pkg->versions[installed_package.pkg_version_index].version_number);
								    if (installed_vers < semver::version(dep_version))
								    {
									    handle_zip(zip_path);
								    }
							    }
						    }

						    const auto zip_path = download_package_version(package->versions[0]);
						    handle_zip(zip_path);

						    on_game_folder_found();
					    })
					    .detach();
				}
				else
				{
					uninstall(package.get());
				}
			}

			if (package->versions[0].dependencies.size())
			{
				if (ImGui::CollapsingHeader("Dependencies"))
				{
					for (const auto& dep : package->versions[0].dependencies)
					{
						ImGui::BulletText(dep.c_str());
					}
				}
			}
			else
			{
				ImGui::Text("No Dependencies");
			}

			ImGui::PopStyleColor();

			ImGui::Separator();

			ImGui::EndChild();

			if (pushed_color_this_frame)
			{
				ImGui::PopStyleColor();
			}
		}
		ImGui::EndChild();
	}

	ImGui::End();
}

struct process_running_info
{
	bool running = false;
	std::wstring path{};
};

static process_running_info is_process_running(const std::wstring& process_name)
{
	const auto process_snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (process_snapshot_handle == INVALID_HANDLE_VALUE)
	{
		SPDLOG_LOGGER_INFO(logger, "Invalid handle value for process snapshot");
		return {};
	}

	PROCESSENTRY32 pe32 = {0};
	pe32.dwSize         = sizeof(PROCESSENTRY32);
	if (!Process32First(process_snapshot_handle, &pe32))
	{
		SPDLOG_LOGGER_INFO(logger, "Failed Process32First");
		CloseHandle(process_snapshot_handle);
		return {};
	}

	do
	{
		if (wcscmp(pe32.szExeFile, process_name.c_str()) == 0)
		{
			MODULEENTRY32 me32                 = {0};
			const auto modules_snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe32.th32ProcessID);
			if (modules_snapshot_handle == INVALID_HANDLE_VALUE)
			{
				CloseHandle(process_snapshot_handle);
				return {};
			}

			me32.dwSize = sizeof(MODULEENTRY32);
			if (Module32First(modules_snapshot_handle, &me32))
			{
				do
				{
					if (wcscmp(pe32.szExeFile, process_name.c_str()) == 0)
					{
						CloseHandle(modules_snapshot_handle);
						CloseHandle(process_snapshot_handle);
						return {.running = true, .path = me32.szExePath};
					}
				} while (Module32Next(modules_snapshot_handle, &me32));
			}

			CloseHandle(modules_snapshot_handle);
			CloseHandle(process_snapshot_handle);
			return {};
		}
	} while (Process32Next(process_snapshot_handle, &pe32));

	CloseHandle(process_snapshot_handle);
	return {};
}

void gui::render_installed_mods_panel()
{
	ImGui::Begin(installed_mods_title);

	if (ImGui::Button("Launch Game", ImVec2(0, 50)))
	{
		ShellExecuteW(NULL, NULL, L"steam://run/1337520", NULL, NULL, SW_SHOW);
	}

	static bool need_to_init = true;
	if (need_to_init)
	{
		need_to_init = false;

		app_cache_path = get_root_cache_folder();
		if (!std::filesystem::exists(app_cache_path))
		{
			std::filesystem::create_directories(app_cache_path);

			if (std::filesystem::exists(app_cache_path))
			{
				SPDLOG_LOGGER_INFO(logger, L"Created app_cache_path directories {}", app_cache_path.wstring());
			}
			else
			{
				SPDLOG_LOGGER_INFO(logger, L"Didnt manage to create app_cache_path directories {}", app_cache_path.wstring());
			}
		}
		app_cache_path /= "app_cache.json";
		if (std::filesystem::exists(app_cache_path))
		{
			SPDLOG_LOGGER_INFO(logger, L"reading from app cache {}", app_cache_path.wstring());

			std::ifstream app_cache_file_stream(app_cache_path);
			s_app_cache = nlohmann::json::parse(app_cache_file_stream, nullptr, false, true);
			if (std::filesystem::exists(s_app_cache.game_folder_path))
			{
				s_app_cache.game_folder_path_utf8 =
				    (char*)std::filesystem::path(s_app_cache.game_folder_path).u8string().c_str();

				s_app_cache.rom_folder_path_utf8 =
				    (char*)(std::filesystem::path(s_app_cache.game_folder_path_utf8) / "ReturnOfModding").u8string().c_str();

				has_valid_game_folder_path = true;
				on_game_folder_found();
			}
			else
			{
				SPDLOG_LOGGER_INFO(logger, L"game folder path does not exists {}", s_app_cache.game_folder_path);
			}
		}
		else
		{
			SPDLOG_LOGGER_INFO(logger, L"No app cache {}", app_cache_path.wstring());
		}

		std::thread(
		    []
		    {
			    while (true)
			    {
				    const process_running_info risk_of_rain_returns_process_info =
				        is_process_running(L"Risk of Rain Returns.exe");
				    if (risk_of_rain_returns_process_info.running)
				    {
					    const auto new_path = std::filesystem::path(risk_of_rain_returns_process_info.path).parent_path();

					    static bool first_time_here = true;
					    if (first_time_here)
					    {
						    SPDLOG_LOGGER_INFO(logger, L"Got risk of rain returns path {}", new_path.wstring());

						    first_time_here = false;
					    }

					    if (new_path != s_app_cache.game_folder_path)
					    {
						    s_app_cache.game_exe_path    = risk_of_rain_returns_process_info.path;
						    s_app_cache.game_folder_path = new_path;

						    s_app_cache.game_folder_path_utf8 =
						        (char*)std::filesystem::path(s_app_cache.game_folder_path).u8string().c_str();

						    s_app_cache.rom_folder_path_utf8 = (char*)(std::filesystem::path(s_app_cache.game_folder_path_utf8) / "ReturnOfModding")
						                                           .u8string()
						                                           .c_str();

						    has_valid_game_folder_path = true;
						    on_game_folder_found();

						    s_app_cache.save();
					    }
				    }

				    using namespace std::chrono_literals;
				    std::this_thread::sleep_for(1s);
			    }
		    })
		    .detach();

		std::thread(
		    []
		    {
			    while (!available_packages_ready)
			    {
				    using namespace std::chrono_literals;
				    std::this_thread::sleep_for(100ms);
			    }

			    while (true)
			    {
				    {
					    std::unique_lock t_queue_lock(t_queue_mutex);
					    while (t_queue.size())
					    {
						    auto& bla = t_queue.front();

						    bla();

						    t_queue.pop();
					    }
				    }

				    using namespace std::chrono_literals;
				    std::this_thread::sleep_for(250ms);
			    }
		    })
		    .detach();
	}

	if (has_valid_game_folder_path)
	{
		ImGui::SeparatorText("Folders");
		ImGui::TextWrapped("Game Folder");
		ImGui::SameLine();
		if (ImGui::Button("Open##game_folder"))
		{
			ShellExecuteW(NULL, NULL, L"explorer.exe", std::filesystem::path(s_app_cache.game_folder_path_utf8).c_str(), NULL, SW_NORMAL);
		}
		ImGui::TextWrapped(s_app_cache.game_folder_path_utf8.c_str());

		ImGui::Separator();

		ImGui::TextWrapped("ReturnOfModding Folder");
		ImGui::SameLine();
		if (ImGui::Button("Open##rom_folder"))
		{
			ShellExecuteW(NULL, NULL, L"explorer.exe", std::filesystem::path(s_app_cache.rom_folder_path_utf8).c_str(), NULL, SW_NORMAL);
		}
		ImGui::TextWrapped(s_app_cache.rom_folder_path_utf8.c_str());

		ImGui::SeparatorText("Share Profile");
		if (ImGui::Button("Create rorr_mod_list.txt File"))
		{
			std::filesystem::path file_path = std::filesystem::absolute("./rorr_mod_list.txt");
			if (std::filesystem::exists(file_path))
			{
				std::filesystem::remove_all(file_path);
			}
			std::ofstream o(file_path, std::ios::out | std::ios::binary);

			if (!o.is_open())
			{
				std::wstring error_msg = L"Error opening file for writing: " + file_path.wstring();
				MessageBox(0, error_msg.c_str(), L"IMM", 0);
				return;
			}

			bool has_any_local_mod = false;
			std::unique_lock installed_packages_lock(installed_packages_mutex);
			for (const auto& installed_pkg : installed_packages)
			{
				if (installed_pkg.is_local)
				{
					has_any_local_mod = true;
				}

				o << installed_pkg.pkg->versions[installed_pkg.pkg_version_index].full_name << '\n';
			}

			o.close();

			if (has_any_local_mod)
			{
				std::wstring error_msg = L"Profile contains local mods, those mods can't be properly shared currently.";
				MessageBox(0, error_msg.c_str(), L"IMM", 0);
			}

			std::wstring param = std::wstring(L"/select, ") + file_path.c_str();
			ShellExecuteW(NULL, NULL, L"explorer.exe", param.c_str(), NULL, SW_NORMAL);
		}

		ImGui::SeparatorText("Search & Sort");

		static std::string search_text_input;
		if (ImGui::InputText("Search##installed_mods", &search_text_input))
		{
			search_text_input = imm::string::to_lower(search_text_input);
		}
		if (ImGui::Button("A to Z"))
		{
			std::unique_lock installed_packages_lock(installed_packages_mutex);
			std::sort(installed_packages.begin(),
			          installed_packages.end(),
			          [](installed_package& a, installed_package& b)
			          {
				          return a.pkg->full_name < b.pkg->full_name;
			          });
		}
		ImGui::SameLine();
		if (ImGui::Button("Z to A"))
		{
			std::unique_lock installed_packages_lock(installed_packages_mutex);
			std::sort(installed_packages.begin(),
			          installed_packages.end(),
			          [](installed_package& a, installed_package& b)
			          {
				          //   std::cout << "A: " << a.pkg->full_name << " (res:" << (a.pkg->full_name > b.pkg->full_name)
				          //             << ") B: " << b.pkg->full_name << std::endl;
				          return a.pkg->full_name > b.pkg->full_name;
			          });
		}

		ImGui::SeparatorText(std::format("Installed Mods ({})", installed_packages.size()).c_str());

		int i = 0;
		std::unique_lock installed_packages_lock(installed_packages_mutex);
		for (auto& installed_package : installed_packages)
		{
			if (strlen(search_text_input.data()))
			{
				if (!installed_package.pkg->versions[installed_package.pkg_version_index].full_name_lower.contains(search_text_input))
				{
					continue;
				}
			}


			if (installed_package.is_enabled)
			{
				// ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 1.0f, 0.0f, 0.25f));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
			}
			else if (installed_package.pkg->is_deprecated)
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, DEPRECATED_COLOR_BG);
			}
			else
			{
				// ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 1.0f, 0.0f, 0.1f));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
			}

			ImGui::BeginChild(installed_package.pkg->name.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle);

			ImGui::Image((void*)installed_package.pkg->versions[installed_package.pkg_version_index].icon_texture, ImVec2(256 / 2, 256 / 2));
			ImGui::SameLine();
			if (installed_package.is_local)
			{
				ImGui::TextWrapped("Author: %s\n\nName: %s\n\nDescription: %s\n\nVersion: %s%s",
				                   installed_package.pkg->owner.c_str(),
				                   installed_package.pkg->name.c_str(),
				                   installed_package.pkg->versions[installed_package.pkg_version_index].description.c_str(),
				                   installed_package.pkg->versions[installed_package.pkg_version_index].version_number.c_str(),
				                   installed_package.is_local ? "\n\n(Local Package)" : "");
			}
			else
			{
				ImGui::TextWrapped("Author: %s\n\nName: %s\n\nDescription: %s\n\nLatest Version: %s%s",
				                   installed_package.pkg->owner.c_str(),
				                   installed_package.pkg->name.c_str(),
				                   installed_package.pkg->versions[installed_package.pkg_version_index].description.c_str(),
				                   installed_package.pkg->versions[installed_package.pkg_version_index].version_number.c_str(),
				                   installed_package.is_local ? "\n\n(Local Package)" : "");
			}

			if (installed_package.is_local)
			{
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				{
					ImGui::SetTooltip("Local package. Could not find it on the thunderstore website.");
				}
			}

			ImGui::PushID(i);

			if (installed_package.is_enabled)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
			}
			if (ImGui::Toggle(installed_package.is_enabled ? "Enabled" : "Disabled", &installed_package.is_enabled, ImGuiToggleFlags_Animated))
			{
				for (auto& enabled_state : s_app_cache.active_profile->package_enabled_states)
				{
					if (installed_package.pkg->full_name == enabled_state.full_name)
					{
						const auto manifest_file_path          = installed_package.folder / "manifest.json";
						const auto manifest_disabled_file_path = installed_package.folder / "manifest_disabled.json";
						if (std::filesystem::exists(manifest_file_path) && !std::filesystem::exists(manifest_disabled_file_path))
						{
							std::filesystem::rename(manifest_file_path, manifest_disabled_file_path);
						}
						else if (std::filesystem::exists(manifest_disabled_file_path) && !std::filesystem::exists(manifest_file_path))
						{
							std::filesystem::rename(manifest_disabled_file_path, manifest_file_path);
						}

						enabled_state.is_enabled = installed_package.is_enabled;
						s_app_cache.save();

						break;
					}
				}
			}
			ImGui::PopStyleColor();

			if (ImGui::Button("Open Folder"))
			{
				ShellExecuteW(NULL, NULL, L"explorer.exe", installed_package.folder.c_str(), NULL, SW_NORMAL);
			}
			if (installed_package.pkg->is_installed)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.75f));
				if (ImGui::Button(std::format("Uninstall {}", installed_package.pkg->installed_version_number).c_str(), ImVec2(200, 0)))
				{
					uninstall(installed_package.pkg);
				}
				ImGui::PopStyleColor();
			}

			ImGui::PopID();

			ImGui::Separator();

			ImGui::EndChild();

			ImGui::PopStyleColor();

			i++;
		}
	}
	else
	{
		ImGui::TextWrapped(
		    "To show installed mods, please launch the game at least once while the mod manager is running");
	}

	ImGui::End();
}

void gui::render()
{
	static bool init_theme = true;
	if (init_theme)
	{
		auto& colors               = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg]  = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
		colors[ImGuiCol_MenuBarBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

		// Border
		colors[ImGuiCol_Border]       = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
		colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.24f};

		// Text
		colors[ImGuiCol_Text]         = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
		colors[ImGuiCol_TextDisabled] = ImVec4{0.5f, 0.5f, 0.5f, 1.0f};

		// Headers
		colors[ImGuiCol_Header]        = ImVec4{0.13f, 0.13f, 0.17, 1.0f};
		colors[ImGuiCol_HeaderHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
		colors[ImGuiCol_HeaderActive]  = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

		// Buttons
		colors[ImGuiCol_Button]        = ImVec4{0.13f, 0.13f, 0.17, 1.0f};
		colors[ImGuiCol_ButtonHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
		colors[ImGuiCol_ButtonActive]  = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
		colors[ImGuiCol_CheckMark]     = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};

		// Popups
		colors[ImGuiCol_PopupBg] = ImVec4{0.1f, 0.1f, 0.13f, 0.92f};

		// Slider
		colors[ImGuiCol_SliderGrab]       = ImVec4{0.44f, 0.37f, 0.61f, 0.54f};
		colors[ImGuiCol_SliderGrabActive] = ImVec4{0.74f, 0.58f, 0.98f, 0.54f};

		// Frame BG
		colors[ImGuiCol_FrameBg]        = ImVec4{0.13f, 0.13, 0.17, 1.0f};
		colors[ImGuiCol_FrameBgHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
		colors[ImGuiCol_FrameBgActive]  = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

		// Tabs
		colors[ImGuiCol_Tab]                = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
		colors[ImGuiCol_TabHovered]         = ImVec4{0.24, 0.24f, 0.32f, 1.0f};
		colors[ImGuiCol_TabActive]          = ImVec4{0.2f, 0.22f, 0.27f, 1.0f};
		colors[ImGuiCol_TabUnfocused]       = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

		// Title
		colors[ImGuiCol_TitleBg]          = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
		colors[ImGuiCol_TitleBgActive]    = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg]          = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
		colors[ImGuiCol_ScrollbarGrab]        = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
		colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4{0.24f, 0.24f, 0.32f, 1.0f};

		// Seperator
		colors[ImGuiCol_Separator]        = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};
		colors[ImGuiCol_SeparatorHovered] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};
		colors[ImGuiCol_SeparatorActive]  = ImVec4{0.84f, 0.58f, 1.0f, 1.0f};

		// Resize Grip
		colors[ImGuiCol_ResizeGrip]        = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.74f, 0.58f, 0.98f, 0.29f};
		colors[ImGuiCol_ResizeGripActive]  = ImVec4{0.84f, 0.58f, 1.0f, 0.29f};

		// Docking
		colors[ImGuiCol_DockingPreview] = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};

		auto& style             = ImGui::GetStyle();
		style.TabRounding       = 4;
		style.ScrollbarRounding = 9;
		style.WindowRounding    = 7;
		style.GrabRounding      = 3;
		style.FrameRounding     = 6;
		style.PopupRounding     = 4;
		style.ChildRounding     = 4;
		style.FrameBorderSize   = 1;

		init_theme = false;
	}

	// static bool id_stack_open = true;
	// ImGui::ShowIDStackToolWindow(&id_stack_open);

	// ImGui::ShowStyleEditor();

	// if (m_show_demo_window)
	// {
	// 	ImGui::ShowDemoWindow(&m_show_demo_window);
	// }

	render_docking_layout();

	render_main_menu_bar();

	render_installed_mods_panel();

	render_available_mods_panel();
}
