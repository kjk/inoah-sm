
#if !defined(AFX_SM_INOAH_H__7A9C1511_DE39_4376_A573_C6693074425B__INCLUDED_)
#define AFX_SM_INOAH_H__7A9C1511_DE39_4376_A573_C6693074425B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "iNoahSession.h"
#include "BaseTypes.hpp"

extern HINSTANCE g_hInst;  // Local copy of hInstance
extern HWND hwndMain;    // Handle to Main window returned from CreateWindow

extern ArsLexis::String recentWord;
extern ArsLexis::String wordList;
extern ArsLexis::String regCode;
extern iNoahSession session;

TCHAR szAppName[];
TCHAR szTitle[];

#define MENU_HEIGHT 26

#endif // !defined(AFX_SM_INOAH_H__7A9C1511_DE39_4376_A573_C6693074425B__INCLUDED_)
