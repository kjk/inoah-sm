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

using ArsLexis::String;
using ArsLexis::char_t;

const char_t * errorStr        = _T("ERROR");
const char_t * cookieStr       = _T("COOKIE");
const char_t * messageStr      = _T("MESSAGE");
const char_t * definitionStr   = _T("DEF");
const char_t * wordListStr     = _T("WORDLIST");
// const char_t * registrationStr = _T("REGISTRATION");
const char_t * requestsLeftStr = _T("REQUESTS_LEFT");
const char_t * pronunciationStr= _T("PRON");
const char_t * regFailedStr    = _T("REGISTRATION_FAILED");
const char_t * regOkStr        = _T("REGISTRATION_OK");

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

// those are ids representing all fields that a server can possibly send
// must start with 0 and increase by 1 because we're using them as indexes
// to arrays.
enum eFieldId {
    fieldIdInvalid = -1,
    fieldIdFirst = 0,
    errorField  = fieldIdFirst,
    cookieField,
    messageField,
    definitionField,
    wordListField,
    requestsLeftField,
    pronField,
    registrationFailedField,
    registrationOkField,
    fieldsCount = registrationOkField + 1 // because we start from 0
};

enum eFieldType {
    fieldNoValue,     // field has no value e.g. "REGISTRATION_OK\n"
    fieldValueInline,  // field has arguments before new-line e.g. "REQUESTS_LEFT 5\n"
    fieldValueFollow,  // field has arguments after new-line e.g. "WORD\nhero\n"
    fieldValueFollowNoSep  // a special case for COOKIE where we don't put "\n" after field value
};

typedef struct _fieldDef {
    const char_t *  fieldName;
    eFieldType      fieldType;
} FieldDef;
    

// those must be in the same order as fieldId. They describe the format
// of each field
static FieldDef fieldsDef[fieldsCount] = {
    { errorStr, fieldValueFollow },
    { cookieStr, fieldValueFollowNoSep },
    { messageStr, fieldValueFollow },
    { definitionStr, fieldValueFollow },
    { wordListStr, fieldValueFollow },
    { requestsLeftStr, fieldValueInline },
    { pronunciationStr,fieldValueInline },
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
    if (fieldNoValue==fieldsDef[(int)fieldId].fieldType)
        return false;
    return true;
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

    
// this class does parsing of the response returned by iNoah server
// and a high-level interface for accessing various parts of this response.
class ServerResponseParser
{
public:
    ServerResponseParser(String &content);
    bool    fMalformed();
    bool    fHasValue(eFieldId fieldId);
    void    GetFieldValue(eFieldId fieldId, String& fieldValueOut);

private:
    void   ParseResponse();
    String _content;
    bool   _fParsed;    // did we parse the response already?
    bool   _fMalformed; // was the response malformed (having incorrect format)?

    // contains results of parsing. 3 values per each field. All values are
    // offsets into _content. First value is an offset of the field itself
    // - not strictly necessary. Second and third are beginning/end of 
    // field's value, without the separating new-line. String::npos marks
    // non-existing value (e.g. field or its value do not exist).
    String::size_type _fieldPos[3*fieldsCount];

    String::size_type GetFieldStart(eFieldId fieldId) { return _fieldPos[3*(int)fieldId]; }
    String::size_type GetFieldValueStart(eFieldId fieldId) { return _fieldPos[(3*(int)fieldId)+1]; }
    String::size_type GetFieldValueLen(eFieldId fieldId) { return _fieldPos[(3*(int)fieldId)+2]; }

    void SetFieldStart(eFieldId fieldId, String::size_type pos) { _fieldPos[3*(int)fieldId]=pos; }
    void SetFieldValueStart(eFieldId fieldId, String::size_type pos) { _fieldPos[(3*(int)fieldId)+1]=pos; }
    void SetFieldValueLen(eFieldId fieldId, String::size_type len) { _fieldPos[(3*(int)fieldId)+2]=len; }

};

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

bool ServerResponseParser::fHasValue(eFieldId fieldId)
{
    assert(_fParsed);
    if (String::npos==GetFieldStart(fieldId))
        return false;
        
    return true;
}

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

void ServerResponseParser::ParseResponse()
{
    if (_fParsed)
        return;

    String::size_type curPos = 0;
    String::size_type delimEndPos;
    eFieldId          fieldId;
    while (true)
    {
        fieldId = GetFieldId(_content,curPos);
        if (fieldIdInvalid==fieldId)
        {
            _fMalformed = true;
            break;
        }

        eFieldType fieldType = fieldsDef[(int)fieldId].fieldType;

        delimEndPos = _content.find(_T("\n"), curPos);
        if (String::npos == delimEndPos)
        {
            _fMalformed = true;
            break;
        }

        if (fieldNoValue==fieldType)
        {
            if (delimEndPos!=curPos+FieldStrLen(fieldId))
            {
                _fMalformed = true;
                break;
            }
            curPos = delimEndPos+1;
        } else if (fieldValueInline==fieldType)
        {
            String::size_type valueStart = curPos+FieldStrLen(fieldId);
            if ( _T(' ') != _content[valueStart])
            {
                _fMalformed = true;
                break;
            }

            valueStart += 1;
            String::size_type valueLen = delimEndPos-valueStart;
            if (0==valueLen)
            {
                _fMalformed = true;
                break;
            }
        }
        else if (fieldValueFollow==fieldType)
        {
            // TODO:
        }
        else if (fieldValueFollowNoSep==fieldType)
        {
            //TODO:
        }
#ifndef NDEBUG
        else
            assert(false);
#endif

        if (curPos == _content.length())
        {
            // this is the last character in the response, so we successfully
            // parsed the whole response
            break;
        }

    }
}

bool ServerResponseParser::fMalformed()
{
    ParseResponse();
    return _fMalformed;
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

/*    // Check whether server returned errror
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
    } */

    ServerResponseParser responseParser(ret); 

    if (responseParser.fMalformed())
    {
        tr.getResponse(content_); // TODO: does it make sense?
        responseCode = resultMalformed;
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

    //TODO: sendRequest(tmp,registrationStr,ret);
    if (ret.compare(TEXT("OK\n"))==0)
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
        cookie.assign(tmp2,tstrlen(cookieStr)+1,-1);
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
