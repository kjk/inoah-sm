// Device.h: interface for the Device class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICE_H__A3A57524_5023_4224_8F95_8F37FAFD5E07__INCLUDED_)
#define AFX_DEVICE_H__A3A57524_5023_4224_8F95_8F37FAFD5E07__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "TAPIDevice.h"
#include "TAPILine.h"
#include "..\ipedia\src\BaseTypes.hpp"
#include <Tapi.h>
#include <list>

#define ERR_NONE              0
#define TAPI_VERSION_1_0      0x00010003
#define TAPI_VERSION_1_4      0x00010004
#define TAPI_VERSION_2_0      0x00020000



typedef struct tagLINEINFO
{
    HLINE hLine;              // Line handle returned by lineOpen
    BOOL  bVoiceLine;         // Indicates if the line is a voice line
    DWORD dwAPIVersion;       // API version that the line supports
    DWORD dwNumOfAddress;     // Number of available addresses on the line
    DWORD dwPermanentLineID;  // Permanent line identifier
    TCHAR szLineName[256];    // Name of the line
} LINEINFO, *LPLINEINFO;

class TAPIDevice  
{
public:
    ArsLexis::String getIMEI();
    static HLINEAPP& gethLineApp() {return g_hLineApp; }
    static void initInstance();
protected:
    TAPIDevice();
private:
    
    //static LINEINFO g_CurrentLineInfo; // Contains the current line information
    //static LINEINFO *g_lpLineInfo;     // Array that contains all the lines' 
    //static DWORD g_dwCurrentLineID ;   // Current line device identifier
    //static DWORD g_dwCurrentLineAddr;  // Current line address

    // (lineInitialize)
    //static HCALL g_hCall;       // Handle to the open line device on
    // which the call is to be originated 
    // (lineMakeCall)
    //static LONG g_MakeCallRequestID;   // Request identifier returned by 
    // lineMakeCall.
    //static LONG g_DropCallRequestID;   // Request identifier returned by 
    // lineDrop.
    //static BOOL g_bCurrentLineAvail ;// Indicates line availability
    //static TCHAR g_szCurrentNum[TAPIMAXDESTADDRESSSIZE + 1];    
    // Current phone number
    //static TCHAR g_szLastNum[TAPIMAXDESTADDRESSSIZE + 1];    

    static DWORD g_dwNumDevs;          // Number of line devices available
    static HLINEAPP g_hLineApp;        // Application's use handle for TAPI
    DWORD error;
    std::list<TAPILine*> lines;
    static TAPIDevice* instance;
    
    static VOID FAR PASCAL lineCallbackFunc( 
            DWORD hDevice, 
            DWORD dwMsg, 
            DWORD dwCallbackInstance, 
            DWORD dwParam1, 
            DWORD dwParam2, 
            DWORD dwParam3);

};

#endif // !defined(AFX_DEVICE_H__A3A57524_5023_4224_8F95_8F37FAFD5E07__INCLUDED_)
