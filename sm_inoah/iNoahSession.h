#if !defined(_INOAH_SESSION_H_)
#define _INOAH_SESSION_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseTypes.hpp"
#include "Transmission.h"

using ArsLexis::String;

class iNoahSession
{
    
public:
    enum ResponseCode
    {
        serverMessage,
        serverError,
        error,       // connection errors e.g. unable to connect, 
        definition,
        wordlist
    };
    
    iNoahSession();

    void  getRandomWord(String& ret);
    void  getWord(String word, String& ret);
    void  getWordList(String& ret);
    void  registerNoah(String registerCode, String& ret);

    String         getLastResponse() { return content_; }
    ResponseCode   getLastResponseCode() { return responseCode; }

    String  getDeviceInfo();
    void    clearCache();
    virtual ~iNoahSession();

private:
    bool    fCookieReceived_;
    void    storeCookie(String cookie);
    String  loadString(String fileName);
    void    iNoahSession::storeString(String fileName, String str);
    String  cookie;
    String  content_;
    void    sendRequest(String url, String answer, String& ret);
    bool    fErrorPresent(Transmission &tr, String &ret);
    int     CreateAppFolder(HWND hWnd, TCHAR *pszAppFolder, int nMax);
    bool    getCookie();

    ResponseCode responseCode;
};

#endif
