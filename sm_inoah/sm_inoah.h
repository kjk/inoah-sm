#ifndef _SM_INOAH_H__
#define _SM_INOAH_H__

#include "resource.h"
#include "iNoahSession.h"
#include "BaseTypes.hpp"

extern HINSTANCE g_hInst;       // Local copy of hInstance
extern HWND      g_hwndMain;    // Handle to Main window returned from CreateWindow

extern ArsLexis::String g_recentWord;
extern ArsLexis::String g_wordList;
extern iNoahSession     g_session;

TCHAR szAppName[];
TCHAR szTitle[];

#define MENU_HEIGHT 26

#define KJK_BUILD 1
#ifdef KJK_BUILD
#define server     TEXT("dict-pc.arslexis.com")
#define serverPort 4080
//#define server     TEXT("dict.arslexis.com")
//#define serverPort 80
#else
//#define server     TEXT("arslex.no-ip.info")
//#define serverPort 80
#define server     TEXT("dict.arslexis.com")
#define serverPort 80
#endif

#endif
