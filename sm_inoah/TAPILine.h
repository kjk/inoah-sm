// TAPILine.h: interface for the TAPILine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TAPILINE_H__A0C91177_AD39_4D7E_82AE_5E7BDC24DABA__INCLUDED_)
#define AFX_TAPILINE_H__A0C91177_AD39_4D7E_82AE_5E7BDC24DABA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "BaseTypes.hpp"
#include <TAPI.h>
#include <ExTAPI.h>
#include <windows.h>

class TAPIDevice;
class TAPILine  
{
private:
    DWORD dwAPIVersion;
    LINEDEVCAPS* lpLineDevCaps;
    LINEGENERALINFO* lplineGeneralInfo;
    HLINE hLine;
    DWORD error;
public:
    TAPILine(DWORD lineID,TAPIDevice* device);
    
    virtual ~TAPILine();
    
};

#endif // !defined(AFX_TAPILINE_H__A0C91177_AD39_4D7E_82AE_5E7BDC24DABA__INCLUDED_)
