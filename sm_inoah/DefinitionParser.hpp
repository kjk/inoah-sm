#ifndef _INOAH_PARSER_H_
#define _INOAH_PARSER_H_

#include <BaseTypes.hpp>
#include <DefinitionElement.hpp>

using ArsLexis::String;

Definition *ParseAndFormatDefinition(const String& def, String& wordOut);

#endif
