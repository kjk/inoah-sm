// Device.cpp: implementation of the Device class.
//
//////////////////////////////////////////////////////////////////////

#include "TAPIDevice.h"
#include "sm_inoah.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////




HWND g_hwndDial = NULL;         // Handle to the dialing window
// Main window class name

//LINEINFO *TAPIDevice::g_lpLineInfo = NULL;  // Array that contains all the lines' 
//HCALL TAPIDevice::g_hCall = NULL;
//DWORD TAPIDevice::g_dwCurrentLineID=-1 ;   // Current line device identifier
//DWORD TAPIDevice::g_dwCurrentLineAddr=-1; // Current line address
//LONG TAPIDevice::g_MakeCallRequestID = 0;   // Request identifier returned by 
//LONG TAPIDevice::g_DropCallRequestID = 0;   // Request identifier returned by 
//BOOL TAPIDevice::g_bCurrentLineAvail = TRUE;// Indicates line availability

DWORD TAPIDevice::g_dwNumDevs = 0;          // Number of line devices available
HLINEAPP TAPIDevice::g_hLineApp = NULL;     // Application's use handle for TAPI

TAPIDevice* TAPIDevice::instance=NULL;

TAPIDevice::TAPIDevice() 
{ 
    DWORD dwLineID,
        dwReturn,
        dwTimeCount = GetTickCount ();
    
    //TCHAR szWarning[] = 
    //   TEXT("Cannot initialize the application's use\n") 
    //   TEXT("of tapi.dll. Quit all other telephony\n")
    //   TEXT("programs, and then try again.");
    
    // Initialize the application's use of Tapi.dll. Keep trying until the
    // user cancels or the application stops returning LINEERR_REINIT.
    while ( (dwReturn = lineInitialize (&g_hLineApp, 
        g_hInst, 
        (LINECALLBACK) TAPIDevice::lineCallbackFunc, 
        szAppName, 
        &g_dwNumDevs)) == LINEERR_REINIT)
    {
        // Display the message box if five seconds have passed.
        if (GetTickCount () > 5000 + dwTimeCount)
            break; 
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
    // Get the line data such as line name, permanent identifier,
    // and number of available addresses on the line by calling 
    // GetLineInfo discussed later in this chapter.
    for (dwLineID = 0; dwLineID < g_dwNumDevs; ++dwLineID)
        lines.push_back(new TAPILine(dwLineID, instance));    
    error = ERR_NONE;
    return ;
}

VOID FAR PASCAL TAPIDevice::lineCallbackFunc( 
    DWORD hDevice, DWORD dwMsg, 
    DWORD dwCallbackInstance, 
    DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
    
}
void TAPIDevice::initInstance()
{
    instance = new TAPIDevice();
}
