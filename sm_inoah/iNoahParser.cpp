#include "iNoahParser.h"
#include <DefinitionElement.hpp>
#include <DynamicNewLineElement.h>
#include <ParagraphElement.hpp>
#include <HorizontalLineElement.hpp>
#include <list>

using namespace ArsLexis;

// given a string str and curPos which is a valid index within str, return
// a substring from curPos until newline or end of string. Removes the newline
// from the string. Updates curPos so that it can be called in sequence.
// Sets fEnd to true if there are no more lines.
// Handles the following kinds of new lines: "\n", "\r", "\n\r", "\r\n"
String GetNextLine(const ArsLexis::String& str, String::size_type& curPos, bool& fEnd)
{
    fEnd = false;
    if (curPos==str.length())
    {
        fEnd = true;
        return String();
    }

    String::size_type lineStartPos = curPos;
    String::size_type lineEndPos;
    String::size_type delimPos   = str.find_first_of(_T("\n\r"), lineStartPos);

    if (String::npos == delimPos)
    {
        lineEndPos = str.length()-1;
        curPos = str.length();     
    }
    else
    {
        if (0==delimPos)
            lineEndPos = 0;
        else
            lineEndPos = delimPos;
        assert ( (_T('\n')!=str[lineEndPos]) && (_T('\r')!=str[lineEndPos]))

        curPos = delimPos+1;
        while ( (_T('\n')==str[curPos]) || (_T('\r')==str[curPos]))
        {
            curPos++;
        }
    }
    String::size_type lineLen = lineEndPos - lineStartPos;
    return str.substr(lineStartPos, lineLen);
}

const String iNoahParser::arabNums[] = 
{ 
    TEXT("I"), 
    TEXT("II"), 
    TEXT("III"), 
    TEXT("IV"), 
    TEXT("V")
};

const iNoahParser::pOfSpeechCnt = 5;
const int iNoahParser::abbrev = 0;
const int iNoahParser::fullForm = 1;

const String iNoahParser::pOfSpeach[2][5] = 
{ 
    {
        TEXT("v"), TEXT("a"), 
        TEXT("r"), TEXT("n"), 
        TEXT("s") 
    }, 
    {
        TEXT("verb "), TEXT("adj. "), 
        TEXT("adv. "), TEXT("noun "), 
        TEXT("adj. ")
    }
};

String getFullPosFromAbbrev(const String& posAbbrev)
{
    if (_T("v")==posAbbrev)
        return String(_T("verb "));

    if (_T("a")==posAbbrev)
        return String(_T("adj. "));

    if (_T("s")==posAbbrev)
        return String(_T("adj. "));

    if (_T("n")==posAbbrev)
        return String(_T("noun "));

    if (_T("r")==posAbbrev)
        return String(_T("adv. "));

    return String(_T("unknown "));
}

Definition* iNoahParser::parse(const String& text)
{
    int sepPos = text.find_first_of(char_t('\n'));

    String word(text.substr(0, sepPos-1));

    std::list<ElementsList*> sorted[pOfSpeechCnt];

    String txtRest(text.substr(sepPos+1,text.length()-sepPos-1));

    while (!txtRest.empty())
    {
        int     nextIdx = 0;
        char_t  currBeg = txtRest[0];
        String  metBegs;

        while (true)
        {
            char_t nextBeg;
            nextBeg=currBeg;
            do
            {
                nextIdx = txtRest.find_first_of(char_t('\n'), nextIdx+1);
                if ( (nextIdx != String::npos) && (txtRest.length() - nextIdx - 2 > 0 ))
                {
                    nextBeg = txtRest[nextIdx+1];
                }
            } while ( (nextBeg==currBeg) && (nextIdx != String::npos));

            if ( (String::npos == nextIdx) ||
                 (metBegs.find_first_of(nextBeg) != String::npos))
            {
                ElementsList* mean = new ElementsList();
                int pOfSpeech = 0;
                String txtToParse;
                if (nextIdx==-1)
                {
                    txtToParse.assign(txtRest.substr(0, txtRest.length() - 1));
                    txtRest.clear();
                }
                else
                {
                    txtToParse.assign(txtRest.substr(0, nextIdx));
                    txtRest = txtRest.substr(nextIdx + 1);
                }
                if (this->parseDefinitionList(
                    txtToParse,
                    word, pOfSpeech))
                {
                    if (explanation)
                        (*mean).push_back(explanation);
                    if (examples)
                        (*mean).merge(*examples);
                    if (synonyms)
                        (*mean).merge(*synonyms);
                }
                
                if (mean->size()>0)
                    sorted[pOfSpeech].push_back(mean);
                break;
            }
            metBegs += currBeg;
            currBeg = nextBeg;
        }
    }

    Definition* def=new Definition();
    if (NULL!=def)
        return NULL;
    ParagraphElement* parent=0;
    appendElement(parent=new ParagraphElement());
    DynamicNewLineElement *el=new DynamicNewLineElement(word);
    appendElement(el);
    el->setStyle(styleWord);
    el->setParent(parent);
    int cntPOS=0;

    for (int i=0;i<pOfSpeechCnt;i++)
    {
#ifdef HORLINES
        if (sorted[i].size() > 0)
            appendElement(new HorizontalLineElement());
#endif
        parent=0;
        int cntMeaning=1;
        if (sorted[i].size() > 0)
        {   
            DynamicNewLineElement *pofsl=new DynamicNewLineElement(arabNums[cntPOS] + TEXT(" "));
            DynamicNewLineElement *pofs=new DynamicNewLineElement(pOfSpeach[fullForm][i]);
            appendElement(parent=new ParagraphElement());
            appendElement(pofsl);
            appendElement(pofs);
            pofsl->setStyle(stylePOfSpeechList);
            pofs->setStyle(stylePOfSpeech);
            pofs->setParent(parent);
            pofsl->setParent(parent);
            cntPOS++;
            char_t buffer[20];
            int j=1;
            std::list<ElementsList*>::iterator iter;
            for (iter=sorted[i].begin();!(iter==sorted[i].end());iter++,j++ )
            {
                _itow( j, buffer, 10 );
                ArsLexis::String str(buffer);
                str+=TEXT(") ");
                DynamicNewLineElement* el=new DynamicNewLineElement(str);
                el->setStyle(styleDefinitionList);
                appendElement(el);
                //el->setParent(parent);
                (*iter)->merge(elements_);
            }
        }
    }
    def->replaceElements(elements_);
    return def;
}

bool iNoahParser::parseDefinitionList(String &text, String &word, int& partOfSpeach)
{
    delete examples;
    delete synonyms;
    delete explanation;

    explanation = NULL;
    examples    = NULL;
    synonyms    = NULL;
    
    int currIndx = 0;
    while (currIndx < static_cast<int>(text.length()))
    {
        int start = currIndx;
        char_t c = text[currIndx];
        currIndx = text.find_first_of(char_t('\n'), currIndx);
        if (currIndx == -1)
            currIndx = text.length();
        switch (c)
        {
        case '!' :
        case '#' :
            {
                while ((currIndx < static_cast<int>(text.length()) - 2)
                    && (currIndx != -1)
                    && (text[currIndx + 1] == c))
                    currIndx = text.find_first_of(char_t('\n'), currIndx + 1);
                if (currIndx == -1)
                    currIndx = text.length();
                // Create synonyms subtoken on the basis of
                if (c != '!')
                {
                    if(!(examples = this->parseExamplesList(
                        char_t('\n') + 
                        text.substr(start, currIndx - start))))
                        return false;
                }
                // Create examples subtoken on the basis of
                else
                {
                    if (!(synonyms = this->parseSynonymsList(
                        char_t('\n') + 
                        text.substr(start, currIndx - start),word)))
                        return false;
                }
                break;
            }
        case '@' :
            {
                // Create Explanation subtoken
                explanation =
                    new DynamicNewLineElement(text.substr(start + 1, 
                    currIndx-start-1)+TEXT(" "));
                break;
            }
        case '$' :
            { 
                if (start + 1 > static_cast<int>(text.length()))
                {
                    error.assign(TEXT("No part of speach description"));
                    return false;
                }
                String readAbbrev = text.substr(start + 1, currIndx-start-1);
                char i = 0;
                for (; i < pOfSpeechCnt; i++)
                {
                    if (pOfSpeach[abbrev][i].compare(readAbbrev) == 0)
                    {
                        partOfSpeach = (i == 4 ? 2 : i);
                        break;
                    }
                }
                
                if (i == pOfSpeechCnt)
                {
                    error.assign(TEXT("Part of speach badly defined."));
                    return false;
                }
                break;
            }
        }
        currIndx++;
    }
    if (partOfSpeach < 0)
    {
        error.assign(TEXT("Part of speach not defined."));
        return false;
    }
    if(explanation==NULL)
    {
        //throw new Exception("No explanation of meaning.");
        error.assign(TEXT("No explanation of meaning."));
        return false;
    }
    /*if(examples!=null)
    this.addSubToken(examples);
    if((synonyms!=null)&&(synonyms.getSynonymsCount()>0))
    this.addSubToken(synonyms);*/
    return true;
}

iNoahParser::ElementsList* iNoahParser::parseSynonymsList(String &text, String &word)
{
    int currIndx = 0;
    ElementsList* lst=new ElementsList();
    DynamicNewLineElement* last=NULL;
    
    while ((currIndx < static_cast<int>(text.length()) - 2)
        && (text[currIndx+1] == '!'))
    {
        int start = currIndx;
        currIndx = text.find_first_of(char_t('\n'), currIndx+1);
        if (currIndx == -1)
            currIndx = text.length();
        if(start+2>=currIndx)
            //throw new Exception("One of synonyms malformed");
        {
            delete lst;
            error.assign(TEXT("Synonym malformed."));
            return NULL;
        }
        
        String newSynonym=text.substr(start + 2, currIndx-start-2);
        if(word.compare(newSynonym)!=0)
        {
            if(last)
                last->setText(last->text()+TEXT(", "));
            last = new DynamicNewLineElement(newSynonym);
            last->setStyle(styleSynonyms);
            lst->push_back(last);
        }
    }
    if(last)
    {
        last->setText(last->text()+TEXT(" "));
        DynamicNewLineElement *el=new DynamicNewLineElement(String(TEXT("Synonyms: ")));
        el->setStyle(styleSynonymsList);
        lst->push_front(el);
        
    }
    return lst;
}

iNoahParser::ElementsList* iNoahParser::parseExamplesList(String &text)
{
    int currIndx = 0;
    ElementsList* lst=new ElementsList();
    DynamicNewLineElement* last=NULL;
    
    while ((currIndx < static_cast<int>(text.length()) - 2)
        && (text[currIndx+1] == '#'))
    {
        int start = currIndx;
        currIndx = text.find_first_of(char_t('\n'), currIndx+1);
        if (currIndx == -1)
            currIndx = text.length();
        if(start+2>=currIndx)
        {
            delete lst;
            error.assign(TEXT("Example malformed."));
            return NULL;
        }
        if(last!=NULL)
            last->setText(last->text()+TEXT(", "));
        last = new DynamicNewLineElement(
            TEXT("\"") + 
            text.substr(start + 2, currIndx-start-2) + 
            TEXT("\" "));
        last->setStyle(styleExample);
        lst->push_back(last);
    }
    if(last!=NULL)
    {
        last->setText(last->text()+TEXT("  "));
        DynamicNewLineElement *el=new DynamicNewLineElement(String(TEXT("Examples: ")));
        el->setStyle(styleExampleList);
        lst->push_front(el);
    }
    return lst;
}

void iNoahParser::appendElement(DefinitionElement* element)
{
    elements_.push_back(element);
}

iNoahParser::ElementsList::~ElementsList()
{
    std::for_each(lst.begin(), lst.end(), ObjectDeleter<DefinitionElement>());
}

void iNoahParser::ElementsList::push_back(DefinitionElement* el)
{
    lst.push_back(el);
}

void iNoahParser::ElementsList::push_front(DefinitionElement* el)
{
    lst.push_front(el);
}

Definition *parseDefinitionOld(ArsLexis::String& defTxt)
{
    iNoahParser  parser;
    Definition * parsedDef = parser.parse(defTxt);
    return parsedDef;
}

static bool FIsPartOfSpeech(const String& str)
{
    if ( (str.length()>0) && (_T('$')==str[0]) )
        return true;
    return false;
}

static bool FIsSynonim(const String& str)
{
    if ( (str.length()>0) && (_T('!')==str[0]) )
        return true;
    return false;
}

static bool FIsDef(const String& str)
{
    if ( (str.length()>0) && (_T('@')==str[0]) )
        return true;
    return false;
}

static bool FIsExample(const String& str)
{
    if ( (str.length()>0) && (_T('#')==str[0]) )
        return true;
    return false;
}

const char_t *pronTxt    = _T("PRON");
const char_t *reqLeftTxt = _T("REQUESTS_LEFT");

static bool FIsPron(const String& str)
{
    if ( 0==str.find(pronTxt) )
        return true;
    return false;
}

static bool FIsReqLeft(const String& str)
{
    if (0==str.find(reqLeftTxt))
        return true;
    return false;
}

typedef std::vector<String> StringVector_t;

void formatSynset(Definition::Elements_t& elements,
                  const String& posAbbrev, const String& synsetDef,
                  const StringVector_t& synonyms,
                  const StringVector_t& examples,
                  int synsetNo)
{
    assert( synsetNo>=1 );

    DynamicNewLineElement *dnlEl;

    ParagraphElement* synParagraph = new ParagraphElement();
    if (NULL == synParagraph)
        return;
    elements.push_back(synParagraph);


    /* ParagraphElement* posParagraph = new ParagraphElement();
    if (NULL == posParagraph)
        return;
    elements.push_back(posParagraph); */

    String posFull = getFullPosFromAbbrev(posAbbrev);
    dnlEl = new DynamicNewLineElement(posFull);
    if (NULL == dnlEl)
        return;
    dnlEl->setStyle(stylePOfSpeech);
    //dnlEl->setParent(posParagraph);
    dnlEl->setParent(synParagraph);
    elements.push_back(dnlEl);

/*    ParagraphElement* defParagraph = new ParagraphElement();
    if (NULL == defParagraph)
        return;
    elements.push_back(defParagraph);*/
    String synsetDefNew(synsetDef);
    synsetDefNew.append(_T(" "));
    dnlEl = new DynamicNewLineElement(synsetDefNew);
    if (NULL == dnlEl)
        return;
    dnlEl->setStyle(styleDefinition);
    //dnlEl->setParent(defParagraph);
    dnlEl->setParent(synParagraph);
    elements.push_back(dnlEl);

    uint_t synCount = synonyms.size();
    if (synCount>0)
    {
        /*ParagraphElement* synListParagraph = new ParagraphElement();
        if (NULL == synListParagraph)
            return;
        elements.push_back(synListParagraph);*/
        dnlEl = new DynamicNewLineElement(_T("Synonyms: "));
        if (NULL == dnlEl)
            return;
        dnlEl->setStyle(styleSynonymsList);
        //dnlEl->setParent(synListParagraph);
        dnlEl->setParent(synParagraph);
        elements.push_back(dnlEl);

        String syn;
        String synLine;
        for (uint_t i=0; i<synCount; i++)
        {
            syn = synonyms[i];
            synLine.append(syn);
            if (i!=synCount-1)
            {
                // not the last synonym
                synLine.append(_T(", "));
            }
            else
            {
                synLine.append(_T(" "));
            }
        }

        /* ParagraphElement* synParagraph = new ParagraphElement();
        if (NULL == synParagraph)
            return;
        elements.push_back(synParagraph); */
        
        dnlEl = new DynamicNewLineElement(synLine);
        if (NULL == dnlEl)
            return;
        dnlEl->setStyle(styleSynonyms);
        dnlEl->setParent(synParagraph);
        //dnlEl->setParent(synListParagraph);
        elements.push_back(dnlEl);
    }

    uint_t examplesCount = examples.size();
    if (examplesCount>0)
    {
        dnlEl = new DynamicNewLineElement(String(TEXT("Examples: ")));
        dnlEl->setStyle(styleExampleList);
        elements.push_back(dnlEl);

        String example;
        String exampleLine;
        for (uint_t i=0; i<examplesCount; i++)
        {
            example = examples[i];
            exampleLine.append(_T("\""));
            exampleLine.append(example);
            exampleLine.append(_T("\""));
            if (i!=examplesCount-1)
            {
                exampleLine.append(_T(", "));
            }
        }

        /*ParagraphElement* exParagraph = new ParagraphElement();
        if (NULL == exParagraph)
            return;
        elements.push_back(exParagraph);*/
        dnlEl = new DynamicNewLineElement(exampleLine);
        if (NULL == dnlEl)
            return;
        dnlEl->setStyle(styleExample);
        //dnlEl->setParent(exParagraph);
        dnlEl->setParent(synParagraph);
        elements.push_back(dnlEl);
    }
}

Definition *parseDefinition(const ArsLexis::String& defTxt)
{
    StringVector_t curExamples;
    StringVector_t curSynonyms;
    String         curSynsetDef;
    String         curPosAbbrev;
    int            curSynsetNo = 1;

    Definition::Elements_t elements;

    String syn;
    String example;

    bool    fNeedToAddSynset = false;
    bool    fEnd;
    String  line;
    String::size_type curPos = 0;

    // first in the definition is a word
    line = GetNextLine(defTxt,curPos,fEnd);
    if (fEnd)
        return NULL;

    String word = line;

    // currently not shown
    String pron;
    String requestsLeft;

    ParagraphElement* wordParagraph = new ParagraphElement();
    if (NULL == wordParagraph)
        return NULL;
    elements.push_back(wordParagraph);

    DynamicNewLineElement *dnlEl = new DynamicNewLineElement(word);
    if (NULL == dnlEl)
        return NULL;
    dnlEl->setStyle(styleWord);
    dnlEl->setParent(wordParagraph);
    elements.push_back(dnlEl);

    bool fAfterSynonyms = false;
    while (true)
    {
        line = GetNextLine(defTxt,curPos,fEnd);
        if (fEnd)
            break;

        if (FIsPron(line))
        {
            pron = line.substr(tstrlen(pronTxt)+1,-1);
        }
        else if (FIsReqLeft(line))
        {
            requestsLeft = line.substr(tstrlen(reqLeftTxt)+1,-1);
        }
        else if (FIsPartOfSpeech(line))
        {
            fAfterSynonyms = true;
            assert(2==line.len());
            curPosAbbrev = line.substr(1,1);
        }
        else if (FIsSynonim(line))
        {
            if (fAfterSynonyms)
            {
                formatSynset(elements, curPosAbbrev, curSynsetDef,
                    curSynonyms, curExamples, curSynsetNo);
                curPosAbbrev.clear();
                curSynsetDef.clear();
                curSynonyms.clear();
                curExamples.clear();
                ++curSynsetNo;

                fAfterSynonyms = false;
            }
            syn = line.substr(1,line.length()-1);
            if (0!=syn.compare(word))
            {
                curSynonyms.push_back(syn);
            }
        }
        else if (FIsDef(line))
        {
            fAfterSynonyms = true;
            curSynsetDef = line.substr(1,line.length()-1);
        }
        else if (FIsExample(line))
        {
            fAfterSynonyms = true;
            example = line.substr(1,line.length()-1);
            curExamples.push_back(example);
        }            
    }

    if (fAfterSynonyms)
    {
        formatSynset(elements, curPosAbbrev, curSynsetDef,
            curSynonyms, curExamples, curSynsetNo);
    }

    Definition* def=new Definition();
    if (NULL==def)
        return NULL;

    def->replaceElements(elements);
    return def;
}

