#include <list>

#include <Text.hpp>

#include <DefinitionElement.hpp>
#include <DynamicNewLineElement.h>
#include <ParagraphElement.hpp>
#include <HorizontalLineElement.hpp>

#include "iNoahParser.h"

using namespace ArsLexis;

const char_t *romanNumbers[] = 
{ 
    _T("I"), _T("II"), _T("III"), _T("IV"), _T("V"),
    _T("VI"), _T("VII"), _T("VIII"), _T("IX"), _T("X")
};

String GetRomanNumber(int i)
{
    assert(i!=0);
    --i;
    if (i>(sizeof(romanNumbers)/sizeof(romanNumbers[0])))
        return _T("UNKNOWN");
    return romanNumbers[i];
}

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

#define POS_VERB _T("v")
#define POS_NOUN _T("n")
#define POS_ADJ  _T("a")
#define POS_ADV  _T("r")

bool isPosVerb(const String& pos)
{
    if (POS_VERB==pos)
        return true;
    return false;
}

bool isPosAdj(const String& pos)
{
    if (POS_ADJ==pos)
        return true;
    if (_T("s")==pos)
        return true;
    return false;
}

bool isPosAdv(const String& pos)
{
    if (POS_ADV==pos)
        return true;
    return false;
}

bool isPosNoun(const String& pos)
{
    if (POS_NOUN==pos)
        return true;
    return false;
}

String getFullPosFromAbbrev(const String& posAbbrev)
{
    if (isPosVerb(posAbbrev))
        return String(_T("verb "));

    if (isPosAdj(posAbbrev))
        return String(_T("adj. "));

    if (isPosNoun(posAbbrev))
        return String(_T("noun "));

    if (isPosAdv(posAbbrev))
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
            /*DynamicNewLineElement *pofsl=new DynamicNewLineElement(arabNums[cntPOS] + TEXT(" "));
            DynamicNewLineElement *pofs=new DynamicNewLineElement(pOfSpeach[fullForm][i]);
            appendElement(parent=new ParagraphElement());
            appendElement(pofsl);
            appendElement(pofs);
            pofsl->setStyle(stylePOfSpeechList);
            pofs->setStyle(stylePOfSpeech);
            pofs->setParent(parent);
            pofsl->setParent(parent);*/
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

static bool FIsPronunciation(const String& str)
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
typedef struct SynsetDef {
    String         definition;
    StringVector_t examples;
    StringVector_t synonyms;
    void clear()
    {
        definition.clear();
        examples.clear();
        synonyms.clear();
    }
} SynsetDef_t;

typedef std::vector<SynsetDef_t> SynsetDefVector_t;

typedef struct AllSynsetDefs {
    SynsetDefVector_t verb;
    SynsetDefVector_t noun;
    SynsetDefVector_t adj;
    SynsetDefVector_t adv;
} AllSynsetDefs_t;    

bool FAddDynamicLine(const String& txt, ElementStyle style, ParagraphElement *parent, Definition::Elements_t& elements)
{
    DynamicNewLineElement* dnlEl = new DynamicNewLineElement(txt);
    if (NULL == dnlEl)
        return false;
    dnlEl->setStyle(style);
    dnlEl->setParent(parent);
    elements.push_back(dnlEl);
    return true;
}

static void FormatSynsetDef(const SynsetDef_t& synsetDef, int synsetNo, ParagraphElement *parent, Definition::Elements_t& elements)
{
    char_t numberBuf[20];
    _itow(synsetNo, numberBuf, 10 );
    ArsLexis::String synsetNoTxt(numberBuf);
    synsetNoTxt.append(_T(") "));
    FAddDynamicLine(synsetNoTxt,styleDefinitionList,parent,elements);

    String synsetDefNew(synsetDef.definition);
    FAddDynamicLine(synsetDefNew,styleDefinition,parent,elements);

    uint_t synCount = synsetDef.synonyms.size();
    if (synCount>0)
    {
        FAddDynamicLine(_T("Synonyms: "),styleSynonymsList,parent,elements);

        String syn;
        String synLine;
        for (uint_t i=0; i<synCount; i++)
        {
            syn = synsetDef.synonyms[i];
            synLine.append(syn);
            if (i!=synCount-1)
            {
                // not the last synonym
                synLine.append(_T(", "));
            }
        }
        FAddDynamicLine(synLine,styleSynonyms,parent,elements);
    }

    uint_t examplesCount = synsetDef.examples.size();
    if (examplesCount>0)
    {
        FAddDynamicLine(_T("Examples: "),styleExampleList,parent,elements);

        String example;
        String exampleLine;
        for (uint_t i=0; i<examplesCount; i++)
        {
            example = synsetDef.examples[i];
            exampleLine.append(_T("\""));
            exampleLine.append(example);
            exampleLine.append(_T("\""));
            if (i!=examplesCount-1)
            {
                exampleLine.append(_T(", "));
            }
        }
        FAddDynamicLine(exampleLine,styleExample,parent,elements);
    }
}

void FormatSynsetVector(const SynsetDefVector_t& synsetDefVector, int posNo, const String& posAbbrev, Definition::Elements_t& elements)
{
    ParagraphElement *parent = new ParagraphElement();
    elements.push_back(parent);

    String posNumber = GetRomanNumber(posNo);
    posNumber.append(_T(" "));
    FAddDynamicLine(posNumber,stylePOfSpeechList,parent,elements);

    String posFull = getFullPosFromAbbrev(posAbbrev);
    FAddDynamicLine(posFull,stylePOfSpeech,parent,elements);

    assert(synsetDefVector.size()>0);

    for (size_t i=0; i<synsetDefVector.size(); i++)
    {
        FormatSynsetDef(synsetDefVector[i], i+1, parent, elements);
    }
}

// TODO: there will be mem leaks if run out of memory in the middle
void FormatSynsets(const String& word, AllSynsetDefs_t allSynsets, Definition::Elements_t& elements)
{
    int synsetNo = 1;

    ParagraphElement* wordParagraph = new ParagraphElement();
    if (NULL == wordParagraph)
        return;
    elements.push_back(wordParagraph);

    FAddDynamicLine(word,styleWord,wordParagraph,elements);

    if (allSynsets.verb.size()>0)
    {
        FormatSynsetVector(allSynsets.verb, synsetNo, POS_VERB, elements);
        ++synsetNo;
    }

    if (allSynsets.noun.size()>0)
    {
        FormatSynsetVector(allSynsets.noun, synsetNo, POS_NOUN, elements);
        ++synsetNo;
    }

    if (allSynsets.adj.size()>0)
    {
        FormatSynsetVector(allSynsets.adj, synsetNo, POS_ADJ, elements);
        ++synsetNo;
    }

    if (allSynsets.adv.size()>0)
    {
        FormatSynsetVector(allSynsets.adv, synsetNo, POS_ADV, elements);
        ++synsetNo;
    }
}

void AddSynsetDef(AllSynsetDefs_t& allSynsets, const String& posAbbrev, const SynsetDef_t& synset)
{
    if (isPosVerb(posAbbrev))
    {
        allSynsets.verb.push_back(synset);
    }
    else if (isPosNoun(posAbbrev))
    {
        allSynsets.noun.push_back(synset);
    }
    else if (isPosAdj(posAbbrev))
    {
        allSynsets.adj.push_back(synset);
    }
    else if (isPosAdv(posAbbrev))
    {
        allSynsets.adv.push_back(synset);
    }
#ifdef DEBUG
    else
        assert(false);
#endif
}

// how we work: first we collect all synsets grouped by part of speech i.e.:
// for each pos we have a list of: definition text, list of examples, list of synonyms
Definition *ParseAndFormatDefinition(const ArsLexis::String& defTxt)
{
    String          curPosAbbrev;
    SynsetDef_t     curSynset;
    AllSynsetDefs_t allSynsets;

    bool    fNeedToAddSynset = false;
    bool    fEnd;
    String  line;
    String::size_type curPos = 0;

    // first in the definition is a word
    line = GetNextLine(defTxt,curPos,fEnd);
    if (fEnd)
        return NULL;

    String word = line;

    // currently we don't show pronunciation or requestsLeft
    String pronunciation;
    String requestsLeft;

    bool fAfterPos = false;
    while (true)
    {
        line = GetNextLine(defTxt,curPos,fEnd);
        if (fEnd)
            break;

        if (FIsPronunciation(line))
        {
            pronunciation = line.substr(tstrlen(pronTxt)+1,-1);
        }
        else if (FIsReqLeft(line))
        {
            requestsLeft = line.substr(tstrlen(reqLeftTxt)+1,-1);
        }
        else if (FIsPartOfSpeech(line))
        {
            fAfterPos = true;
            assert(2==line.len());
            curPosAbbrev = line.substr(1,1);
        }
        else if (FIsSynonim(line))
        {
            if (fAfterPos)
            {
                AddSynsetDef(allSynsets, curPosAbbrev, curSynset);
                curSynset.clear();
                fAfterPos = false;
            }
            String syn = line.substr(1,line.length()-1);
            if (0!=syn.compare(word))
            {
                curSynset.synonyms.push_back(syn);
            }
        }
        else if (FIsDef(line))
        {
            fAfterPos = true;
            curSynset.definition = line.substr(1,line.length()-1);
        }
        else if (FIsExample(line))
        {
            fAfterPos = true;
            String example = line.substr(1,line.length()-1);
            curSynset.examples.push_back(example);
        }            
    }

    if (fAfterPos)
    {
        AddSynsetDef(allSynsets, curPosAbbrev, curSynset);
    }

    Definition::Elements_t elements;
    FormatSynsets(word, allSynsets, elements);

    Definition* def=new Definition();
    if (NULL==def)
        return NULL;
    def->replaceElements(elements);
    return def;
}

