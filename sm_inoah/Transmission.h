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
	static	HINTERNET	hInternet;
	static	DWORD		openInternet();	
	DWORD				lastError;
	ArsLexis::String	localInfo;
	ArsLexis::String	host;
	HINTERNET			hIConnect;
	HINTERNET			hIRequest;
	DWORD				setError();

	ArsLexis::String	content;

public:

	Transmission(const ArsLexis::String& host,const  ArsLexis::String& localInfo);
	
	virtual	DWORD  sendRequest();
	virtual void   getResponse(ArsLexis::String& ret);
	
	virtual ~Transmission();
	
};

#endif // !defined(AFX_TRANSMISSION_H__E386CB42_A1FC_4F99_A23C_2AFCA7E4CE36__INCLUDED_)
