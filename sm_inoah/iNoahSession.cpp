// iNoahSession.cpp: implementation of the iNoahSession class.
//
//////////////////////////////////////////////////////////////////////

#include "iNoahSession.h"
#include "Transmission.h"
#include "sm_inoah.h"
#include <ceutil.h>
#include <aygshell.h>
#include <tpcshell.h>
#include <winuserm.h>
#include <winbase.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
using ArsLexis::String;


// TODO: It should be clear text(for a while just some objects)
// It's really intresting how they will be expanded

const String server=TEXT("arslex.no-ip.info");

const String errorStr=TEXT("ERROR");
const String cookieStr=TEXT("COOKIE");
const String messageStr=TEXT("MESSAGE");
const String definitionStr=TEXT("DEF");
const String wordListStr=TEXT("WORDLIST");

const String script = TEXT("/palm.php?");
const String protocolVersion = TEXT("pv=1");
const String clientVersion = TEXT("cv=0.5");
const String sep = TEXT("&");
const String cookieRequest = TEXT("get_cookie");
const String deviceInfoParam = TEXT("di=");

const String cookieParam = TEXT("c=");
const String randomRequest = TEXT("get_random_word");
const String recentRequest = TEXT("recent_lookups");

const String getWordParam = TEXT("get_word=");

const String cookieFolder = TEXT ("\\iNoah");
const String cookieFile = TEXT ("\\Cookie");

iNoahSession::iNoahSession()
	: cookieReceived(false),
	  responseCode(error),
	  content(TEXT("No request."))
{

}

bool iNoahSession::checkErrors(Transmission &tr, String &ret)
{
	if(tr.sendRequest() != NO_ERROR)
	{
		//TODO: Better error handling
		responseCode = error;
		content = TEXT("Trsnsmission error.");
		return true;
	}
	
	tr.getResponse(ret);

	// Check whether server returned errror
	if (ret.find(errorStr, 0) == 0 )
	{
		content.assign(ret,errorStr.length()+1,-1);
		responseCode = srverror;
		return true;
	}
	
	if (ret.find(messageStr, 0) == 0 )
	{
		content.assign(ret,messageStr.length()+1,-1);
		responseCode = srvmessage;
		return true;
	}
	return false;
}

void iNoahSession::getRandomWord(String& ret)
{
	if ((!cookieReceived)&&(getCookie()))
    {
			ret=content;
            return;
    }
    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
		cookieParam.length()+cookie.length()+sep.length()+
        randomRequest.length()); 
	tmp+=script;tmp+=protocolVersion;tmp+=sep; tmp+=clientVersion;
    tmp+=sep; tmp+=cookieParam; tmp+=cookie;
    tmp+=sep; tmp+=randomRequest;
    sendRequest(tmp,definitionStr,ret);
}

void iNoahSession::getWord(String word, String& ret)
{
	if ((!cookieReceived)&&(getCookie()))
    {
		ret=content;
        return;
    }
    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+sep.length()+
        clientVersion.length()+sep.length()+cookieParam.length()+
        cookie.length()+sep.length()+getWordParam.length()+word.length());
    
    tmp+=script; tmp+=protocolVersion; tmp+=sep;
    tmp+=clientVersion;tmp+=sep; tmp+=cookieParam;
    tmp+=cookie;tmp+=sep;tmp+=getWordParam;tmp+=word;
	
    sendRequest(tmp,definitionStr,ret);
}

void iNoahSession::getWordList(String& ret )
{
	if ((!cookieReceived)&&(getCookie()))
    {
		ret=content;
        return;
    }
    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+sep.length()+
        clientVersion.length()+sep.length()+cookieParam.length()+
        cookie.length()+sep.length()+recentRequest.length());

    tmp+=script;tmp+=protocolVersion;tmp+=sep;tmp+=clientVersion;
    tmp+=sep; tmp+=cookieParam; tmp+=cookie;
    tmp+=sep;tmp+=recentRequest;
	sendRequest(tmp,wordListStr,ret);
}

void iNoahSession::sendRequest(String url,
							String answer,
                            String& ret)
{

	Transmission tr(server, url);
	String tmp;
    tr.getResponse(tmp);
	if(checkErrors(tr,tmp)) ; 
	else 
        if (tmp.find(answer, 0) == 0 )
		    content.assign(tmp,answer.length()+1,-1);
	ret=content;
    return;
}

bool iNoahSession::getCookie()
{
    String storedCookie=loadCookie();
    if (storedCookie.length())
    {
        cookie = storedCookie;
        cookieReceived = true;
        return false;
    }

	String deviceInfo =deviceInfoParam + getDeviceInfo();
    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        deviceInfo.length()+sep.length()+cookieRequest.length());
	tmp+=script; tmp+=protocolVersion; tmp+=sep; tmp+=clientVersion;
    tmp+=sep; tmp+=deviceInfo; tmp+=sep; tmp+=cookieRequest;
    
    Transmission tr(server,tmp);
	String tmp2;
	if(checkErrors(tr,tmp2)) return true;
	if ( tmp2.find(cookieStr) == 0 )
	{
		cookie.assign(tmp2,cookieStr.length()+1,-1);
		cookieReceived = true;
        storeCookie(cookie);
		return false;
	}

	content = TEXT("Bad answer received.");
	responseCode = error;
	return true;
}

String iNoahSession::loadCookie()
{
    TCHAR szPath[MAX_PATH];
    // It doesn't help to have a path longer than MAX_PATH
    BOOL f = SHGetSpecialFolderPath(hwndMain, szPath, CSIDL_APPDATA, FALSE);
    // Append directory separator character as needed
    
    String fullPath = szPath +  cookieFolder + cookieFile;

    HANDLE fHandle = CreateFile(fullPath.c_str(), 
        GENERIC_READ, FILE_SHARE_READ, NULL, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
        NULL); 
    
    if(fHandle==INVALID_HANDLE_VALUE)
        return TEXT("");
    
    TCHAR cookie[254];
    DWORD nBytesRead = -1;
    BOOL bResult = 1;
    String ret;
    while (bResult && nBytesRead!=0)
    {
        bResult = ReadFile(fHandle, &cookie, sizeof(TCHAR)*254, 
                            &nBytesRead, NULL);
        if (!bResult)
            break;
        ret.append(cookie, nBytesRead/2);
    }   
    CloseHandle(fHandle);
    return ret;
}

void iNoahSession::storeCookie(String cookie)
{
    TCHAR szPath[MAX_PATH];
    BOOL f = SHGetSpecialFolderPath(hwndMain, szPath, CSIDL_APPDATA, FALSE);
    String fullPath = szPath + cookieFolder;
    CreateDirectory (fullPath.c_str(), NULL);    
    fullPath+=cookieFile;
    HANDLE fHandle = CreateFile(fullPath.c_str(), 
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL); 
    
    if(fHandle==INVALID_HANDLE_VALUE)
        return ;
    DWORD written;
    WriteFile(fHandle, cookie.c_str(), cookie.length()*2,&written, NULL);
    CloseHandle(fHandle);
}

String iNoahSession::getDeviceInfo()
{
	return TEXT("SP6172736c657869735f73696d");
	// candidates to use: 
	// TSPI_providerInit
	// lineGetID
}

iNoahSession::~iNoahSession()
{
	
}
