#ifdef _WIN32
	#include <windows.h> // for warning
#endif
#include "Config.h"
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <json.hpp>
#include <browedit/BrowEdit.h>
#include <browedit/util/FileIO.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <glm/gtc/type_ptr.hpp>
#include <ShlObj_core.h>
#include <magic_enum.hpp>
#include <browedit/HotkeyRegistry.h>
#include <browedit/util/I18n.h>

using json = nlohmann::json;

std::string Config::isValid() const
{
	if (ropath == "")
		return _("Ro path is empty");

	if (!std::filesystem::exists(util::utf8_to_iso_8859_1(ropath)))
		return _("Ro path does not exist");

	if (ropath[ropath.size() - 1] != '\\')
		return _("Ro path should end with a \\");

	if (!std::filesystem::exists(util::utf8_to_iso_8859_1(ropath) + "data"))
		return _("Please create a data directory in your RO directory");

	if (grfEditorPath != "" && !std::filesystem::exists(util::utf8_to_iso_8859_1(grfEditorPath) + "GrfCL.exe"))
		return _("GrfCL.exe not found in grf editor path");
	for (const auto& grf : grfs)
	{
		if (!std::filesystem::exists(util::utf8_to_iso_8859_1(grf)))
			return "Grf " + grf + " does not exist";
		if (grf.substr(std::max(0, (int)grf.size() - 4), 4) != ".grf")
			return "Grf " + grf + " does not end with .grf";
	}
	return "";
}

bool Config::showWindow(BrowEdit* browEdit)
{
	bool close = false;
	static bool showStyleEditor = false;
	if (ImGui::Begin(_("Configuration"), 0, ImGuiWindowFlags_NoCollapse))
	{
		std::string error = isValid();
		if (error != "")
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ImGui::Text(error.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::Text(_("RO Path"));
		ImGui::InputText("##RO Path", &ropath);
		ImGui::SameLine();
		if (ImGui::Button(_("Browse##rodir")))
		{
			auto path = util::SelectPathDialog(util::utf8_to_iso_8859_1(ropath));
			if (!path.empty()) ropath = util::iso_8859_1_to_utf8(path);
		}
		ImGui::Text(_("GRF Files"));
		if(ImGui::BeginListBox("##GRFs"))
		{
			for (int i = 0; i < grfs.size(); i++)
			{
				ImGui::PushID(i);
				ImGui::InputText("##", &grfs[i]);
				ImGui::SameLine();
				if (ImGui::Button(_("Browse##grf")))
				{
					std::string initial = util::utf8_to_iso_8859_1(grfs[i]);
					if (initial.find(":") == std::string::npos)
						initial = std::filesystem::current_path().string() + "\\" + initial;

					if (!std::filesystem::is_regular_file(initial))
						initial = "";

					auto path = util::SelectFileDialog(initial, "All\0*.*\0GRF files\0*.grf\0");
					if (!path.empty()) grfs[i] = util::iso_8859_1_to_utf8(path);
				}
				ImGui::SameLine();
				if (ImGui::Button(_("Remove")))
				{
					grfs.erase(grfs.begin() + i);
					i--;
					ImGui::PopID();
					continue;
				}
				ImGui::SameLine();
				if (ImGui::Button(_("Up")))
				{
					if (i > 0)
						std::swap(grfs[i], grfs[i - 1]);
				}
				ImGui::SameLine();
				if (ImGui::Button(_("Down")))
				{
					if (i < grfs.size() - 1)
						std::swap(grfs[i], grfs[i + 1]);
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}
		ImGui::SameLine();
		if (ImGui::Button(_("Add")))
		{
			grfs.push_back(ropath + "data.grf");
		}

		ImGui::DragFloat(_("Field of View"), &fov, 0.1f, 1.0f, 180.0f);
		ImGui::DragFloat(_("Camera Mouse Speed"), &cameraMouseSpeed, 0.05f, 0.01f, 3.0f);

		if (ImGui::Combo(_("Skin"), &style, "Dark\0Light\0Classic\0Tarq\0Mina's Hot Fudge 1.2\0"))
		{
			setStyle(style);
		}
		{
			auto& langs = i18n::getAvailableLanguages();
			int langIdx = 0;
			std::string cur = i18n::getLanguage();
			for (int i = 0; i < (int)langs.size(); i++)
				if (langs[i].first == cur) { langIdx = i; break; }
			std::string langItems;
			for (auto& l : langs) { langItems += l.second; langItems.push_back('\0'); }
			if (ImGui::Combo(_("Language"), &langIdx, langItems.c_str()))
			{
				language = langs[langIdx].first;
				i18n::setLanguage(language);
			}
		}
		if (ImGui::Button(_("Style editor")))
			showStyleEditor = !showStyleEditor;

		ImGui::ColorEdit3(_("Background Color"), glm::value_ptr(backgroundColor));
		ImGui::ColorEdit3(_("Wall Edit Selection Color"), glm::value_ptr(wallEditSelectionColor));
		ImGui::ColorEdit3(_("Wall Edit Highlight Color"), glm::value_ptr(wallEditHighlightColor));
		ImGui::DragFloat2 (_("Object Window thumbnail size"), &thumbnailSize.x, 1, 32, 512);
		ImGui::Checkbox(_("Close object window when adding a model"), &closeObjectWindowOnAdd);
		ImGui::DragFloat(_("Toolbar Button Size"), &toolbarButtonSize, 1, 1, 100);
		ImGui::ColorEdit4(_("Toolbar Button Tint"), &toolbarButtonTint.x);
		ImGui::ColorEdit4(_("Toolbar View Buttons"), &toolbarButtonsViewOptions.x);
		ImGui::ColorEdit4(_("Toolbar Height Edit"), &toolbarButtonsHeightEdit.x);
		ImGui::ColorEdit4(_("Toolbar Texture Edit"), &toolbarButtonsTextureEdit.x);
		ImGui::ColorEdit4(_("Toolbar Object Edit"), &toolbarButtonsObjectEdit.x);
		ImGui::ColorEdit4(_("Toolbar Wall Edit"), &toolbarButtonsWallEdit.x);
		ImGui::ColorEdit4(_("Toolbar Gat Edit"), &toolbarButtonsGatEdit.x);
		ImGui::InputInt(_("Lightmapper threadcount"), &lightmapperThreadCount);
		if (lightmapperThreadCount < 0)
			lightmapperThreadCount = 0;
		if (lightmapperThreadCount > 50)
			lightmapperThreadCount = 50;

		std::string editModes = "";
		for (auto e : magic_enum::enum_entries<BrowEdit::EditMode>())
		{
			editModes += std::string(e.second) + '\0';
		}

		ImGui::Combo(_("Default Editmode"), &defaultEditMode, editModes.c_str());

		ImGui::Checkbox(_("Start in maximized window"), &startMaximizedWindow);
		ImGui::Checkbox(_("Save a backup of maps when saving"), &backup);
		ImGui::Checkbox(_("Recalculate quadtree on save"), &recalculateQuadtreeOnSave);

		ImGui::Text(_("Grid Sizes"));
		if (ImGui::BeginListBox(_("Translate Grid Sizes")))
		{
			for (int i = 0; i < translateGridSizes.size(); i++)
			{
				ImGui::PushID(i);
				ImGui::InputFloat("##", &translateGridSizes[i]);
				ImGui::SameLine();
				if (ImGui::Button("-"))
				{
					translateGridSizes.erase(translateGridSizes.begin() + i);
					i--;
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}
		if (ImGui::Button("+##Translate"))
			translateGridSizes.push_back(1);

		if (ImGui::BeginListBox(_("Rotate Grid Sizes")))
		{
			for (int i = 0; i < rotateGridSizes.size(); i++)
			{
				ImGui::PushID(i);
				ImGui::InputFloat("##", &rotateGridSizes[i]);
				ImGui::SameLine();
				if (ImGui::Button("-"))
				{
					rotateGridSizes.erase(rotateGridSizes.begin() + i);
					i--;
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}
		if (ImGui::Button("+##Rotate"))
			rotateGridSizes.push_back(1);


		ImGui::InputText(_("GRF Editor path"), &grfEditorPath);
		ImGui::SameLine();
		if (ImGui::Button(_("Browse##grfeditor")))
		{
			std::string initial = util::utf8_to_iso_8859_1(grfEditorPath);
			if (initial == "")
				initial = "c:\\Program Files (x86)\\GRF Editor\\";

			auto path = util::SelectPathDialog(initial, "Select your GRF editor path (not data)");
			if (!path.empty()) grfEditorPath = util::iso_8859_1_to_utf8(path);
		}

		ImGui::InputText(_("ffmpeg path"), &ffmpegPath);
		ImGui::SameLine();
		if (ImGui::Button(_("Browse##ffmpegPath")))
		{
			std::string initial = util::utf8_to_iso_8859_1(ffmpegPath);
			if (initial.find(":") == std::string::npos)
				initial = std::filesystem::current_path().string() + "\\" + initial;

			if (!std::filesystem::is_regular_file(initial))
				initial = "";

			auto path = util::SelectFileDialog(initial, "ffmpeg\0ffmpeg.exe\0All Files\0*.*\0");
			if (!path.empty()) ffmpegPath = util::iso_8859_1_to_utf8(path);
		}

		if (ImGui::Button(_("Save")))
		{
			if (isValid() == "")
			{
				save();
				close = true;

				browEdit->windowData.objectWindowSelectedTreeNode = nullptr;
				setupFileIO();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(_("Cancel")))
		{
			browEdit->configBegin();
			if (isValid() == "")
			{
				close = true;
				browEdit->windowData.objectWindowSelectedTreeNode = nullptr;
			}
		}

	}
	ImGui::End();

	if(showStyleEditor)
		ImGui::ShowStyleEditor();

	return close;
}

void Config::save()
{
	json configJson = *this;
	std::ofstream configFile("config.json");
	configFile << std::setw(2) << configJson;
}

void Config::setupFileIO()
{
	util::FileIO::begin();
	util::FileIO::addDirectory(".\\");
	util::FileIO::addDirectory(util::utf8_to_iso_8859_1(ropath));
	for (const auto& grf : grfs)
		util::FileIO::addGrf(util::utf8_to_iso_8859_1(grf));
	util::FileIO::end();
}


void Config::setStyle(int style)
{

	ImVec4* colors = ImGui::GetStyle().Colors;
	switch (style)
	{
	case 0: ImGui::StyleColorsDark(); break;
	case 1: ImGui::StyleColorsLight(); break;
	case 2: ImGui::StyleColorsClassic(); break;
	case 3:
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 0.94f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.25f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.94f, 0.55f, 0.23f, 0.32f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.94f, 0.55f, 0.23f, 0.65f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.62f, 0.31f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.56f, 0.24f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.98f, 0.62f, 0.31f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(1.00f, 0.56f, 0.24f, 0.58f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.56f, 0.21f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.98f, 0.56f, 0.22f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(1.00f, 0.56f, 0.24f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.56f, 0.24f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.56f, 0.24f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.98f, 0.62f, 0.31f, 1.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.98f, 0.56f, 0.22f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.98f, 0.62f, 0.31f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.94f, 0.55f, 0.23f, 0.54f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.98f, 0.56f, 0.22f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.56f, 0.24f, 0.74f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.29f, 0.16f, 0.05f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.77f, 0.48f, 0.24f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.94f, 0.55f, 0.23f, 1.00f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.94f, 0.55f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.94f, 0.55f, 0.23f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		break;
	case 4: //mina's hot fudge
		colors[ImGuiCol_Text] = ImVec4(0.90f, 0.75f, 0.57f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.48f, 0.30f, 0.16f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.48f, 0.30f, 0.16f, 0.54f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.48f, 0.30f, 0.16f, 0.54f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.51f, 0.42f, 0.26f, 0.86f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.19f, 0.09f, 0.88f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.76f, 0.41f, 0.17f, 0.86f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.76f, 0.41f, 0.17f, 0.86f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.76f, 0.41f, 0.17f, 0.86f);
		colors[ImGuiCol_Button] = ImVec4(0.69f, 0.41f, 0.22f, 0.86f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.51f, 0.22f, 0.86f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.81f, 0.47f, 0.23f, 0.86f);
		colors[ImGuiCol_Header] = ImVec4(0.76f, 0.41f, 0.17f, 0.86f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.51f, 0.22f, 0.86f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.51f, 0.22f, 0.86f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.47f, 0.26f, 0.13f, 0.86f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.47f, 0.26f, 0.13f, 0.86f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_Tab] = ImVec4(0.28f, 0.18f, 0.11f, 0.97f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_TabActive] = ImVec4(0.47f, 0.26f, 0.13f, 0.86f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.10f, 0.07f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.47f, 0.26f, 0.13f, 0.86f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.17f, 0.10f, 0.08f, 0.94f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.58f, 0.33f, 0.18f, 0.86f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.FrameRounding = 4;
			style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
			style.WindowBorderSize = 1.0f;
			style.FrameBorderSize = 0.0f;
			style.PopupBorderSize = 1.0f;
		}

		break;
	}
}

void Config::defaultHotkeys()
{
	hotkeys.clear();
	std::ifstream in("data\\defaulthotkeys.json");
	if (!in.is_open())
	{
		char exePath[MAX_PATH] = {0};
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
		std::vector<std::filesystem::path> candidates = {
			exeDir,
			exeDir.parent_path(),
			exeDir.parent_path().parent_path(),
			exeDir.parent_path().parent_path().parent_path(),
			std::filesystem::current_path(),
		};
		bool found = false;
		for (auto& p : candidates)
		{
			if (p.empty()) continue;
			auto hotkeyPath = p / "data" / "defaulthotkeys.json";
			if (std::filesystem::exists(hotkeyPath))
			{
				std::error_code ec;
				std::filesystem::current_path(p, ec);
				if (!ec)
				{
					in.open("data\\defaulthotkeys.json");
					if (in.is_open()) { found = true; break; }
				}
			}
		}
		if (!found)
		{
			std::cout << "Warning, data folder is not found. Please use a proper browedit3 distribution, don't try to compile it yourself if you don't know what you're doing" << std::endl;
			MessageBox(NULL, "Warning, data folder is not found. Please use a proper browedit3 distribution, don't try to compile it yourself if you don't know what you're doing", "Warning, improper use detected", MB_OK);
			exit(0);
		}
	}
	json j;
	in >> j;

	HotkeyRegistry::defaultHotkeys = j.get< std::map<std::string, Hotkey>>();
	hotkeys = HotkeyRegistry::defaultHotkeys;
}