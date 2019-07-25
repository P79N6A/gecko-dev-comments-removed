






































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
JSString::length2String(jschar c1, jschar c2)
{
    JS_ASSERT(fitsInSmallChar(c1));
    JS_ASSERT(fitsInSmallChar(c2));
    return &length2StringTable[(((size_t)toSmallChar[c1]) << 6) + toSmallChar[c2]];
}

inline JSString *
JSString::intString(jsint i)
{
    jsuint u = jsuint(i);
    JS_ASSERT(u < INT_STRING_LIMIT);
    return JSString::intStringTable[u];
}

inline
JSRopeBuilder::JSRopeBuilder(JSContext *cx) {
    mStr = cx->runtime->emptyString;
}

#endif 
