#include "reclookups.h"
#include "sm_inoah.h"

WNDPROC oldListWndProc;
HWND hRecentLookupaDlg=NULL;

LRESULT CALLBACK ListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        //What the hell is constatnt - any idea VK_F24 ??
        case 0x87: 
        {
            switch(wp)
            {
                case 0x32:
                    SendMessage(hRecentLookupaDlg, WM_COMMAND, ID_SELECT, 0);
                    break;
            }
        }
    }
    return CallWindowProc(oldListWndProc, hwnd, msg, wp, lp);
}

BOOL CALLBACK RecentLookupsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return InitRecentLookups(hDlg);
        case WM_COMMAND:
        {
            switch (wp)
            {
                case ID_CANCEL:
                    EndDialog(hDlg, 0);
                    break;
                case ID_SELECT:
                {
                    HWND ctrl = GetDlgItem(hDlg, IDC_LIST_RECENT);
                    int idx = SendMessage(ctrl, LB_GETCURSEL, 0, 0);
                    int len = SendMessage(ctrl, LB_GETTEXTLEN, idx, 0);
                    TCHAR *buf = new TCHAR[len+1];
                    SendMessage(ctrl, LB_GETTEXT, idx, (LPARAM) buf);
                    recentWord.assign(buf);
                    delete buf;
                    EndDialog(hDlg, 1);
                    break;
                }
            }
        }
    }
    return FALSE;
}

BOOL InitRecentLookups(HWND hDlg)
{
    // Specify that the dialog box should stretch full screen
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
    shmbi.nToolBarId = IDR_RECENT_MENUBAR ;
    shmbi.hInstRes = g_hInst;
    
    // If we could not initialize the dialog box, return an error
    if (!SHInitDialog(&shidi))
        return FALSE;
    
    if (!SHCreateMenuBar(&shmbi))
        return FALSE;
    
    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
    
    hRecentLookupaDlg=hDlg;

    HWND ctrl=GetDlgItem(hDlg, IDC_LIST_RECENT);
    oldListWndProc=(WNDPROC)SetWindowLong(ctrl, GWL_WNDPROC, (LONG)ListWndProc);
    RegisterHotKey( ctrl, 50, 0, VK_TACTION);

    int last=wordList.find_first_of(TCHAR('\n'));
    int first=0;
    while((last!=-1)&&(last!=wordList.length()))
    {
        ArsLexis::String tmp=wordList.substr(first,last-first);
        if (tmp.compare(TEXT(""))!=0)
        {
            TCHAR *str  =new TCHAR [tmp.length()+1];
            wcscpy(str,tmp.c_str());
            SendMessage(
                ctrl,
                LB_ADDSTRING,
                0,
                (LPARAM)str);
        }
        first=last+1;
        last=wordList.find_first_of(TCHAR('\n'),first);
    }
    SendMessage (ctrl, LB_SETCURSEL, 0, 0);
    //UpdateWindow(ctrl);
    return TRUE;
}
