// Transmission.cpp: implementation of the Transmission class.
//
//////////////////////////////////////////////////////////////////////

#include "Transmission.h"
#include "winerror.h"
#include "tchar.h"
#include <BaseTypes.hpp>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HINTERNET Transmission::hInternet = NULL;

// TODO: Here port should be added
Transmission::Transmission(
                           const ArsLexis::String& host,
                           const ArsLexis::String& localInfo)
{
    if (!hInternet)
        lastError = openInternet();
    if (!hInternet)
        return;
    this->host = host;
    this->localInfo = localInfo;
}

DWORD Transmission::openInternet()
{
    hInternet = InternetOpen(TEXT("inoah-client"),
        INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    //INTERNET_FLAG_ASYNC in case of callback
    //InternetSetStatusCallback(hInternet, dispatchCallback); 
    return GetLastError();
}

DWORD Transmission::sendRequest()
{
    if (!hInternet)
        return lastError;
    
    HINTERNET hIConnect = InternetConnect(
        hInternet,host.c_str(),
        INTERNET_DEFAULT_HTTP_PORT,TEXT(" "),TEXT(" "),
        INTERNET_SERVICE_HTTP, 0, 0);
    
    if(!hIConnect)
        return setError();
    //todo: No thread-safe change of static variable 
    //todo: need change in case of multithreaded enviroment
    //InternetSetStatusCallback(hIConnect, dispatchCallback);
    hIRequest = HttpOpenRequest(
        hIConnect, NULL , localInfo.c_str(),
        NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    DWORD dwordLen=sizeof(DWORD);
    /*DWORD status;	
    HttpQueryInfo(hIRequest ,HTTP_QUERY_FLAG_NUMBER |
    HTTP_QUERY_STATUS_CODE , &status, &dwordLen, 0);
    DWORD size;
    HttpQueryInfo(hIRequest ,HTTP_QUERY_FLAG_NUMBER |
    HTTP_QUERY_CONTENT_LENGTH , &size, &dwordLen, 0);
    */
    BOOL succ = FALSE;
    DWORD buffSize=2048;
    succ = InternetSetOption(hIRequest, 
        INTERNET_OPTION_READ_BUFFER_SIZE , &buffSize, dwordLen);
        /*std::wstring fullURL(server+url+TEXT(" HTTP/1.0"));
        hIRequest = InternetOpenUrl(
        hInternet,
        fullURL.c_str(),
    NULL,0,0, context=transContextCnt++);*/
    if(!hIRequest)
        return setError();
    //InternetSetStatusCallback(hIRequest, dispatchCallback);
    //if(!HttpSendRequestEx(hIRequest,NULL,NULL,HSR_ASYNC,context))
    if(!HttpSendRequest(hIRequest,NULL,0,NULL,0))
        return setError();
    
    CHAR sbcsbuffer[255]; 
    TCHAR wcsbuffer[255];
    DWORD dwRead;
    content.clear();
    while( InternetReadFile( hIRequest, sbcsbuffer, 255, &dwRead )&& dwRead)
    {
        sbcsbuffer[dwRead] = 0;
        _stprintf( wcsbuffer , TEXT("%hs"), sbcsbuffer);
        content += ArsLexis::String(wcsbuffer);
    };
    InternetCloseHandle(hIRequest);
    InternetCloseHandle(hIConnect);
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
    _itow(lastError, buffer, 10 );
    
    content.assign(TEXT("Network connection unavailable. iNoah cannot retrieve information. Error code:"));
    content+=buffer;
    //LocalFree( lpMsgBuf );
    InternetCloseHandle(hIConnect);
    InternetCloseHandle(hIRequest);
    return lastError;
}

Transmission::~Transmission()
{
    
}
