#ifndef INOAH_PARSER_H
#define INOAH_PARSER_H

#include "BaseTypes.hpp"
#include <DefinitionElement.hpp>
#include <GenericTextElement.hpp>
#include <list.h>

class iNoahParser
{
public:
    void parse(ArsLexis::String text);
private:
    class ElementsList
    {
        list<DefinitionElement*> lst;
    public:
        void push_back(DefinitionElement* el);        
        ~ElementsList();
        int size() {return lst.size();}
        void merge(ElementsList& r)
        {   
            lst.merge(r.lst);
        }
    };
    //ArsLexis::String definition;
    static const ArsLexis::String arabNums[];
	static const pOfSpeechCnt;
    static const int abbrev;
    static const int fullForm;
    static const ArsLexis::String pOfSpeach[2][5];
    
    DefinitionElement* explanation;
    ElementsList* examples;
    ElementsList* synonyms;

    ArsLexis::String error;
    bool parseDefinitionList(ArsLexis::String &text, ArsLexis::String &word, int& pOfSpeech);
    ElementsList* parseSynonymsList(ArsLexis::String &text, ArsLexis::String &word);
    ElementsList* parseExamplesList(ArsLexis::String &text);
};
#endif