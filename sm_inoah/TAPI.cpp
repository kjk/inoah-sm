// Device.cpp: implementation of the Device class.
//
//////////////////////////////////////////////////////////////////////

#include "TAPI.h"
#include "sm_inoah.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////




HWND g_hwndDial = NULL;         // Handle to the dialing window
// Main window class name

LINEINFO *Device::g_lpLineInfo = NULL;  // Array that contains all the lines' 

HCALL Device::g_hCall = NULL;
DWORD Device::g_dwNumDevs=0;          // Number of line devices available
DWORD Device::g_dwCurrentLineID=-1 ;   // Current line device identifier
DWORD Device::g_dwCurrentLineAddr=-1; // Current line address
HLINEAPP Device::g_hLineApp = NULL;     // Application's use handle for TAPI
LONG Device::g_MakeCallRequestID = 0;   // Request identifier returned by 
LONG Device::g_DropCallRequestID = 0;   // Request identifier returned by 
BOOL Device::g_bCurrentLineAvail = TRUE;// Indicates line availability


Device Device::instance = Device();

Device::Device() 
{ 
    DWORD dwLineID,
        dwReturn,
        dwTimeCount = GetTickCount ();
    
    TCHAR szWarning[] = 
        TEXT("Cannot initialize the application's use\n") 
        TEXT("of tapi.dll. Quit all other telephony\n")
        TEXT("programs, and then try again.");
    
    // Initialize the application's use of Tapi.dll. Keep trying until the
    // user cancels or the application stops returning LINEERR_REINIT.
    while ( (dwReturn = lineInitialize (&g_hLineApp, 
        g_hInst, 
        (LINECALLBACK) Device::lineCallbackFunc, 
        szAppName, 
        &g_dwNumDevs)) == LINEERR_REINIT)
    {
        // Display the message box if five seconds have passed.
        if (GetTickCount () > 5000 + dwTimeCount)
        //{
            //if (MessageBox (hwndMain, szWarning, TEXT("Warning"), 
             //   MB_OKCANCEL) == IDOK)
                break; 
            // Reset the time counter.
            //dwTimeCount = GetTickCount ();      
        //}  
    }
    
    // If the lineInitialize function fails, then return.
    if (dwReturn)
    {
        error = dwReturn;
        return ;
    }
    // If there is no device, then return.
    if (g_dwNumDevs == 0)
    {
        //ErrorBox (TEXT("There are no line devices available."));
        error = LINEERR_NODEVICE;
        return ;
    }
    
    // Allocate buffer for storing LINEINFO for all the available lines.
    if (! (g_lpLineInfo = (LPLINEINFO) LocalAlloc (
        LPTR, 
        sizeof (LINEINFO) * g_dwNumDevs)))
    {
        error = LINEERR_NOMEM;
        return ;
    }
    // Get the line data such as line name, permanent identifier,
    // and number of available addresses on the line by calling 
    // GetLineInfo discussed later in this chapter.
    for (dwLineID = 0; dwLineID < g_dwNumDevs; ++dwLineID)
    {
        GetLineInfo (dwLineID, g_lpLineInfo);
        //TODO: Create new line object on the basis of g_lpLineInfo 
        //structure
    }
    error = ERR_NONE;
    return ;
}

DWORD Device::GetLineInfo(DWORD dwLineID, LPLINEINFO lpLineInfo)
{
  DWORD dwSize,
        dwReturn;

  LPTSTR lpszLineName = NULL; 
  LPLINEDEVCAPS lpLineDevCaps = NULL;
  
  // Negotiate the API version number. If it fails, return to dwReturn.
  if (dwReturn = lineNegotiateAPIVersion (
        g_hLineApp,                   // TAPI registration handle
        dwLineID,                     // Line device to be queried
        TAPI_VERSION_1_0,             // Least recent API version 
        TAPI_CURRENT_VERSION,         // Most recent API version 
        &(lpLineInfo->dwAPIVersion),  // Negotiated API version 
        NULL))                        // Must be NULL; the provider-
                                      // specific extension is not 
                                      // supported by Windows CE.
  {
    goto exit;
  }

  dwSize = sizeof (LINEDEVCAPS);

  // Allocate enough memory for lpLineDevCaps.
  do
  {
    if (!(lpLineDevCaps = (LPLINEDEVCAPS) LocalAlloc (LPTR, dwSize)))
    {
      dwReturn = LINEERR_NOMEM;
      goto exit;
    }

    lpLineDevCaps->dwTotalSize = dwSize;

    if (dwReturn = lineGetDevCaps (g_hLineApp,
                                   dwLineID,
                                   lpLineInfo->dwAPIVersion,
                                   0,
                                   lpLineDevCaps))
    {
      goto exit;
    }

    // Stop if the allocated memory is equal to or greater than the 
    // needed memory.
    if (lpLineDevCaps->dwNeededSize <= lpLineDevCaps->dwTotalSize)
      break;  

    dwSize = lpLineDevCaps->dwNeededSize;
    LocalFree (lpLineDevCaps);
    lpLineDevCaps = NULL;
    
  } while (TRUE);

  // Store the line data in *lpLineInfo.
  lpLineInfo->dwPermanentLineID = lpLineDevCaps->dwPermanentLineID;
  lpLineInfo->dwNumOfAddress = lpLineDevCaps->dwNumAddresses;
  lpLineInfo->bVoiceLine = 
    (lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE);

  // Allocate memory for lpszLineName.
  if (!(lpszLineName = (LPTSTR) LocalAlloc (LPTR, 512)))
  {
    dwReturn = LINEERR_NOMEM;
    goto exit;
  }  
  
  // Store the line name in *lpszLineName.
  if (lpLineDevCaps->dwLineNameSize >= 512)
  {
    wcsncpy (
      lpszLineName, 
      (LPTSTR)((LPSTR)lpLineDevCaps + lpLineDevCaps->dwLineNameOffset),
      512);
  }
  else if (lpLineDevCaps->dwLineNameSize > 0)
  {
    wcsncpy (
      lpszLineName, 
      (LPTSTR)((LPSTR)lpLineDevCaps + lpLineDevCaps->dwLineNameOffset),
      lpLineDevCaps->dwLineNameSize);
  }
  else 
    wsprintf (lpszLineName, TEXT("Line %d"), dwLineID);

  // Copy lpszLineName to lpLineInfo->lpszLineName.
  lstrcpy (lpLineInfo->szLineName, lpszLineName);

  dwReturn = ERR_NONE;

exit:
    
  if (lpLineDevCaps)
    LocalFree (lpLineDevCaps);

  if (lpszLineName)
    LocalFree (lpszLineName);

  return dwReturn; 
}


VOID FAR PASCAL Device::lineCallbackFunc( 
    DWORD hDevice, DWORD dwMsg, 
    DWORD dwCallbackInstance, 
    DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
    
}
