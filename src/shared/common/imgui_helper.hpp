#pragma once

namespace ImGui
{
	void Spacing(const float& x, const float& y);
	void SetItemTooltipWrapper(const char* fmt, ...);

	void CenterText(const char* text, bool disabled = false);
	void AddUnterline(ImColor col);
	void TextURL(const char* name, const char* url, bool use_are_you_sure_popup = false);
	void SetCursorForCenteredText(const char* text);
}
