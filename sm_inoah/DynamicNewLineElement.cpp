#include "DynamicNewLineElement.h"

DynamicNewLineElement::DynamicNewLineElement(ElementStyle style):
	style_(style)
{
}

bool DynamicNewLineElement::breakBefore() const
{
	// TODO: read this from prefs depending on style_
    // return prefs.styleFormatting(this->style()).requiresNewLine;
	return true;
}

DynamicNewLineElement::~DynamicNewLineElement()
{
}

void DynamicNewLineElement::toText(String& appendTo, uint_t from, uint_t to) const
{
/*
    TextElement::toText(appendTo, from, to);
    if (from != to)
    {
        String::size_type strLen = appendTo.length();            
        if ((strLen>0) && (appendTo[strLen - 1] != ' '))
            appendTo += _T(' ');

#ifdef _PALM_OS
        appendTo += _T('\n');
#else
        appendTo += _T("\r\n");
#endif
    }
*/
	if (from != to && breakBefore())
#ifdef _PALM_OS
        appendTo += _T('\n');
#else
        appendTo += _T("\r\n");
#endif
}
