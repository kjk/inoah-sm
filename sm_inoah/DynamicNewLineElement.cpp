#include "DynamicNewLineElement.h"

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
