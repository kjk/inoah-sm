#ifndef _INOAH_SESSION_H_
#define _INOAH_SESSION_H_

#include "BaseTypes.hpp"

using ArsLexis::String;

bool    FGetRandomDef(String& defOut);
bool    FGetWord(const String& word, String& defOut);
bool    FGetRecentLookups(String& wordListOut);
bool    FCheckRegCode(const String& regCode, bool& fRegCodeValid);

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
   // requestsLeftField,
   // pronField,
    registrationFailedField,
    registrationOkField,
    fieldsCount = registrationOkField + 1 // because we start from 0
};

// this class does parsing of the response returned by iNoah server
// and a high-level interface for accessing various parts of this response.
class ServerResponseParser
{
public:
    ServerResponseParser(String &content);
    bool    fMalformed();
    bool    fHasField(eFieldId fieldId);
    void    GetFieldValue(eFieldId fieldId, String& fieldValueOut);

private:
    bool   FParseResponseIfNot();
    bool   FParseResponse();
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

    String::size_type GetFollowValueEnd(String::size_type fieldValueStart);

    void SetFieldStart(eFieldId fieldId, String::size_type pos) { _fieldPos[3*(int)fieldId]=pos; }
    void SetFieldValueStart(eFieldId fieldId, String::size_type pos) { _fieldPos[(3*(int)fieldId)+1]=pos; }
    void SetFieldValueLen(eFieldId fieldId, String::size_type len) { _fieldPos[(3*(int)fieldId)+2]=len; }
};

#endif
