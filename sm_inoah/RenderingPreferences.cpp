// RenderingPreferences.cpp: implementation of the RenderingPreferences class.
//
//////////////////////////////////////////////////////////////////////

#include <RenderingPreferences.hpp>
#include "FontEffects.hpp"
#include "WinFont.h"
#include "sm_inoah.h"
#include <wingdi.h>
using ArsLexis::FontEffects;
using ArsLexis::Graphics;

RenderingPreferences::RenderingPreferences()
    //:standardIndentation_(16)
    :needSynch(noChange)
{
    this->setFontSize(0);
    //int screenDepths=0;
    //bool color=false;
    /*Err error=WinScreenMode(winScreenModeGet, NULL, NULL, &screenDepths, &color);*/
    /* (screenDepths>=8) // 16+ colors
    {
        hyperlinkDecorations_[hyperlinkTerm].textColor=40;
        hyperlinkDecorations_[hyperlinkExternal].textColor=35;
    }
    else if (screenDepths>=2) // 4 colors
    {
        hyperlinkDecorations_[hyperlinkTerm].textColor=2; // Dark gray
        hyperlinkDecorations_[hyperlinkExternal].textColor=1; // Light gray
    }
    if (screenDepths>=2)
    {
        //for (uint_t i=0; i<stylesCount_; ++i)
          //styles_[i].textColor=UIColorGetTableEntryIndex(UIObjectForeground);        
    }*/
    setClassicView();
    FontEffects fx;
    fx.setUnderline(FontEffects::underlineDotted);
    for (uint_t i=0; i<hyperlinkTypesCount_; ++i) 
        hyperlinkDecorations_[i].font.setEffects(fx);
    
    Graphics::Font_t font(WinFont());
    TCHAR bullet[3];
    bullet[0]=(bulletType()==bulletCircle)?TCHAR('*'):TCHAR('>');
    bullet[1]=TCHAR(' ');
    bullet[2]=TCHAR(' ');
    Graphics graphics(GetDC(g_hwndMain),g_hwndMain);
    Graphics::FontSetter setFont(graphics, font);
    //standardIndentation_=graphics.textWidth(bullet, 2);
}

void RenderingPreferences::setCompactView()
{
    styles_[styleDefault].requiresNewLine = false;
    styles_[styleWord].requiresNewLine = false;
    styles_[styleDefinitionList].requiresNewLine = false;
    styles_[styleDefinition].requiresNewLine = false;
    styles_[styleExampleList].requiresNewLine = false;
    styles_[styleExample].requiresNewLine = false;
    styles_[styleSynonymsList].requiresNewLine = false;
    styles_[styleSynonyms].requiresNewLine = false;
    styles_[stylePOfSpeechList].requiresNewLine = false;
    styles_[stylePOfSpeech].requiresNewLine = false;
    needSynch=recalculateLayout;
}

void RenderingPreferences::setClassicView()
{
    styles_[styleDefault].requiresNewLine = false;
    styles_[styleWord].requiresNewLine = false;
    styles_[styleDefinitionList].requiresNewLine = true;
    styles_[styleDefinition].requiresNewLine = false;
    styles_[styleExampleList].requiresNewLine = true;
    styles_[styleExample].requiresNewLine = false;
    styles_[styleSynonymsList].requiresNewLine = true;
    styles_[styleSynonyms].requiresNewLine = false;
    styles_[stylePOfSpeechList].requiresNewLine = false;
    styles_[stylePOfSpeech].requiresNewLine = false;
    needSynch=recalculateLayout;
}

void RenderingPreferences::setFontSize(int diff)
{
    LOGFONT logfnt;
    HFONT fntHandle=(HFONT)GetStockObject(SYSTEM_FONT);
    GetObject(fntHandle, sizeof(logfnt), &logfnt);

    int newHeight =  logfnt.lfHeight + 1 + diff;

    WinFont fnt = WinFont(newHeight);
    for(int k=0;k<stylesCount_;k++)
        styles_[k].font = fnt;

    fnt = WinFont(newHeight);
    FontEffects fx;
    fx.setItalic(true);
    fnt.setEffects(fx);
    styles_[styleExample].font = fnt;
    styles_[styleExample].textColor = RGB(0,0,255);

    fnt = WinFont(newHeight);
    FontEffects fx2;
    fx2.setItalic(false);
    fnt.setEffects(fx2);
    styles_[styleExampleList].font = fnt;
    styles_[styleExampleList].textColor = RGB(0,0,255);

    fnt = WinFont(newHeight);
    FontEffects fx3;
    fx3.setWeight(FontEffects::weightBold);
    fnt.setEffects(fx3);
    styles_[stylePOfSpeechList].font = fnt;
    styles_[stylePOfSpeechList].textColor = RGB(0,200,0);
    styles_[stylePOfSpeech].font = fnt;
    styles_[stylePOfSpeech].textColor = RGB(0,200,0);
    styles_[styleSynonyms].font = fnt;
    styles_[styleSynonyms].textColor = RGB(0,0,0);
    styles_[styleSynonymsList].font = fnt;
    styles_[styleSynonymsList].textColor = RGB(0,0,0);

    int stylesHeight = logfnt.lfHeight - 4 + diff;
    fnt = WinFont(stylesHeight);
    styles_[styleWord].font = fnt;
    styles_[styleWord].textColor = RGB(0,0,255);
    needSynch=recalculateLayout;
}
