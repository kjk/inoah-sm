#include "registration.h"
#include "BaseTypes.hpp"
#include "sm_inoah.h"
#include "iNoahSession.h"

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
                {
                    ArsLexis::String text;
                    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    regCode.assign(buf);
                    session.registerNoah(regCode,text);
                    delete buf;
                    iNoahSession::ResponseCode code=session.getLastResponseCode();
                    switch(code)
                    {
                        case iNoahSession::srvmessage:
                        {
                            MessageBox(hDlg,text.c_str(),TEXT("Information"), 
                                MB_OK|MB_ICONINFORMATION|MB_APPLMODAL|MB_SETFOREGROUND);
                            EndDialog(hDlg, 1);
                            break;
                        }
                        case iNoahSession::srverror:
                        case iNoahSession::error:
                        {
                            MessageBox(hDlg,text.c_str(),TEXT("Error"), 
                                MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
                            break;
                        }
                    } 
                    break;

                }
            }
        }
    }
    return FALSE;
}

BOOL InitRegistrationDlg(HWND hDlg)
{
    SHINITDLGINFO shidi;
    ZeroMemory(&shidi, sizeof(shidi));
    shidi.dwMask = SHIDIM_FLAGS;
    shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
    shidi.hDlg = hDlg;

    // Set up the menu bar
    SHMENUBARINFO shmbi;
    ZeroMemory(&shmbi, sizeof(shmbi));
    shmbi.cbSize = sizeof(shmbi);
    shmbi.hwndParent = hDlg;
    shmbi.nToolBarId = IDR_REGISTER_MENUBAR;
    shmbi.hInstRes = g_hInst;

    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

    SetFocus(GetDlgItem(hDlg,IDC_EDIT_REGCODE));
    return TRUE;
}
