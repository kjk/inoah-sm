// sm_inoah.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <winuser.h>
#include <aygshell.h>
#include "resource.h"
#include "iNoahSession.h"
#include "..\ipedia\src\BaseTypes.hpp"


HINSTANCE g_hInst = NULL;  // Local copy of hInstance
HWND hwndMain = NULL;    // Handle to Main window returned from CreateWindow

TCHAR szAppName[] = TEXT("iNoah");
TCHAR szTitle[]   = TEXT("iNoah");


iNoahSession session;
			
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
    static ArsLexis::String text=TEXT("Enter word and press look up");
	switch(msg)
	{
		case WM_CREATE:
		{
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
			break;
		}
        case WM_SIZE:
            MoveWindow(hwndEdit,2,2,LOWORD(lp)-4,20,TRUE);
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
                case IDM_MENU_REGULAR:
                {
				    HWND hwndMB = SHFindMenuBar (hwnd);
                    if (hwndMB) 
                    {
                       HMENU hMenu;
                       hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
    				   CheckMenuItem(hMenu, IDM_MENU_REGULAR, MF_CHECKED | MF_BYCOMMAND);
                       CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_UNCHECKED | MF_BYCOMMAND);
                    }
				    break;
                }
                case IDM_MENU_COMPACT:
                {
				    HWND hwndMB = SHFindMenuBar (hwnd);
                    if (hwndMB) 
                    {
                       HMENU hMenu;
                       hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
    				   CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_CHECKED | MF_BYCOMMAND);
                       CheckMenuItem(hMenu, IDM_MENU_REGULAR, MF_UNCHECKED | MF_BYCOMMAND);
                    }
				    break;
                }
                case ID_LOOKUP:
                {
                    int len = SendMessage(hwndEdit, EM_LINELENGTH, 0,0);
                    TCHAR *buf=new TCHAR[len+1];
                    len = SendMessage(hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
                    ArsLexis::String word(buf);
        			text.assign(session.getWord(word));
                    delete buf;
                    InvalidateRect(hwnd,NULL,TRUE);
                    break;
                }
			    default:
				    return DefWindowProc(hwnd, msg, wp, lp);
			}
			break;
        }
		case WM_PAINT:
		{
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			DrawText (hdc, text.c_str(), -1, &rect, 
                DT_SINGLELINE | DT_CENTER | DT_VCENTER);
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

// end sm_inoah.cpp
