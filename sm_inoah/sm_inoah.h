#ifndef _SM_INOAH_H__
#define _SM_INOAH_H__

#include <BaseTypes.hpp>
#include "resource.h"

extern HINSTANCE g_hInst;       // Local copy of hInstance
extern HWND      g_hwndMain;    // Handle to Main window returned from CreateWindow

void GetRegCode(String& code);
void GetCookie(String& cookie);

void   SetCookie(const String& cookie);

#define APP_NAME      _T("iNoah")
#define WINDOW_TITLE  _T("iNoah")

#define MENU_HEIGHT 26

//#define server     TEXT("arslex.no-ip.info")
//#define serverPort 80

//#define server     _T("dict-pc.arslexis.com")
//#define serverPort 4080

#define server     _T("dict.arslexis.com")
#define serverPort 80

//#define server     _T("dict-pc.local.org")
//#define serverPort 4080

enum FontSize {
	fontSizeSmall,
	fontSizeMedium,
	fontSizeLarge
};

int GetPrefFontSize();

int GetPrefLayoutType();

#endif
