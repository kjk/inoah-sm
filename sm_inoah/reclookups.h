#include "resource.h"
#include <windows.h>

BOOL CALLBACK RecentLookupsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
BOOL InitRecentLookups(HWND hDlg);
LRESULT CALLBACK ListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

