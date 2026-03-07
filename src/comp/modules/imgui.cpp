#include "std_include.hpp"
#include "imgui.hpp"

#include "comp_settings.hpp"
#include "imgui_internal.h"
#include "renderer.hpp"
#include "shared/common/imgui_helper.hpp"

// Allow us to directly call the ImGui WndProc function.
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

#define CENTER_URL(text, link)					\
	ImGui::SetCursorForCenteredText((text));	\
	ImGui::TextURL((text), (link), true);

#define SPACEY16 ImGui::Spacing(0.0f, 16.0f);
#define SPACEY12 ImGui::Spacing(0.0f, 12.0f);
#define SPACEY8 ImGui::Spacing(0.0f, 8.0f);
#define SPACEY4 ImGui::Spacing(0.0f, 4.0f);

#define TT(TXT) ImGui::SetItemTooltipWrapper((TXT));

namespace comp
{
	WNDPROC g_game_wndproc = nullptr;
	
	LRESULT __stdcall wnd_proc_hk(HWND window, UINT message_type, WPARAM wparam, LPARAM lparam)
	{
		if (message_type != WM_MOUSEMOVE && message_type != WM_NCMOUSEMOVE)
		{
			if (imgui::get()->input_message(message_type, wparam, lparam)) {
			//	return true;
			}
		}

		// if your game has issues with floating cursors
		/*if (message_type == WM_KILLFOCUS)
		{
			uint32_t counter = 0u;
			while (::ShowCursor(TRUE) < 0 && ++counter < 3) {}
			ClipCursor(NULL);
		}*/

		//printf("MSG 0x%x -- w: 0x%x -- l: 0x%x\n", message_type, wparam, lparam);
		return CallWindowProc(g_game_wndproc, window, message_type, wparam, lparam);
	}

	bool imgui::input_message(const UINT message_type, const WPARAM wparam, const LPARAM lparam)
	{
		if (message_type == WM_KEYUP && wparam == VK_F4) 
		{
			const auto& io = ImGui::GetIO();
			if (!io.MouseDown[1]) {
				shared::globals::imgui_menu_open = !shared::globals::imgui_menu_open;
			} else {
				ImGui_ImplWin32_WndProcHandler(shared::globals::main_window, message_type, wparam, lparam);
			}
		}

		if (shared::globals::imgui_menu_open)
		{
			//auto& io = ImGui::GetIO();
			ImGui_ImplWin32_WndProcHandler(shared::globals::main_window, message_type, wparam, lparam);
		} else {
			shared::globals::imgui_allow_input_bypass = false; // always reset if there is no imgui window open
		}

		return shared::globals::imgui_menu_open;
	}

	// ------

	void imgui::tab_about()
	{
		if (tex_addons::berry)
		{
			const float cursor_y = ImGui::GetCursorPosY();
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.85f, 24));
			ImGui::Image((ImTextureID)tex_addons::berry, ImVec2(48.0f, 48.0f), ImVec2(0.03f, 0.03f), ImVec2(0.96f, 0.96f));
			ImGui::SetCursorPosY(cursor_y);
		}

		ImGui::Spacing(0.0f, 20.0f);

		ImGui::CenterText("BARNYARD RTX-REMIX COMPATIBILITY MOD");
		ImGui::CenterText("                      by #xoxor4d");

		ImGui::Spacing(0.0f, 24.0f);
		ImGui::CenterText("current version");

		const char* version_str = shared::utils::va("%d.%d.%d :: %s", 
			COMP_MOD_VERSION_MAJOR, COMP_MOD_VERSION_MINOR, COMP_MOD_VERSION_PATCH, __DATE__);
		ImGui::CenterText(version_str);

#if DEBUG
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.64f, 0.23f, 0.18f, 1.0f));
		ImGui::CenterText("DEBUG BUILD");
		ImGui::PopStyleColor();
#endif

		SPACEY16;
		CENTER_URL("GitHub Repository", "https://github.com/xoxor4d/barnyard-rtx");

		SPACEY16;
		ImGui::Separator();
		SPACEY16;

		const char* credits_title_str = "Credits / Thanks to:";
		ImGui::CenterText(credits_title_str);

		ImGui::Spacing(0.0f, 8.0f);

		CENTER_URL("NVIDIA - RTX Remix", "https://github.com/NVIDIAGameWorks/rtx-remix");
		CENTER_URL("Dear Imgui", "https://github.com/ocornut/imgui");
		CENTER_URL("Minhook", "https://github.com/TsudaKageyu/minhook");
		CENTER_URL("Toml11", "https://github.com/ToruNiina/toml11");
		CENTER_URL("Ultimate-ASI-Loader", "https://github.com/ThirteenAG/Ultimate-ASI-Loader");
		CENTER_URL("OpenBarnyard - InfiniteC0re", "https://github.com/InfiniteC0re/OpenBarnyard");

		ImGui::Spacing(0.0f, 24.0f);
		ImGui::CenterText("And of course, all my fellow Ko-Fi and Patreon supporters");
		ImGui::CenterText("and all the people that helped along the way.");
		ImGui::Spacing(0.0f, 4.0f);
		ImGui::CenterText("Thank you!");
	}

	// draw imgui widget
	void imgui::ImGuiStats::draw_stats()
	{
		if (!m_tracking_enabled) {
			return;
		}

		for (const auto& p : m_stat_list)
		{
			if (p.second) {
				display_single_stat(p.first, *p.second);
			}
			else {
				ImGui::Spacing(0, 4);
			}
		}
	}

	void imgui::ImGuiStats::display_single_stat(const char* name, const StatObj& stat)
	{
		switch (stat.get_mode())
		{
		case StatObj::Mode::Single:
			ImGui::Text("%s", name);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.65f);
			ImGui::Text("%d total", stat.get_total());
			break;

		case StatObj::Mode::ConditionalCheck:
			ImGui::Text("%s", name);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.65f);
			ImGui::Text("%d total, %d successful", stat.get_total(), stat.get_successful());
			break;

		default:
			throw std::runtime_error("Uncovered Mode in StatObj");
		}
	}

	void compsettings_var_reset_logic(comp_settings::variable& var)
	{
		std::string popup_id = "Reset "s + var.m_name + " ?";

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
		{
			if (!ImGui::IsPopupOpen(popup_id.c_str())) {
				ImGui::OpenPopup(popup_id.c_str());
			}
		}

		ImGui::SetNextWindowSize(ImVec2(300.0f, 140.0f));
		if (ImGui::BeginPopupModal(popup_id.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::Spacing(0.0f, 0.0f);

			ImGui::Spacing();
			ImGui::CenterText("This will reset the current variable");

			ImGui::CenterText("Are you sure?");

			ImGui::Spacing(0, 8);
			ImGui::Spacing(0, 0); ImGui::SameLine();

			const auto half_width = ImGui::GetContentRegionMax().x * 0.5f;
			ImVec2 button_size(half_width - (ImGui::GetStyle().WindowPadding.x * 2.0f) - ImGui::GetStyle().ItemSpacing.x, 0.0f);
			if (ImGui::Button("Yes", button_size))
			{
				var.reset();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", button_size)) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	bool compsettings_bool_widget(const char* desc, comp_settings::variable& var)
	{
		const auto cs_var_ptr = var.get_as<bool*>();
		const bool result = ImGui::Checkbox(desc, cs_var_ptr);
		TT(var.get_tooltip_string().c_str());
		compsettings_var_reset_logic(var);
		return result;
	}

	bool compsettings_float_widget(const char* desc, comp_settings::variable& var, const float& min = 0.0f, const float& max = 0.0f, const float& speed = 0.02f)
	{
		const auto cs_var_ptr = var.get_as<float*>();
		const bool result = ImGui::DragFloat(desc, cs_var_ptr, speed, min, max, "%.2f", (min != 0.0f || max != 0.0f) ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None);
		TT(var.get_tooltip_string().c_str());
		compsettings_var_reset_logic(var);
		return result;
	}

	void dev_debug_container()
	{
		SPACEY16;
		const auto& im = imgui::get();

		if (ImGui::CollapsingHeader("Temp Debug Values"))
		{
			SPACEY8;
			ImGui::DragFloat3("Debug Vector", &im->m_debug_vector.x, 0.01f, 0, 0, "%.6f");
			ImGui::DragFloat3("Debug Vector 2", &im->m_debug_vector2.x, 0.1f, 0, 0, "%.6f");
			ImGui::DragFloat3("Debug Vector 3", &im->m_debug_vector3.x, 0.1f, 0, 0, "%.6f");
			ImGui::DragFloat3("Debug Vector 4", &im->m_debug_vector4.x, 0.1f, 0, 0, "%.6f");
			ImGui::DragFloat3("Debug Vector 5", &im->m_debug_vector5.x, 0.1f, 0, 0, "%.6f");

			ImGui::Checkbox("Debug Bool 1", &im->m_dbg_debug_bool01);
			ImGui::Checkbox("Debug Bool 2", &im->m_dbg_debug_bool02);
			ImGui::Checkbox("Debug Bool 3", &im->m_dbg_debug_bool03);
			ImGui::Checkbox("Debug Bool 4", &im->m_dbg_debug_bool04);
			ImGui::Checkbox("Debug Bool 5", &im->m_dbg_debug_bool05);
			ImGui::Checkbox("Debug Bool 6", &im->m_dbg_debug_bool06);
			ImGui::Checkbox("Debug Bool 7", &im->m_dbg_debug_bool07);
			ImGui::Checkbox("Debug Bool 8", &im->m_dbg_debug_bool08);
			ImGui::Checkbox("Debug Bool 9", &im->m_dbg_debug_bool09);

			ImGui::DragInt("Debug Int 1", &im->m_dbg_int_01, 0.01f);
			ImGui::DragInt("Debug Int 2", &im->m_dbg_int_02, 0.01f);
			ImGui::DragInt("Debug Int 3", &im->m_dbg_int_03, 0.01f);
			ImGui::DragInt("Debug Int 4", &im->m_dbg_int_04, 0.01f);
			ImGui::DragInt("Debug Int 5", &im->m_dbg_int_05, 0.01f);
			SPACEY8;
		}

		if (ImGui::CollapsingHeader("Fake Camera"))
		{
			SPACEY8;
			ImGui::Checkbox("Use Fake Camera", &im->m_dbg_use_fake_camera);
			ImGui::BeginDisabled(!im->m_dbg_use_fake_camera);
			{
				ImGui::SliderFloat3("Camera Position (X, Y, Z)", im->m_dbg_camera_pos, -10000.0f, 10000.0f);
				ImGui::SliderFloat("Yaw (Y-axis)", &im->m_dbg_camera_yaw, -180.0f, 180.0f);
				ImGui::SliderFloat("Pitch (X-axis)", &im->m_dbg_camera_pitch, -90.0f, 90.0f);

				// Projection matrix adjustments
				ImGui::SliderFloat("FOV", &im->m_dbg_camera_fov, 1.0f, 180.0f);
				ImGui::SliderFloat("Aspect Ratio", &im->m_dbg_camera_aspect, 0.2f, 3.555f);
				ImGui::SliderFloat("Near Plane", &im->m_dbg_camera_near_plane, 0.1f, 1000.0f);
				ImGui::SliderFloat("Far Plane", &im->m_dbg_camera_far_plane, 1.0f, 100000.0f);

				ImGui::EndDisabled();
			}
			SPACEY8;
		}

		if (ImGui::CollapsingHeader("Statistics ..."))
		{
			SPACEY8;
			im->m_stats.enable_tracking(true);
			im->m_stats.draw_stats();
			SPACEY8;
		} else {
			im->m_stats.enable_tracking(false);
		}
	}

	void imgui::tab_dev()
	{
		dev_debug_container();
	}

	void cont_compsettings_quick_cmd()
	{
		if (ImGui::Button("Save Current Settings", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0))) {
			comp_settings::write_toml();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload CompSettings", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			if (!ImGui::IsPopupOpen("Reload CompSettings?")) {
				ImGui::OpenPopup("Reload CompSettings?");
			}
		}

		SPACEY4;
		ImGui::CenterText("Use Middle Mouse Button to Reset Variables.");
		SPACEY4;

		// popup
		if (ImGui::BeginPopupModal("Reload CompSettings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::Spacing(0.0f, 0.0f);

			const auto half_width = ImGui::GetContentRegionMax().x * 0.5f;
			auto line1_str = "You'll loose all unsaved changes if you continue!   ";
			auto line2_str = "To save your changes, use:";
			auto line3_str = "Save Current Settings";

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line1_str).x * 0.5f));
			ImGui::TextUnformatted(line1_str);

			ImGui::Spacing();
			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line2_str).x * 0.5f));
			ImGui::TextUnformatted(line2_str);

			ImGui::SetCursorPosX(5.0f + half_width - (ImGui::CalcTextSize(line3_str).x * 0.5f));
			ImGui::TextUnformatted(line3_str);

			ImGui::Spacing(0, 8);
			ImGui::Spacing(0, 0); ImGui::SameLine();

			ImVec2 button_size(half_width - 6.0f - ImGui::GetStyle().WindowPadding.x, 0.0f);
			if (ImGui::Button("Reload", button_size))
			{
				comp_settings::parse_toml();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, 6.0f);
			if (ImGui::Button("Cancel", button_size)) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void imgui::tab_compsettings()
	{
		auto cs = comp_settings::get();

		// quick commands
		cont_compsettings_quick_cmd();
		SPACEY12;

		auto save_logo = []()
			{
				ImGui::SameLine(0, 3.0f); ImGui::TextDisabled("  *  ");

				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.124f, 0.124f, 0.124f, 0.776f));
				if (!ImGui::BeginItemTooltip())
				{
					ImGui::PopStyleColor();
					return;
				}
				ImGui::PopStyleColor();

				const auto padding = 4.0f;

				ImGui::Spacing(0, padding);	// top padding
				ImGui::Spacing(padding, 0); ImGui::SameLine(); // left padding

				ImGui::TextUnformatted("This is a saved Setting.");

				ImGui::SameLine(); ImGui::Spacing(padding, 0); // right padding
				ImGui::Spacing(0, padding);	// bottom padding

				ImGui::EndTooltip();
			};

		const float widget_width = ImGui::GetContentRegionAvail().x * 0.3f;
		ImGui::SetNextItemWidth(widget_width); compsettings_float_widget("Object NoCull Distance", cs->object_nucull_distance, 0.0f, 500.0f, 0.01f); save_logo();
		ImGui::SetNextItemWidth(widget_width); compsettings_float_widget("World NoCull Distance", cs->world_nucull_distance, 0.0f, 500.0f, 0.01f); save_logo();
	}

	// -----------

	void imgui::devgui()
	{
		ImGui::SetNextWindowSize(ImVec2(900, 800), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Barnyard Remix Compatibility-Mod Settings", &shared::globals::imgui_menu_open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImGui::End();
			return;
		}

		m_im_window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
		m_im_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);

		static bool im_demo_menu = false;
		if (im_demo_menu) {
			ImGui::ShowDemoWindow(&im_demo_menu);
		}


#define ADD_TAB(NAME, FUNC) \
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0)));			\
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x + 12.0f, 8));	\
	if (ImGui::BeginTabItem(NAME)) {																		\
		ImGui::PopStyleVar(1);																				\
		if (ImGui::BeginChild("##child_" NAME, ImVec2(0, ImGui::GetContentRegionAvail().y - 38), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar )) {	\
			FUNC(); ImGui::EndChild();																		\
		} else {																							\
			ImGui::EndChild();																				\
		} ImGui::EndTabItem();																				\
	} else { ImGui::PopStyleVar(1); } ImGui::PopStyleColor();

		// ---------------------------------------

		const auto col_top = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.0f));
		const auto col_bottom = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.4f));
		const auto col_border = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.8f));
		const auto pre_tabbar_spos = ImGui::GetCursorScreenPos() - ImGui::GetStyle().WindowPadding;

		ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pre_tabbar_spos, pre_tabbar_spos + ImVec2(ImGui::GetWindowWidth(), 40.0f),
			col_top, col_top, col_bottom, col_bottom);

		ImGui::GetWindowDrawList()->AddLine(pre_tabbar_spos + ImVec2(0, 40.0f), pre_tabbar_spos + ImVec2(ImGui::GetWindowWidth(), 40.0f),
			col_border, 1.0f);

		ImGui::SetCursorScreenPos(pre_tabbar_spos + ImVec2(12,8));

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x + 12.0f, 8));
		ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		if (ImGui::BeginTabBar("devgui_tabs"))
		{
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(1);
			ADD_TAB("CompSettings", tab_compsettings);
			ADD_TAB("Dev", tab_dev);
			ADD_TAB("About", tab_about);
			ImGui::EndTabBar();
		}
		else {
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(1);
		}
#undef ADD_TAB

		{
			ImGui::Separator();
			const char* movement_hint_str = "Hold Right Mouse to enable Game Input ";
			const auto avail_width = ImGui::GetContentRegionAvail().x;
			float cur_pos = avail_width - 54.0f;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			{
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
				const auto spos = ImGui::GetCursorScreenPos();
				ImGui::TextUnformatted(m_devgui_custom_footer_content.c_str());
				ImGui::SetCursorScreenPos(spos);
				m_devgui_custom_footer_content.clear();
			}
			

			ImGui::SetCursorPos(ImVec2(cur_pos, ImGui::GetCursorPosY() + 2.0f));
			if (ImGui::Button("Demo", ImVec2(50, 0))) {
				im_demo_menu = !im_demo_menu;
			}

			ImGui::SameLine();
			cur_pos = cur_pos - ImGui::CalcTextSize(movement_hint_str).x - 6.0f;
			ImGui::SetCursorPosX(cur_pos);
			ImGui::TextUnformatted(movement_hint_str);
		}
		ImGui::PopStyleVar(1);
		ImGui::End();
	}

	void imgui::on_present()
	{
		if (auto* im = imgui::get(); im)
		{
			if (const auto dev = shared::globals::d3d_device; dev)
			{
				if (!im->m_initialized_device)
				{
					//Sleep(1000);
					shared::common::log("ImGui", "ImGui_ImplDX9_Init");
					ImGui_ImplDX9_Init(dev);
					im->m_initialized_device = true;
				}

				// else so we render the first frame one frame later
				else if (im->m_initialized_device)
				{
					// handle srgb
					DWORD og_srgb_samp, og_srgb_write;
					dev->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &og_srgb_samp);
					dev->GetRenderState(D3DRS_SRGBWRITEENABLE, &og_srgb_write);
					dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);
					dev->SetRenderState(D3DRS_SRGBWRITEENABLE, 1);

					ImGui_ImplDX9_NewFrame();
					ImGui_ImplWin32_NewFrame();
					ImGui::NewFrame();

					auto& io = ImGui::GetIO();

					if (shared::globals::imgui_allow_input_bypass_timeout) {
						shared::globals::imgui_allow_input_bypass_timeout--;
					}

					shared::globals::imgui_wants_text_input = ImGui::GetIO().WantTextInput;

					if (shared::globals::imgui_menu_open) 
					{
						io.MouseDrawCursor = true;
						im->devgui();

						// ---
						// enable game input via right mouse button logic

						if (!im->m_im_window_hovered && io.MouseDown[1])
						{
							// reset stuck rmb if timeout is active 
							if (shared::globals::imgui_allow_input_bypass_timeout)
							{
								io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
								shared::globals::imgui_allow_input_bypass_timeout = 0u;
							}

							// enable game input if no imgui window is hovered and right mouse is held
							else
							{
								ImGui::SetWindowFocus(); // unfocus input text
								shared::globals::imgui_allow_input_bypass = true;
							}
						}

						// ^ wait until mouse is up
						else if (shared::globals::imgui_allow_input_bypass && !io.MouseDown[1] && !shared::globals::imgui_allow_input_bypass_timeout)
						{
							shared::globals::imgui_allow_input_bypass_timeout = 2u;
							shared::globals::imgui_allow_input_bypass = false;
						}
					}
					else 
					{
						io.MouseDrawCursor = false;
						shared::globals::imgui_allow_input_bypass_timeout = 0u;
						shared::globals::imgui_allow_input_bypass = false;
					}

					if (im->m_stats.is_tracking_enabled()) {
						im->m_stats.reset_stats();
					}

					shared::globals::imgui_is_rendering = true;
					ImGui::EndFrame();
					ImGui::Render();
					ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
					shared::globals::imgui_is_rendering = false;

					// restore
					dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, og_srgb_samp);
					dev->SetRenderState(D3DRS_SRGBWRITEENABLE, og_srgb_write);
				}
			}
		}
	}

	void imgui::theme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.5f;

		style.WindowPadding = ImVec2(8.0f, 10.0f);
		style.FramePadding = ImVec2(14.0f, 6.0f);
		style.ItemSpacing = ImVec2(10.0f, 5.0f);
		style.ItemInnerSpacing = ImVec2(4.0f, 8.0f);
		style.IndentSpacing = 16.0f;
		style.ColumnsMinSpacing = 10.0f;
		style.ScrollbarSize = 14.0f;
		style.GrabMinSize = 10.0f;

		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 0.0f;

		style.WindowRounding = 4.0f;
		style.ChildRounding = 2.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 2.0f;
		style.ScrollbarRounding = 2.0f;
		style.GrabRounding = 1.0f;
		style.TabRounding = 4.0f;

		style.CellPadding = ImVec2(5.0f, 4.0f);

		auto& colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.23f, 0.23f, 0.23f, 0.96f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.49f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.48f, 0.36f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.21f, 0.61f, 0.46f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.16f, 0.48f, 0.36f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.16f, 0.48f, 0.36f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.21f, 0.60f, 0.45f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.59f, 0.44f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.58f, 0.44f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.58f, 0.44f, 0.06f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.20f, 0.58f, 0.44f, 0.06f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.31f, 0.31f, 0.31f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.38f, 0.38f, 0.38f, 0.95f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.57f, 0.43f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_TabSelected] = ImVec4(0.16f, 0.48f, 0.36f, 1.00f);
		colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.16f, 0.48f, 0.36f, 1.00f);
		colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.22f, 0.16f, 1.00f);
		colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.12f, 0.33f, 0.24f, 1.00f);
	}
	
	imgui::imgui()
	{
		p_this = this;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		//ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.MouseDrawCursor = true;
		//io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
		//io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		theme();

		ImGui_ImplWin32_Init(shared::globals::main_window);
		g_game_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(shared::globals::main_window, GWLP_WNDPROC, LONG_PTR(wnd_proc_hk)));

		// ---
		m_initialized = true;
		shared::common::log("ImGui", "Module initialized.", shared::common::LOG_TYPE::LOG_TYPE_DEFAULT, false);
	}
}



