#include "pch.h"
#include "ColorsSelectionDlg.h"
#include "DialogHelper.h"
#include "Settings.h"

static WCHAR iniFileFilter[] = L"Ini files (*.ini)\0*.ini\0All Files\0*.*\0";

CColorsSelectionDlg::CColorsSelectionDlg(HighlightColor* colors, int count) : m_Colors(colors, colors + count), m_CountColors(count) {
    ATLASSERT(colors);
}

const HighlightColor* CColorsSelectionDlg::GetColors() const {
    return m_Colors.data();
}

// These "checkboxes" are entirely custom-painted color swatches (background =
// the highlight color, label = its name). NM_CUSTOMDRAW erase-stage
// notifications for plain (non-push-like) BS_AUTOCHECKBOX buttons turned out
// to be unreliable - not just under dark mode, but in general - so painting
// is done the guaranteed-reliable way: BS_OWNERDRAW + WM_DRAWITEM. That means
// we also draw the checkbox glyph itself and manage its checked state
// ourselves (see OnToggleEnabled) instead of relying on BM_GETCHECK/SETCHECK.
LRESULT CColorsSelectionDlg::OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
    auto dis = (LPDRAWITEMSTRUCT)lParam;
    auto id = dis->CtlID;
    if (id < IDC_ENABLED || id >= IDC_ENABLED + m_CountColors) {
        bHandled = FALSE;
        return 0;
    }

    UINT i = id - IDC_ENABLED;
    CDCHandle dc(dis->hDC);
    CRect rcItem(dis->rcItem);

    dc.FillSolidRect(&rcItem, ::GetSysColor(COLOR_BTNFACE));

    CRect rcCheck(rcItem.left, rcItem.CenterPoint().y - 8, rcItem.left + 16, rcItem.CenterPoint().y + 8);
    UINT state = DFCS_BUTTONCHECK;
    if (m_Colors[i].Enabled)
        state |= DFCS_CHECKED;
    dc.DrawFrameControl(&rcCheck, DFC_BUTTON, state);

    CRect rc(rcItem);
    rc.InflateRect(-20, 10, -10, 10);
    dc.FillSolidRect(&rc, m_Colors[i].Color);
    dc.SetTextColor(m_Colors[i].TextColor);
    dc.SetBkMode(TRANSPARENT);
    dc.DrawText(m_Colors[i].Name, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

    if (dis->itemState & ODS_FOCUS)
        dc.DrawFocusRect(&rcItem);

    return TRUE;
}

COLORREF CColorsSelectionDlg::SelectColor(COLORREF initial) {
    CColorDialog dlg(initial, CC_FULLOPEN, *this);
    if (dlg.DoModal() == IDOK)
        initial = dlg.GetColor();

    return initial;
}

LRESULT CColorsSelectionDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    DialogHelper::AdjustOKCancelButtons(this);
    DialogHelper::SetDialogIcon(this, IDI_COLORWHEEL);
    SetWindowText(m_Title);

    CButton cbMain(GetDlgItem(IDC_ENABLED));
    CRect rc;
    cbMain.GetWindowRect(&rc);
    ScreenToClient(&rc);
    auto cbStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_CLIPSIBLINGS | WS_TABSTOP;
    cbMain.DestroyWindow();

    CButton changeButton(GetDlgItem(IDC_CHANGE));
    CRect rcChange;
    changeButton.GetWindowRect(&rcChange);
    ScreenToClient(&rcChange);
    changeButton.DestroyWindow();

    for (UINT i = 0; i < m_CountColors; i++) {
        CButton cb;
        cb.Create(*this, &rc, L"", cbStyle, 0, IDC_ENABLED + i);
        cb.SetFont(GetFont());
        rc.OffsetRect(0, rc.Height() + 12);

        CButton split;
        split.Create(*this, rcChange, L"Change", WS_VISIBLE | WS_CHILD | BS_SPLITBUTTON | WS_CLIPSIBLINGS, 0, IDC_CHANGE + i);
        split.SetFont(GetFont());
        rcChange.OffsetRect(0, rc.Height() + 12);
    }
    return 0;
}

LRESULT CColorsSelectionDlg::OnButtonColor(UINT, WPARAM, LPARAM, BOOL&) {
    return LRESULT(::GetSysColorBrush(BLACK_BRUSH));
}

LRESULT CColorsSelectionDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
    // m_Colors[i].Enabled is kept live-updated by OnToggleEnabled (matching how
    // color changes are already applied immediately, with no cancel/rollback).
    EndDialog(wID);
    return 0;
}

LRESULT CColorsSelectionDlg::OnToggleEnabled(WORD, WORD wID, HWND hWndCtl, BOOL&) {
    UINT i = wID - IDC_ENABLED;
    m_Colors[i].Enabled = !m_Colors[i].Enabled;
    ::InvalidateRect(hWndCtl, nullptr, TRUE);
    return 0;
}

LRESULT CColorsSelectionDlg::OnChangeColor(WORD, WORD wID, HWND hWnd, BOOL&) {
    DoChangeColors(hWnd);

    return 0;
}

LRESULT CColorsSelectionDlg::OnChangeDropdown(int, LPNMHDR hdr, BOOL&) {
    DoChangeColors(hdr->hwndFrom);

    return 0;
}

void CColorsSelectionDlg::DoChangeColors(HWND button) {
    CRect rc;
    ::GetWindowRect(button, &rc);

    CMenu menu;
    menu.LoadMenu(IDR_SPLIT);
    m_CurrentSelection = CButton(button).GetDlgCtrlID() - IDC_CHANGE;
    TrackPopupMenu(menu.GetSubMenu(0), 0, rc.left, rc.bottom, 0, *this, nullptr);
}

LRESULT CColorsSelectionDlg::OnChangeBackground(WORD, WORD wID, HWND, BOOL&) {
    UINT index = m_CurrentSelection;
    ATLASSERT(index >= 0 && index < m_CountColors);
    m_Colors[index].Color = SelectColor(m_Colors[index].Color);
    GetDlgItem(IDC_ENABLED + index).RedrawWindow();

    return 0;
}

LRESULT CColorsSelectionDlg::OnChangeForeground(WORD, WORD wID, HWND, BOOL&) {
    UINT index = m_CurrentSelection;
    ATLASSERT(index < m_CountColors);
    m_Colors[index].TextColor = SelectColor(m_Colors[index].TextColor);
    GetDlgItem(IDC_ENABLED + index).RedrawWindow();

    return 0;
}

LRESULT CColorsSelectionDlg::OnChangeToDefault(WORD, WORD wID, HWND, BOOL&) {
    UINT index = m_CurrentSelection;
    ATLASSERT(index < m_CountColors);
    m_Colors[index].Color = m_Colors[index].DefaultColor;
    m_Colors[index].TextColor = m_Colors[index].DefaultTextColor;
    GetDlgItem(IDC_ENABLED + index).RedrawWindow();

    return 0;
}

LRESULT CColorsSelectionDlg::OnResetColors(WORD, WORD wID, HWND, BOOL&) {
    for (UINT index = 0; index < m_CountColors; index++) {
        m_Colors[index].Color = m_Colors[index].DefaultColor;
        m_Colors[index].TextColor = m_Colors[index].DefaultTextColor;
        GetDlgItem(IDC_ENABLED + index).RedrawWindow();
    }
    return 0;
}

LRESULT CColorsSelectionDlg::OnSave(WORD, WORD wID, HWND, BOOL&) {
    CSimpleFileDialog dlg(FALSE, L"ini", nullptr, OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT, iniFileFilter, *this);
    if (dlg.DoModal() == IDOK) {
        Settings::SaveColors(dlg.m_szFileName, L"ProcessColor", m_Colors.data(), m_CountColors);
    }
    return 0;
}

LRESULT CColorsSelectionDlg::OnLoad(WORD, WORD wID, HWND, BOOL&) {
    CSimpleFileDialog dlg(TRUE, L"ini", nullptr, OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST, iniFileFilter, *this);
    if (dlg.DoModal() == IDOK) {
        if (Settings::LoadColors(dlg.m_szFileName, L"ProcessColor", m_Colors.data(), m_CountColors))
            RedrawWindow();
    }
    return 0;
}
