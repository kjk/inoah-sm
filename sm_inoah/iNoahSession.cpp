// iNoahSession.cpp: implementation of the iNoahSession class.
//
//////////////////////////////////////////////////////////////////////

#include "iNoahSession.h"
#include "Transmission.h"
#include <aygshell.h>
#include <tpcshell.h>
#include <winuserm.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



// TODO: It should be clear text(for a while just some objects)
// It's really intresting how they will be expanded

const ArsLexis::String server=TEXT("arslex.no-ip.info");

const ArsLexis::String errorStr=TEXT("ERROR");
const ArsLexis::String cookieStr=TEXT("COOKIE");
const ArsLexis::String messageStr=TEXT("MESSAGE");
const ArsLexis::String definitionStr=TEXT("DEF");
const ArsLexis::String wordListStr=TEXT("WORDLIST");

const ArsLexis::String script = TEXT("/palm.php?");
const ArsLexis::String protocolVersion = TEXT("pv=1");
const ArsLexis::String clientVersion = TEXT("cv=0.5");
const ArsLexis::String sep = TEXT("&");
const ArsLexis::String cookieRequest = TEXT("get_cookie");
const ArsLexis::String deviceInfoParam = TEXT("di=");

const ArsLexis::String cookieParam = TEXT("c=");
const ArsLexis::String randomRequest = TEXT("get_random_word");
const ArsLexis::String recentRequest = TEXT("recent_lookups");

const ArsLexis::String getWordParam = TEXT("get_word=");


iNoahSession::iNoahSession()
	: cookieReceived(false),
	  responseCode(error),
	  content(TEXT("No request."))
{

}

bool iNoahSession::checkErrors(Transmission &tr, ArsLexis::String &ret)
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

void iNoahSession::getRandomWord(ArsLexis::String& ret)
{
	if ((!cookieReceived)&&(getCookie()))
    {
			ret=content;
            return;
    }
    ArsLexis::String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
		cookieParam.length()+cookie.length()+sep.length()+
        randomRequest.length()); 
	tmp+=script;tmp+=protocolVersion;tmp+=sep; tmp+=clientVersion;
    tmp+=sep; tmp+=cookieParam; tmp+=cookie;
    tmp+=sep; tmp+=randomRequest;
    sendRequest(tmp,definitionStr,ret);
}

void iNoahSession::getWord(ArsLexis::String word, ArsLexis::String& ret)
{
	if ((!cookieReceived)&&(getCookie()))
    {
		ret=content;
        return;
    }
    ArsLexis::String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+sep.length()+
        clientVersion.length()+sep.length()+cookieParam.length()+
        cookie.length()+sep.length()+getWordParam.length()+word.length());
    
    tmp+=script; tmp+=protocolVersion; tmp+=sep;
    tmp+=clientVersion;tmp+=sep; tmp+=cookieParam;
    tmp+=cookie;tmp+=sep;tmp+=getWordParam;tmp+=word;
	
    sendRequest(tmp,definitionStr,ret);
}

void iNoahSession::getWordList(ArsLexis::String& ret )
{
	if ((!cookieReceived)&&(getCookie()))
    {
		ret=content;
        return;
    }
    ArsLexis::String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+sep.length()+
        clientVersion.length()+sep.length()+cookieParam.length()+
        cookie.length()+sep.length()+recentRequest.length());

    tmp+=script;tmp+=protocolVersion;tmp+=sep;tmp+=clientVersion;
    tmp+=sep; tmp+=cookieParam; tmp+=cookie;
    tmp+=sep;tmp+=recentRequest;
	sendRequest(tmp,wordListStr,ret);
}

void iNoahSession::sendRequest(ArsLexis::String url,
							ArsLexis::String answer,
                            ArsLexis::String& ret)
{

	Transmission tr(server, url);
	ArsLexis::String tmp;
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
	ArsLexis::String deviceInfo =deviceInfoParam + getDeviceInfo();
    ArsLexis::String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        deviceInfo.length()+sep.length()+cookieRequest.length());
	tmp+=script; tmp+=protocolVersion; tmp+=sep; tmp+=clientVersion;
    tmp+=sep; tmp+=deviceInfo; tmp+=sep; tmp+=cookieRequest;
    
    Transmission tr(server,tmp);
	ArsLexis::String tmp2;
	if(checkErrors(tr,tmp2)) return true;
	if ( tmp2.find(cookieStr) == 0 )
	{
		cookie.assign(tmp2,cookieStr.length()+1,-1);
		cookieReceived = true;
		return false;
	}

	content = TEXT("Bad answer received.");
	responseCode = error;
	return true;
}

ArsLexis::String iNoahSession::getDeviceInfo()
{
	return TEXT("DN6172736c657869735f73696d");
	// candidates to use: 
	// TSPI_providerInit
	// lineGetID
}

iNoahSession::~iNoahSession()
{
	
}
