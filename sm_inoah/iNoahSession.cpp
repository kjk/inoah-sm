// iNoahSession.cpp: implementation of the iNoahSession class.
//
//////////////////////////////////////////////////////////////////////

#include "iNoahSession.h"
#include "Transmission.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



// TODO: It should be clear text(for a while just some objects)
// It's really intresting how they will be expanded

const ArsLexis::String server=TEXT("arslex.no-ip.info");

const ArsLexis::String errorStr=TEXT("ERROR\n");
const ArsLexis::String cookieStr=TEXT("COOKIE\n");
const ArsLexis::String messageStr=TEXT("MESSAGE\n");
const ArsLexis::String definitionStr=TEXT("DEF\n");
const ArsLexis::String wordListStr=TEXT("WORDLIST\n");

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
	  content(TEXT("No request in."))
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
	
	ret = tr.getResponse();

	// Check whether server returned errror
	if (ret.find_first_of(errorStr.c_str(), 0, errorStr.length()) == 0 )
	{
		content = ret.substr(errorStr.length());
		responseCode = srverror;
		return true;
	}
	
	if (ret.find_first_of(messageStr.c_str(), 0, messageStr.length()) == 0 )
	{
		content = ret.substr(errorStr.length());
		responseCode = srvmessage;
		return true;
	}
	return false;
}

ArsLexis::String iNoahSession::getRandomWord()
{
	if ((!cookieReceived)&&(getCookie()))
			return content;
	return sendRequest(script+protocolVersion+sep+clientVersion+sep+
		cookieParam+cookie+sep+randomRequest,definitionStr);
}

ArsLexis::String iNoahSession::getWord(ArsLexis::String word)
{
	if ((!cookieReceived)&&(getCookie()))
		return content;
	return sendRequest(script+protocolVersion+sep+clientVersion+sep+
		cookieParam+cookie+sep+getWordParam+word,definitionStr);
}

ArsLexis::String iNoahSession::getWordList()
{
	if ((!cookieReceived)&&(getCookie()))
		return content;
	return sendRequest(script+protocolVersion+sep+clientVersion+sep+
		cookieParam+cookie+sep+recentRequest,wordListStr);
}

ArsLexis::String iNoahSession::sendRequest(ArsLexis::String url,
												ArsLexis::String answer)
{

	Transmission tr(server, url);
	ArsLexis::String tmp = tr.getResponse();
	if(checkErrors(tr,tmp)) 
		return content;
	
	if (tmp.find(answer.c_str(), 0, answer.length()) == 0 )
	{
		content = tmp.substr(errorStr.length());
		return content;
	}
	return content;	
}

bool iNoahSession::getCookie()
{
	ArsLexis::String deviceInfo = deviceInfoParam + getDeviceInfo();
	Transmission tr(server,script+protocolVersion+sep+clientVersion+sep+
		deviceInfo +sep+cookieRequest);
	ArsLexis::String tmp;
	if(checkErrors(tr,tmp)) return true;

	if (tmp.find(cookieStr.c_str(), 0, cookieStr.length()) == 0 )
	{
		cookie = tmp.substr(errorStr.length()+1);
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
