#include "registration.h"
#include "BaseTypes.hpp"
#include "sm_inoah.h"
#include "iNoahSession.h"

#define REGISTER_PRESSED 1
#define LATER_PRESSED    2

static void GetEditWinText(HWND hwnd, ArsLexis::String &txtOut)
{
    // 128 should be enough for anybody
    TCHAR buf[128];

    ZeroMemory(buf,sizeof(buf));

    int len = SendMessage(hwnd, EM_LINELENGTH, 0,0)+1;

    if (len < sizeof(buf)/sizeof(buf[0]) )
    {
        len = SendMessage(hwnd, WM_GETTEXT, len, (LPARAM)buf);
        txtOut.assign(buf);
    }
}

static void SetEditWinText(HWND hwnd, String& txt)
{
    if (!txt.empty())
    {
        SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)txt.c_str());
    }
}

static String g_oldRegCode;
static String g_newRegCode;

static BOOL OnInitDialog(HWND hDlg)
{
    SHINITDLGINFO shidi;
    ZeroMemory(&shidi, sizeof(shidi));
    shidi.dwMask   = SHIDIM_FLAGS;
    shidi.dwFlags  = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg     = hDlg;

    SHMENUBARINFO shmbi;
    ZeroMemory(&shmbi, sizeof(shmbi));
    shmbi.cbSize     = sizeof(shmbi);
    shmbi.hwndParent = hDlg;
    shmbi.nToolBarId = IDR_REGISTER_MENUBAR;
    shmbi.hInstRes   = g_hInst;

    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);

    SetEditWinText(hwndEdit, g_oldRegCode);
    SendMessage(hwndEdit, EM_SETINPUTMODE, 0, EIM_NUMBERS);

    SetFocus(hwndEdit);
    return TRUE;
}

static BOOL CALLBACK RegCodeDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    if (WM_INITDIALOG==msg)
    {
        return OnInitDialog(hDlg);
    }

    if (WM_COMMAND==msg)
    {
        assert( (IDM_LATER==wp) || (IDM_REGISTER==wp));

        if (IDM_LATER==wp)
        {
            EndDialog(hDlg, LATER_PRESSED);
            return TRUE;
        }
        else if (IDM_REGISTER==wp)
        {
            HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
            GetEditWinText(hwndEdit, g_newRegCode);
            EndDialog(hDlg, REGISTER_PRESSED);
            return TRUE;
        }
    }
    return FALSE;
}

bool FGetRegCodeFromUser(const String& currentRegCode, String& newRegCode)
{
    g_oldRegCode.assign(currentRegCode);

    int result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REGISTER), g_hwndMain, RegCodeDlgProc);

    if (REGISTER_PRESSED==result)
    {
        newRegCode.assign(g_newRegCode);
        return true;
    }
    else
        return false;
}

