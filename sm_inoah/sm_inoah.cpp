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


HINSTANCE g_hInst = NULL;  // Local copy of hInstance
HWND hwndMain = NULL;    // Handle to Main window returned from CreateWindow

TCHAR szAppName[] = TEXT("iNoah");
TCHAR szTitle[]   = TEXT("iNoah");
Definition definition_ = Definition();

iNoahSession session;
void initialize()
{
    //Err error=iPediaForm::initialize();
    //if (!error)
    //{
        ParagraphElement* parent=0;
        definition_.appendElement(parent=new ParagraphElement());
        parent->setChildIndentation(16);
        DefinitionElement* element=0;
        definition_.appendElement(element=new GenericTextElement(
            TEXT("For large UNIX projects, the traditional method of building the project is to use recursive ")
            TEXT("make. On some projects, this results in build times which are unacceptably large, when ")
            TEXT("all you want to do is change one file. In examining the source of the overly long build ")
            TEXT("times, it became evident that a number of apparently unrelated problems combine to produce ")
            TEXT("the delay, but on analysis all have the same root cause. ")
        ));
        element->setParent(parent);
        definition_.appendElement(element=new LineBreakElement());
        element->setParent(parent);
        definition_.appendElement(element=new GenericTextElement(
            TEXT("This paper explores a number of problems regarding the use of recursive make, and ")
            TEXT("shows that they are all symptoms of the same problem. Symptoms that the UNIX community ")
            TEXT("have long accepted as a fact of life, but which need not be endured any longer. ")
            TEXT("These problems include recursive makes which take ‘‘forever’’ to work out that they need ")
            TEXT("to do nothing, recursive makes which do too much, or too little, recursive makes which ")
            TEXT("are overly sensitive to changes in the source code and require constant Makefile intervention ")
            TEXT("to keep them working. ")
        ));
        element->setParent(parent);
        definition_.appendElement(new LineBreakElement());
        BulletElement* bullet=0;
        definition_.appendElement(bullet=new BulletElement());
        definition_.appendElement(element=new GenericTextElement(
            TEXT("The resolution of these problems can be found by looking at what make does, from first ")
            TEXT("principles, and then analyzing the effects of introducing recursive make to this activity. ")
            TEXT("The analysis shows that the problem stems from the artificial partitioning of the build into ")
            TEXT("separate subsets. This, in turn, leads to the symptoms described. To avoid the symptoms, ")
            TEXT("it is only necessary to avoid the separation; to use a single make session to build the ")
            TEXT("whole project, which is not quite the same as a single Makefile. ")
        ));
        element->setParent(bullet);
        definition_.appendElement(element=new HorizontalLineElement());
        element->setParent(bullet);
        definition_.appendElement(element=new GenericTextElement(
            TEXT("This conclusion runs counter to much accumulated folk wisdom in building large projects ")
            TEXT("on UNIX. Some of the main objections raised by this folk wisdom are examined and ")
            TEXT("shown to be unfounded. The results of actual use are far more encouraging, with routine ")
            TEXT("development performance improvements significantly faster than intuition may indicate, ")
            TEXT("and without the intuitvely expected compromise of modularity. The use of a whole ")
            TEXT("project make is not as difficult to put into practice as it may at first appear. ")
        ));
        element->setParent(bullet);
    //}
    //return error;
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

            // In order to make Back work properly, it's necessary to 
	        // override it and then call the appropriate SH API
	        (void)SendMessage(mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
		        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
		        SHMBOF_NODEFAULT | SHMBOF_NOTIFY));            
            initialize();
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
            if ( HIWORD(lp) == VK_TBACK && (0 != (MOD_KEYUP & LOWORD(lp))))
            {
                // check box is enabled, so we process the back key
                SHSendBackToFocusWindow( msg, wp, lp );
            }
            break;
            
		case WM_PAINT:
		{
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
            rect.top+=22;
            rect.left+=2;
            rect.right-=2;
            rect.bottom-=2;
			DrawText (hdc, text.c_str(), -1, &rect, DT_LEFT);
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


// end sm_inoah.cpp
