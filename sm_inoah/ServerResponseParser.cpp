#include <Debug.hpp>
#include <Text.hpp>

#include "ServerResponseParser.hpp"

using namespace ArsLexis;

const char_t * errorStr        = _T("ERROR");
const char_t * messageStr      = _T("MESSAGE");

const char_t * cookieStr       = _T("COOKIE");

const char_t * wordListStr     = _T("WORDLIST");

const char_t * definitionStr   = _T("DEF");

const char_t * regFailedStr    = _T("REGISTRATION_FAILED");
const char_t * regOkStr        = _T("REGISTRATION_OK");

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
    eFieldId          fieldId;

    assert(!_fParsed);

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
        else
            return true;
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

