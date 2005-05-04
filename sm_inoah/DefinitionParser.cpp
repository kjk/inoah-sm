#include <Text.hpp>

#include <DefinitionElement.hpp>
#include <DynamicNewLineElement.h>
#include <ParagraphElement.hpp>
#include <HorizontalLineElement.hpp>
#include <TextElement.hpp>

#include "DefinitionParser.hpp"

#include "iNoahStyles.hpp"

const char_t* romanNumbers[] = 
{ 
    _T("I"), _T("II"), _T("III"), _T("IV"), _T("V"),
    _T("VI"), _T("VII"), _T("VIII"), _T("IX"), _T("X")
};

static const char_t* GetRomanNumber(int i)
{
    assert(i > 0);
    --i;
    if (i > ARRAY_SIZE(romanNumbers))
        return _T("UNKNOWN");
    return romanNumbers[i];
}

#define POS_VERB _T("v")
#define POS_NOUN _T("n")
#define POS_ADJ  _T("a")
#define POS_ADV  _T("r")

static bool isPosVerb(const String& pos)
{
    if (POS_VERB == pos)
        return true;
    return false;
}

static bool isPosAdj(const String& pos)
{
    if (POS_ADJ == pos)
        return true;
    if (_T("s") == pos)
        return true;
    return false;
}

static bool isPosAdv(const String& pos)
{
    if (POS_ADV == pos)
        return true;
    return false;
}

static bool isPosNoun(const String& pos)
{
    if (POS_NOUN == pos)
        return true;
    return false;
}

static const char_t* getFullPosFromAbbrev(const String& posAbbrev)
{
    if (isPosVerb(posAbbrev))
        return _T("verb ");

    if (isPosAdj(posAbbrev))
        return _T("adj. ");

    if (isPosNoun(posAbbrev))
        return _T("noun ");

    if (isPosAdv(posAbbrev))
        return _T("adv. ");

    return _T("unknown ");
}

static bool FIsPartOfSpeech(const String& str)
{
    if (!str.empty() && _T('$') == str[0])
        return true;
    return false;
}

static bool FIsSynonim(const String& str)
{
    if (!str.empty() && _T('!') == str[0])
        return true;
    return false;
}

static bool FIsDef(const String& str)
{
    if (!str.empty() && _T('@') == str[0])
        return true;
    return false;
}

static bool FIsExample(const String& str)
{
    if (!str.empty() && _T('#') == str[0])
        return true;
    return false;
}

const char_t* pronTxt    = _T("PRON");
const char_t* reqLeftTxt = _T("REQUESTS_LEFT");

static bool FIsPronunciation(const String& str)
{
    if (StrStartsWith(str, pronTxt))
        return true;
    return false;
}

static bool FIsReqLeft(const String& str)
{
    if (StrStartsWith(str, reqLeftTxt))
        return true;
    return false;
}

typedef std::vector<String> StringVector_t;

struct SynsetDef {
    
	String         definition;
    StringVector_t examples;
    StringVector_t synonyms;
  
    void clear()
    {
        definition.clear();
        examples.clear();
        synonyms.clear();
    }

};

typedef std::vector<SynsetDef> SynsetDefVector_t;

struct AllSynsetDefs {
    SynsetDefVector_t verb;
    SynsetDefVector_t noun;
    SynsetDefVector_t adj;
    SynsetDefVector_t adv;
};    

static void AddDynamicLine(const String& txt, ElementStyle type, const char* style, ParagraphElement* parent, Definition::Elements_t& elements, bool newLine)
{
	DefinitionElement* el = new DynamicNewLineElement(type);
	elements.push_back(el);
	el->setParent(parent);
	el = new TextElement(txt);
	elements.push_back(el);
	el->setParent(parent);
	if (NULL != style)
		el->setStyle(StyleGetStaticStyle(style));

	if (!newLine)
		return;

	el = new LineBreakElement();
	el->setParent(parent);
	elements.push_back(el);
}

static void FormatSynsetDef(const SynsetDef& synsetDef, int synsetNo, ParagraphElement* parent, Definition::Elements_t& elements)
{
    char_t numberBuf[20];
    _itot(synsetNo, numberBuf, 10);

    String synsetNoTxt(numberBuf);
    synsetNoTxt += _T(") ");
    AddDynamicLine(synsetNoTxt, styleDefinitionList, styleNameDefinitionList, parent, elements, false);

    String synsetDefNew(synsetDef.definition);
    AddDynamicLine(synsetDefNew, styleDefinition, styleNameDefinition, parent, elements, true);

    uint_t synCount = synsetDef.synonyms.size();
    if (synCount>0)
    {
        AddDynamicLine(_T("Synonyms: "), styleSynonymsList, styleNameSynonymsList, parent, elements, false);
        String synLine;
        for (uint_t i = 0; i<synCount; i++)
        {
            synLine += synsetDef.synonyms[i];
            if (i != synCount - 1)
                synLine += _T(", ");
        }
        AddDynamicLine(synLine, styleSynonyms, styleNameSynonyms, parent, elements, true);
    }

    uint_t examplesCount = synsetDef.examples.size();
    if (examplesCount > 0)
    {
        for (uint_t i = 0; i < examplesCount; i++)
        {
            String exampleLine = _T("\"");
            exampleLine += synsetDef.examples[i];
            exampleLine += _T('\"');
            AddDynamicLine(exampleLine, styleExample, styleNameExample, parent, elements, true);
        }
    }
}

static void FormatSynsetVector(const SynsetDefVector_t& synsetDefVector, int posNo, const String& posAbbrev, Definition::Elements_t& elements)
{
    ParagraphElement* parent = new ParagraphElement();
    elements.push_back(parent);

    String posNumber = GetRomanNumber(posNo);
    posNumber += _T(' ');
    AddDynamicLine(posNumber, stylePOfSpeechList, styleNamePOfSpeechList, parent, elements, false);

    String posFull = getFullPosFromAbbrev(posAbbrev);
    AddDynamicLine(posFull, stylePOfSpeech, styleNamePOfSpeech, parent, elements, true);

    assert(!synsetDefVector.empty());

    for (size_t i = 0; i < synsetDefVector.size(); i++)
        FormatSynsetDef(synsetDefVector[i], i + 1, parent, elements);
}

// TODO: there will be mem leaks if run out of memory in the middle
static void FormatSynsets(const String& word, const AllSynsetDefs& allSynsets, Definition::Elements_t& elements)
{
    int synsetNo = 1;

    ParagraphElement* wordParagraph = new ParagraphElement();
    elements.push_back(wordParagraph);

    AddDynamicLine(word, styleWord, styleNameWord, wordParagraph, elements, true);

    if (!allSynsets.verb.empty())
    {
        FormatSynsetVector(allSynsets.verb, synsetNo, POS_VERB, elements);
        ++synsetNo;
    }

    if (!allSynsets.noun.empty())
    {
        FormatSynsetVector(allSynsets.noun, synsetNo, POS_NOUN, elements);
        ++synsetNo;
    }

    if (!allSynsets.adj.empty())
    {
        FormatSynsetVector(allSynsets.adj, synsetNo, POS_ADJ, elements);
        ++synsetNo;
    }

    if (!allSynsets.adv.empty())
    {
        FormatSynsetVector(allSynsets.adv, synsetNo, POS_ADV, elements);
        ++synsetNo;
    }
}

static void AddSynsetDef(AllSynsetDefs& allSynsets, const String& posAbbrev, const SynsetDef& synset)
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
DefinitionModel* ParseAndFormatDefinition(const String& defTxt, String& wordOut)
{
    String          curPosAbbrev;
    SynsetDef     curSynset;
    AllSynsetDefs allSynsets;

    bool    fNeedToAddSynset = false;
    bool    fEnd;
    String  line;
    String::size_type curPos = 0;

    // first in the definition is a word
    line = GetNextLine(defTxt, curPos, fEnd);
    if (fEnd)
        return NULL;

    String word = line;

    // currently we don't show pronunciation or requestsLeft
    String pronunciation;
    String requestsLeft;

    bool fAfterPos = false;
    while (true)
    {
        line = GetNextLine(defTxt, curPos, fEnd);
        if (fEnd)
            break;

        if (FIsPronunciation(line))
        {
            pronunciation.assign(line, tstrlen(pronTxt) + 1, String::npos);
        }
        else if (FIsReqLeft(line))
        {
            requestsLeft.assign(line, tstrlen(reqLeftTxt) + 1, String::npos);
        }
        else if (FIsPartOfSpeech(line))
        {
            fAfterPos = true;
            assert(2 == line.length());
            curPosAbbrev.assign(line, 1, 1);
        }
        else if (FIsSynonim(line))
        {
            if (fAfterPos)
            {
                AddSynsetDef(allSynsets, curPosAbbrev, curSynset);
                curSynset.clear();
                fAfterPos = false;
            }
            String syn(line, 1, line.length() - 1);
            if (0 != syn.compare(word))
            {
                curSynset.synonyms.push_back(syn);
            }
        }
        else if (FIsDef(line))
        {
            fAfterPos = true;
            curSynset.definition.assign(line, 1, line.length() - 1);
        }
        else if (FIsExample(line))
        {
            fAfterPos = true;
            String example(line, 1, line.length() - 1);
            curSynset.examples.push_back(example);
        }            
    }

    if (fAfterPos)
    {
        AddSynsetDef(allSynsets, curPosAbbrev, curSynset);
    }

	DefinitionModel::Elements_t elements;
    FormatSynsets(word, allSynsets, elements);

    DefinitionModel* model = new DefinitionModel();
	model->elements.swap(elements);
    wordOut = word;
    return model;
}

