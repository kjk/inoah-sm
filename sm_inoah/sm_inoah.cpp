#include "resource.h"
#include "sm_inoah.h"
#include "iNoahSession.h"
#include "iNoahParser.h"
#include "TAPIDevice.h"
#include "reclookups.h"
#include "registration.h"

#include "BaseTypes.hpp"
#include "GenericTextElement.hpp"
#include "BulletElement.hpp"
#include "ParagraphElement.hpp"
#include "HorizontalLineElement.hpp"
#include "Definition.hpp"
#include <Debug.hpp>

#include <windows.h>
#ifndef WIN32_PLATFORM_PSPC
#include <tpcshell.h>
#endif
#include <wingdi.h>
#include <fonteffects.hpp>
#include <sms.h>
#include <uniqueid.h>

HINSTANCE g_hInst      = NULL;  // Local copy of hInstance
HWND      g_hwndMain   = NULL;    // Handle to Main window returned from CreateWindow
HWND      g_hwndScroll = NULL;
HWND      g_hwndEdit   = NULL;

WNDPROC   g_oldEditWndProc = NULL;

static bool g_forceLayoutRecalculation=false;

TCHAR szAppName[] = TEXT("iNoah");
TCHAR szTitle[]   = TEXT("iNoah");

Definition *g_definition = NULL;
bool        g_fRec=false;

ArsLexis::String g_wordList;
ArsLexis::String g_recentWord;
iNoahSession     g_session;

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);


void    drawProgressInfo(HWND hwnd, TCHAR* text);
void    setFontSize(int diff,HWND hwnd);
void    paint(HWND hwnd, HDC hdc);
void    setScrollBar(Definition* definition);

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

static void setDefinition2(ArsLexis::String& defTxt)
{
    Definition * newDef = parseDefinition(defTxt);

    if (NULL==newDef)
        return;

    delete g_definition;
    g_definition = newDef;

    ArsLexis::Graphics gr(GetDC(g_hwndMain), g_hwndMain);
    g_fRec = true;
    InvalidateRect(g_hwndMain,NULL,TRUE);
}

static void setDefinition(ArsLexis::String& defs, HWND hwnd)
{
    iNoahSession::ResponseCode code=g_session.getLastResponseCode();

    switch (code)
    {
        case iNoahSession::serverMessage:
        {
            MessageBox(hwnd,defs.c_str(),TEXT("Information"), 
            MB_OK|MB_ICONINFORMATION|MB_APPLMODAL|MB_SETFOREGROUND);
            return;
        }
        case iNoahSession::serverError:
        case iNoahSession::error:
        {
            MessageBox(hwnd,defs.c_str(),TEXT("Error"), 
            MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND);
            return;
        }
        default:
        {
            setDefinition2(defs);
        }
    }
}

#define MAX_WORD_LEN 64
static void DoLookup(HWND hwnd)
{
    TCHAR buf[MAX_WORD_LEN+1];
    int len = SendMessage(g_hwndEdit, EM_LINELENGTH, 0,0);
    if (0==len)
        return;

    memset(buf,0,sizeof(buf));
    len = SendMessage(g_hwndEdit, WM_GETTEXT, len+1, (LPARAM)buf);
    SendMessage(g_hwndEdit, EM_SETSEL, 0,-1);

    ArsLexis::String word(buf); 
    drawProgressInfo(hwnd, _T("definition..."));

    String def;
    bool fOk = FGetWord(word,def);
    if (!fOk)
        return;
    setDefinition2(def);
}

static void DoRandom(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    paint(hwnd, hdc);
    ReleaseDC(hwnd, hdc);
    drawProgressInfo(hwnd, TEXT("random definition..."));

    String def;
    bool fOk = FGetRandomDef(def);
    if (!fOk)
        return;
    setDefinition2(def);
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

static void DoRecent(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    paint(hwnd, hdc);
    ReleaseDC(hwnd, hdc);
    drawProgressInfo(hwnd, TEXT("recent lookups list..."));

    bool fOk = FGetWordList(g_wordList);
    if (!fOk)
        return;

    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RECENT), hwnd, RecentLookupsDlgProc))
    {
        ArsLexis::String word(g_recentWord); 
        drawProgressInfo(hwnd, _T("definition..."));                
        String def;
        bool fOk = FGetWord(word,def);
        if (!fOk)
            return;
        setDefinition2(def);
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
    
    g_hwndEdit = CreateWindow(
        TEXT("edit"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | 
        WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        0,0,0,0,hwnd,
        (HMENU) ID_EDIT,
        g_hInst,
        NULL);

    g_oldEditWndProc = (WNDPROC)SetWindowLong(g_hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);

    g_hwndScroll = CreateWindow(
        TEXT("scrollbar"),
        NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
        0,0,0,0,hwnd,
        (HMENU) ID_SCROLL,
        g_hInst,
        NULL);

    setScrollBar(g_definition);

    // In order to make Back work properly, it's necessary to 
    // override it and then call the appropriate SH API
    (void)SendMessage(
        mbi.hwndMB, SHCMBM_OVERRIDEKEY, VK_TBACK,
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
        SHMBOF_NODEFAULT | SHMBOF_NOTIFY)
        );
    
    setFontSize(IDM_FNT_STANDARD, hwnd);
    SetFocus(g_hwndEdit);
}

void static OnHotKey(WPARAM wp, LPARAM lp)
{
    ArsLexis::Graphics gr(GetDC(g_hwndMain), g_hwndMain);

    int page=0;
    if (NULL!=g_definition)
        page=g_definition->shownLinesCount();

    switch(HIWORD(lp))
    {
        case VK_TBACK:
#ifndef WIN32_PLATFORM_PSPC
            if (0 != (MOD_KEYUP & LOWORD(lp)))
                SHSendBackToFocusWindow(WM_HOTKEY, wp, lp);
#endif
            break;
        case VK_TDOWN:
            if(NULL!=g_definition)
                g_definition->scroll(gr, renderingPrefs(), page);

            setScrollBar(g_definition);
            break;
    }
}

// Try to launch IE with a given url
static bool GotoURL(LPCTSTR lpszUrl)
{
    SHELLEXECUTEINFO sei;
    memset(&sei, 0, sizeof(SHELLEXECUTEINFO));
    sei.cbSize  = sizeof(SHELLEXECUTEINFO);
    sei.fMask   = SEE_MASK_FLAG_NO_UI;
    sei.lpVerb  = _T("open");
    sei.lpFile  = lpszUrl;
    sei.nShow   = SW_SHOWMAXIMIZED;

    if (ShellExecuteEx(&sei))
        return true;
    return false;
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
            setFontSize(IDM_FNT_LARGE, hwnd);
            break;

        case IDM_FNT_STANDARD:
            setFontSize(IDM_FNT_STANDARD, hwnd);
            break;

        case IDM_FNT_SMALL:
            setFontSize(IDM_FNT_SMALL, hwnd);
            break;

        case ID_LOOKUP:
            DoLookup(hwnd);
            InvalidateRect(hwnd,NULL,TRUE);
            break;

        case IDM_MENU_RANDOM:
            DoRandom(hwnd);
            break;

        case IDM_MENU_RECENT:
            DoRecent(hwnd);
            break;

        case IDM_MENU_REGISTER:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REGISTER), hwnd, RegistrationDlgProc);
            break;

        case IDM_CACHE:
            g_session.clearCache();
            break;

        case IDM_MENU_HOME:
            // Try to open hyperlink
            GotoURL(_T("http://arslexis.com/pda/sm.html"));
            break;
        
        case IDM_MENU_UPDATES:
            GotoURL(_T("http://arslexis.com/updates/sm-inoah-1-0.html"));
            break;

        case IDM_MENU_ABOUT:
            // TODO:
            //isAboutVisible = true;
            //setMenu(hwnd);
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
            paint(hwnd, hdc);
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

bool InitInstance (HINSTANCE hInstance, int CmdShow )
{
    g_hInst = hInstance;

    g_hwndMain = CreateWindow(szAppName,
        szTitle,
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
    TAPIDevice::initInstance();    
    return true;
}

BOOL InitApplication ( HINSTANCE hInstance )
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
    wc.lpszClassName = szAppName;

    BOOL fOk = RegisterClass(&wc);
    return fOk;
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR    lpCmdLine,
                   int        CmdShow)
                   
{
    // if we're already running, then just bring our window to front
    HWND hwndPrev = FindWindow(szAppName, szTitle);
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
    Transmission::closeInternet();

    return msg.wParam;
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
    DrawText(hdc, TEXT("(enter word and press \"Lookup\")"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
    tmpRect.top += 46;
    DrawText(hdc, TEXT("ArsLexis iNoah 1.0"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
    tmpRect.top += 18;
    DrawText(hdc, TEXT("http://www.arslexis.com"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
    tmpRect.top += 18;
    DrawText(hdc, TEXT("Unregistered"), -1, &tmpRect, DT_SINGLELINE|DT_CENTER);
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

    bool fCouldntDoubleBuffer = false;
    HDC offscreenDc = ::CreateCompatibleDC(hdc);
    if (offscreenDc) 
    {
        HBITMAP bitmap=::CreateCompatibleBitmap(hdc, bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            {
                ArsLexis::Graphics offscreen(offscreenDc, NULL);
                g_definition->render(offscreen, defRect, renderingPrefs(), g_forceLayoutRecalculation);
                offscreen.copyArea(defRect, gr, defRect.topLeft);
            }
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
        }
        else
            fCouldntDoubleBuffer = true;
        ::DeleteDC(offscreenDc);
    }
    else
        fCouldntDoubleBuffer = true;

    if (fCouldntDoubleBuffer)
        g_definition->render(gr, defRect, renderingPrefs(), g_forceLayoutRecalculation);
    g_forceLayoutRecalculation=false;
}

void paint(HWND hwnd, HDC hdc)
{
    RECT  rect;
    GetClientRect(hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

    rect.top    += 22;
    rect.left   += 2;
    rect.right  -= 7;
    rect.bottom -= 2;

    if (NULL==g_definition)
        PaintAbout(hdc,rect);
    else
        PaintDefinition(hwnd, hdc, rect);

    if (g_fRec)
    {
        setScrollBar(g_definition);
        g_fRec = false;
    }
}

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_KEYDOWN:
        {

            if (VK_TACTION==wp)
            {
                DoLookup(GetParent(hwnd));
                return 0;
            }

            if (NULL!=g_definition)
            {
                int page=0;
                switch (wp) 
                {
                    case VK_DOWN:
                        page=g_definition->shownLinesCount();
                        break;
                    case VK_UP:
                        page=-static_cast<int>(g_definition->shownLinesCount());
                        break;
                }
                if (0!=page)
                {
                    RECT b;
                    GetClientRect (g_hwndMain, &b);
                    ArsLexis::Rectangle bounds=b;
                    ArsLexis::Rectangle defRect=bounds;
                    defRect.explode(2, 22, -9, -24);
                    ArsLexis::Graphics gr(GetDC(g_hwndMain), g_hwndMain);
                    bool doubleBuffer=true;
                    
                    HDC offscreenDc=::CreateCompatibleDC(gr.handle());
                    if (offscreenDc)
                    {
                        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
                        if (bitmap)
                        {
                            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
                            {
                                ArsLexis::Graphics offscreen(offscreenDc, NULL);
                                g_definition->scroll(offscreen, renderingPrefs(), page);
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
                        g_definition->scroll(gr, renderingPrefs(), page);
                    
                    setScrollBar(g_definition);
                    return 0;
                }
            }
            break;
       }
    }

    return CallWindowProc(g_oldEditWndProc, hwnd, msg, wp, lp);
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
    renderingPrefsPtr()->setFontSize(delta);
    InvalidateRect(hwnd,NULL,TRUE);
}

void setScrollBar(Definition* definition)
{
    int frst=0;
    int total=0;
    int shown=0;
    if (definition)
    {
        frst=definition->firstShownLine();
        total=definition->totalLinesCount();
        shown=definition->shownLinesCount();
    }
    
    SetScrollPos(
        g_hwndScroll, 
        SB_CTL,
        frst,
        TRUE);
    
    SetScrollRange(
        g_hwndScroll,
        SB_CTL,
        0,
        total-shown,
        TRUE);
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

