#if !defined(_TRANSMISSION_H_)
#define _TRANSMISSION_H_

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

    ArsLexis::String    content_;

public:

    Transmission(const ArsLexis::String& host, const INTERNET_PORT port, const  ArsLexis::String& localInfo);

    virtual DWORD  sendRequest();
    virtual void   getResponse(ArsLexis::String& ret);
    static void    closeInternet();
    virtual        ~Transmission();

};

#endif
