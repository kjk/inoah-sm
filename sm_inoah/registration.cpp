#include "registration.h"
#include "BaseTypes.hpp"
#include "sm_inoah.h"
#include "iNoahSession.h"


// Given a handle of text edit window, return text of that window
// in txt
static void GetEditWinText(HWND hwndEdit, ArsLexis::String &txt)
{
    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);

    TCHAR *buf=new TCHAR[len+1];
    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
    txt.assign(buf);
    delete [] buf;
}

static void OnRegister(HWND hDlg)
{
    static ArsLexis::String regCode;

    ArsLexis::String text;

    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
    GetEditWinText(hwndEdit, regCode);

    g_session.registerNoah(regCode, text);
    iNoahSession::ResponseCode code = g_session.getLastResponseCode();
    switch (code)
    {
        case iNoahSession::serverMessage:
        {
            MessageBox(hDlg,text.c_str(),TEXT("Information"), 
                MB_OK|MB_ICONINFORMATION|MB_APPLMODAL|MB_SETFOREGROUND);
            EndDialog(hDlg, 1);
            break;
        }
        case iNoahSession::serverError:
        case iNoahSession::error:
        {
            MessageBox(hDlg,text.c_str(),TEXT("Error"), 
                MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
            break;
        }
    }
}

BOOL CALLBACK RegistrationDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return InitRegistrationDlg(hDlg);
        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;
                case IDM_REGISTER:
                    OnRegister(hDlg);
                    break;
            }
        }
    }
    return FALSE;
}

BOOL InitRegistrationDlg(HWND hDlg)
{
    SHINITDLGINFO shidi;
    ZeroMemory(&shidi, sizeof(shidi));
    shidi.dwMask   = SHIDIM_FLAGS;
    shidi.dwFlags  = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg     = hDlg;

    // Set up the menu bar
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

    //TODO: set existing reg code
    //SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)newRegCode_.c_str());
    SendMessage(hwndEdit, EM_SETINPUTMODE, 0, EIM_NUMBERS);

    SetFocus(hwndEdit);
    return TRUE;
}
