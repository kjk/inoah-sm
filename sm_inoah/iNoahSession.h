// iNoahSession.h: interface for the iNoahSession class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INOAHTRANSMISSION_H__2E89FAF6_91F8_46A0_8AB5_7C2397EEC8DB__INCLUDED_)
#define AFX_INOAHTRANSMISSION_H__2E89FAF6_91F8_46A0_8AB5_7C2397EEC8DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\ipedia\src\BaseTypes.hpp"
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
	
	ArsLexis::String getRandomWord();
	ArsLexis::String getWord(ArsLexis::String word);
	ArsLexis::String getWordList();
	
	ArsLexis::String getLastResponse() { return content; }
	ResponseCode getLastResponseCode() { return responseCode; }
	
	ArsLexis::String getDeviceInfo();
	virtual ~iNoahSession();

private:
	bool cookieReceived;

	ArsLexis::String cookie;
	ArsLexis::String content;
	ArsLexis::String sendRequest(ArsLexis::String url,ArsLexis::String answer);
	bool checkErrors(Transmission &tr, ArsLexis::String &ret);
	
	bool getCookie();
	
	ResponseCode responseCode;
};

#endif // !defined(AFX_INOAHTRANSMISSION_H__2E89FAF6_91F8_46A0_8AB5_7C2397EEC8DB__INCLUDED_)
