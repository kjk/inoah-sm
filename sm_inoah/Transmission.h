#if !defined(_TRANSMISSION_H_)
#define _TRANSMISSION_H_

#include "BaseTypes.hpp"
#include "wininet.h"

using ArsLexis::String;

class Transmission
{
    static  HINTERNET   hInternet_;
    static  HINTERNET   openInternet();
    DWORD               lastError_;
    String              localInfo;
    String              host;
    INTERNET_PORT       port;
    HINTERNET           hIConnect_;
    HINTERNET           hIRequest_;
    DWORD               setError();

    String              content_;

public:

    Transmission(const ArsLexis::String& host, const INTERNET_PORT port, const  ArsLexis::String& localInfo);

    virtual DWORD  sendRequest();
    virtual void   getResponse(ArsLexis::String& ret);
    static void    closeInternet();
    virtual        ~Transmission();

};

DWORD GetHttpBody(const String& host, const INTERNET_PORT port, const String& url, String& bodyOut);

#endif
