#if !defined(_TRANSMISSION_H_)
#define _TRANSMISSION_H_

#include "BaseTypes.hpp"
#include "wininet.h"

using ArsLexis::String;

void DeinitConnection();
bool FInitConnection();

class Transmission
{
    static  HINTERNET   openInternet();
    DWORD               lastError_;
    String              url;
    String              host;
    INTERNET_PORT       port;
    HINTERNET           hIConnect_;
    HINTERNET           hIRequest_;
    DWORD               setError();

    String              content_;

public:

    Transmission(const ArsLexis::String& host, const INTERNET_PORT port, const  ArsLexis::String& url);

    virtual DWORD  sendRequest();
    virtual void   getResponse(ArsLexis::String& ret);
    static void    closeInternet();
    virtual        ~Transmission();

};

DWORD GetHttpBody(const String& host, const INTERNET_PORT port, const String& url, String& bodyOut);

#endif
