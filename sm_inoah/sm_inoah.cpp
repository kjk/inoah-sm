// sm_inoah.cpp : Defines the entry point for the application.
//

#include "resource.h"
#include "iNoahSession.h"
#include "iNoahParser.h"
#include "TAPIDevice.h"
#include "BaseTypes.hpp"
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
BOOL CALLBACK RecentLookupsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
void drawProgressInfo(HWND hwnd);

WNDPROC oldEditWndProc;

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

void setDefinition(ArsLexis::String& defs)
{
        delete definition_;
        ParagraphElement* parent=0;
        int start=0;
        iNoahParser parser;
        definition_=parser.parse(defs);
        ArsLexis::Graphics gr(GetDC(hwndMain));
        rec=true;
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
    LRESULT		lResult = TRUE;
	HDC			hdc;
	PAINTSTRUCT	ps;
	RECT		rect;
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
/*            RegisterHotKey(hwndEdit, 0x0100, MOD_WIN, VK_THOME);
            RegisterHotKey(hwndEdit, 0x0101, MOD_WIN, VK_TUP);
            RegisterHotKey(hwndEdit, 0x0102, MOD_WIN, VK_TDOWN);*/

            /*RegisterHotKey(hwndMain, 1, MOD_KEYUP, VK_UP);
            RegisterHotKey(hwndMain, 2, MOD_KEYUP, VK_NEXT);
            RegisterHotKey(hwndMain, 3, MOD_KEYUP, VK_PRIOR);
            RegisterHotKey(hwndMain, 4, MOD_KEYUP, VK_DOWN);*/
            //(void)SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TDOWN,
		    //    MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
		    //    SHMBOF_NODEFAULT | SHMBOF_NOTIFY));
			break;
		}
        case WM_SIZE:
            MoveWindow(hwndEdit,2,2,LOWORD(lp)-4,20,TRUE);
            MoveWindow(hwndScroll,LOWORD(lp)-5, 28 , 5, HIWORD(lp)-28, false);
            break;

        /*case WM_VSCROLL:
        {
            int page=definition_.shownLinesCount();
            ArsLexis::Graphics gr(GetDC(hwndMain));   
            
            switch(LOWORD(wp))
            {
                case SB_PAGEDOWN:
                    definition_.scroll(gr,*prefs,page);
                    break;
                case SB_LINEDOWN:
                    definition_.scroll(gr,*prefs,1);
                    break;
                case SB_PAGEUP:
                    definition_.scroll(gr,*prefs,-page);
                    break;
                case SB_LINEUP:
                    definition_.scroll(gr,*prefs,-1);
                    break;
            }
            break;
        }*/
        
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
                case ID_LOOKUP:
                {
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    SendMessage(hwndEdit, EM_SETSEL, 0,len);
                    ArsLexis::String word(buf);
                    drawProgressInfo(hwnd);
                    session.getWord(word,text);
                    iNoahSession::ResponseCode code=session.getLastResponseCode();
                    if( (code==iNoahSession::srvmessage)||
                        (code==iNoahSession::srverror)||
                        (code==iNoahSession::error))
                        MessageBox(hwnd,text.c_str(),TEXT("Error"), 
                        MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
                    else
                        setDefinition(text);
                    delete buf;
                    InvalidateRect(hwnd,NULL,TRUE);
                    break;
                }
                case IDM_MENU_RECENT:
                {
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RECENT), hwnd,RecentLookupsDlgProc);
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
			GetClientRect (hwnd, &rect);
            rect.top+=22;
            rect.left+=2;
            rect.right-=7;
            rect.bottom-=2;
            //RenderingPreferences* prefs= new RenderingPreferences();
			if (!definition_)
            {
                LOGFONT logfnt;
        
                HFONT fnt=(HFONT)GetStockObject(SYSTEM_FONT);
                GetObject(fnt, sizeof(logfnt), &logfnt);
                logfnt.lfHeight+=1;
                HFONT fnt2=(HFONT)CreateFontIndirect(&logfnt);
                SelectObject(hdc, fnt2);
                DrawText (hdc, TEXT("Enter word and press \"Look up\""), -1, &rect, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
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
                   LPWSTR     lpCmdLine,
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

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
        case WM_KEYDOWN:
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
    return CallWindowProc(oldEditWndProc, hwnd, msg, wp, lp);
}

BOOL CALLBACK RecentLookupsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            return InitRecentLookups(hDlg);
        
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
    shmbi.nToolBarId = IDR_RECENT_MENUBAR;
    shmbi.hInstRes = g_hInst;

    //if (!SHCreateMenuBar(&shmbi))
	//	return FALSE;
	// If we could not initialize the dialog box, return an error
	if (!SHInitDialog(&shidi))
		return FALSE;

    (void)SendMessage(shmbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK, 
			  MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, 
	            SHMBOF_NODEFAULT | SHMBOF_NOTIFY));

	return TRUE;
}

void drawProgressInfo(HWND hwnd)
{
	RECT		rect;
    HDC hdc=GetDC(hwnd);
    GetClientRect (hwnd, &rect);
    rect.top+=22;
    rect.left+=2;
    rect.right-=7;
    rect.bottom-=2;
    LOGFONT logfnt;
    
    Rectangle(hdc, 20, 85, 150, 121);
    
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
    DrawText (hdc, TEXT("definition..."), -1, &rect, DT_VCENTER|DT_CENTER);
    SelectObject(hdc,fnt);
    DeleteObject(fnt2);
    ReleaseDC(hwnd,hdc);
    
}
// end sm_inoah.cpp
