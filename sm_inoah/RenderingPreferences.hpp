#ifndef _RENDERING_PREFERENCES_H_
#define _RENDERING_PREFERENCES_H_

#include "Debug.hpp"
#include "BaseTypes.hpp"
#include "Graphics.hpp"
#include "WinFont.h"

enum HyperlinkType
{
    hyperlinkBookmark,
    hyperlinkTerm,
    hyperlinkExternal
};

enum ElementStyle
{
    styleDefault,
    styleWord,
    styleDefinitionList,
    styleDefinition,
    styleExampleList,
    styleExample,
    styleSynonymsList,
    styleSynonyms,
    stylePOfSpeechList,
    stylePOfSpeech
};

class RenderingPreferences
{

    enum {
        stylesCount_= 10,
        hyperlinkTypesCount_=3,
    };        
    
    uint_t standardIndentation_;
    uint_t bulletIndentation_;
    
public:

    enum BulletType 
    {
        bulletCircle,
        bulletDiamond
    };
    
    RenderingPreferences();
    
    enum SynchronizationResult 
    {
        noChange,
        repaint,
        recalculateLayout
    };
    SynchronizationResult needSynch;
    /**
     * @todo Implement RenderingPreferences::synchronize()
     */
    SynchronizationResult synchronize(const RenderingPreferences&)
    {
        SynchronizationResult ret=needSynch;
        needSynch = noChange;
        return ret;
    }
    
    BulletType bulletType() const
    {return bulletCircle;}

    struct StyleFormatting
    {
        ArsLexis::Graphics::Font_t font;
        ArsLexis::Graphics::Color_t textColor;
        bool requiresNewLine;
        StyleFormatting():
            font(WinFont()),
            textColor(RGB(0,0,0)),
            requiresNewLine(false)
        {}
        
    };
    
    const StyleFormatting& hyperlinkDecoration(HyperlinkType hyperlinkType) const
    {
        assert(hyperlinkType<hyperlinkTypesCount_);
        return hyperlinkDecorations_[hyperlinkType];
    }
    
    const StyleFormatting& styleFormatting(ElementStyle style) const
    {
        assert(style<stylesCount_);
        return styles_[style];
    }
    
    uint_t standardIndentation() const
    {return standardIndentation_;}

    uint_t bulletIndentation() const
    {return bulletIndentation_;}

    ArsLexis::Graphics::Color_t backgroundColor() const
    {
#ifdef DEBUG
    return RGB(12,255,12);
#else
        return RGB(255,255,255);
#endif
    }
    
    void setCompactView();
    void setClassicView();
    void setFontSize(int diff);

private:
    
    StyleFormatting hyperlinkDecorations_[hyperlinkTypesCount_];
    StyleFormatting styles_[stylesCount_];
};

#endif
