
























#ifndef RegexCompiler_h
#define RegexCompiler_h

#include "RegexParser.h"
#include "RegexPattern.h"

namespace JSC { namespace Yarr {

int compileRegex(const UString& patternString, RegexPattern& pattern);

} } 

#endif 
