#ifndef _DYNAMIC_NEW_LINE_H_
#define _DYNAMIC_NEW_LINE_H_

#include <FormattedTextElement.hpp>
#include "RenderingPreferences.hpp"

using ArsLexis::String;

class DynamicNewLineElement : public FormattedTextElement  
{
public:
    DynamicNewLineElement(const String& text);
    virtual bool breakBefore(const RenderingPreferences& prefs) const;    

    virtual ~DynamicNewLineElement();

    virtual void toText(String& appendTo, uint_t from, uint_t to) const
    {
        GenericTextElement::toText(appendTo, from, to);
        if (from!=to)
        {
            String::size_type strLen = appendTo.length();            
            if ((strLen>0) && (appendTo[strLen-1]!=' '))
            {
                appendTo.append(_T(" "));
            }
#ifdef _PALM_OS
            appendTo.append(_T("\n"));
#else
            appendTo.append(_T("\r\n"));
#endif
        }
    }

};

#endif
