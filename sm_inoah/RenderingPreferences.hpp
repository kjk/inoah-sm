// RenderingPreferences.hpp: interface for the RenderingPreferences class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RENDERINGPREFERENCES_HPP__7B18916C_BD53_4ED8_921A_F9A5E283862F__INCLUDED_)
#define AFX_RENDERINGPREFERENCES_HPP__7B18916C_BD53_4ED8_921A_F9A5E283862F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
    styleHeader
};

class RenderingPreferences
{

    enum {
        stylesCount_=2,
        hyperlinkTypesCount_=3,
    };        
    
    uint_t standardIndentation_;
    
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

    /**
     * @todo Implement RenderingPreferences::synchronize()
     */
    SynchronizationResult synchronize(const RenderingPreferences&)
    {return noChange;}
    
    BulletType bulletType() const
    {return bulletCircle;}

    struct StyleFormatting
    {
        ArsLexis::Graphics::Font_t font;
        ArsLexis::Graphics::Color_t textColor;
        
        StyleFormatting():
            font(WinFont((HFONT)GetStockObject(SYSTEM_FONT))),
            textColor(RGB(0,0,0))
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
    
    ArsLexis::Graphics::Color_t backgroundColor() const
    {return RGB(255,255,255);}

private:
    
    StyleFormatting hyperlinkDecorations_[hyperlinkTypesCount_];
    StyleFormatting styles_[stylesCount_];
};

#endif // !defined(AFX_RENDERINGPREFERENCES_HPP__7B18916C_BD53_4ED8_921A_F9A5E283862F__INCLUDED_)
