// iNoahSession.h: interface for the iNoahSession class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INOAHTRANSMISSION_H__2E89FAF6_91F8_46A0_8AB5_7C2397EEC8DB__INCLUDED_)
#define AFX_INOAHTRANSMISSION_H__2E89FAF6_91F8_46A0_8AB5_7C2397EEC8DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseTypes.hpp"
#include "Transmission.h"

class iNoahSession
{
    
public:
    enum ResponseCode {
        srvmessage, //MESSAGE
            srverror,	//ERROR
            error,		//other errors: unable to connect, 
            //resolve name etc.
            definition, //DEFINITION
            wordlist,	//WORDLIST
    };
    
    iNoahSession();
    
    void getRandomWord(ArsLexis::String& ret);
    void getWord(ArsLexis::String word, ArsLexis::String& ret);
    void getWordList(ArsLexis::String& ret);
    void registerNoah(ArsLexis::String registerCode, ArsLexis::String& ret);

    ArsLexis::String getLastResponse() { return content; }
    ResponseCode getLastResponseCode() { return responseCode; }
    
    ArsLexis::String getDeviceInfo();
    virtual ~iNoahSession();
    
private:
    bool cookieReceived;
    void storeCookie(ArsLexis::String cookie);
    ArsLexis::String loadString(ArsLexis::String fileName);
    void iNoahSession::storeString(ArsLexis::String fileName, ArsLexis::String str);
    ArsLexis::String loadCookie();
    ArsLexis::String cookie;
    ArsLexis::String content;
    void sendRequest(ArsLexis::String url,ArsLexis::String answer,ArsLexis::String& ret);
    bool checkErrors(Transmission &tr, ArsLexis::String &ret);
    int CreateAppFolder (HWND hWnd, TCHAR *pszAppFolder, int nMax);
    bool getCookie();
    
    ResponseCode responseCode;
};

#endif // !defined(AFX_INOAHTRANSMISSION_H__2E89FAF6_91F8_46A0_8AB5_7C2397EEC8DB__INCLUDED_)
