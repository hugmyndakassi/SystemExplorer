#pragma once

#include <WTLHelper.h>

//
// Default row text color for list/tree view custom draw. Returns CLR_INVALID
// (control/theme default) outside dark mode, so light-mode appearance is
// unchanged; in dark mode the control's own default text is unreliable for
// LVS_OWNERDATA views, so an explicit light color is used instead.
//
inline COLORREF GetDefaultTextColor() {
	return WTLHelper::IsDarkMode() ? RGB(255, 255, 255) : CLR_INVALID;
}

//
// Picks black or white text for good contrast against an arbitrary highlight
// background color, based on perceived luminance.
//
inline COLORREF GetContrastingTextColor(COLORREF bk) {
	auto luminance = (GetRValue(bk) * 299 + GetGValue(bk) * 587 + GetBValue(bk) * 114) / 1000;
	return luminance > 128 ? RGB(0, 0, 0) : RGB(255, 255, 255);
}
