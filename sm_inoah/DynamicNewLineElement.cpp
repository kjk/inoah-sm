#include "DynamicNewLineElement.h"

#ifdef _WIN32
#include "sm_inoah.h"
#endif

#include "iNoahStyles.hpp"

DynamicNewLineElement::DynamicNewLineElement(ElementStyle style):
	style_(style)
{
}

bool DynamicNewLineElement::breakBefore() const
{
	return LineBreakForLayoutStyle(GetPrefLayoutType(), style_);
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

void DynamicNewLineElement::calculateOrRender(LayoutContext& layoutContext, bool render)
{
	if (breakBefore())
		LineBreakElement::calculateOrRender(layoutContext, render);
	else
		layoutContext.markElementCompleted(0);
}
