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
};

#endif
