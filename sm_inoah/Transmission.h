// Transmission.h: interface for the Transmission class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRANSMISSION_H__E386CB42_A1FC_4F99_A23C_2AFCA7E4CE36__INCLUDED_)
#define AFX_TRANSMISSION_H__E386CB42_A1FC_4F99_A23C_2AFCA7E4CE36__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseTypes.hpp"
#include "wininet.h"

const TCHAR HTTPver[]=TEXT("HTTP/1.0");

class Transmission
{
    static  HINTERNET   hInternet_;
    static  HINTERNET   openInternet();
    DWORD               lastError_;
    ArsLexis::String    localInfo;
    ArsLexis::String    host;
    INTERNET_PORT       port;
    HINTERNET           hIConnect_;
    HINTERNET           hIRequest_;
    DWORD               setError();

    ArsLexis::String    content;

public:

    Transmission(const ArsLexis::String& host, const INTERNET_PORT port, const  ArsLexis::String& localInfo);

    virtual DWORD  sendRequest();
    virtual void   getResponse(ArsLexis::String& ret);
    static void    closeInternet();
    virtual        ~Transmission();

};

#endif // !defined(AFX_TRANSMISSION_H__E386CB42_A1FC_4F99_A23C_2AFCA7E4CE36__INCLUDED_)
