// WinFont.cpp: implementation of the WinFont class.
//
//////////////////////////////////////////////////////////////////////

#include "WinFont.h"
#include "FontEffects.hpp"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WinFont::WinFont()
{

}

WinFont::WinFont(HFONT fnt)
{
    this->fntWrapper = new FontWrapper(fnt);
}

WinFont::WinFont(const WinFont& r)
{
    this->fntWrapper = r.fntWrapper;
    fntWrapper->attach();
}

WinFont& WinFont::operator=(const WinFont& r)
{
    if (this==&r) 
        return *this;
    fntWrapper->detach();
    if (fntWrapper->getRefsCount())
        delete fntWrapper;
    fntWrapper = r.fntWrapper;
    fntWrapper->attach();
    return *this;
}

HFONT WinFont::getHandle() const
{
    return fntWrapper->font;
}
void WinFont::SetEffects(ArsLexis::FontEffects& fx)
{

}
WinFont::~WinFont()
{
    fntWrapper->detach();
    if (fntWrapper->getRefsCount())
        delete fntWrapper;
}

FontWrapper::FontWrapper(HFONT fnt):
    font(fnt),
    refsCount(1)
{
    
}

FontWrapper::~FontWrapper()
{
    DeleteObject(this->font);
}
