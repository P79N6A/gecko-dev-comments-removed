






































#ifndef jsstr_inlines_h___
#define jsstr_inlines_h___

#include "jsstr.h"

JS_BEGIN_EXTERN_C

inline JSString *
JSString::unitString(jschar c)
{
    JS_ASSERT(c < UNIT_STRING_LIMIT);
    return js_UnitStrings + c;
}

inline JSString *
JSString::getUnitString(JSContext *cx, JSString *str, size_t index)
{
    JS_ASSERT(index < str->length());
    jschar c = str->chars()[index];
    if (c < UNIT_STRING_LIMIT)
        return unitString(c);
    return js_NewDependentString(cx, str, index, 1);
}

JS_END_EXTERN_C

#endif 
