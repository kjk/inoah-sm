// TAPILine.cpp: implementation of the TAPILine class.
//
//////////////////////////////////////////////////////////////////////

#include "TAPILine.h"
#include "TAPIDevice.h"
#include <ExTAPI.h>
#include <tsp.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


TAPILine::TAPILine(DWORD dwLineID, TAPIDevice* device)
{   
    DWORD dwSize;

    lpLineDevCaps = NULL;    
    
    error = ERR_NONE;
    
    // Negotiate the API version number. If it fails, return to dwReturn.
    if (error = lineNegotiateAPIVersion (
        device->gethLineApp(),        // TAPI registration handle
        dwLineID,                     // Line device to be queried
        TAPI_VERSION_1_0,             // Least recent API version 
        TAPI_CURRENT_VERSION,         // Most recent API version
        &(this->dwAPIVersion),        // Negotiated API version
        NULL))                        // Must be NULL; the provider-
                                      // specific extension is not 
                                      // supported by Windows CE.
        return;

    dwSize = sizeof (LINEDEVCAPS);
    
    // Allocate enough memory for lpLineDevCaps.
    do
    {
        if (!(lpLineDevCaps = (LPLINEDEVCAPS) LocalAlloc (LPTR, dwSize)))
        {   
            error = LINEERR_NOMEM;
            return;
        }
        
        lpLineDevCaps->dwTotalSize = dwSize;
        
        if (error = lineGetDevCaps (
            device->gethLineApp(),
            dwLineID,
            this->dwAPIVersion,
            0,
            lpLineDevCaps))

            return;    
        
        // Stop if the allocated memory is equal to or greater than the 
        // needed memory.
        if (lpLineDevCaps->dwNeededSize <= lpLineDevCaps->dwTotalSize)
            break;  
        
        dwSize = lpLineDevCaps->dwNeededSize;
        LocalFree (lpLineDevCaps);
        lpLineDevCaps = NULL;
        
    } while (TRUE);
               
    if((error = lineOpen(device->gethLineApp(), dwLineID,
        &(this->hLine), this->dwAPIVersion, 0, 
        dwLineID, LINECALLPRIVILEGE_NONE, LINEMEDIAMODE_DATAMODEM , 
        NULL)))
        return;
    
    dwSize = sizeof(LINEGENERALINFO);
    do
    {
        if (!(lplineGeneralInfo = 
            (LPLINEGENERALINFO) LocalAlloc (LPTR, dwSize)))
        {
            error = LINEERR_NOMEM;
            return;
        }

    
        lplineGeneralInfo->dwTotalSize=dwSize;
        
        if (error = lineGetGeneralInfo(this->hLine, lplineGeneralInfo))
            return;
        if (lplineGeneralInfo->dwNeededSize <= lplineGeneralInfo->dwTotalSize)
            break;  
        
        dwSize = lplineGeneralInfo->dwNeededSize;
        LocalFree (lplineGeneralInfo);
        lplineGeneralInfo = NULL;
    }while(TRUE);
}

TAPILine::~TAPILine()
{
    if (lpLineDevCaps) LocalFree (lpLineDevCaps);
    if (lplineGeneralInfo) LocalFree (lplineGeneralInfo);   
}
