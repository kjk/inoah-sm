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

#include <BaseTypes.hpp>
#include <Debug.hpp>

#ifndef WIN32_PLATFORM_PSPC
    #define STORE_FOLDER CSIDL_APPDATA
#else
    #define STORE_FOLDER CSIDL_PROGRAMS
#endif

extern String GetNextLine(const ArsLexis::String& str, String::size_type& curPos, bool& fEnd);

using ArsLexis::String;
using ArsLexis::char_t;

const char_t * errorStr        = _T("ERROR");
const char_t * messageStr      = _T("MESSAGE");

const char_t * cookieStr       = _T("COOKIE");

const char_t * wordListStr     = _T("WORDLIST");

const char_t * definitionStr   = _T("DEF");

//const char_t * pronunciationStr= _T("PRON");
//const char_t * requestsLeftStr = _T("REQUESTS_LEFT");

const char_t * regFailedStr    = _T("REGISTRATION_FAILED");
const char_t * regOkStr        = _T("REGISTRATION_OK");

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

const String iNoahFolder     = _T("\\iNoah");
const String cookieFile      = _T("\\Cookie");
const String regCodeFile     = _T("\\RegCode");

enum eFieldType {
    fieldNoValue,     // field has no value e.g. "REGISTRATION_OK\n"
    fieldValueInline,  // field has arguments before new-line e.g. "REQUESTS_LEFT 5\n"
    fieldValueFollow,  // field has arguments after new-line e.g. "WORD\nhero\n"
};

typedef struct _fieldDef {
    const char_t *  fieldName;
    eFieldType      fieldType;
} FieldDef;

// those must be in the same order as fieldId. They describe the format
// of each field
static FieldDef fieldsDef[fieldsCount] = {
    { errorStr, fieldValueFollow },
    { cookieStr, fieldValueFollow},
    { messageStr, fieldValueFollow },
    { definitionStr, fieldValueFollow },
    { wordListStr, fieldValueFollow },
//    { requestsLeftStr, fieldValueInline },
//    { pronunciationStr,fieldValueInline },
    { regFailedStr, fieldNoValue },
    { regOkStr, fieldNoValue }
};

static String::size_type FieldStrLen(eFieldId fieldId)
{
    const char_t *    fieldName = fieldsDef[(int)fieldId].fieldName;
    String::size_type fieldLen = tstrlen(fieldName);
    return fieldLen;
}

static bool fFieldHasValue(eFieldId fieldId)
{
    switch (fieldId)
    {
        case registrationFailedField:
        case registrationOkField:
            return false;
        default:
            return true;
    }
}

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

void SaveCookie(const String& cookie)
{
    SaveStringToFile(cookieFile,cookie);
}

void SaveRegCode(const String& regCode)
{
    SaveStringToFile(regCodeFile,regCode);
}

static String LoadStringFromFile(String fileName)
{
    TCHAR szPath[MAX_PATH];
    // It doesn't help to have a path longer than MAX_PATH
    BOOL fOk = SHGetSpecialFolderPath(g_hwndMain, szPath, STORE_FOLDER, FALSE);
    // Append directory separator character as needed
    if (!fOk)
        return _T("");
    
    String fullPath = szPath +  iNoahFolder + fileName;
    
    HANDLE handle = CreateFile(fullPath.c_str(), 
        GENERIC_READ, FILE_SHARE_READ, NULL, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
        NULL); 

    if (INVALID_HANDLE_VALUE==handle)
        return TEXT("");
    
    TCHAR  buf[254];
    DWORD  bytesRead;
    String ret;

    while (TRUE)
    {
        fOk = ReadFile(handle, &buf, sizeof(buf), &bytesRead, NULL);

        if (!fOk || (0==bytesRead))
            break;
        ret.append(buf, bytesRead/sizeof(TCHAR));
    }

    CloseHandle(handle);
    return ret;
}

ServerResponseParser::ServerResponseParser(String &content)
{
    _content    = content;
    _fParsed    = false;
    _fMalformed = false;

    for (int id=(int)fieldIdFirst; id<(int)fieldsCount; id++)
    {
        SetFieldStart((eFieldId)id,String::npos);
        SetFieldValueStart((eFieldId)id,String::npos);
        SetFieldValueLen((eFieldId)id,String::npos);
    }
}

// return true if this response contains a given field
bool ServerResponseParser::fHasField(eFieldId fieldId)
{
    assert(_fParsed);
    if (String::npos==GetFieldStart(fieldId))
        return false;
        
    return true;
}

// return a value for a given field
void ServerResponseParser::GetFieldValue(eFieldId fieldId, String& fieldValueOut)
{
    assert(_fParsed);
    if (String::npos==GetFieldStart(fieldId))
    {
        fieldValueOut.assign(_T(""));
        return;
    }

    if (!fFieldHasValue(fieldId))
    {
        assert(String::npos==GetFieldValueStart(fieldId));
        assert(String::npos==GetFieldValueLen(fieldId));
        fieldValueOut.assign(_T(""));
        return;
    }        

    assert(String::npos!=GetFieldValueStart(fieldId));
    assert(String::npos!=GetFieldValueLen(fieldId));

    fieldValueOut.assign(_content, GetFieldValueStart(fieldId), GetFieldValueLen(fieldId));
}

// given a string and a start position in that string, returned
// fieldId represented by that string at this position. Return
// field id or fieldIdInvalid if the string doesn't represent any
// known field.
// Just compare field strings with a given string starting with startPos
static eFieldId GetFieldId(String &str, String::size_type startPos)
{
    const char_t *      fieldStr;
    String::size_type   fieldStrLen;

    for (int id=(int)fieldIdFirst; id<(int)fieldsCount; id++)
    {
        fieldStr = fieldsDef[id].fieldName;
        fieldStrLen = tstrlen(fieldStr);
        if (0==str.compare(startPos, fieldStrLen, fieldStr))
        {
            return (eFieldId)id;
        }
    }
    return fieldIdInvalid;
}

// This is kind of broken.
// It looks for the end of the value that follows a field. Value can be multi-line so basically
// we look until the first line that looks like one of fields or the end of text.
String::size_type ServerResponseParser::GetFollowValueEnd(String::size_type fieldValueStart)
{
    String::size_type fieldValueEnd;
    String::size_type newLineStart;
    eFieldId          fieldId;

    String::size_type startLookingFrom = fieldValueStart;
    while (true)
    {
        fieldValueEnd = _content.find(_T("\n"), startLookingFrom);
        if (String::npos==fieldValueEnd)
        {
            fieldValueEnd = _content.length();
            break;        
        }

        newLineStart = fieldValueEnd + 1;
        fieldId = GetFieldId(_content, newLineStart);
        if (fieldId!=fieldIdInvalid)
            break;
        startLookingFrom = newLineStart;
    }
    return fieldValueEnd;
}

bool ServerResponseParser::FParseResponseIfNot()
{
    if (_fParsed)
        return !_fMalformed;

    bool fOk = FParseResponse();
    _fParsed = true;
    _fMalformed = !fOk;       
    return fOk;
}

// parse response. return true if everythings good, false if the response
// is malformed
bool ServerResponseParser::FParseResponse()
{
    String             line;
    bool               fEnd;
    String::size_type  curPos = 0;
    String::size_type  lineStartPos;

    assert( !_fParsed );

    eFieldId          fieldId;

    lineStartPos = curPos;
    line = GetNextLine(_content, curPos, fEnd);
    if (fEnd)
        return false;

    fieldId = GetFieldId(line,0);
    if (fieldIdInvalid==fieldId)
        return false;

    if ( (registrationFailedField==fieldId) || (registrationOkField==fieldId) )
    {
        SetFieldStart(fieldId, lineStartPos);
        // this should be the only thing in the line
        line = GetNextLine(_content, curPos, fEnd);
        if (!fEnd)
            return false;
    }

    String::size_type fieldValueStart = curPos;
    String::size_type fieldValueEnd;

    if ( (errorField==fieldId)  || (messageField==fieldId) ||
         (cookieField==fieldId) || (wordListField==fieldId) )
    {
        fieldValueEnd = GetFollowValueEnd(fieldValueStart);
    }
    else if (definitionField==fieldId)
    {
        // now we must have definition field, followed by word,
        // optional pronField, optional requestsLeftField and definition itself
        // we treat is as one thing
        fieldValueEnd = _content.length();
    }
    else
        return false;

    String::size_type fieldValueLen = fieldValueEnd - fieldValueStart;
    SetFieldStart(fieldId, lineStartPos);
    SetFieldValueStart(fieldId, fieldValueStart);
    SetFieldValueLen(fieldId, fieldValueLen);
    return true;
}

bool ServerResponseParser::fMalformed()
{    
    bool fOk = FParseResponseIfNot();
    assert(fOk==!_fMalformed);
    return _fMalformed;
}

static String BuildGetCookieUrl()
{
    String deviceInfo = getDeviceInfo();
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
    String url;
    url.reserve(urlCommonLen +
                cookieParam.length() +
                cookie.length() +
                sep.length() + 
                randomRequest.length());

    url.assign(urlCommon);
    url.append(cookieParam);
    url.append(cookie);
    url.append(sep);
    url.append(randomRequest);
    return url;
}

static String BuildGetWordListUrl(const String& cookie)
{    
    String url;
    url.reserve(urlCommonLen +
                cookieParam.length() +
                cookie.length() +
                sep.length() + 
                recentRequest.length());

    url.assign(urlCommon);
    url.append(cookieParam);
    url.append(cookie);
    url.append(sep);
    url.append(recentRequest);
    return url;
}

String g_regCode;

static String GetRegCode()
{
    if (!g_regCode.empty())
        return g_regCode;
    // TODO: this will always try to reg code from file if we don't
    // have reg code. Could avoid that by using additional flag
    g_regCode = LoadStringFromFile(regCodeFile);
    return g_regCode;
}

static String BuildGetWordUrl(const String& cookie, const String& word)
{
    String regCode = GetRegCode();
    if ( !regCode.empty() )
        regCode = sep+regCodeParam+regCode;

    String url;
    url.reserve(urlCommonLen +
                cookieParam.length() +
                cookie.length() +
                sep.length() + 
                getWordParam.length() +
                word.length() +
                regCode.length()                
                );

    url.assign(urlCommon);
    url.append(cookieParam);
    url.append(cookie);
    url.append(sep);
    url.append(getWordParam);
    url.append(word);
    url.append(regCode);
    return url;
}

static String BuildRegisterUrl(const String& cookie, const String& regCode)
{
    String url;
    url.reserve(urlCommonLen +
                cookieParam.length() +
                cookie.length() +
                sep.length() + 
                registerParam.length() +
                regCode.length()
                );

    url.assign(urlCommon);
    url.append(cookieParam);
    url.append(cookie);
    url.append(sep);
    url.append(registerParam);
    url.append(regCode);
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
}    

static void HandleMalformedResponse()
{
    MessageBox(g_hwndMain,
        _T("Server returned malformed response. Please contact support@arslexis.com if the problem persists."),
        _T("Error"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
}

static void HandleServerError(const String& errorStr)
{
    MessageBox(g_hwndMain,
        errorStr.c_str(),
        _T("Error"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
}

static void HandleServerMessage(const String& msg)
{
    MessageBox(g_hwndMain,
        msg.c_str(),
       _T("Information"), 
        MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND );
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

String g_cookie;

// Returns a cookie in cookieOut. If cookie is not present, it'll get it from
// the server. If there's a problem retrieving the cookie, it'll display
// appropriate error messages to the client and return false.
// Return true if cookie was succesfully obtained.
bool FGetCookie(String& cookieOut)
{
    if (!g_cookie.empty())
    {
        cookieOut = g_cookie;
        return true;
    }

    String storedCookie = LoadStringFromFile(cookieFile);
    if (!storedCookie.empty())
    {
        g_cookie = storedCookie;
        cookieOut = g_cookie;
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
    g_cookie = cookieOut;
    SaveCookie(cookieOut);
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

bool FGetWordList(String& wordListOut)
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

void ClearCache()
{
    TCHAR szPath[MAX_PATH];
    BOOL  fOk = SHGetSpecialFolderPath(g_hwndMain, szPath, STORE_FOLDER, FALSE);
    String fullPath = szPath + iNoahFolder;
    fullPath += cookieFile;
    fOk = DeleteFile(fullPath.c_str());
    fullPath = szPath + iNoahFolder + regCodeFile;
    fOk = DeleteFile(fullPath.c_str());
    g_cookie.clear();
    g_regCode.clear();
}

// According to this msdn info:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnppc2k/html/ppc_hpocket.asp
// 128 is enough for buffer size
#define INFO_BUF_SIZE 128
String getDeviceInfo()
{
    SMS_ADDRESS      address;
    ArsLexis::String regsInfo;
    ArsLexis::String text;
    TCHAR            buffer[INFO_BUF_SIZE];
    BOOL             fOk;

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

    fOk = SystemParametersInfo(SPI_GETOEMINFO, sizeof(buffer), buffer, 0);

    if (fOk)
    {
        if (text.length()>0)
            text += TEXT(":");
        text += TEXT("OC");
        stringAppendHexified(text,buffer);
    }

    memset(buffer,0,sizeof(buffer));
    fOk = SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(buffer), buffer, 0);

    if (fOk)
    {
        if (text.length()>0)
            text += TEXT(":");
        text += TEXT("OD");
        stringAppendHexified(text,buffer);
    }
    return text;
}

