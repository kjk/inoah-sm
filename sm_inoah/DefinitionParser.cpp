#include <Text.hpp>

#include <DefinitionElement.hpp>
#include <DynamicNewLineElement.h>
#include <ParagraphElement.hpp>
#include <HorizontalLineElement.hpp>

#include "DefinitionParser.hpp"

using namespace ArsLexis;

const char_t *romanNumbers[] = 
{ 
    _T("I"), _T("II"), _T("III"), _T("IV"), _T("V"),
    _T("VI"), _T("VII"), _T("VIII"), _T("IX"), _T("X")
};

static String GetRomanNumber(int i)
{
    assert(i!=0);
    --i;
    if (i>(sizeof(romanNumbers)/sizeof(romanNumbers[0])))
        return _T("UNKNOWN");
    return romanNumbers[i];
}

#define POS_VERB _T("v")
#define POS_NOUN _T("n")
#define POS_ADJ  _T("a")
#define POS_ADV  _T("r")

static bool isPosVerb(const String& pos)
{
    if (POS_VERB==pos)
        return true;
    return false;
}

static bool isPosAdj(const String& pos)
{
    if (POS_ADJ==pos)
        return true;
    if (_T("s")==pos)
        return true;
    return false;
}

static bool isPosAdv(const String& pos)
{
    if (POS_ADV==pos)
        return true;
    return false;
}

static bool isPosNoun(const String& pos)
{
    if (POS_NOUN==pos)
        return true;
    return false;
}

static String getFullPosFromAbbrev(const String& posAbbrev)
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

static bool FAddDynamicLine(const String& txt, ElementStyle style, ParagraphElement *parent, Definition::Elements_t& elements)
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
        String example;
        String exampleLine;
        for (uint_t i=0; i<examplesCount; i++)
        {
            example = synsetDef.examples[i];
            exampleLine.assign(_T("\""));
            exampleLine.append(example);
            exampleLine.append(_T("\""));
            FAddDynamicLine(exampleLine,styleExample,parent,elements);
        }

    }
}

static void FormatSynsetVector(const SynsetDefVector_t& synsetDefVector, int posNo, const String& posAbbrev, Definition::Elements_t& elements)
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
static void FormatSynsets(const String& word, AllSynsetDefs_t allSynsets, Definition::Elements_t& elements)
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

static void AddSynsetDef(AllSynsetDefs_t& allSynsets, const String& posAbbrev, const SynsetDef_t& synset)
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

// how we work: first we collect all synsets grouped by part of speech
// for each pos we have a list of: definition text, list of examples, list of synonyms
Definition *ParseAndFormatDefinition(const String& defTxt, String& wordOut)
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

    wordOut.assign(word);
    return def;
}

