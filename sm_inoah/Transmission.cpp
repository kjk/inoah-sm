// Transmission.cpp: implementation of the Transmission class.
//
//////////////////////////////////////////////////////////////////////

#include "Transmission.h"
#include "winerror.h"
#include "tchar.h"
#include <BaseTypes.hpp>
#include "sm_inoah.h"

HINTERNET Transmission::hInternet = NULL;

Transmission::Transmission(
                           const ArsLexis::String& host,
                           INTERNET_PORT           port,
                           const ArsLexis::String& localInfo)
{
    if (!hInternet)
        lastError = openInternet();
    if (!hInternet)
        return;
    this->host = host;
    this->port = port;
    this->localInfo = localInfo;
    hIConnect_ = NULL;
    hIRequest_ = NULL;
}

DWORD Transmission::openInternet()
{
    hInternet = InternetOpen(TEXT("inoah-client"),
        INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    //INTERNET_FLAG_ASYNC in case of callback
    //InternetSetStatusCallback(hInternet, dispatchCallback); 
    if (hInternet)
        assert( 0==GetLastError() );
    return GetLastError();
}

void Transmission::closeInternet()
{
    if (hInternet)
    {
        InternetCloseHandle(hInternet);
        hInternet = NULL;
    }
}

DWORD Transmission::sendRequest()
{
    if (!hInternet)
        return lastError;

    hIConnect_ = InternetConnect(
        hInternet,host.c_str(),port,
        TEXT(" "),TEXT(" "),
        INTERNET_SERVICE_HTTP, 0, 0);
    
    if(!hIConnect_)
        return setError();
    //todo: No thread-safe change of static variable 
    //todo: need change in case of multithreaded enviroment
    //todo: check that HttpOpenRequest succeded
    //InternetSetStatusCallback(hIConnect, dispatchCallback);
    hIRequest_ = HttpOpenRequest(
        hIConnect_, NULL , localInfo.c_str(),
        NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    DWORD dwordLen=sizeof(DWORD);

    // add Host header so that it works with virtual hosting
    ArsLexis::String hostHdr = TEXT("Host: ");
    hostHdr.append( server );
    hostHdr.append( TEXT("\r\n") );
    if ( !HttpAddRequestHeaders(hIRequest_, hostHdr.c_str(), -1, HTTP_ADDREQ_FLAG_ADD_IF_NEW) )
        return setError();

    /*DWORD status;	
    HttpQueryInfo(hIRequest ,HTTP_QUERY_FLAG_NUMBER |
    HTTP_QUERY_STATUS_CODE , &status, &dwordLen, 0);
    DWORD size;
    HttpQueryInfo(hIRequest ,HTTP_QUERY_FLAG_NUMBER |
    HTTP_QUERY_CONTENT_LENGTH , &size, &dwordLen, 0);
    */
    BOOL succ;
    DWORD buffSize=2048;
    succ = InternetSetOption(hIRequest_, 
        INTERNET_OPTION_READ_BUFFER_SIZE, &buffSize, dwordLen);
        /*std::wstring fullURL(server+url+TEXT(" HTTP/1.0"));
        hIRequest = InternetOpenUrl(
        hInternet,
        fullURL.c_str(),
    NULL,0,0, context=transContextCnt++);*/

    /* todo: is it a bug? should we check succ instead? */
    if(!hIRequest_)
        return setError();
    //InternetSetStatusCallback(hIRequest, dispatchCallback);
    //if(!HttpSendRequestEx(hIRequest,NULL,NULL,HSR_ASYNC,context))
    if(!HttpSendRequest(hIRequest_,NULL,0,NULL,0))
        return setError();
    
    CHAR sbcsbuffer[255]; 
    TCHAR wcsbuffer[255];
    DWORD dwRead;
    content.clear();
    while( InternetReadFile( hIRequest_, sbcsbuffer, 255, &dwRead ) && dwRead)
    {
        sbcsbuffer[dwRead] = 0;
        _stprintf( wcsbuffer , TEXT("%hs"), sbcsbuffer);
        content += ArsLexis::String(wcsbuffer);
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
    ret=content;
}

DWORD Transmission::setError()
{    
    //LPVOID lpMsgBuf;
    lastError = GetLastError();
    /*FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    lastError,
    0, // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
    ); FUCK DOESN'T WORK AT ALL*/
    //content=ArsLexis::String((TCHAR*)lpMsgBuf);
    TCHAR buffer[20];
    memset(buffer,0,sizeof(buffer));
    _itow(lastError, buffer, 10);
    
    content.assign(TEXT("Network connection unavailable. iNoah cannot retrieve information. Error code:"));
    content+=buffer;
    if (ERROR_INTERNET_CANNOT_CONNECT==lastError)
    {
        content += TEXT(" (cannot connect to ");
        content += host;
        content += TEXT(":");
        memset(buffer,0,sizeof(buffer));
        _itow(port, buffer, 10);
        content += buffer;
        content += TEXT(")");
    }

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
    return lastError;
}

Transmission::~Transmission()
{
    
}
