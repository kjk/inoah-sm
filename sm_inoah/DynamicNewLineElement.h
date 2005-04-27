#ifndef _DYNAMIC_NEW_LINE_H_
#define _DYNAMIC_NEW_LINE_H_

#include <LineBreakElement.hpp>

enum ElementStyle
{
    styleDefault,
    styleWord,
    styleDefinitionList,
    styleDefinition,
    styleExampleList,
    styleExample,
    styleSynonymsList,
    styleSynonyms,
    stylePOfSpeechList,
    stylePOfSpeech
};

class DynamicNewLineElement : public LineBreakElement  
{
public:

    DynamicNewLineElement(ElementStyle type);

    bool breakBefore() const;    

    ~DynamicNewLineElement();

    void toText(String& appendTo, uint_t from, uint_t to) const;

private:

	ElementStyle style_;
};

#endif
