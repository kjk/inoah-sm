#ifndef INOAH_PARSER_H
#define INOAH_PARSER_H

#include "BaseTypes.hpp"
#include <DefinitionElement.hpp>
#include <list.h>

class iNoahParser
{
public:
    void parse(ArsLexis::String text);
private:
    ArsLexis::String definition;
    static const ArsLexis::String arabNums[];
	static const pOfSpeechCnt;
    static const int abbrev;
    static const int fullForm;
    static const ArsLexis::String pOfSpeach[2][5];
    list<DefinitionElement*>* parseDefinitionList(ArsLexis::String &text, ArsLexis::String &word, int& pOfSpeech);
    list<DefinitionElement*>* parseSynonymsList(ArsLexis::String &text, ArsLexis::String &word);
    list<DefinitionElement*>* parseExamplesList(ArsLexis::String &text);
};
#endif