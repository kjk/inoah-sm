// sm_inoah.cpp : Defines the entry point for the application.
//

#include "resource.h"
#include "iNoahSession.h"
#include "TAPIDevice.h"
#include "BaseTypes.hpp"
#include "GenericTextElement.hpp"
#include "BulletElement.hpp"
#include "ParagraphElement.hpp"
#include "HorizontalLineElement.hpp"
#include "Definition.hpp"
#include <windows.h>
#include <tpcshell.h>

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

WNDPROC oldEditWndProc;

HINSTANCE g_hInst = NULL;  // Local copy of hInstance
HWND hwndMain = NULL;    // Handle to Main window returned from CreateWindow
HWND hwndScroll;

TCHAR szAppName[] = TEXT("iNoah");
TCHAR szTitle[]   = TEXT("iNoah");
Definition *definition_ = new Definition();

RenderingPreferences* prefs= new RenderingPreferences();
iNoahSession session;

void initialize()
{
        definition_->clear();
        ParagraphElement* parent=0;
        definition_->appendElement(parent=new ParagraphElement());
        DefinitionElement* element=0;
        definition_->appendElement(element=new GenericTextElement(
            TEXT("Enter word and press lookup")
         ));
        element->setParent(parent);
}

void setDefinition(ArsLexis::String& defs)
{
        delete definition_;
        definition_=new Definition();
        ParagraphElement* parent=0;
        //parent->setChildIndentation(16);
        int start=0;
        for(int i=0;i<defs.length(); i++)
        {
            if(defs[i]==TCHAR('\n'))
            {
                definition_->appendElement(parent=new ParagraphElement());
                DefinitionElement* element=0;
                ArsLexis::String txt=defs.substr(start,i-start);
                definition_->appendElement(element=new GenericTextElement(txt));
                start=i+1;
                element->setParent(parent);
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
			initialize();
            // create the menu bar
			SHMENUBARINFO mbi;
			ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
			mbi.cbSize = sizeof(SHMENUBARINFO);
			mbi.hwndParent = hwnd;
			mbi.nToolBarId = IDR_HELLO_MENUBAR;
			mbi.hInstRes = g_hInst;

			//if (tr.sendRequest()==NO_ERROR);
			//	tr.getResponse();
			
			if (!SHCreateMenuBar(&mbi)) {
				PostQuitMessage(0);
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
            
            SetScrollPos(
                hwndScroll, 
                SB_CTL, 
                definition_->firstShownLine(),
                TRUE);
            SetScrollRange(
                hwndScroll, 
                SB_CTL, 
                0,
                definition_->totalLinesCount()-
                definition_->shownLinesCount(), 
                TRUE);

            // In order to make Back work properly, it's necessary to 
	        // override it and then call the appropriate SH API
	        (void)SendMessage(
                mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
		        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
		        SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
                );
            RegisterHotKey(hwndEdit, 0x0100, MOD_WIN, VK_THOME);
            RegisterHotKey(hwndEdit, 0x0101, MOD_WIN, VK_TUP);
            RegisterHotKey(hwndEdit, 0x0102, MOD_WIN, VK_TDOWN);

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
    				       CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_CHECKED | MF_BYCOMMAND);
                       else
                           CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_UNCHECKED | MF_BYCOMMAND);
                    }
				    break;
                }
                case ID_LOOKUP:
                {
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    ArsLexis::String word(buf);
        			session.getWord(word,text);
                    setDefinition(text);
                    delete buf;
                    InvalidateRect(hwnd,NULL,TRUE);
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
            int page=definition_->shownLinesCount();
            switch(HIWORD(lp))
            {
                case VK_TBACK:
                    if ( 0 != (MOD_KEYUP & LOWORD(lp)))
                        SHSendBackToFocusWindow( msg, wp, lp );
                    break;
                case VK_TDOWN:
                    definition_->scroll(gr,*prefs,page);
                    int page=definition_->shownLinesCount();
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
            ArsLexis::Graphics gr(hdc);   
            //RenderingPreferences* prefs= new RenderingPreferences();
			//DrawText (hdc, text.c_str(), -1, &rect, DT_LEFT);            
            definition_->render(gr, rect, *prefs, true);
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
        {
            int page=definition_->shownLinesCount();
            ArsLexis::Graphics gr(GetDC(hwndMain));

            switch(wp)
            {
                case VK_DOWN:
                    definition_->scroll(gr,*prefs,page);
                    SetScrollPos(
                        hwndScroll, 
                        SB_CTL, 
                        definition_->firstShownLine(),
                        FALSE);
                    SetScrollRange(
                        hwndScroll, 
                        SB_CTL, 
                        0,
                        definition_->totalLinesCount()-
                        definition_->shownLinesCount(), 
                        TRUE);
                    return 0;
                case VK_UP:
                    definition_->scroll(gr,*prefs,-page);
                    SetScrollPos(
                        hwndScroll, 
                        SB_CTL, 
                        definition_->firstShownLine(),
                        FALSE);
                    SetScrollRange(
                        hwndScroll, 
                        SB_CTL, 
                        0,
                        definition_->totalLinesCount()-
                        definition_->shownLinesCount(), 
                        TRUE);
                    return 0;
            }
            break;
        }
    }
    return CallWindowProc(oldEditWndProc, hwnd, msg, wp, lp);
}
// end sm_inoah.cpp
