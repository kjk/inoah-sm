#ifndef INOAH_PARSER_H
#define INOAH_PARSER_H

#include "BaseTypes.hpp"
#include <DefinitionElement.hpp>
#include "DynamicNewLineElement.h"
#include <list>
#include <algorithm>
class iNoahParser
{
public:
    iNoahParser()
        : explanation(NULL), examples(NULL), synonyms(NULL)
    {
    }
    Definition* parse(ArsLexis::String text);
    void appendElement(DefinitionElement* element);
private:
    class ElementsList
    {
        std::list<DefinitionElement*> lst;
    public:
        void push_back(DefinitionElement* el);
        void push_front(DefinitionElement* el);
        ~ElementsList();
        int size() {return lst.size();}
        void merge(ElementsList& r)
        {   
            std::list<DefinitionElement*>::iterator iter;
            for (iter=r.lst.begin();!(iter==r.lst.end());iter++ )
                std::back_inserter(this->lst) = *iter;
            r.lst.clear();
        }
        void merge(Definition::Elements_t& el)
        {
            std::list<DefinitionElement*>::iterator iter;
            for (iter=lst.begin();!(iter==lst.end());iter++)
                el.push_back(*iter);
            lst.clear();
        }
        
        
    };
    //ArsLexis::String definition;
    Definition::Elements_t elements_;
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
