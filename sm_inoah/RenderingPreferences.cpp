#include <windows.h>

#include <BaseTypes.hpp>
#include <WinFont.h>
#include <FontEffects.hpp>

#include "RenderingPreferences.hpp"

#include "sm_inoah.h"

using ArsLexis::FontEffects;
using ArsLexis::Graphics;
using ArsLexis::char_t;

RenderingPreferences::RenderingPreferences()
    :needSynch(noChange)
{
    this->setFontSize(0);
    setClassicView();
    FontEffects fx;
    fx.setUnderline(FontEffects::underlineDotted);
    for (uint_t i=0; i<hyperlinkTypesCount_; ++i) 
        hyperlinkDecorations_[i].font.setEffects(fx);
    
    Graphics::Font_t font(WinFont());
    char_t bullet[3];

    if (bulletCircle==bulletType())
        bullet[0] = _T('*');
    else
        bullet[0] = _T('>');
    bullet[1] = _T(' ');
    bullet[2] = _T(' ');
    Graphics graphics(GetDC(g_hwndMain),g_hwndMain);
    Graphics::FontSetter setFont(graphics, font);
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
    needSynch = recalculateLayout;
}

void RenderingPreferences::setClassicView()
{
    styles_[styleDefault].requiresNewLine = false;
    styles_[styleWord].requiresNewLine = false;
    styles_[styleDefinitionList].requiresNewLine = true;
    styles_[styleDefinition].requiresNewLine = false;
    styles_[styleExampleList].requiresNewLine = true;
    styles_[styleExample].requiresNewLine = true;
    styles_[styleSynonymsList].requiresNewLine = true;
    styles_[styleSynonyms].requiresNewLine = false;
    styles_[stylePOfSpeechList].requiresNewLine = false;
    styles_[stylePOfSpeech].requiresNewLine = false;
    needSynch = recalculateLayout;
}

static int GetStandardFontDy()
{
    static int standardDy = -1;
    if (-1!=standardDy)
        return standardDy;

    HFONT fntHandle = (HFONT)GetStockObject(SYSTEM_FONT);
    if (NULL==fntHandle)
    {
        standardDy = 12;
        return standardDy;
    }
    LOGFONT logfnt;
    GetObject(fntHandle, sizeof(logfnt), &logfnt);

    assert(logfnt.lfHeight<0);

    standardDy = -logfnt.lfHeight;
    standardDy -= 1;
    return standardDy;
}

void RenderingPreferences::setFontSize(int diff)
{
    int newDy = GetStandardFontDy() + diff;

    WinFont fnt = WinFont(newDy);
    for (int k=0; k<stylesCount_; k++)
        styles_[k].font = fnt;

    fnt = WinFont(newDy);
    FontEffects fx;
    fx.setItalic(true);
    fnt.setEffects(fx);
    styles_[styleExample].font = fnt;
    styles_[styleExample].textColor = RGB(0,0,255);

    fnt = WinFont(newDy);
    FontEffects fx2;
    fx2.setItalic(false);
    fnt.setEffects(fx2);
    styles_[styleExampleList].font = fnt;
    styles_[styleExampleList].textColor = RGB(0,0,255);

    fnt = WinFont(newDy);
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

    int stylesDy = GetStandardFontDy() + 2 + diff;
    fnt = WinFont(stylesDy);
    styles_[styleWord].font = fnt;
    styles_[styleWord].textColor = RGB(0,0,255);

    needSynch = recalculateLayout;
}
