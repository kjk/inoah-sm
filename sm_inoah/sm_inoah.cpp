#include <windows.h>

#ifdef WIN32_PLATFORM_WFSP
#include <tpcshell.h>
#endif

#include <BaseTypes.hpp>
#include <Debug.hpp>
#include <Text.hpp>
#include <SysUtils.hpp>
#include <WinPrefsStore.hpp>
#include <EnterRegCodeDialog.hpp>
#include <StringListDialog.hpp>
#include <Definition.hpp>

#include "Transmission.hpp"
#include "DefinitionParser.hpp"
#include "sm_inoah.h"
#include "resource.h"

// this is not really used, it's just needed by a framework needed in sm_ipedia
HWND g_hwndForEvents = NULL;

HINSTANCE g_hInst      = NULL;  // Local copy of hInstance
HWND      g_hwndMain   = NULL;  // Handle to Main window returned from CreateWindow
HWND      g_hwndScroll = NULL;
HWND      g_hwndEdit   = NULL;
HWND      g_hwndSearchButton = NULL;

WNDPROC   g_oldEditWndProc = NULL;

SHACTIVATEINFO g_sai;

static bool g_forceLayoutRecalculation=false;

String      g_currentWord;
bool        g_fUpdateScrollbars = false;

String g_recentWord;

LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

Definition* g_definition = NULL;
bool g_showAbout = true;


static void SetDefinition(ArsLexis::String& defTxt)
{
    String word;
    DefinitionModel* model = ParseAndFormatDefinition(defTxt, word);
	assert(NULL != g_definition);

	g_definition->setModel(model, Definition::ownModel);
	g_showAbout = false;

    g_currentWord = word;

    SetEditWinText(g_hwndEdit, word);
    SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
    g_fUpdateScrollbars = true;
    InvalidateRect(g_hwndMain, NULL, TRUE);
}

struct Preferences {
    String cookie;
    String regCode;

    int    fontSize;

	enum ViewType {
		viewCompact,
		viewClassic
	};
	int viewType;

	Preferences(): fontSize(fontSizeMedium), viewType(viewCompact) {}
};

Preferences g_prefs;

enum PreferenceId 
{
    cookiePrefId,
    regCodePrefId,
    fontSizePrefId,
	viewTypePrefId,
};

#define PREFS_FILE_NAME _T("iNoah.prefs")

static void LoadPreferences()
{
    static bool fLoaded = false;
    if (fLoaded)
        return;

    std::auto_ptr<PrefsStoreReader> reader(new PrefsStoreReader(PREFS_FILE_NAME));

    Preferences  prefs;
    status_t        error;
    const char_t*   text;

    if (errNone != (error = reader->ErrGetStr(cookiePrefId, &text))) 
        goto Exit;
    prefs.cookie = text;

    if (errNone != (error = reader->ErrGetStr(regCodePrefId, &text))) 
        goto Exit;
    prefs.regCode = text;
    
    if (errNone != (error = reader->ErrGetInt(fontSizePrefId, &prefs.fontSize))) 
        goto Exit;

    if (errNone != (error = reader->ErrGetInt(viewTypePrefId, &prefs.viewType))) 
        goto Exit;
	
Exit:
    fLoaded = true;
    g_prefs = prefs;
}

static void SavePreferences()
{
    status_t error;
    std::auto_ptr<PrefsStoreWriter> writer(new PrefsStoreWriter(PREFS_FILE_NAME));

    if (errNone != (error = writer->ErrSetStr(cookiePrefId, g_prefs.cookie.c_str())))
        goto OnError;

    if (errNone != (error = writer->ErrSetStr(regCodePrefId, g_prefs.regCode.c_str())))
        goto OnError;

    if (errNone != (error = writer->ErrSetInt(fontSizePrefId, g_prefs.fontSize)))
        goto OnError;

    if (errNone != (error = writer->ErrSetInt(viewTypePrefId, g_prefs.viewType)))
        goto OnError;

    if (errNone!=(error=writer->ErrSavePreferences()))
        goto OnError;

    return;        
OnError:
    return; 
}

void GetRegCode(String& code)
{
    LoadPreferences();
    code = g_prefs.regCode;
}

void SetRegCode(const String& regCode)
{
    g_prefs.regCode = regCode;
    SavePreferences();
}

static void DeleteRegCode()
{
    g_prefs.regCode.clear();
    SavePreferences();
}

static bool FRegCodeExists()
{
    LoadPreferences();
    if (g_prefs.regCode.empty())
        return false;
    else
        return true;
}

void GetCookie(String& cookie)
{
    LoadPreferences();
    cookie = g_prefs.cookie;
}

void SetCookie(const String& cookie)
{    
    g_prefs.cookie = cookie;
    SavePreferences();
}

static void DeleteCookie()
{
    g_prefs.cookie.clear();
    SavePreferences();
}

static void ClearCache()
{
    DeleteCookie();
    DeleteRegCode();
}


int GetPrefsFontSize()
{
    LoadPreferences();
    return g_prefs.fontSize;
}

void SetPrefsFontSize(int size)
{
    g_prefs.fontSize = size;
    SavePreferences();
}

void* g_ClipboardText = NULL;

static void FreeClipboardData()
{
    if (NULL != g_ClipboardText)
    {
        LocalFree(g_ClipboardText);
        g_ClipboardText = NULL;
    }
}

static void* CreateNewClipboardData(const String& str)
{
    FreeClipboardData();

    int     strLen = str.length();

    g_ClipboardText = LocalAlloc(LPTR, (strLen + 1) * sizeof(char_t));
    if (NULL == g_ClipboardText)
        return NULL;

    ZeroMemory(g_ClipboardText, (strLen + 1) * sizeof(char_t));
    memcpy(g_ClipboardText, str.data(), strLen * sizeof(char_t));
    return g_ClipboardText;
}

// copy definition to clipboard
static void CopyToClipboard(HWND hwndMain, const Definition* def)
{
	if (NULL == def || def->empty())
		return;

    void*    clipData;
    String    text;

    if (0 == OpenClipboard(hwndMain))
        return;

    // TODO: should we put it anyway?
    if (0 == EmptyClipboard())
    {
        // EmptyClipboard() failed to free the clipboard
        goto Exit;
    }
    
    def->selectionToText(text);

    clipData = CreateNewClipboardData(text);
    if (NULL == clipData)
        goto Exit;
    
    SetClipboardData(CF_UNICODETEXT, clipData);
Exit:
    CloseClipboard();
}

static void SetScrollBar(const Definition* definition)
{
    int first = 0;
    int total = 0;
    int shown = 0;

    if (!g_showAbout && !definition->empty())
    {
        first = definition->firstShownLine();
        total = definition->totalLinesCount();
        shown = definition->shownLinesCount();
    }

	SCROLLINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	if (shown == total)
	{
		si.nMax = 0;
		si.nPage = 0;
	}
	else
	{
		si.nMax = total - 1;
		si.nPage = shown;
	}
	si.nPos = first;

	SetScrollInfo(g_hwndScroll, SB_CTL, &si, TRUE);
}


enum ScrollUnit
{
    scrollLine,
    scrollPage,
    scrollHome,
    scrollEnd,
    scrollPosition
};

// we draw a text inside a rectangle
// rectangle size depends on the text size
// i.e. dy must be font's dy + good spacing
// dx will depend on the size of the screen i.e. it's 
// also, rectangle is centered on the screen
static void DrawProgressInfo(HWND hwnd, TCHAR* text)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    rect.right -= GetScrollBarDx();
    // space between edges of the client area for the box
    const int borderDxPadding = 10;

    // calculte dx of the box
    rect.left    += borderDxPadding;
    rect.right   -= borderDxPadding;

    // now calculate dy of the box to fit 2 lines of the text plus
    // some padding

    LOGFONT logfnt;
    HFONT fnt = (HFONT)GetStockObject(SYSTEM_FONT);
    GetObject(fnt, sizeof(logfnt), &logfnt);
    logfnt.lfHeight += 1; // one point smaller
    logfnt.lfWeight = 800;  // bold
    DeleteObject(fnt);

    fnt = (HFONT)CreateFontIndirect(&logfnt);
    if (NULL==fnt)
    {
        // if we didn't get the font we wanted, just use standard
        HFONT fnt = (HFONT)GetStockObject(SYSTEM_FONT);
        GetObject(fnt, sizeof(logfnt), &logfnt);
    }

    int fontDy = -logfnt.lfHeight;

    const int yPadding = 5;
    const int lineSpacing = 3;

    int rectDy = 2*fontDy + lineSpacing + yPadding*2;

    int clientDy = rect.bottom-rect.top;

    rect.top     += (clientDy-rectDy)/2;
    rect.bottom  -= (clientDy-rectDy)/2;

    HDC hdc=GetDC(hwnd);
    HFONT prevFnt = (HFONT)SelectObject(hdc, fnt);

/*    rect.top -= 1;
    rect.bottom += 1;
    rect.left -= 1;
    rect.right += 1;
    DrawRectangle(hdc, &rect);

    rect.top += 1;
    rect.bottom -= 1;
    rect.left += 1;
    rect.right -= 1; */

    DrawFancyRectangle(hdc, &rect);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc,RGB(255,255,255));
    //SetTextColor(hdc,RGB(255,0,0));

    RECT rectTxt = rect;
    rectTxt.top += (yPadding - 2);
    rectTxt.bottom = rectTxt.top + fontDy + 2*2;
    DrawText(hdc, _T("Downloading"), -1, &rectTxt, DT_VCENTER | DT_CENTER);

    rectTxt.top = rect.top + yPadding + fontDy + lineSpacing - 2;
    rectTxt.bottom = rectTxt.top + fontDy + 2*2;
    DrawText(hdc, text, -1, &rectTxt, DT_VCENTER | DT_CENTER);

    SelectObject(hdc,prevFnt);
    DeleteObject(fnt);
    ReleaseDC(hwnd,hdc);
}

static void PaintAbout(HDC hdc, RECT& rect)
{
    HFONT   fnt = (HFONT)GetStockObject(SYSTEM_FONT);
    if (NULL==fnt)
        return;

    rect.top += 2;
#ifdef WIN32_PLATFORM_PSPC
    rect.top += 6;
#endif

    LOGFONT logfnt;
    GetObject(fnt, sizeof(logfnt), &logfnt);
    logfnt.lfHeight += 1;
    int fontDy = -logfnt.lfHeight;
    HFONT fntSmall = (HFONT)CreateFontIndirect(&logfnt);
    if (NULL!=fntSmall)
        SelectObject(hdc, fntSmall);
    else
        SelectObject(hdc, fnt);

    logfnt.lfHeight += 2;
    HFONT fntVerySmall = (HFONT)CreateFontIndirect(&logfnt);

    int lineSpace = fontDy+5;

    RECT tmpRect = rect;
#ifdef WIN32_PLATFORM_PSPC
    DrawText(hdc, _T("(enter word and press \"Search\")"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#else
    DrawText(hdc, _T("(enter word and press \"Lookup\")"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#endif
    tmpRect.top += 36;

#ifdef DEBUG
    DrawText(hdc, _T("ArsLexis iNoah 1.0 (debug)"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#else
    DrawText(hdc, _T("ArsLexis iNoah 1.0"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#endif
    tmpRect.top += lineSpace;

    if (FRegCodeExists())
    {
        DrawText(hdc, _T("Registered"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
        tmpRect.top += lineSpace;
        DrawText(hdc, _T("http://www.arslexis.com"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
    }
    else
    {
        DrawText(hdc, _T("Unregistered"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
        tmpRect.top += lineSpace;
        DrawText(hdc, _T("Purchase registration code at:"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
        tmpRect.top += lineSpace;
#ifdef HANDANGO
        DrawText(hdc, _T("http://handango.com"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#else
        DrawText(hdc, _T("http://www.arslexis.com"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#endif
    }

    if (NULL!=fntVerySmall)
        SelectObject(hdc, fntVerySmall);

#ifdef WIN32_PLATFORM_WFSP
    tmpRect.top += (lineSpace+22);
    DrawText(hdc, _T("Downloading uses data connection"), -1, &tmpRect, DT_SINGLELINE | DT_CENTER);
#endif

    if (NULL!=fntSmall)
        DeleteObject(fntSmall);

    if (NULL!=fntVerySmall)
        DeleteObject(fntVerySmall);

    SelectObject(hdc,fnt);
}

static void RepaintDefinition(int scrollDelta)
{
    RECT    clientRect;
    GetClientRect(g_hwndMain, &clientRect);

    ArsRectangle bounds = clientRect;

    RECT defRectTmp = clientRect;
    defRectTmp.top    += 24;  // TODO: should it depend on the size of edit window?
    defRectTmp.left   += 2;
    defRectTmp.right  -= 2 + GetScrollBarDx();
    defRectTmp.bottom -= 2;

    ArsRectangle defRect = defRectTmp;
    Graphics gr(g_hwndMain);

    bool fDidDoubleBuffer = false;
    HDC  offscreenDc = ::CreateCompatibleDC(gr.handle());
    if (offscreenDc) 
    {
        Graphics offscreen(offscreenDc);
        HBITMAP bitmap=::CreateCompatibleBitmap(gr.handle(), bounds.width(), bounds.height());
        if (bitmap) 
        {
            HBITMAP oldBitmap=(HBITMAP)::SelectObject(offscreenDc, bitmap);
            gr.copyArea(defRect, offscreen, defRect.topLeft);
            if (0 != scrollDelta)
                g_definition->scroll(offscreen, scrollDelta);
            else
                g_definition->render(offscreen, defRect, g_forceLayoutRecalculation);

            offscreen.copyArea(defRect, gr, defRect.topLeft);
            ::SelectObject(offscreenDc, oldBitmap);
            ::DeleteObject(bitmap);
            fDidDoubleBuffer = true;
        }
    }

    if (!fDidDoubleBuffer)
    {
        if (0 != scrollDelta)
            g_definition->scroll(gr, scrollDelta);
        else
            g_definition->render(gr, defRect, g_forceLayoutRecalculation);
    }
    g_forceLayoutRecalculation = false;
}

static void ScrollDefinition(int units, ScrollUnit unit, bool updateScrollbar)
{
    switch (unit)
    {
        case scrollPage:
            units = units * g_definition->shownLinesCount();
            break;
        case scrollEnd:
            units = g_definition->totalLinesCount();
            break;
        case scrollHome:
            units = -int(g_definition->totalLinesCount());
            break;
        case scrollPosition:
            units -= g_definition->firstShownLine();
            break;
    }

    RepaintDefinition(units);

    SetScrollBar(g_definition);
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

    if (g_showAbout)
        PaintAbout(hdc, rect);
    else
        RepaintDefinition(0);

    if (g_fUpdateScrollbars)
    {
        SetScrollBar(g_definition);
        g_fUpdateScrollbars = false;
    }
}

static void SetFontSize(int fontSize, HWND hwnd)
{
    HWND hwndMB = SHFindMenuBar(hwnd);
    if (NULL == hwndMB) 
        return;

#ifdef WIN32_PLATFORM_PSPC
    HMENU hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_OPTIONS_MENU_BTN);
#else
    HMENU hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);
#endif

    CheckMenuItem(hMenu, IDM_FNT_LARGE, MF_UNCHECKED | MF_BYCOMMAND);
    CheckMenuItem(hMenu, IDM_FNT_SMALL, MF_UNCHECKED | MF_BYCOMMAND);
    CheckMenuItem(hMenu, IDM_FNT_STANDARD, MF_UNCHECKED | MF_BYCOMMAND);

    int menuItemId = IDM_FNT_STANDARD;
    switch (fontSize)
    {
        case fontSizeSmall:
            menuItemId = IDM_FNT_SMALL;
            break;
        case fontSizeMedium:
            menuItemId = IDM_FNT_STANDARD;
            break;
        case fontSizeLarge:
            menuItemId = IDM_FNT_LARGE;
            break;

        default:
            assert(false);
            break;
    }

    CheckMenuItem(hMenu, menuItemId, MF_CHECKED | MF_BYCOMMAND);
    g_forceLayoutRecalculation = true;
    g_fUpdateScrollbars = true;
    InvalidateRect(hwnd, NULL, TRUE);
}

#define MAX_WORD_LEN 64
static void DoLookup(HWND hwnd)
{
    String word;
    GetEditWinText(g_hwndEdit, word);

    if (word == g_currentWord)
        return;

    DrawProgressInfo(hwnd, _T("definition..."));

    String def;
    bool fOk = FGetWord(word, def);
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
    HWND hwndMB = SHFindMenuBar(hwnd);
    if (!hwndMB)
    {
        // can it happen?
        return;
    }

    HMENU hMenu = (HMENU)SendMessage (hwndMB, SHCMBM_GETSUBMENU, 0, ID_MENU_BTN);

    if (Preferences::viewCompact == g_prefs.viewType)
    {
        CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_UNCHECKED | MF_BYCOMMAND);
		g_prefs.viewType = Preferences::viewClassic;
		SavePreferences();
    }
    else
    {
        CheckMenuItem(hMenu, IDM_MENU_COMPACT, MF_CHECKED | MF_BYCOMMAND);
		g_prefs.viewType = Preferences::viewCompact;
		SavePreferences();
    }

    g_forceLayoutRecalculation = true;
    g_fUpdateScrollbars = true;
    InvalidateRect(hwnd,NULL,TRUE);
}

static void DoRecentLookups(HWND hwnd)
{
    int wordCount;

    HDC hdc = GetDC(hwnd);
    Paint(hwnd, hdc);
    ReleaseDC(hwnd, hdc);
    DrawProgressInfo(hwnd, TEXT("recent lookups list..."));

    String recentLookups;
    bool fOk = FGetRecentLookups(recentLookups);
    if (!fOk)
        return;

    if (recentLookups.empty())
    {
        MessageBox(hwnd,
            _T("No lookups have been made so far."),
            _T("Information"), MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
        InvalidateRect(hwnd,NULL,TRUE);
        return;
    }

    CharPtrList_t strList;
    wordCount = AddLinesToList(recentLookups, strList);
    if (0 == wordCount)
        return;

    std::reverse(strList.begin(), strList.end());

    String selectedString;
    bool fSelected = FGetStringFromList(hwnd, strList, NULL, selectedString);
    if (!fSelected)
        return;

    FreeStringsFromCharPtrList(strList);
    DrawProgressInfo(hwnd, _T("definition..."));                
    String def;
    fOk = FGetWord(selectedString,def);
    if (!fOk)
        return;
    SetDefinition(def);
}

HWND g_hwndMenuBar = NULL;

static void OnSettingChange(HWND hwnd, WPARAM wp, LPARAM lp)
{
    SHHandleWMSettingChange(hwnd, wp, lp, &g_sai);
}

static void OnActivateMain(HWND hwnd, WPARAM wp, LPARAM lp)
{
    SHHandleWMActivate(hwnd, wp, lp, &g_sai, 0);
}

static void OnHibernate(HWND hwnd, WPARAM wp, LPARAM lp)
{
    // do nothing
}

static void OnCreate(HWND hwnd)
{
    SHMENUBARINFO mbi = {0};
    mbi.cbSize     = sizeof(mbi);
    mbi.hwndParent = hwnd;
    mbi.nToolBarId = IDR_MAIN_MENUBAR;
    mbi.hInstRes   = g_hInst;

    if (!SHCreateMenuBar(&mbi)) 
    {
        PostQuitMessage(0);
        return;
    }

    g_hwndMenuBar = mbi.hwndMB;
    OverrideBackButton(mbi.hwndMB);

#ifdef WIN32_PLATFORM_PSPC
    ZeroMemory(&g_sai, sizeof(g_sai));
    g_sai.cbSize = sizeof(g_sai);

    // size the main window taking into account SIP state and menu bar size
    SIPINFO si = {0};
    si.cbSize = sizeof(si);
    SHSipInfo(SPI_GETSIPINFO, 0, (PVOID)&si, FALSE);

    int visibleDx = RectDx(&si.rcVisibleDesktop);
    int visibleDy = RectDy(&si.rcVisibleDesktop);

    if ( FDesktopIncludesMenuBar(&si) )
    {
        RECT rectMenuBar;
        GetWindowRect(mbi.hwndMB, &rectMenuBar);
        int menuBarDy = RectDy(&rectMenuBar);
        visibleDy -= menuBarDy;
    }
    SetWindowPos(hwnd, NULL, 0, 0, visibleDx, visibleDy, SWP_NOMOVE | SWP_NOZORDER);
#endif

#ifdef WIN32_PLATFORM_PSPC
    g_hwndSearchButton = CreateWindow(
        _T("button"),  
        _T("Search"),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON ,//| BS_OWNERDRAW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd,
        (HMENU)ID_SEARCH_BTN, g_hInst, NULL);
#endif

    g_hwndEdit = CreateWindow( _T("edit"), NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        0, 0, 0, 0, hwnd,
        (HMENU) ID_EDIT, g_hInst, NULL);

    g_oldEditWndProc = (WNDPROC)SetWindowLong(g_hwndEdit, GWL_WNDPROC, (LONG)EditWndProc);

    g_hwndScroll = CreateWindow( _T("scrollbar"), NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP| SBS_VERT,
        0, 0, 0, 0, hwnd,
        (HMENU) ID_SCROLL, g_hInst, NULL);

    SetScrollBar(g_definition);

    int fontSize = GetPrefsFontSize();
    SetFontSize(fontSize, hwnd);
    SetFocus(g_hwndEdit);
}

// Try to launch IE with a given url
static void OnRegister(HWND hwnd)
{
    String newRegCode;
    String oldRegCode;
	GetRegCode(oldRegCode);
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
            MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
        // so that we change "Unregistered" to "Registered" in "About" screen
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else
    {
        if ( IDNO == MessageBox(g_hwndMain, 
            _T("Wrong registration code. Please contact support@arslexis.com if problem persists.\n\nRe-enter the code?"),
            _T("Wrong reg code"), MB_YESNO | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND ) )
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
    LRESULT result = 0;

    switch (wp)
    {
        case IDM_MENU_CLIPBOARD:
            CopyToClipboard(g_hwndMain, g_definition);
            break;

        case IDM_EXIT: // intentional no break
        case IDOK:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case IDM_MENU_COMPACT:
            assert(false); // doesn't work currently
            DoCompact(hwnd);
            break;

        case IDM_FNT_LARGE:
            SetFontSize(fontSizeLarge, hwnd);
            SetPrefsFontSize(fontSizeLarge);
            break;

        case IDM_FNT_STANDARD:
            SetFontSize(fontSizeMedium, hwnd);
            SetPrefsFontSize(fontSizeMedium);
            break;

        case IDM_FNT_SMALL:
            SetFontSize(fontSizeSmall, hwnd);
            SetPrefsFontSize(fontSizeSmall);
            break;

        case ID_SEARCH_BTN:
        case IDM_SEARCH:
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
#ifdef WIN32_PLATFORM_WFSP
            GotoURL(_T("http://arslexis.com/pda/sm.html"));
#endif
#ifdef WIN32_PLATFORM_PSPC
            GotoURL(_T("http://arslexis.com/pda/ppc.html"));
#endif
            break;

        case IDM_MENU_UPDATES:
#ifdef WIN32_PLATFORM_WFSP
            GotoURL(_T("http://arslexis.com/updates/sm-inoah-1-0.html"));
#endif
#ifdef WIN32_PLATFORM_PSPC
            GotoURL(_T("http://arslexis.com/updates/ppc-inoah-1-0.html"));      
#endif
            break;

        case IDM_MENU_ABOUT:
            g_showAbout = true;
            g_fUpdateScrollbars = true;
            InvalidateRect(hwnd,NULL,TRUE);
            break;

        default:
            // can it happen?
            return DefWindowProc(hwnd, msg, wp, lp);
    }

    SetFocus(g_hwndEdit);
    return result;
}

static void OnSize(HWND hwnd, LPARAM lp)
{
    int dx = LOWORD(lp);
    int dy = HIWORD(lp);

#ifdef WIN32_PLATFORM_PSPC
    int searchButtonDX = 50;
    int searchButtonX = dx - searchButtonDX - 2;

    MoveWindow(g_hwndSearchButton, searchButtonX, 2, searchButtonDX, 20, TRUE);
    MoveWindow(g_hwndEdit, 2, 2, searchButtonX - 6, 20, TRUE);
#else
    MoveWindow(g_hwndEdit, 2, 2, dx-4, 20, TRUE);
#endif

    // should that depend on the size of edit window?
    int scrollStartY = 24;
    int scrollDy = dy - scrollStartY - 2;
    MoveWindow(g_hwndScroll, dx-GetScrollBarDx(), scrollStartY, GetScrollBarDx(), scrollDy, FALSE);
}

static void OnScroll(WPARAM wp)
{
    int code = LOWORD(wp);
    switch (code)
    {
        case SB_TOP:
            ScrollDefinition(0, scrollHome, false);
            break;
        case SB_BOTTOM:
            ScrollDefinition(0, scrollEnd, false);
            break;
        case SB_LINEUP:
            ScrollDefinition(-1, scrollLine, false);
            break;
        case SB_LINEDOWN:
            ScrollDefinition(1, scrollLine, false);
            break;
        case SB_PAGEUP:
            ScrollDefinition(-1, scrollPage, false);
            break;
        case SB_PAGEDOWN:
            ScrollDefinition(1, scrollPage, false);
            break;

        case SB_THUMBPOSITION:
        {
            SCROLLINFO info = {0};
            info.cbSize = sizeof(info);
            info.fMask = SIF_TRACKPOS;
            GetScrollInfo(g_hwndScroll, SB_CTL, &info);
            ScrollDefinition(info.nTrackPos, scrollPosition, true);
        }
     }
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
            OnSize(hwnd, lp);
            break;

        case WM_SETFOCUS:
            SetFocus(g_hwndEdit);
            break;

        case WM_COMMAND:
            OnCommand(hwnd, msg, wp, lp);
            break;

#ifdef WIN32_PLATFORM_WFSP
        case WM_HOTKEY:
            SHSendBackToFocusWindow(msg, wp, lp);
            break;
#endif

        case WM_VSCROLL:
            OnScroll(wp);
            break;

        case WM_PAINT:
            hdc = BeginPaint (hwnd, &ps);
            Paint(hwnd, hdc);
            EndPaint (hwnd, &ps);
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_SETTINGCHANGE:
            OnSettingChange(hwnd, wp, lp);
            break;

        case WM_ACTIVATE:
            OnActivateMain(hwnd, wp, lp);
            break;

        case WM_HIBERNATE:
            OnHibernate(hwnd, wp, lp);
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
    if (VK_TACTION == wp)
    {
        DoLookup(GetParent(hwnd));
        return true;
    }

    // check for up/down to see if we should do scrolling
    if (g_showAbout)
        return false;

	assert(NULL != g_definition);

    if (VK_DOWN==wp)
    {
        ScrollDefinition(1, scrollPage, false);
        return true;
    }

    if (VK_UP==wp)
    {
        ScrollDefinition(-1, scrollPage, false);
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
    HWND hwndExisting = FindWindow(APP_NAME, NULL);
    if (hwndExisting) 
    {
        // without | 0x01 we would block if MessageBox was displayed
        // I haven't seen it documented anywhere but official MSDN examples do that:
        // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/win_ce/html/pwc_ASmartphoneApplication.asp
        SetForegroundWindow ((HWND)(((DWORD)hwndExisting) | 0x01));    
        return 0;
    }
   
    if (!hPrevInstance)
    {
        if (!InitApplication(hInstance))
            return FALSE; 
    }

    if (!InitInstance(hInstance, CmdShow))
        return FALSE;

	PrepareStaticStyles();
	g_definition = new Definition();
	g_showAbout = true;

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCEL));
    MSG     msg;
    while (TRUE == GetMessage( &msg, NULL, 0,0 ))
    {
        if (!TranslateAccelerator(g_hwndMain, hAccel, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage(&msg);
        }
    }

    DeinitDataConnection();
    DeinitWinet();

	delete g_definition;
	g_definition = NULL;
	DisposeStaticStyles();

    return msg.wParam;
}

void ArsLexis::handleBadAlloc()
{
    RaiseException(1, 0, 0, NULL);    
}
