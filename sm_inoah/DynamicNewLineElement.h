// DynamicNewLineElement.h: interface for the DynamicNewLineElement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DYNAMICNEWLINEELEMENT_H__CCF182EC_3120_458C_B54C_4C90BFD4D2DB__INCLUDED_)
#define AFX_DYNAMICNEWLINEELEMENT_H__CCF182EC_3120_458C_B54C_4C90BFD4D2DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RenderingPreferences.hpp"
#include <FormattedTextElement.hpp>

class DynamicNewLineElement : public FormattedTextElement  
{
public:
    DynamicNewLineElement(const ArsLexis::String& text);
    virtual bool breakBefore(const RenderingPreferences& prefs) const;    

    virtual ~DynamicNewLineElement();

};

#endif // !defined(AFX_DYNAMICNEWLINEELEMENT_H__CCF182EC_3120_458C_B54C_4C90BFD4D2DB__INCLUDED_)
