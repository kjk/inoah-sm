#include <windows.h>

#ifdef WIN32_PLATFORM_WFSP
#include <tpcshell.h>
#endif

#include <BaseTypes.hpp>
#include <Debug.hpp>
#include <SysUtils.hpp>
#include <EnterRegCodeDialog.hpp>
#include <Definition.hpp>

#include "Transmission.h"
#include "DefinitionParser.hpp"
#include "reclookups.h"
#include "sm_inoah.h"
#include "resource.h"

using ArsLexis::String;

const String iNoahFolder  = _T("\\iNoah");
const String cookieFile   = _T("\\Cookie");
const String regCodeFile  = _T("\\RegCode");

HINSTANCE g_hInst      = NULL;  // Local copy of hInstance
HWND      g_hwndMain   = NULL;    // Handle to Main window returned from CreateWindow
HWND      g_hwndScroll = NULL;
HWND      g_hwndEdit   = NULL;

WNDPROC   g_oldEditWndProc = NULL;

static bool g_forceLayoutRecalculation=false;

String      g_currentWord;
bool        g_fRec=false;

String g_wordList;
String g_recentWord;

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

RenderingPreferences* g_renderingPrefs = NULL;

static RenderingPreferences* renderingPrefsPtr(void)
{
    if (NULL==g_renderingPrefs)
        g_renderingPrefs = new RenderingPreferences();

    return g_renderingPrefs;
}

static RenderingPreferences& renderingPrefs(void)
{
    if (NULL==g_renderingPrefs)
        g_renderingPrefs = new RenderingPreferences();

    return *g_renderingPrefs;
}

Definition *g_definition = NULL;

static Definition *GetDefinition()
{
    return g_definition;

}
static void ReplaceDefinition(Definition *newDefinition)
{
    delete g_definition;
    g_definition = newDefinition;
}

static void DeleteDefinition()
{
    ReplaceDefinition(NULL);
}
    

static void SetDefinition(ArsLexis::String& defTxt)
{
    String word;
    Definition * newDef = ParseAndFormatDefinition(defTxt, word);

    if (NULL==newDef)
        return;

    ReplaceDefinition(newDef);
    g_currentWord = word;

    SetEditWinText(g_hwndEdit, word);
    SendMessage(g_hwndEdit, EM_SETSEL, 0,-1);

    ArsLexis::Graphics gr(GetDC(g_hwndMain), g_hwndMain);
    g_fRec = true;
    InvalidateRect(g_hwndMain,NULL,TRUE);
}

void DeleteFile(const String& fileName)
{
    String path;
    if (!GetSpecialFolderPath(path))
        return;
    String fullPath = path + iNoahFolder + fileName;
    DeleteFile(fullPath.c_str());
}

static void SaveStringToFile(const String& fileName, const String& str)
{
    String path;
    BOOL fOk = GetSpecialFolderPath(path);
    if (!fOk)
        return;

    String fullPath = path + iNoahFolder;
    fOk = CreateDirectory (fullPath.c_str(), NULL);  
    if (!fOk)
        return;
    fullPath += fileName;

    HANDLE handle = CreateFile(fullPath.c_str(), 
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL); 
    
    if (INVALID_HANDLE_VALUE==handle)
        return;

    DWORD written;
    WriteFile(handle, str.c_str(), str.length()*sizeof(TCHAR), &written, NULL);
    CloseHandle(handle);
}

static String LoadStringFromFile(String fileName)
{
    String path;
    BOOL fOk = GetSpecialFolderPath(path);
    if (!fOk)
        return _T("");

    String fullPath = path + iNoahFolder + fileName;
    
    HANDLE handle = CreateFile(fullPath.c_str(), 
        GENERIC_READ, FILE_SHARE_READ, NULL, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
        NULL); 

    if (INVALID_HANDLE_VALUE==handle)
        return _T("");
    
    TCHAR  buf[254];
    DWORD  bytesRead;
    String ret;

    while (TRUE)
    {
        fOk = ReadFile(handle, &buf, sizeof(buf), &bytesRead, NULL);

        if (!fOk || (0==bytesRead))
            break;
        ret.append(buf, bytesRead/sizeof(TCHAR));
    }

    CloseHandle(handle);
    return ret;
}

String g_regCode;

String GetRegCode()
{
    static bool fTriedFile = false;
    if (!g_regCode.empty())
        return g_regCode;

    if (fTriedFile)
        return g_regCode;

    g_regCode = LoadStringFromFile(regCodeFile);
    fTriedFile = true;
    return g_regCode;
}

void SetRegCode(const String& regCode)
{
    g_regCode = regCode;
    SaveStringToFile(regCodeFile,regCode);
}

static void DeleteRegCode()
{
    DeleteFile(regCodeFile);
    g_regCode.clear();
}

static bool FRegCodeExists()
{
    if (g_regCode.empty())
        return false;
    else
        return true;
}

String g_cookie;

String GetCookie()
{
    static bool fTriedFile = false;
    if (!g_cookie.empty())
        return g_cookie;

    if (fTriedFile)
        return g_cookie;

    g_cookie = LoadStringFromFile(cookieFile);
    fTriedFile = true;
    return g_cookie;
}

void SetCookie(const String& cookie)
{    
    g_cookie = cookie;
    SaveStringToFile(cookieFile,cookie);
}

static void DeleteCookie()
{
    DeleteFile(cookieFile);
    g_cookie.clear();
}

static void ClearCache()
{
    DeleteCookie();
    DeleteRegCode();
}

static void SetScrollBar(Definition* definition)
{
    int first=0;
    int range=0;

    if (definition)
    {
        first = definition->firstShownLine();
        range = definition->totalLinesCount() - definition->shownLinesCount();
    }
    
    SetScrollPos(g_hwndScroll, SB_CTL, first, TRUE);
    SetScrollRange(g_hwndScroll, SB_CTL, 0, range, TRUE);
}

static void ScrollDefinition(int page)
{
    RECT b;
    GetClientRect (g_hwndMain, &b);
    ArsLexis::Rectangle bounds=b;
    ArsLexis::Rectangle defRect=bounds;
    defRect.explode(2, 22, -9, -24);
    ArsLexis::Graphics gr(GetDC(g_hwndMain), g_hwndMain);

    bool fCouldDoubleBuffer = false;
    HDC  offscreenDc=::CreateCompatibleDC(gr.handle());
    if (offscreenDc)
    {
        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
        if (bitmap)
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            {
                ArsLexis::Graphics offscreen(offscreenDc, NULL);
                GetDefinition()->scroll(offscreen, renderingPrefs(), page);
                offscreen.copyArea(defRect, gr, defRect.topLeft);
            }
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
            fCouldDoubleBuffer = true;
        }
        ::DeleteDC(offscreenDc);
    }

    if (!fCouldDoubleBuffer)
        GetDefinition()->scroll(gr, renderingPrefs(), page);

    SetScrollBar(GetDefinition());
}

static void PaintAbout(HDC hdc, RECT& rect)
{
    LOGFONT logfnt;
    HFONT   fnt=(HFONT)GetStockObject(SYSTEM_FONT);
    GetObject(fnt, sizeof(logfnt), &logfnt);
    logfnt.lfHeight+=1;
    int fontDy = logfnt.lfHeight;
    HFONT fnt2=(HFONT)CreateFontIndirect(&logfnt);
    SelectObject(hdc, fnt2);
    
    RECT tmpRect=rect;
    DrawText(hdc, TEXT("(enter word and press \"Lookup\")"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
    tmpRect.top += 46;
    DrawText(hdc, TEXT("ArsLexis iNoah 1.0"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
    tmpRect.top += 18;
    DrawText(hdc, TEXT("http://www.arslexis.com"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
    tmpRect.top += 18;

    if (FRegCodeExists())
        DrawText(hdc, TEXT("Registered"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
    else
        DrawText(hdc, TEXT("Unregistered"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
    SelectObject(hdc,fnt);
    DeleteObject(fnt2);
}

static void PaintDefinition(HWND hwnd, HDC hdc, RECT& rect)
{
    ArsLexis::Graphics gr(hdc, hwnd);
    RECT b;
    GetClientRect(hwnd, &b);
    ArsLexis::Rectangle bounds = b;
    ArsLexis::Rectangle defRect = rect;

    bool fCouldDoubleBuffer = false;
    HDC  offscreenDc = ::CreateCompatibleDC(hdc);
    if (offscreenDc) 
    {
        HBITMAP bitmap=::CreateCompatibleBitmap(hdc, bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            {
                ArsLexis::Graphics offscreen(offscreenDc, NULL);
                GetDefinition()->render(offscreen, defRect, renderingPrefs(), g_forceLayoutRecalculation);
                offscreen.copyArea(defRect, gr, defRect.topLeft);
            }
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
            fCouldDoubleBuffer = true;
        }
        ::DeleteDC(offscreenDc);
    }

    if (!fCouldDoubleBuffer)
        GetDefinition()->render(gr, defRect, renderingPrefs(), g_forceLayoutRecalculation);
    g_forceLayoutRecalculation=false;
}

static void Paint(HWND hwnd, HDC hdc)
{
    RECT  rect;
    GetClientRect(hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

    rect.top    += 22;
    rect.left   += 2;
    rect.right  -= 7;
    rect.bottom -= 2;

    if (NULL==GetDefinition())
        PaintAbout(hdc,rect);
    else
        PaintDefinition(hwnd, hdc, rect);

    if (g_fRec && (NULL!=GetDefinition()))
    {
        SetScrollBar(GetDefinition());
        g_fRec = false;
    }
}


static void DrawProgressInfo(HWND hwnd, TCHAR* text)
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

static void SetFontSize(int diff, HWND hwnd)
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
                delta = 2;
                break;
            case IDM_FNT_STANDARD:
                CheckMenuItem(hMenu, IDM_FNT_STANDARD, MF_CHECKED | MF_BYCOMMAND);
                break;
            case IDM_FNT_SMALL:
                CheckMenuItem(hMenu, IDM_FNT_SMALL, MF_CHECKED | MF_BYCOMMAND);
                delta = -2;
                break;
        }
    }
    g_forceLayoutRecalculation = true;
    renderingPrefsPtr()->setFontSize(delta);
    InvalidateRect(hwnd,NULL,TRUE);
}

#define MAX_WORD_LEN 64
static void DoLookup(HWND hwnd)
{
    String word;
    GetEditWinText(g_hwndEdit, word);

    if (word==g_currentWord)
        return;

    DrawProgressInfo(hwnd, _T("definition..."));

    String def;
    bool fOk = FGetWord(word,def);
    if (!fOk)
        return;
    SetDefinition(def);
}

static void DoRandom(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    Paint(hwnd, hdc);
    ReleaseDC(hwnd, hdc);
    DrawProgressInfo(hwnd, TEXT("random definition..."));

    String def;
    bool fOk = FGetRandomDef(def);
    if (!fOk)
        return;
    SetDefinition(def);
}

static void DoCompact(HWND hwnd)
{
    static bool fCompactView = false;

    HWND hwndMB = SHFindMenuBar (hwnd);
    if (!hwndMB)
    {
        // can it happen?
        return;
    }

    HMENU hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);

    if (fCompactView)
    {
        CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_UNCHECKED | MF_BYCOMMAND);
        renderingPrefsPtr()->setClassicView();
        fCompactView = false;
    }
    else
    {
        CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_CHECKED | MF_BYCOMMAND);
        renderingPrefsPtr()->setCompactView();
        fCompactView = true;
    }

    g_forceLayoutRecalculation = true;
    g_fRec = true;
    InvalidateRect(hwnd,NULL,TRUE);
}

static void DoRecentLookups(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    Paint(hwnd, hdc);
    ReleaseDC(hwnd, hdc);
    DrawProgressInfo(hwnd, TEXT("recent lookups list..."));

    bool fOk = FGetRecentLookups(g_wordList);
    if (!fOk)
        return;

    if (g_wordList.empty())
    {
        MessageBox(g_hwndMain,
            _T("No lookups have been made so far."),
            _T("Information"), MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
    }
    else
    {
        if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RECENT), hwnd, RecentLookupsDlgProc))
        {
            ArsLexis::String word(g_recentWord); 
            DrawProgressInfo(hwnd, _T("definition..."));                
            String def;
            bool fOk = FGetWord(word,def);
            if (!fOk)
                return;
            SetDefinition(def);
        }
    }
}

static void OnCreate(HWND hwnd)
{
    SHMENUBARINFO mbi;
    ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
    mbi.cbSize     = sizeof(SHMENUBARINFO);
    mbi.hwndParent = hwnd;
    mbi.nToolBarId = IDR_HELLO_MENUBAR;
    mbi.hInstRes   = g_hInst;

    if (!SHCreateMenuBar(&mbi)) 
    {
        PostQuitMessage(0);
        return;
    }
    
    g_hwndEdit = CreateWindow( _T("edit"), NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        0, 0, 0, 0, hwnd,
        (HMENU) ID_EDIT, g_hInst, NULL);

    g_oldEditWndProc = (WNDPROC)SetWindowLong(g_hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);

    g_hwndScroll = CreateWindow( _T("scrollbar"), NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
        0, 0, 0, 0, hwnd,
        (HMENU) ID_SCROLL, g_hInst, NULL);

    SetScrollBar(GetDefinition());

    // In order to make Back work properly, it's necessary to 
    // override it and then call the appropriate SH API
    (void)SendMessage(
        mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
        );
    
    SetFontSize(IDM_FNT_STANDARD, hwnd);
    SetFocus(g_hwndEdit);
}

static void OnHotKey(WPARAM wp, LPARAM lp)
{
    int keyCode = HIWORD(lp);

#ifdef WIN32_PLATFORM_WFSP
    if (VK_TBACK==keyCode)
    {
        if (0 != (MOD_KEYUP & LOWORD(lp)))
        {
            SHSendBackToFocusWindow(WM_HOTKEY, wp, lp);
        }
        return;
    }
#endif
    if (VK_TBACK==keyCode)
    {
        if (NULL!=GetDefinition())
        {
            ScrollDefinition(GetDefinition()->shownLinesCount());
        }
    }
}

// Try to launch IE with a given url
static void OnRegister(HWND hwnd)
{
    String newRegCode;
    String oldRegCode = GetRegCode();
DoItAgain:
    bool fOk = FGetRegCodeFromUser(hwnd, oldRegCode, newRegCode);
    if (!fOk)
        return;

    bool fRegCodeOk = false;
    if (!FCheckRegCode(newRegCode, fRegCodeOk))
    {
        assert( false == fRegCodeOk );
        return;
    }

    if (fRegCodeOk)
    {
        SetRegCode(newRegCode);        
        MessageBox(g_hwndMain, 
            _T("Thank you for registering iNoah."), 
            _T("Registration successful"), 
            MB_OK | MB_ICONINFORMATION);
        // so that we change "Unregistered" to "Registered" in "About" screen
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else
    {
        if ( IDNO == MessageBox(g_hwndMain, 
            _T("Wrong registration code. Please contact support@arslexis.com.\n\nRe-enter the code?"),
            _T("Wrong reg code"), MB_YESNO) )
        {
            // this is "Ok" button. Clear-out registration code (since it was invalid)
            SetRegCode(_T(""));
        }
        else
        {
            oldRegCode.assign(newRegCode);
            goto DoItAgain;
        }
    }
}

static LRESULT OnCommand(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT result = TRUE;

    switch (wp)
    {
        case IDOK:
            SendMessage(hwnd,WM_CLOSE,0,0);
            break;

        case IDM_MENU_COMPACT:
            DoCompact(hwnd);
            break;

        case IDM_FNT_LARGE:
            SetFontSize(IDM_FNT_LARGE, hwnd);
            break;

        case IDM_FNT_STANDARD:
            SetFontSize(IDM_FNT_STANDARD, hwnd);
            break;

        case IDM_FNT_SMALL:
            SetFontSize(IDM_FNT_SMALL, hwnd);
            break;

        case ID_LOOKUP:
            DoLookup(hwnd);
            break;

        case IDM_MENU_RANDOM:
            DoRandom(hwnd);
            break;

        case IDM_MENU_RECENT:
            DoRecentLookups(hwnd);
            break;

        case IDM_MENU_REGISTER:
            OnRegister(hwnd);
            break;

        case IDM_CACHE:
            ClearCache();
            break;

        case IDM_MENU_HOME:
            GotoURL(_T("http://arslexis.com/pda/sm.html"));
            break;
        
        case IDM_MENU_UPDATES:
            GotoURL(_T("http://arslexis.com/updates/sm-inoah-1-0.html"));
            break;

        case IDM_MENU_ABOUT:
            DeleteDefinition();
            g_currentWord.clear();
            InvalidateRect(hwnd,NULL,TRUE);
            break;

        default:
            // can it happen?
            return DefWindowProc(hwnd, msg, wp, lp);
    }

    SetFocus(g_hwndEdit);
    return result;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT      lResult = TRUE;
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (msg)
    {
        case WM_CREATE:
            OnCreate(hwnd);
            break;

        case WM_SIZE:
            MoveWindow(g_hwndEdit, 2, 2, LOWORD(lp)-4, 20, true);
            MoveWindow(g_hwndScroll, LOWORD(lp)-5, 28 , 5, HIWORD(lp)-28, false);
            break;

        case WM_SETFOCUS:
            SetFocus(g_hwndEdit);
            break;

        case WM_COMMAND:
            OnCommand(hwnd, msg, wp, lp);
            break;

        case WM_HOTKEY:
            OnHotKey(wp,lp);
            break;

        case WM_PAINT:
            hdc = BeginPaint (hwnd, &ps);
            Paint(hwnd, hdc);
            EndPaint (hwnd, &ps);
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
    return lResult;
}

// return true if no more processing is needed, false otherwise
static bool OnEditKeyDown(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (VK_TACTION==wp)
    {
        DoLookup(GetParent(hwnd));
        return true;
    }

    // check for up/down to see if we should do scrolling
    if (NULL==GetDefinition())
    {
        // no definition => no need for scrolling
        return false;
    }

    if (VK_DOWN==wp)
    {
        ScrollDefinition(GetDefinition()->shownLinesCount());
        return true;
    }

    if (VK_UP==wp)
    {
        ScrollDefinition(-static_cast<int>(GetDefinition()->shownLinesCount()));
        return true;
    }

    return false;
}

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (WM_KEYDOWN==msg)
    {
        bool fDone = OnEditKeyDown(hwnd, msg, wp, lp);
        if (fDone)
            return 0;
    }

    return CallWindowProc(g_oldEditWndProc, hwnd, msg, wp, lp);
}

static bool InitInstance (HINSTANCE hInstance, int CmdShow )
{
    g_hInst = hInstance;

    g_hwndMain = CreateWindow(APP_NAME,
        WINDOW_TITLE,
        WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL );

    if (!g_hwndMain)
        return false;

    ShowWindow(g_hwndMain, CmdShow );
    UpdateWindow(g_hwndMain);
    return true;
}

static BOOL InitApplication ( HINSTANCE hInstance )
{
    WNDCLASS wc;
    
    wc.style         = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hIcon         = NULL;
    wc.hInstance     = hInstance;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH );
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = APP_NAME;

    BOOL fOk = RegisterClass(&wc);
    return fOk;
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR    lpCmdLine,
                   int        CmdShow)
                   
{
    // if we're already running, then just bring our window to front
    HWND hwndPrev = FindWindow(APP_NAME, WINDOW_TITLE);
    if (hwndPrev) 
    {
        SetForegroundWindow(hwndPrev);    
        return 0;
    }
    
    if (!hPrevInstance)
    {
        if (!InitApplication(hInstance))
            return FALSE; 
    }

    if (!InitInstance(hInstance, CmdShow))
        return FALSE;
    
    MSG     msg;
    while (TRUE == GetMessage( &msg, NULL, 0,0 ))
    {
        TranslateMessage (&msg);
        DispatchMessage(&msg);
    }

    DeinitConnection();
    DeinitWinet();
    DeleteDefinition();

    return msg.wParam;
}

void ArsLexis::handleBadAlloc()
{
    RaiseException(1,0,0,NULL);    
}

void* ArsLexis::allocate(size_t size)
{
    void* ptr=0;
    if (size) 
        ptr=malloc(size);
    else
        ptr=malloc(1);
    if (!ptr)
        handleBadAlloc();
    return ptr;
}

