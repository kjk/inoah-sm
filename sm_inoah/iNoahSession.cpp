// iNoahSession.cpp: implementation of the iNoahSession class.
//
//////////////////////////////////////////////////////////////////////

#include "iNoahSession.h"
#include "Transmission.h"
#include "sm_inoah.h"
#include <aygshell.h>
#ifndef PPC
#include <tpcshell.h>
#include <winuserm.h>
#endif
#include <shellapi.h>
#include <winbase.h>
#include <sms.h>

#ifndef PPC
    #define STORE_FOLDER CSIDL_APPDATA
#else
    #define STORE_FOLDER CSIDL_PROGRAMS
#endif

using ArsLexis::String;

// TODO: It should be clear text (for a while just some objects)
// It's really intresting how they will be expanded

const String errorStr        = TEXT("ERROR");
const String cookieStr       = TEXT("COOKIE");
const String messageStr      = TEXT("MESSAGE");
const String definitionStr   = TEXT("DEF");
const String wordListStr     = TEXT("WORDLIST");
const String registrationStr = TEXT("REGISTRATION");

const String script          = TEXT("/dict-2.php?");
const String protocolVersion = TEXT("pv=2");
const String clientVersion   = TEXT("cv=1.0");
const String sep             = TEXT("&");
const String cookieRequest   = TEXT("get_cookie");
const String deviceInfoParam = TEXT("di=");

const String cookieParam   = TEXT("c=");
const String registerParam = TEXT("register=");
const String regCodeParam  = TEXT("rc=");
const String getWordParam  = TEXT("get_word=");

const String randomRequest = TEXT("get_random_word");
const String recentRequest = TEXT("recent_lookups");

const String iNoahFolder = TEXT ("\\iNoah");
const String cookieFile  = TEXT ("\\Cookie");
const String regCodeFile = TEXT ("\\RegCode");


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
        tr.getResponse(content);
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

void iNoahSession::registerNoah(String registerCode, String& ret)
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
        registerParam.length()+registerCode.length());
    
    tmp+=script; tmp+=protocolVersion; tmp+=sep;
    tmp+=clientVersion;tmp+=sep; tmp+=cookieParam;
    tmp+=cookie;tmp+=sep;tmp+=registerParam;tmp+=registerCode;
    
    sendRequest(tmp,registrationStr,ret);
    if(ret.compare(TEXT("OK\n"))==0)
    {
        ret.assign(TEXT("Registration successful."));
        responseCode=srvmessage;
        storeString(regCodeFile,registerCode);
    }
    else
    {
        ret.assign(TEXT("Registration unsuccessful."));
        responseCode=srverror;
    }
}

void iNoahSession::getWord(String word, String& ret)
{
    if ((!cookieReceived)&&(getCookie()))
    {
        ret=content;
        return;
    }
    String rc=loadString(regCodeFile);
    String tmp;
    if(rc.compare(TEXT(""))!=0)
        rc=sep+regCodeParam+rc;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        cookieParam.length()+cookie.length()+sep.length()+
        getWordParam.length()+word.length()+
        rc.length());
    
    tmp+=script; tmp+=protocolVersion; tmp+=sep;
    tmp+=clientVersion;tmp+=sep; tmp+=cookieParam;
    tmp+=cookie;tmp+=sep;tmp+=getWordParam;tmp+=word;
    tmp+=rc;
    
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
    
    Transmission tr(server, serverPort, url);
    String tmp;
    tr.getResponse(tmp);
    if(checkErrors(tr,tmp))
        ; 
    else 
        if (tmp.find(answer, 0) == 0 )
        {
            content.assign(tmp,answer.length()+1,-1);
            this->responseCode=definition;
        }
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
    
    Transmission tr(server, serverPort, tmp);
    String tmp2;
    if(checkErrors(tr,tmp2))
        return true;
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
    return loadString(cookieFile);
}

void iNoahSession::storeCookie(String cookie)
{
    storeString(cookieFile,cookie);
}

String iNoahSession::loadString(String fileName)
{
    TCHAR szPath[MAX_PATH];
    // It doesn't help to have a path longer than MAX_PATH
    BOOL f = SHGetSpecialFolderPath(hwndMain, szPath, 
        STORE_FOLDER,
        FALSE);
    // Append directory separator character as needed
    
    String fullPath = szPath +  iNoahFolder + fileName;
    
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

void iNoahSession::storeString(String fileName, String str)
{
    TCHAR szPath[MAX_PATH];
    BOOL f = SHGetSpecialFolderPath(hwndMain, szPath, 
        STORE_FOLDER, 
        FALSE);
    String fullPath = szPath + iNoahFolder;
    CreateDirectory (fullPath.c_str(), NULL);    
    fullPath+=fileName;
    HANDLE fHandle = CreateFile(fullPath.c_str(), 
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL); 
    
    if(fHandle==INVALID_HANDLE_VALUE)
        return ;
    DWORD written;
    WriteFile(fHandle, str.c_str(), str.length()*2,&written, NULL);
    CloseHandle(fHandle);
}

TCHAR numToHex(TCHAR num)
{    
    if (num>=0 && num<=9)
        return TCHAR('0')+num;
    // assert (num<16);
    return TCHAR('A')+num-10;
}

// Append hexbinary-encoded string toHexify to the input/output string str
void stringAppendHexified(String& str, const String& toHexify)
{
    size_t toHexifyLen = toHexify.length();
    for(size_t i=0;i<toHexifyLen;i++)
    {
        str += numToHex((toHexify[i] & 0xF0) >> 4);
        str += numToHex(toHexify[i] & 0x0F);
    }
}

void iNoahSession::clearCache()
{
    TCHAR szPath[MAX_PATH];
    BOOL f = SHGetSpecialFolderPath(hwndMain, szPath, STORE_FOLDER, FALSE);
    String fullPath = szPath + iNoahFolder;
    CreateDirectory (fullPath.c_str(), NULL);
    fullPath+=cookieFile;
    f = DeleteFile(fullPath.c_str());
    fullPath = szPath + iNoahFolder + regCodeFile;
    f = DeleteFile(fullPath.c_str());
    cookieReceived = false;
}

// According to this msdn info:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnppc2k/html/ppc_hpocket.asp
// 128 is enough for buffer size
#define INFO_BUF_SIZE 128
String iNoahSession::getDeviceInfo()
{
    SMS_ADDRESS      address;
    ArsLexis::String regsInfo;
    ArsLexis::String text;
    TCHAR            buffer[INFO_BUF_SIZE];

    memset(&address,0,sizeof(SMS_ADDRESS));
    #ifndef PPC
    HRESULT res = SmsGetPhoneNumber(&address); 
    if (SUCCEEDED(res))
    {
        text.assign(TEXT("PN"));
        stringAppendHexified(text, address.ptsAddress);
    }
    #endif
    memset(&buffer,0,sizeof(buffer));
    if (SystemParametersInfo(SPI_GETOEMINFO, sizeof(buffer), buffer, 0))
    {
        if (text.length()>0)
            text+=TEXT(":");
        text+=TEXT("OC");
        stringAppendHexified(text,buffer);
    }

    memset(&buffer,0,sizeof(buffer));
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(buffer), buffer, 0))
    {
        if (text.length()>0)
            text+=TEXT(":");
        text+=TEXT("OD");
        stringAppendHexified(text,buffer);
    }
    return text;
}

iNoahSession::~iNoahSession()
{
    
}
