// sm_inoah.cpp : Defines the entry point for the application.
//

#include "resource.h"
#include "iNoahSession.h"
#include "iNoahParser.h"
#include "TAPIDevice.h"
#include <BaseTypes.hpp>
#include "GenericTextElement.hpp"
#include "BulletElement.hpp"
#include "ParagraphElement.hpp"
#include "HorizontalLineElement.hpp"
#include "Definition.hpp"
#include <connmgr.h>
#include <windows.h>
#include <tpcshell.h>
#include <wingdi.h>
#include <fonteffects.hpp>

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK ListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL CALLBACK RecentLookupsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
BOOL CALLBACK RegistrationDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
void drawProgressInfo(HWND hwnd, TCHAR* text);
void setFontSize(int diff,HWND hwnd);
void paint(HWND hwnd, HDC hdc);
WNDPROC oldEditWndProc;
WNDPROC oldListWndProc;

HWND hRecentLookupaDlg=NULL;
HINSTANCE g_hInst = NULL;  // Local copy of hInstance
HWND hwndMain = NULL;    // Handle to Main window returned from CreateWindow
HWND hwndScroll;
static bool g_forceLayoutRecalculation=false;

TCHAR szAppName[] = TEXT("iNoah");
TCHAR szTitle[]   = TEXT("iNoah");
Definition *definition_ = NULL;

RenderingPreferences* prefs= new RenderingPreferences();
iNoahSession session;
bool rec=false;

BOOL InitRecentLookups(HWND hDlg);
BOOL InitRegistrationDlg(HWND hDlg);

ArsLexis::String wordList;
ArsLexis::String recentWord;
ArsLexis::String regCode;

bool initializeConnection()
{
    // Establish a synchronous connection
    HANDLE hConnection = NULL;
    DWORD dwStatus = 0;
    DWORD dwTimeout = 5000;
    
    // Get the network information where we want to establish a
    // connection
    TCHAR tchRemoteUrl[256] = TEXT("\0");
    wsprintf(tchRemoteUrl,
        TEXT("http://arslex.no-ip.info"));
    GUID guidNetworkObject;
    DWORD dwIndex = 0;
    
    if(ConnMgrMapURL(tchRemoteUrl, &guidNetworkObject, &dwIndex)
        == E_FAIL) 
        /*MessageBox(
        NULL,
        TEXT("Could not map the request to a network identifier."),
        TEXT("Error"),
        MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);*/
        return false;
    
    // Now that we've got the network address, set up the
    // connection structure
    CONNMGR_CONNECTIONINFO ccInfo;
    
    memset(&ccInfo, 0, sizeof(CONNMGR_CONNECTIONINFO));
    ccInfo.cbSize = sizeof(CONNMGR_CONNECTIONINFO);
    ccInfo.dwParams = CONNMGR_PARAM_GUIDDESTNET;
    ccInfo.dwFlags = CONNMGR_FLAG_PROXY_HTTP;
    ccInfo.dwPriority = CONNMGR_PRIORITY_USERINTERACTIVE;
    ccInfo.guidDestNet = guidNetworkObject;
    
    // Make the connection request (timeout in 5 seconds)
    if(ConnMgrEstablishConnectionSync(&ccInfo, &hConnection,
        dwTimeout, &dwStatus) == E_FAIL) 
        return false;
    return true;
}

void setScrollBar(Definition* definition_)
{
    int frst=0;
    int total=0;
    int shown=0;
    if(definition_)
    {
        frst=definition_->firstShownLine();
        total=definition_->totalLinesCount();
        shown=definition_->shownLinesCount();
    }
    
    SetScrollPos(
        hwndScroll, 
        SB_CTL,
        frst,
        TRUE);
    
    SetScrollRange(
        hwndScroll,
        SB_CTL,
        0,
        total-shown,
        TRUE);
}

void setDefinition(ArsLexis::String& defs, HWND hwnd)
{
    iNoahSession::ResponseCode code=session.getLastResponseCode();
    switch(code)
    {
        case iNoahSession::srvmessage:
        {
            MessageBox(hwnd,defs.c_str(),TEXT("Information"), 
            MB_OK|MB_ICONINFORMATION|MB_APPLMODAL|MB_SETFOREGROUND);
            return;
        }
        case iNoahSession::srverror:
        case iNoahSession::error:
        {
            MessageBox(hwnd,defs.c_str(),TEXT("Error"), 
            MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
            return;
        }
        default:
        {
            delete definition_;
            ParagraphElement* parent=0;
            int start=0;
            iNoahParser parser;
            definition_=parser.parse(defs);
            ArsLexis::Graphics gr(GetDC(hwndMain));
            rec=true;
            InvalidateRect(hwnd,NULL,TRUE);
        }
    }
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT     lResult = TRUE;
    HDC         hdc;
    PAINTSTRUCT ps;
    static HWND hwndEdit;

    static bool compactView=FALSE;
    static ArsLexis::String text=TEXT("");
    
    switch(msg)
    {
        case WM_CREATE:
        {
            //if (
            if (!initializeConnection())
            {
                MessageBox(
                    hwnd,
                    TEXT("Can't establish connection."),
                    TEXT("Error"),
                    MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND
                    );
                //SetFocus(hwndEdit);
                //PostQuitMessage(0);
                //break;
            }
            //)break;
            //initialize();
            // create the menu bar
            SHMENUBARINFO mbi;
            ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
            mbi.cbSize = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hwnd;
            mbi.nToolBarId = IDR_HELLO_MENUBAR;
            mbi.hInstRes = g_hInst;
            
            //if (tr.sendRequest()==NO_ERROR);
            //	tr.getResponse();
            
            if (!SHCreateMenuBar(&mbi)) 
            {
                PostQuitMessage(0);
                break;
            }
            
            hwndEdit = CreateWindow(
                TEXT("edit"),
                NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | 
                WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                0,0,0,0,hwnd,
                (HMENU) ID_EDIT,
                ((LPCREATESTRUCT)lp)->hInstance,
                NULL);
            oldEditWndProc=(WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);
            hwndScroll = CreateWindow(
                TEXT("scrollbar"),
                NULL,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
                0,0,0,0,hwnd,
                (HMENU) ID_SCROLL,
                ((LPCREATESTRUCT)lp)->hInstance,
                NULL);
            setScrollBar(definition_);
            // In order to make Back work properly, it's necessary to 
            // override it and then call the appropriate SH API
            (void)SendMessage(
                mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
                MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
                );

            setFontSize(IDM_FNT_STANDARD, hwnd);
            break;
        }
        case WM_SIZE:
            MoveWindow(hwndEdit,2,2,LOWORD(lp)-4,20,TRUE);
            MoveWindow(hwndScroll,LOWORD(lp)-5, 28 , 5, HIWORD(lp)-28, false);
            break;
        
        case WM_SETFOCUS:
            SetFocus(hwndEdit);
            break;
        
        case WM_COMMAND:
        {
            switch (wp)
            {
                case IDOK:
                    SendMessage(hwnd,WM_CLOSE,0,0);
                    break;
                case IDM_MENU_COMPACT:
                {
                    HWND hwndMB = SHFindMenuBar (hwnd);
                    if (hwndMB) 
                    {
                        HMENU hMenu;
                        hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
                        compactView=!compactView;
                        if(compactView)
                        {
                            CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_CHECKED | MF_BYCOMMAND);
                            prefs->setCompactView();
                        }
                        else
                        {
                            CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_UNCHECKED | MF_BYCOMMAND);
                            prefs->setClassicView();
                        }
                        g_forceLayoutRecalculation=true;
                        rec=true;
                        InvalidateRect(hwnd,NULL,TRUE);
                    }
                    break;
                }
    
                case IDM_FNT_LARGE:
                    setFontSize(IDM_FNT_LARGE, hwnd);
                    break;
                case IDM_FNT_STANDARD:
                    setFontSize(IDM_FNT_STANDARD, hwnd);
                    break;
                case IDM_FNT_SMALL:
                    setFontSize(IDM_FNT_SMALL, hwnd);
                    break;
                case ID_LOOKUP:
                {
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    SendMessage(hwndEdit, EM_SETSEL, 0,len);
                    ArsLexis::String word(buf); 
                    drawProgressInfo(hwnd, TEXT("definition..."));
                    session.getWord(word,text);
                    setDefinition(text,hwnd);
                    delete buf;
                    InvalidateRect(hwnd,NULL,TRUE);
                    break;
                }
                
                case IDM_MENU_RANDOM:
                {         
                    ArsLexis::String word;
                    HDC hdc = GetDC(hwnd);
                    paint(hwnd, hdc);
                    ReleaseDC(hwnd, hdc);
                    drawProgressInfo(hwnd, TEXT("random definition..."));
                    session.getRandomWord(word);
                    setDefinition(word,hwnd);
                    break;
                }
                case IDM_MENU_RECENT:
                {
                    wordList.assign(TEXT(""));
                    HDC hdc = GetDC(hwnd);
                    paint(hwnd, hdc);
                    ReleaseDC(hwnd, hdc);
                    drawProgressInfo(hwnd, TEXT("recent lookups list..."));
                    session.getWordList(wordList);
                    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RECENT), hwnd,RecentLookupsDlgProc))
                    {                        
                        drawProgressInfo(hwnd, TEXT("definition..."));
                        session.getWord(recentWord,text);
                        setDefinition(text,hwnd);
                    }
                    break;
                }
                case IDM_MENU_REGISTER:
                {
                    if(DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REGISTER), hwnd,RegistrationDlgProc))
                    {
                        session.registerNoah(regCode,text);
                        setDefinition(text,hwnd);
                    }
                    break;
                }
                default:
                    return DefWindowProc(hwnd, msg, wp, lp);
            }
            break;
        }
        
        case WM_HOTKEY:
        {
            ArsLexis::Graphics gr(GetDC(hwndMain));
            int page=0;
            if (definition_)
                page=definition_->shownLinesCount();
            switch(HIWORD(lp))
            {
                case VK_TBACK:
                    if ( 0 != (MOD_KEYUP & LOWORD(lp)))
                        SHSendBackToFocusWindow( msg, wp, lp );
                    break;
                case VK_TDOWN:
                    if(definition_)
                        definition_->scroll(gr,*prefs,page);
                    setScrollBar(definition_);
                    break;
            }
            break;
        }    
        
        case WM_PAINT:
        {
            hdc = BeginPaint (hwnd, &ps);
            paint(hwnd, hdc);
            EndPaint (hwnd, &ps);
        }		
        break;
        
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            lResult = DefWindowProc(hwnd, msg, wp, lp);
            break;
    }
    return (lResult);
}


//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
BOOL InitInstance (HINSTANCE hInstance, int CmdShow )
{
    
    g_hInst = hInstance;
    hwndMain = CreateWindow(szAppName,						
        szTitle,
        WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL );
    
    if ( !hwndMain )		
    {
        return FALSE;
    }
    ShowWindow(hwndMain, CmdShow );
    UpdateWindow(hwndMain);
    TAPIDevice::initInstance();    
    return TRUE;
}

//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Sets the properties for our window.
//
BOOL InitApplication ( HINSTANCE hInstance )
{
    WNDCLASS wc;
    BOOL f;
    
    wc.style = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.hInstance = hInstance;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szAppName;
    
    f = (RegisterClass(&wc));
    
    return f;
}


//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPWSTR, int)
//
//  PURPOSE: Entry point for the application
//
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR    lpCmdLine,
                   int        CmdShow)
                   
{
    MSG msg;
    HWND hHelloWnd = NULL;	
    
    //Check if Hello.exe is running. If it's running then focus on the window
    hHelloWnd = FindWindow(szAppName, szTitle);	
    if (hHelloWnd) 
    {
        SetForegroundWindow (hHelloWnd);    
        return 0;
    }
    
    if ( !hPrevInstance )
    {
        if ( !InitApplication ( hInstance ) )
        { 
            return (FALSE); 
        }
    }
    if ( !InitInstance( hInstance, CmdShow )  )
    {
        return (FALSE);
    }
    
    while ( GetMessage( &msg, NULL, 0,0 ) == TRUE )
    {
        TranslateMessage (&msg);
        DispatchMessage(&msg);
    }
    return (msg.wParam);
}

LRESULT CALLBACK ListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        //What the hell is constatnt - I have no idea VK_F24 ??
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

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            if(definition_)
            {
                int page=0;
                switch (wp) 
                {
                    case VK_DOWN:
                        page=definition_->shownLinesCount();
                        break;
                    case VK_UP:
                        page=-static_cast<int>(definition_->shownLinesCount());
                        break;
                }
                if (0!=page)
                {
                    RECT b;
                    GetClientRect (hwndMain, &b);
                    ArsLexis::Rectangle bounds=b;
                    ArsLexis::Rectangle defRect=bounds;
                    defRect.explode(2, 22, -9, -24);
                    ArsLexis::Graphics gr(GetDC(hwndMain));
                    bool doubleBuffer=true;
                    
                    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
                    if (offscreenDc) {
                        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
                        if (bitmap) {
                            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
                            {
                                ArsLexis::Graphics offscreen(offscreenDc);
                                definition_->scroll(offscreen,*prefs, page);
                                offscreen.copyArea(defRect, gr, defRect.topLeft);
                            }
                            ::SelectObject(offscreenDc, oldBitmap);
                            ::DeleteObject(bitmap);
                        }
                        else
                            doubleBuffer=false;
                        ::DeleteDC(offscreenDc);
                    }
                    else
                        doubleBuffer=false;
                    if (!doubleBuffer)
                        definition_->scroll(gr,*prefs, page);
                    
                    setScrollBar(definition_);
                    return 0;
                }
            }
            break;
       }
    }
    return CallWindowProc(oldEditWndProc, hwnd, msg, wp, lp);
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
                {
                    HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT_REGCODE);
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    SendMessage(hwndEdit, EM_SETSEL, 0,len);
                    regCode.assign(buf);
                    delete buf;
                    EndDialog(hDlg, 1);
                    break;

                }
            }
        }
    }
    return FALSE;
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

void drawProgressInfo(HWND hwnd, TCHAR* text)
{
    RECT rect;
    HDC hdc=GetDC(hwnd);
    GetClientRect (hwnd, &rect);
    rect.top+=22;
    rect.left+=2;
    rect.right-=7;
    rect.bottom-=2;
    LOGFONT logfnt;
    
    Rectangle(hdc, 18, 83, 152, 123);
    
    POINT p[2];
    p[0].y=85;
    p[1].y=121;
    LOGPEN pen;
    HGDIOBJ hgdiobj = GetCurrentObject(hdc,OBJ_PEN);
    GetObject(hgdiobj, sizeof(pen), &pen);
    for(p[0].x=20;p[0].x<150;p[0].x++)
    {                           
        HPEN newPen=CreatePenIndirect(&pen);
        pen.lopnColor = RGB(0,0,p[0].x+100);
        SelectObject(hdc,newPen);
        DeleteObject(hgdiobj);
        hgdiobj=newPen;
        p[1].x=p[0].x;
        Polyline(hdc, p, 2);
    }
    DeleteObject(hgdiobj);
    
    SelectObject(hdc,GetStockObject(HOLLOW_BRUSH));
    HFONT fnt=(HFONT)GetStockObject(SYSTEM_FONT);
    GetObject(fnt, sizeof(logfnt), &logfnt);
    logfnt.lfHeight+=1;
    logfnt.lfWeight=800;
    SetTextColor(hdc,RGB(255,255,255));
    SetBkMode(hdc, TRANSPARENT);
    HFONT fnt2=(HFONT)CreateFontIndirect(&logfnt);
    SelectObject(hdc, fnt2);
    rect.top-=10;
    DrawText (hdc, TEXT("Downloading"), -1, &rect, DT_VCENTER|DT_CENTER);
    rect.top+=32;
    DrawText (hdc, text, -1, &rect, DT_VCENTER|DT_CENTER);
    SelectObject(hdc,fnt);
    DeleteObject(fnt2);
    ReleaseDC(hwnd,hdc);
}

void setFontSize(int diff, HWND hwnd)
{
    int delta=0;
    HWND hwndMB = SHFindMenuBar (hwnd);
    if (hwndMB) 
    {
        HMENU hMenu;
        hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
        CheckMenuItem(hMenu, IDM_FNT_LARGE, MF_UNCHECKED | MF_BYCOMMAND);
        CheckMenuItem(hMenu, IDM_FNT_SMALL, MF_UNCHECKED | MF_BYCOMMAND);
        CheckMenuItem(hMenu, IDM_FNT_STANDARD, MF_UNCHECKED | MF_BYCOMMAND);
        switch(diff)
        {
            case IDM_FNT_LARGE:
                CheckMenuItem(hMenu, IDM_FNT_LARGE, MF_CHECKED | MF_BYCOMMAND);
                delta=-2;
                break;
            case IDM_FNT_STANDARD:
                CheckMenuItem(hMenu, IDM_FNT_STANDARD, MF_CHECKED | MF_BYCOMMAND);
                break;
            case IDM_FNT_SMALL:
                CheckMenuItem(hMenu, IDM_FNT_SMALL, MF_CHECKED | MF_BYCOMMAND);
                delta=2;
                break;
        }
    }
    g_forceLayoutRecalculation=true;
    prefs->setFontSize(delta);
    InvalidateRect(hwnd,NULL,TRUE);
}

void paint(HWND hwnd, HDC hdc)
{
    RECT rect;
    GetClientRect (hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    rect.top+=22;
    rect.left+=2;
    rect.right-=7;
    rect.bottom-=2;
    if (!definition_)
    {
        LOGFONT logfnt;
        
        HFONT fnt=(HFONT)GetStockObject(SYSTEM_FONT);
        GetObject(fnt, sizeof(logfnt), &logfnt);
        logfnt.lfHeight+=1;
        HFONT fnt2=(HFONT)CreateFontIndirect(&logfnt);
        SelectObject(hdc, fnt2);
        DrawText (hdc, TEXT("Enter word and press \"Lookup\""), -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
        SelectObject(hdc,fnt);
        DeleteObject(fnt2);
    }
    else
    {
        ArsLexis::Graphics gr(hdc);
        RECT b;
        GetClientRect(hwnd, &b);
        ArsLexis::Rectangle bounds=b;
        ArsLexis::Rectangle defRect=rect;
        bool doubleBuffer=true;
        HDC offscreenDc=::CreateCompatibleDC(hdc);
        if (offscreenDc) {
            HBITMAP bitmap=::CreateCompatibleBitmap(hdc, bounds.width(), bounds.height());
            if (bitmap) {
                HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
                {
                    ArsLexis::Graphics offscreen(offscreenDc);
                    definition_->render(offscreen, defRect, *prefs, g_forceLayoutRecalculation);
                    offscreen.copyArea(defRect, gr, defRect.topLeft);
                }
                ::SelectObject(offscreenDc, oldBitmap);
                ::DeleteObject(bitmap);
            }
            else
                doubleBuffer=false;
            ::DeleteDC(offscreenDc);
        }
        else
            doubleBuffer=false;
        if (!doubleBuffer)
            definition_->render(gr, defRect, *prefs, g_forceLayoutRecalculation);
        g_forceLayoutRecalculation=false;
    }
    if(rec)
    {
        setScrollBar(definition_);
        rec=false;
    }
}
// end sm_inoah.cpp
