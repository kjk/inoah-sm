#include "iNoahParser.h"
#include <DefinitionElement.hpp>
#include <GenericTextElement.hpp>

using namespace ArsLexis;

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
        TEXT("verb. "), TEXT("adj. "), 
        TEXT("adv. "), TEXT("noun. "), 
        TEXT("adj. ")}
};

void iNoahParser::parse(String text)
{
    int sep = text.find_first_of(char_t('\n'));
    String word(text.substr(0, sep));
    list<ElementsList*> sorted[pOfSpeechCnt];
    String meanings(text.substr(sep+1,text.length()-sep-1));
    while(meanings.compare(TEXT(""))!=0)    
    {
        int nextIdx = 0;
        char_t currBeg = meanings[0];
        String metBegs(TEXT(""));
        while (true)
        {
            char_t nextBeg;
            nextBeg=currBeg;
            do
            {
                nextIdx = meanings.find_first_of(char_t('\n'), nextIdx+1);
                if ((nextIdx != -1)&&(meanings.length()>nextIdx+2))
                    nextBeg = meanings[nextIdx+1];
            }while ((nextBeg==currBeg) && (nextIdx != -1));
            if ((nextIdx == -1)||(metBegs.find_first_of(nextBeg) != -1))
            {
                ElementsList* mean = new ElementsList();
                int pOfSpeech = 0;
                String txtToParse;
                if(nextIdx==-1)
                {
                    txtToParse.assign(meanings.substr(0, meanings.length() - 1));
                    meanings.assign(TEXT(""));
                }
                else
                {

                    txtToParse.assign(meanings.substr(0, nextIdx));
                    meanings = meanings.substr(nextIdx + 1);
                }
                if(this->parseDefinitionList(
                    txtToParse,
                    word, pOfSpeech))
                {
                    if(explanation) (*mean).push_back(explanation);
                    if(examples) (*mean).merge(*examples);
                    if(synonyms) (*mean).merge(*synonyms);
                }
                
                if(examples) delete examples;
                if(synonyms) delete synonyms;

                if(mean->size()>0)
                    sorted[pOfSpeech].push_back(mean);
                break;
            }
            metBegs += currBeg;
            currBeg = nextBeg;
        }
    }
    int cntPOS=0;
    for(int i=0;i<pOfSpeechCnt;i++)
    {
        int cntMeaning=1;
        if (sorted[i].size() > 0)
        {
            //POfSpeechList pofsl =
            //    new POfSpeechList(arabNums[cntPOS] + " ", lineWidth);
            /*pofsl.addSubToken(
                new POfSpeech(
                    DefinitionListToken
                    .pOfSpeach[DefinitionListToken
                    .fullForm][i],
                    lineWidth));
            addSubToken(pofsl);*/
            cntPOS++;
            for (uint j = 0; j < sorted[i].size(); j++)
            {
            /*DefinitionListToken tk =
                (DefinitionListToken) sorted[i].get(j);
                tk.setText(Integer.toString(j + 1) + ") ");
                    addSubToken(tk);*/
            }
        }
    }
}

bool iNoahParser::parseDefinitionList(String &text, String &word, int& partOfSpeach)
{
    explanation = NULL;
    examples = NULL;
    synonyms = NULL;

    int currIndx = 0;
    while (currIndx < text.length())
    {
        int start = currIndx;
        char_t c = text[currIndx];
        currIndx = text.find_first_of('\n', currIndx);
        if (currIndx == -1)
            currIndx = text.length();
        switch (c)
        {
            case '!' :
            case '#' :
            {
                while ((currIndx < text.length() - 2)
                    && (currIndx != -1)
                    && (text[currIndx + 1] == c))
                    currIndx = text.find_first_of('\n', currIndx + 1);
                if (currIndx == -1)
                    currIndx = text.length();
                // Create synonyms subtoken on the basis of
                if (c != '!')
                {
                    if(!(examples = this->parseExamplesList(
                            TCHAR('\n') + 
                            text.substr(start, currIndx - start))))
                        return false;
                }
                // Create examples subtoken on the basis of
                else
                {
                    if (!(synonyms = this->parseSynonymsList(
                            TCHAR('\n') + 
                            text.substr(start, currIndx - start),word)))
                       return false;
                }
                break;
            }
            case '@' :
            {
                // Create Explanation subtoken
                explanation =
                    new GenericTextElement(text.substr(start + 1, 
                    currIndx)+TEXT(". "));
                break;
            }
            case '$' :
            { 
                if (start + 1 > text.length())
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
    GenericTextElement* last=NULL;

    while ((currIndx < text.length() - 2)
        && (text[currIndx+1] == '!'))
    {
        int start = currIndx;
        currIndx = text.find_first_of('\n', currIndx+1);
        if (currIndx == -1)
            currIndx = text.length();
        if(start+2>=currIndx)
            //throw new Exception("One of synonyms malformed");
        {
            if(lst) delete lst;
            error.assign(TEXT("Synonym malformed."));
            return NULL;
        }

        String newSynonym=text.substr(start + 2, currIndx);
        if(word.compare(newSynonym)!=0)
        {
            if(last)
                last->setText(last->text()+TEXT(", "));
            last = new GenericTextElement(newSynonym);
            lst->push_back(last);
        }
    }
    if(last)
        last->setText(last->text()+TEXT(". "));
    return lst;
}

iNoahParser::ElementsList* iNoahParser::parseExamplesList(String &text)
{
    int currIndx = 0;
    ElementsList* lst=new ElementsList();
    GenericTextElement* last=NULL;

    while ((currIndx < text.length() - 2)
        && (text[currIndx+1] == '#'))
    {
        int start = currIndx;
        currIndx = text.find_first_of('\n', currIndx+1);
        if (currIndx == -1)
            currIndx = text.length();
        if(start+2>=currIndx)
        {
            if(lst) delete lst;
            error.assign(TEXT("Example malformed."));
            return NULL;
        }
        if(last!=NULL)
            last->setText(last->text()+TEXT(", "));
        last = new GenericTextElement(
            TEXT("\"") + 
            text.substr(start + 2, currIndx-start-2) + 
            TEXT("\""));
        lst->push_back(last);
    }
    if(last!=NULL)
        last->setText(last->text()+TEXT(". "));
    return lst;
}

iNoahParser::ElementsList::~ElementsList()
{
    std::for_each(lst.begin(), lst.end(), ObjectDeleter<DefinitionElement>());
}

void iNoahParser::ElementsList::push_back(DefinitionElement* el)
{
    lst.push_back(el);
}
