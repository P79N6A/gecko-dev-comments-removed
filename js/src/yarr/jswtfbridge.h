






































#ifndef jswtfbridge_h__
#define jswtfbridge_h__





#include "assembler/wtf/Platform.h"
#include "jsstr.h"
#include "jsprvtd.h"
#include "jstl.h"

typedef jschar UChar;
typedef JSLinearString UString;

class Unicode {
  public:
    static UChar toUpper(UChar c) { return JS_TOUPPER(c); }
    static UChar toLower(UChar c) { return JS_TOLOWER(c); }
};

#endif
