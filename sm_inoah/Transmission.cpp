#include <BaseTypes.hpp>
#include <DeviceInfo.hpp>
#include <WinSysUtils.hpp>
#include "ServerResponseParser.hpp"
#include "sm_inoah.h"
#include "Transmission.hpp"

#define urlCommon    _T("/dict-2.php?pv=2&cv=1.0&")
#define urlCommonLen sizeof(urlCommon)/sizeof(urlCommon[0])

const String sep             = _T("&");
const String cookieRequest   = _T("get_cookie=");
const String deviceInfoParam = _T("di=");

const String cookieParam     = _T("c=");
const String registerParam   = _T("register=");
const String regCodeParam    = _T("rc=");
const String getWordParam    = _T("get_word=");

const String randomRequest   = _T("get_random_word=");
const String recentRequest   = _T("recent_lookups=");

HINTERNET g_hInternet = NULL;

// Init WinINet and return handle (or NULL if failed). Must call DeinitWinet()
// when done using it.
static HINTERNET InitWinet()
{
    if (NULL!=g_hInternet)
        return g_hInternet;

#ifdef WIN32_PLATFORM_WFSP
    g_hInternet = InternetOpen(_T("inoah-sm-client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
#else
    g_hInternet = InternetOpen(_T("inoah-ppc-client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
#endif

    return g_hInternet;
}

void DeinitWinet()
{
    if (g_hInternet)
    {
        InternetCloseHandle(g_hInternet);
        g_hInternet = NULL;
    }
}

DWORD GetHttpBody(const String& host, const INTERNET_PORT port, const String& url, String& bodyOut)
{
    String  hostHdr = _T("Host: ");
    DWORD   buffSize = 2048;
    BOOL    fOk;
    String  content;
    HANDLE  hConnect = NULL;
    HANDLE  hRequest = NULL;

    if (!InitDataConnection())
    {
#ifdef WIN32_PLATFORM_WFSP
        // return errConnectionFailed;
#else
        // just ignore on Pocket PC. I don't know how to make it work 
        // reliably across both Pocket PC and Pocket PC Phone Edition
#endif
    }

    if (NULL==InitWinet())
        goto Error;

    hConnect = InternetConnect(g_hInternet, host.c_str(), port, _T(" "),
        _T(" "), INTERNET_SERVICE_HTTP, 0, 0);
    if (NULL == hConnect)
        goto Error;

    hRequest = HttpOpenRequest( hConnect, NULL , url.c_str(), _T("HTTP/1.0"),
        NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE, 0);

    // add Host header so that it works with virtual hosting
    hostHdr.append( host );
    hostHdr.append( _T("\r\n") );

    fOk = HttpAddRequestHeaders(hRequest, hostHdr.c_str(), -1, HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    if (!fOk)
        goto Error;

    fOk = InternetSetOption(hRequest, INTERNET_OPTION_READ_BUFFER_SIZE, &buffSize, sizeof(buffSize));
    if (!fOk)
        goto Error;

    fOk = HttpSendRequest(hRequest, NULL, 0, NULL, 0);
    if (!fOk)
        goto Error;
    
    char    sbcsbuffer[255]; 
    WCHAR   wcsbuffer[255];
    DWORD   dwRead;

    while (InternetReadFile( hRequest, sbcsbuffer, 255, &dwRead ) && (dwRead!=0))
    {
        sbcsbuffer[dwRead] = 0;
        _stprintf( wcsbuffer , _T("%hs"), sbcsbuffer);
        content.append(String(wcsbuffer));
    }

    assert(NULL!=hRequest);
    InternetCloseHandle(hRequest);
    assert(NULL!=hConnect);
    InternetCloseHandle(hConnect);

    bodyOut.assign(content);
    return errNone;

Error:
    if (hRequest)
        InternetCloseHandle(hRequest);
    if (hConnect)
        InternetCloseHandle(hConnect);
    return GetLastError();
}

#define SIZE_FOR_REST_OF_URL 64
static String BuildCommonWithCookie(const String& cookie, const String& optionalOne=_T(""), const String& optionalTwo=_T(""))
{
    String url;
    url.reserve(urlCommonLen +
                cookieParam.length() + cookie.length() +
                sep.length() +
                optionalOne.length() +
                optionalTwo.length() +
                SIZE_FOR_REST_OF_URL);

    url.assign(urlCommon);
    url.append(cookieParam);
    url.append(cookie);
    url.append(sep);
    url.append(optionalOne);
    url.append(optionalTwo);
    return url;
}

static String BuildGetCookieUrl()
{
    String deviceInfo = deviceInfoToken();
    String url;
    url.reserve(urlCommonLen +
                cookieRequest.length() +
                sep.length() + 
                deviceInfoParam.length() +
                deviceInfo.length());

    url.assign(urlCommon);
    url.append(cookieRequest);
    url.append(sep); 
    url.append(deviceInfoParam);
    url.append(deviceInfo); 
    return url;
}

static String BuildGetRandomUrl(const String& cookie)
{    
    String url = BuildCommonWithCookie(cookie,randomRequest);
    return url;
}

static String BuildGetWordListUrl(const String& cookie)
{    
    String url = BuildCommonWithCookie(cookie,recentRequest);
    return url;
}

static String BuildGetWordUrl(const String& cookie, const String& word)
{
    String url = BuildCommonWithCookie(cookie,getWordParam,word);
    String regCode;
	GetRegCode(regCode);
    if (!regCode.empty())
    {
        url.append(sep);
        url.append(regCodeParam);
        url.append(regCode);
    }
    return url;
}

static String BuildRegisterUrl(const String& cookie, const String& regCode)
{
    String url = BuildCommonWithCookie(cookie,registerParam,regCode);
    return url;
}

static void HandleConnectionError(DWORD errorCode)
{
    if (errConnectionFailed==errorCode)
    {
#ifdef DEBUG
        ArsLexis::String errorMsg = _T("Unable to connect to ");
        errorMsg += server;
#else
        ArsLexis::String errorMsg = _T("Unable to connect");
#endif
        errorMsg.append(_T(". Verify your dialup or proxy settings are correct, and try again."));
        MessageBox(g_hwndMain,errorMsg.c_str(),
            TEXT("Error"), MB_OK | MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND );
    }
    else
    {
        // TODO: show errorCode in the message as well?
        MessageBox(g_hwndMain,
            _T("Connection error. Please contact support@arslexis.com if the problem persists."),
            _T("Error"), MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
    }
    InvalidateRect(g_hwndMain,NULL,TRUE);
}    

static void HandleMalformedResponse()
{
    MessageBox(g_hwndMain,
        _T("Server returned malformed response. Please contact support@arslexis.com if the problem persists."),
        _T("Error"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
    InvalidateRect(g_hwndMain,NULL,TRUE);
}

static void HandleServerError(const String& errorStr)
{
    MessageBox(g_hwndMain,
        errorStr.c_str(),
        _T("Error"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
    InvalidateRect(g_hwndMain,NULL,TRUE);
}

static void HandleServerMessage(const String& msg)
{
    MessageBox(g_hwndMain,
        msg.c_str(),
       _T("Information"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
    InvalidateRect(g_hwndMain,NULL,TRUE);
}

// handle common error cases for parsed message:
// * response returned from the server is malformed
// * server indicated error during processing
// * server sent a message instead of response
// Returns true if response is none of the above and we can proceed
// handling the message. Returns false if further processing should be aborted
bool FHandleParsedResponse(ServerResponseParser& responseParser)
{
    if (responseParser.fMalformed())
    {
        HandleMalformedResponse();
        return false;
    }

    if (responseParser.fHasField(errorField))
    {
        String errorStr;
        responseParser.GetFieldValue(errorField,errorStr);
        HandleServerError(errorStr);
        return false;
    }

    if (responseParser.fHasField(messageField))
    {
        String msg;
        responseParser.GetFieldValue(messageField,msg);
        HandleServerMessage(msg);
        return false;
    }

    return true;
}

// Returns a cookie in cookieOut. If cookie is not present, it'll get it from
// the server. If there's a problem retrieving the cookie, it'll display
// appropriate error messages to the client and return false.
// Return true if cookie was succesfully obtained.
bool FGetCookie(String& cookieOut)
{

    GetCookie(cookieOut);
    if (!cookieOut.empty())
    {
        return true;
    }

    String url = BuildGetCookieUrl();
    String response;
    DWORD  err = GetHttpBody(server,serverPort,url,response);
    if (errNone != err)
    {
        HandleConnectionError(err);
        return false;
    }

    ServerResponseParser responseParser(response);

    bool fOk = FHandleParsedResponse(responseParser);
    if (!fOk)
        return false;

    if (!responseParser.fHasField(cookieField))
    {
        HandleMalformedResponse();
        return false;
    }

    responseParser.GetFieldValue(cookieField,cookieOut);
    SetCookie(cookieOut);
    return true;
}

bool FGetRandomDef(String& defOut)
{
    String cookie;
    bool fOk = FGetCookie(cookie);
    if (!fOk)
        return false;

    String  url = BuildGetRandomUrl(cookie);
    String  response;
    DWORD err = GetHttpBody(server,serverPort,url,response);
    if (errNone != err)
    {
        HandleConnectionError(err);
        return false;
    }

    ServerResponseParser responseParser(response);

    fOk = FHandleParsedResponse(responseParser);
    if (!fOk)
        return false;

    if (!responseParser.fHasField(definitionField))
    {
        HandleMalformedResponse();
        return false;
    }

    responseParser.GetFieldValue(definitionField,defOut);
    return true;
}

bool FGetWord(const String& word, String& defOut)
{
    String cookie;
    bool fOk = FGetCookie(cookie);
    if (!fOk)
        return false;

    String  url = BuildGetWordUrl(cookie,word);
    String  response;
    DWORD err = GetHttpBody(server,serverPort,url,response);
    if (errNone != err)
    {
        HandleConnectionError(err);
        return false;
    }

    ServerResponseParser responseParser(response);

    fOk = FHandleParsedResponse(responseParser);
    if (!fOk)
        return false;

    if (!responseParser.fHasField(definitionField))
    {
        HandleMalformedResponse();
        return false;
    }

    responseParser.GetFieldValue(definitionField,defOut);
    return true;
}

bool FGetRecentLookups(String& wordListOut)
{
    String cookie;
    bool fOk = FGetCookie(cookie);
    if (!fOk)
        return false;

    String  url = BuildGetWordListUrl(cookie);
    String  response;
    DWORD err = GetHttpBody(server,serverPort,url,response);
    if (errNone != err)
    {
        HandleConnectionError(err);
        return false;
    }

    ServerResponseParser responseParser(response);

    fOk = FHandleParsedResponse(responseParser);
    if (!fOk)
        return false;

    if (!responseParser.fHasField(wordListField))
    {
        HandleMalformedResponse();
        return false;
    }

    responseParser.GetFieldValue(wordListField,wordListOut);
    return true;
}

// send the register request to the server to find out, if a registration
// code regCode is valid. Return false if there was a connaction error.
// If return true, fRegCodeValid indicates if regCode was valid or not
bool FCheckRegCode(const String& regCode, bool& fRegCodeValid)
{
    String cookie;
    bool fOk = FGetCookie(cookie);
    if (!fOk)
        return false;

    String  url = BuildRegisterUrl(cookie, regCode);
    String  response;
    DWORD err = GetHttpBody(server,serverPort,url,response);
    if (errNone != err)
    {
        HandleConnectionError(err);
        return false;
    }

    ServerResponseParser responseParser(response);

    fOk = FHandleParsedResponse(responseParser);
    if (!fOk)
        return false;

    if (responseParser.fHasField(registrationOkField))
    {
        fRegCodeValid = true;
    }
    else if (responseParser.fHasField(registrationFailedField))
    {
        fRegCodeValid = false;
    }
    else
    {
        HandleMalformedResponse();
        return false;
    }

    return true;
}


