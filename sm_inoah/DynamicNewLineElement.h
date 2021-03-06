#ifndef _DYNAMIC_NEW_LINE_H_
#define _DYNAMIC_NEW_LINE_H_

#include <LineBreakElement.hpp>
#include "iNoahStyles.hpp"

class DynamicNewLineElement : public LineBreakElement  
{
public:

    DynamicNewLineElement(ElementStyle type);

    bool breakBefore() const;    

    ~DynamicNewLineElement();

    void toText(String& appendTo, uint_t from, uint_t to) const;

protected:

    void calculateOrRender(LayoutContext& layoutContext, bool render);

private:

	ElementStyle style_;
};

#endif
