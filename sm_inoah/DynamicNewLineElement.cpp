// DynamicNewLineElement.cpp: implementation of the DynamicNewLineElement class.
//
//////////////////////////////////////////////////////////////////////

#include "DynamicNewLineElement.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DynamicNewLineElement::DynamicNewLineElement(const ArsLexis::String& text)
    :FormattedTextElement(text)
{
}
bool DynamicNewLineElement::breakBefore(const RenderingPreferences& prefs) const
{
    return prefs.styleFormatting(this->style()).requiresNewLine;
}

DynamicNewLineElement::~DynamicNewLineElement()
{

}
