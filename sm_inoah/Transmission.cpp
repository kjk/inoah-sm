// those 3 must be in this sequence in order to get IID_DestNetInternet
// http://www.smartphonedn.com/forums/viewtopic.php?t=360
#include <objbase.h>
#include <initguid.h>
#include <connmgr.h>

#include <BaseTypes.hpp>
#include "Transmission.h"

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
    ZeroMemory(&ccInfo,sizeof(ccInfo));
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
        return false;
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
    DWORD   buffSize=2048;
	BOOL    fOk;
    String  content;
    HANDLE  hConnect = NULL;
    HANDLE  hRequest = NULL;

    if (!FInitConnection())
        return errConnectionFailed;

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

