






































#ifndef jsstrinlines_h___
#define jsstrinlines_h___

#include "jsstr.h"

inline JSString *
JSString::unitString(jschar c)
{
    JS_ASSERT(c < UNIT_STRING_LIMIT);
    return &unitStringTable[c];
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

inline JSString *
JSString::intString(jsint i)
{
    jsuint u = jsuint(i);

    JS_ASSERT(u < INT_STRING_LIMIT);
    if (u < 10) {
        
        return &JSString::unitStringTable['0' + u];
    }
    return &JSString::intStringTable[u];
}

#endif 
