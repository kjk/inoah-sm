#include "iNoahSession.h"
#include "Transmission.h"
#include "sm_inoah.h"
#include <aygshell.h>
#ifndef WIN32_PLATFORM_PSPC
#include <tpcshell.h>
#include <winuserm.h>
#endif
#include <shellapi.h>
#include <winbase.h>
#include <sms.h>

#ifndef WIN32_PLATFORM_PSPC
    #define STORE_FOLDER CSIDL_APPDATA
#else
    #define STORE_FOLDER CSIDL_PROGRAMS
#endif

using ArsLexis::String;

const String errorStr        = TEXT("ERROR");
const String cookieStr       = TEXT("COOKIE");
const String messageStr      = TEXT("MESSAGE");
const String definitionStr   = TEXT("DEF");
const String wordListStr     = TEXT("WORDLIST");
const String registrationStr = TEXT("REGISTRATION");
const String requestsLeftStr = TEXT("REQUESTS_LEFT");
const String pronunciationStr= TEXT("PRON");
const String regFailedStr    = TEXT("REGISTRATION_FAILED");
const String regOkStr        = TEXT("REGISTRATION_OK");

const String script          = TEXT("/dict-2.php?");
const String protocolVersion = TEXT("pv=2");
const String clientVersion   = TEXT("cv=1.0");
const String sep             = TEXT("&");
const String cookieRequest   = TEXT("get_cookie=");
const String deviceInfoParam = TEXT("di=");

const String cookieParam     = TEXT("c=");
const String registerParam   = TEXT("register=");
const String regCodeParam    = TEXT("rc=");
const String getWordParam    = TEXT("get_word=");

const String randomRequest   = TEXT("get_random_word=");
const String recentRequest   = TEXT("recent_lookups=");

const String iNoahFolder     = TEXT ("\\iNoah");
const String cookieFile      = TEXT ("\\Cookie");
const String regCodeFile     = TEXT ("\\RegCode");

static void SaveStringToFile(const String& fileName, const String& str)
{
    TCHAR szPath[MAX_PATH];
    BOOL fOk = SHGetSpecialFolderPath(g_hwndMain, szPath, STORE_FOLDER, FALSE);
    if (!fOk)
        return;

    String fullPath = szPath + iNoahFolder;
    fOk = CreateDirectory (fullPath.c_str(), NULL);  
    if (!fOk)
        return;
    fullPath += fileName;

    HANDLE handle = CreateFile(fullPath.c_str(), 
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL); 
    
    if (INVALID_HANDLE_VALUE==handle)
        return;

    DWORD written;
    WriteFile(handle, str.c_str(), str.length()*sizeof(TCHAR), &written, NULL);
    CloseHandle(handle);
}



// this class does parsing of the response returned by iNoah server
// and a high-level interface for accessing various parts of this response.
class ServerResponseParser
{
public:
    ServerResponseParser(String &content);

private:
    String _content;
    bool   _fParsed;
};

ServerResponseParser::ServerResponseParser(String &content)
{
    _content = content;
    _fParsed = false;
}

iNoahSession::iNoahSession()
 : fCookieReceived_(false),
   responseCode(error),
   content_(TEXT("No request."))
{
    
}

// TODO: refactor things. This function does two unrelated things.
// return true if there was an error during transmission. Otherwise
// return false and stuff response into ret
bool iNoahSession::fErrorPresent(Transmission &tr, String &ret)
{
    if (NO_ERROR != tr.sendRequest())
    {
        //TODO: Better error handling
        responseCode = error;
        tr.getResponse(content_);
        return true;
    }
    
    tr.getResponse(ret);
    
    // Check whether server returned errror
    if (0==ret.find(errorStr, 0))
    {
        content_.assign(ret,errorStr.length()+1,-1);
        responseCode = serverError;
        return true;
    }
    
    if (0==ret.find(messageStr, 0))
    {
        content_.assign(ret,messageStr.length()+1,-1);
        responseCode = serverMessage;
        return true;
    }
    return false;
}

void iNoahSession::getRandomWord(String& ret)
{
    if ( !fCookieReceived_ && getCookie())
    {
        ret = content_;
        return;
    }
    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        cookieParam.length()+cookie.length()+sep.length()+
        randomRequest.length()); 
    tmp += script;
    tmp += protocolVersion;
    tmp += sep;
    tmp += clientVersion;
    tmp += sep;
    tmp += cookieParam;
    tmp += cookie;
    tmp += sep;
    tmp += randomRequest;

    sendRequest(tmp,definitionStr,ret);
}

void iNoahSession::registerNoah(String registerCode, String& ret)
{
    if (!fCookieReceived_ && getCookie())
    {
        ret = content_;
        return;
    }
    
    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        cookieParam.length()+cookie.length()+sep.length()+
        registerParam.length()+registerCode.length());
    
    tmp += script;
    tmp += protocolVersion; 
    tmp += sep;
    tmp += clientVersion;
    tmp += sep;
    tmp += cookieParam;
    tmp += cookie;
    tmp += sep;
    tmp += registerParam;
    tmp += registerCode;

    sendRequest(tmp,registrationStr,ret);
    if(ret.compare(TEXT("OK\n"))==0)
    {
        ret.assign(TEXT("Registration successful."));
        responseCode = serverMessage;
        SaveStringToFile(regCodeFile,registerCode);
    }
    else
    {
        ret.assign(TEXT("Registration unsuccessful."));
        responseCode = serverError;
    }
}

void iNoahSession::getWord(String word, String& ret)
{
    if ( !fCookieReceived_ && getCookie())
    {
        ret=content_;
        return;
    }

    String regCode = loadString(regCodeFile);

    if ( !regCode.empty() )
        regCode = sep+regCodeParam+regCode;

    String url;
    url.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        cookieParam.length()+cookie.length()+sep.length()+
        getWordParam.length()+word.length()+
        regCode.length());

    url += script;
    url += protocolVersion;
    url += sep;
    url += clientVersion;
    url += sep;
    url += cookieParam;
    url += cookie;
    url += sep;
    url += getWordParam;
    url += word;
    url += regCode;

    sendRequest(url, definitionStr, ret);
}

void iNoahSession::getWordList(String& ret )
{
    if (!fCookieReceived_ && getCookie())
    {
        ret=content_;
        return;
    }

    String tmp;
    tmp.reserve(script.length()+protocolVersion.length()+sep.length()+
        clientVersion.length()+sep.length()+cookieParam.length()+
        cookie.length()+sep.length()+recentRequest.length());
    
    tmp += script;
    tmp += protocolVersion;
    tmp += sep;
    tmp += clientVersion;
    tmp += sep;
    tmp += cookieParam;
    tmp += cookie;
    tmp += sep;
    tmp += recentRequest;

    sendRequest(tmp,wordListStr,ret);
}

void iNoahSession::sendRequest(String url, String answer, String& ret)
{
    Transmission tr(server, serverPort, url);

    String response;
    tr.getResponse(response);

    if (fErrorPresent(tr,response))
        ; 
    else
        if (0 == response.find(answer, 0))
        {
            content_.assign(response, answer.length()+1, -1);
            this->responseCode = definition;
        }
    ret = content_;
    return;
}

bool iNoahSession::getCookie()
{
    String storedCookie = loadString(cookieFile);

    if (storedCookie.length()>0)
    {
        cookie = storedCookie;
        fCookieReceived_ = true;
        return false;
    }

    String deviceInfo = deviceInfoParam + getDeviceInfo();
    String tmp;

    tmp.reserve(script.length()+protocolVersion.length()+
        sep.length()+clientVersion.length()+sep.length()+
        deviceInfo.length()+sep.length()+cookieRequest.length());

    tmp += script; 
    tmp += protocolVersion; 
    tmp += sep; 
    tmp += clientVersion;
    tmp += sep; 
    tmp += deviceInfo; 
    tmp += sep; 
    tmp += cookieRequest;
    
    Transmission tr(server, serverPort, tmp);
    String tmp2;

    if (fErrorPresent(tr,tmp2))
        return true;

    if (0==tmp2.find(cookieStr))
    {
        cookie.assign(tmp2,cookieStr.length()+1,-1);
        fCookieReceived_ = true;
        SaveStringToFile(cookieFile,cookie);
        return false;
    }
    
    content_ = TEXT("Bad answer received.");
    responseCode = error;
    return true;
}

String iNoahSession::loadString(String fileName)
{
    TCHAR szPath[MAX_PATH];
    // It doesn't help to have a path longer than MAX_PATH
    BOOL f = SHGetSpecialFolderPath(g_hwndMain, szPath, STORE_FOLDER, FALSE);
    // Append directory separator character as needed
    
    String fullPath = szPath +  iNoahFolder + fileName;
    
    HANDLE handle = CreateFile(fullPath.c_str(), 
        GENERIC_READ, FILE_SHARE_READ, NULL, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
        NULL); 

    if (INVALID_HANDLE_VALUE==handle)
        return TEXT("");
    
    TCHAR cookie[254];
    DWORD nBytesRead = -1;
    BOOL  bResult = 1;
    String ret;

    while (bResult && nBytesRead!=0)
    {
        bResult = ReadFile(handle, &cookie, sizeof(cookie), &nBytesRead, NULL);

        if (!bResult)
            break;
        ret.append(cookie, nBytesRead/sizeof(TCHAR));
    }   

    CloseHandle(handle);
    return ret;
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
    BOOL f = SHGetSpecialFolderPath(g_hwndMain, szPath, STORE_FOLDER, FALSE);
    String fullPath = szPath + iNoahFolder;
    CreateDirectory(fullPath.c_str(), NULL);
    fullPath += cookieFile;
    f = DeleteFile(fullPath.c_str());
    fullPath = szPath + iNoahFolder + regCodeFile;
    f = DeleteFile(fullPath.c_str());
    fCookieReceived_ = false;
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
#ifndef WIN32_PLATFORM_PSPC
    HRESULT res = SmsGetPhoneNumber(&address); 
    if (SUCCEEDED(res))
    {
        text.assign(TEXT("PN"));
        stringAppendHexified(text, address.ptsAddress);
    }
#endif

    memset(buffer,0,sizeof(buffer));
    if (SystemParametersInfo(SPI_GETOEMINFO, sizeof(buffer), buffer, 0))
    {
        if (text.length()>0)
            text += TEXT(":");
        text += TEXT("OC");
        stringAppendHexified(text,buffer);
    }

    memset(buffer,0,sizeof(buffer));
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(buffer), buffer, 0))
    {
        if (text.length()>0)
            text += TEXT(":");
        text += TEXT("OD");
        stringAppendHexified(text,buffer);
    }
    return text;
}

iNoahSession::~iNoahSession()
{
    
}
