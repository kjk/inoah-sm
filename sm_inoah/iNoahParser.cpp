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
    list<list<DefinitionElement*>* > sorted[pOfSpeechCnt];
    String meanings(text.substr(sep,text.length()-sep));
    while(meanings.compare(TEXT(""))!=0)    
    {
        int nextIdx = 0;
        char_t currBeg = meanings[0];
        String metBegs(TEXT(""));
        while (true)
        {
            char_t nextBeg;
            nextBeg+=currBeg;
            do
            {
                nextIdx = meanings.find_first_of(char_t('\n'), nextIdx+1);
                if ((nextIdx != -1)&&(meanings.length()>nextIdx+2))
                    nextBeg = meanings[nextIdx+1];
            }while ((nextBeg==currBeg) && (nextIdx != -1));
            if ((nextIdx == -1)||(metBegs.find_first_of(nextBeg) != -1))
            {
                list<DefinitionElement*>* mean=NULL;
                int pOfSpeech = 0;
                if(nextIdx==-1)
                {
                    
                /*mean = new DefinitionListToken(
                meanings.substring(0, meanings.length() - 1),
                this.lineWidth,
                    word);*/
                    
                    meanings.assign(TEXT(""));
                }
                else
                {
                /*mean =
                new DefinitionListToken(
                meanings.substring(0, nextIdx),
                this.lineWidth,
                    word);*/
                    meanings = meanings.substr(nextIdx + 1);
                }
                if(mean!=NULL)
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

list<DefinitionElement*>* iNoahParser::parseDefinitionList(String &text, String &word, int& partOfSpeach)
{
    DefinitionElement* explanation = NULL;
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
                    /*examples =
                    new ExmplesListToken(
                    '\n' + text.substring(start, currIndx),
                    this.lineWidth);*/
                    ;
                // Create examples subtoken on the basis of
                else
                    /*synonyms =
                    new SynonymsListToken(
                    '\n' + text.substring(start, currIndx),
                    lineWidth, word);*/
                    ;
                break;
            }
            case '@' :
            {
                // Create Explanation subtoken
                DefinitionElement* explanation =
                    new GenericTextElement(text.substr(start + 1, 
                    currIndx)+TEXT(". "));
                break;
            }
            case '$' :
            { 
                //if (start + 1 > text.length())
                    //throw new Exception("No part of speach description");
                String readAbbrev = text.substr(start + 1, currIndx);
                char i = 0;
                for (; i < pOfSpeechCnt; i++)
                {
                    if (pOfSpeach[abbrev][i].compare(readAbbrev) == 0)
                    {
                        partOfSpeach = (i == 4 ? 2 : i);
                        break;
                    }
                }
                //if (i == pOfSpeechCnt)
                  //  throw new Exception("Part of speach badly defined.");
                break;
            }
        }
        currIndx++;
    }
    /*if (partOfSpeach < 0)
	    throw new Exception("Part of speach not defined.");
    if(explanation!=null)
        this.addSubToken(explanation);
    else
        throw new Exception("No explanation of meaning.");
    if(examples!=null)
        this.addSubToken(examples);
    if((synonyms!=null)&&(synonyms.getSynonymsCount()>0))
        this.addSubToken(synonyms);*/
    return NULL;
}


list<DefinitionElement*>* iNoahParser::parseSynonymsList(String &text, String &word)
{
    int currIndx = 0;
    //TextToken last=null;
    while ((currIndx < text.length() - 2)
        && (text[currIndx+1] == '!'))
    {
        int start = currIndx;
        currIndx = text.find_first_of('\n', currIndx+1);
        if (currIndx == -1)
            currIndx = text.length();
        //if(start+2>=currIndx)
            //throw new Exception("One of synonyms malformed");
        String newSynonym=text.substr(start + 2, currIndx);
        if(word.compare(newSynonym)!=0)
        {
            //if(last!=null)
                //last.setText(last.getText()+", ");
            //this.addSubToken(
            //    last=new SynonymToken(
            //    newSynonym,
            //    lineWidth));
        }
    }
    //if(last!=null)
    //    last.setText(last.getText()+". ");
    return NULL;
}

list<DefinitionElement*>* iNoahParser::parseExamplesList(String &text)
{
    int currIndx = 0;
    //TextToken last=null;
    while ((currIndx < text.length() - 2)
        && (text[currIndx+1] == '#'))
    {
        int start = currIndx;
        currIndx = text.find_first_of('\n', currIndx+1);
        if (currIndx == -1)
            currIndx = text.length();
        //if(start+2>=currIndx)
            //throw new Exception("Example malformed");
        //if(last!=null)
        //    last.setText(last.getText()+", ");
        /*this.addSubToken(
            last=new ExampleToken(
            "\"" + text.substring(start + 2, currIndx) + "\"",
            lineWidth));*/
    }
    /*if(last!=null)
        last.setText(last.getText()+". ");*/
    return NULL;
}

