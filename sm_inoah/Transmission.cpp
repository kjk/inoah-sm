// those 3 must be in this sequence in order to get IID_DestNetInternet
// http://www.smartphonedn.com/forums/viewtopic.php?t=360
#include <objbase.h>
#include <initguid.h>
#include <connmgr.h>

#include "Transmission.h"
#include "winerror.h"
#include "tchar.h"
#include <BaseTypes.hpp>
#include "sm_inoah.h"

HINTERNET g_hInternet = NULL;
HANDLE    g_hConnection = NULL;

// try to establish internet connection.
// If can't (e.g. because tcp/ip stack is not working), display a dialog box
// informing about that and return false
// Return true if established connection.
// Can be called multiple times - will do nothing if connection is already established.
bool FInitConnection()
{
    if (NULL!=g_hConnection)
        return true;

    CONNMGR_CONNECTIONINFO ccInfo;
    memset(&ccInfo, 0, sizeof(CONNMGR_CONNECTIONINFO));
    ccInfo.cbSize      = sizeof(CONNMGR_CONNECTIONINFO);
    ccInfo.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
    ccInfo.dwFlags     = CONNMGR_FLAG_PROXY_HTTP;
    ccInfo.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
    ccInfo.guidDestNet = IID_DestNetInternet;
    
    DWORD dwStatus  = 0;
    DWORD dwTimeout = 5000;     // connection timeout: 5 seconds
    HRESULT res = ConnMgrEstablishConnectionSync(&ccInfo, &g_hConnection, dwTimeout, &dwStatus);

    if (FAILED(res))
    {
        assert(NULL==g_hConnection);
        g_hConnection = NULL;
    }

    if (NULL==g_hConnection)
    {
#ifdef DEBUG
        ArsLexis::String errorMsg = _T("Unable to connect to ");
        errorMsg += server;
#else
        ArsLexis::String errorMsg = _T("Unable to connect");
#endif
        errorMsg.append(_T(". Verify your dialup or proxy settings are correct, and try again."));
        MessageBox(
            g_hwndMain,
            errorMsg.c_str(),
            TEXT("Error"),
            MB_OK|MB_ICONERROR|MB_APPLMODAL|MB_SETFOREGROUND
            );
        return false;
    }
    else
        return true;
}

void DeinitConnection()
{
    if (NULL != g_hConnection)
    {
        ConnMgrReleaseConnection(g_hConnection,1);
        g_hConnection = NULL;
    }
}

Transmission::Transmission(
                           const ArsLexis::String& host,
                           INTERNET_PORT           port,
                           const ArsLexis::String& url)
{
    if (!g_hInternet)
    {
        g_hInternet = openInternet();
        if (!g_hInternet)
        {
            lastError_ = GetLastError();
            return;
        }
    }
    this->host = host;
    this->port = port;
    this->url  = url;
    hIConnect_ = NULL;
    hIRequest_ = NULL;
}

HINTERNET Transmission::openInternet()
{
    HINTERNET handle;
    handle = InternetOpen(TEXT("inoah-client"),
        INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    //INTERNET_FLAG_ASYNC in case of callback
    //InternetSetStatusCallback(g_hInternet, dispatchCallback); 

#ifdef DEBUG
    // a mystery: on emulator, GetLastError() returns 87 (ERROR_INVALID_PARAMETER)
    // even if we get a handle
    DWORD error;
    error = GetLastError();
#endif
    return handle;
}

void Transmission::closeInternet()
{
    if (g_hInternet)
    {
        InternetCloseHandle(g_hInternet);
        g_hInternet = NULL;
    }
}

DWORD Transmission::sendRequest()
{
    if (!g_hInternet)
        return lastError_;

    if (!FInitConnection())
    {
        // TODO: return more sensible error
        return 1;
    }

    hIConnect_ = InternetConnect(
        g_hInternet,host.c_str(),port,
        TEXT(" "),TEXT(" "),
        INTERNET_SERVICE_HTTP, 0, 0);
    
    if(!hIConnect_)
        return setError();
    //todo: No thread-safe change of static variable 
    //todo: need change in case of multithreaded enviroment
    //todo: check that HttpOpenRequest succeded
    //InternetSetStatusCallback(hIConnect, dispatchCallback);
    hIRequest_ = HttpOpenRequest(
        hIConnect_, NULL , url.c_str(),
        _T("HTTP/1.0"), NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    DWORD dwordLen=sizeof(DWORD);

    // add Host header so that it works with virtual hosting
    ArsLexis::String hostHdr = TEXT("Host: ");
    hostHdr.append( server );
    hostHdr.append( TEXT("\r\n") );
    if ( !HttpAddRequestHeaders(hIRequest_, hostHdr.c_str(), -1, HTTP_ADDREQ_FLAG_ADD_IF_NEW) )
        return setError();

    DWORD buffSize=2048;
    BOOL fOk = InternetSetOption(hIRequest_, 
        INTERNET_OPTION_READ_BUFFER_SIZE, &buffSize, dwordLen);
        /*std::wstring fullURL(server+url+TEXT(" HTTP/1.0"));
        hIRequest = InternetOpenUrl(
        g_hInternet,
        fullURL.c_str(),
    NULL,0,0, context=transContextCnt++);*/

    if (!fOk)
        return setError();
    //InternetSetStatusCallback(hIRequest, dispatchCallback);
    //if(!HttpSendRequestEx(hIRequest,NULL,NULL,HSR_ASYNC,context))
    if (!HttpSendRequest(hIRequest_,NULL,0,NULL,0))
        return setError();
    
    CHAR   sbcsbuffer[255]; 
    TCHAR  wcsbuffer[255];
    DWORD  dwRead;
    content_.clear();
    while (InternetReadFile( hIRequest_, sbcsbuffer, 255, &dwRead ) && (dwRead!=0))
    {
        sbcsbuffer[dwRead] = 0;
        _stprintf( wcsbuffer , TEXT("%hs"), sbcsbuffer);
        content_ += ArsLexis::String(wcsbuffer);
    };
    assert(NULL!=hIRequest_);
    InternetCloseHandle(hIRequest_);
    hIRequest_ = NULL;
    assert(NULL!=hIConnect_);
    InternetCloseHandle(hIConnect_);
    hIConnect_ = NULL;
    return NO_ERROR;
}

void Transmission::getResponse(ArsLexis::String& ret)
{
    ret=content_;
}

DWORD Transmission::setError()
{    
    //LPVOID lpMsgBuf;
    lastError_ = GetLastError();
    /*FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    lastError_,
    0, // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
    ); FUCK DOESN'T WORK AT ALL*/
    //content=ArsLexis::String((TCHAR*)lpMsgBuf);
    TCHAR buffer[20];
    memset(buffer,0,sizeof(buffer));
    _itow(lastError_, buffer, 10);
    
    content_.assign(TEXT("Network connection unavailable. iNoah cannot retrieve information. Error code:"));
    content_ += buffer;

    if (ERROR_INTERNET_CANNOT_CONNECT==lastError_)
    {
        content_ += TEXT(" (cannot connect to ");
        content_ += host;
        content_ += TEXT(":");
        memset(buffer,0,sizeof(buffer));
        _itow(port, buffer, 10);
        content_ += buffer;
        content_ += TEXT(")");
    }

    if (ERROR_INTERNET_NAME_NOT_RESOLVED==lastError_)
    {
        content_ += TEXT(" (can't resolve name ");
        content_ += host;
        memset(buffer,0,sizeof(buffer));
        _itow(port, buffer, 10);
        content_ += buffer;
        content_ += TEXT(")");
    }
    content_ += TEXT(":");

    //LocalFree( lpMsgBuf );
    if (hIConnect_)
    {
        InternetCloseHandle(hIConnect_);
        hIConnect_ = NULL;
    }
    if (hIRequest_)
    {
        InternetCloseHandle(hIRequest_);
        hIRequest_ = NULL;
    }
    return lastError_;
}

Transmission::~Transmission()
{
    
}

DWORD GetHttpBody(const String& host, const INTERNET_PORT port, const String& url, String& bodyOut)
{
    Transmission tr(host,port,url);
    DWORD err = tr.sendRequest();
    if (NO_ERROR != err)
        return err;
    tr.getResponse(bodyOut);
    return NO_ERROR;
}

