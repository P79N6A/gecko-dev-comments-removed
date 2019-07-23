






































#ifndef jsstr_inlines_h___
#define jsstr_inlines_h___

#include "jsstr.h"

JS_BEGIN_EXTERN_C

inline JSString *JSString::getUnitString(JSContext *cx, jschar c) {
    JS_ASSERT(c < UNIT_STRING_LIMIT);
    JSRuntime *rt = cx->runtime;
    JSString **unitStrings = rt->unitStrings;
    JSString *ustr;
    if (unitStrings && (ustr = unitStrings[c]) != NULL)
        return ustr;
    return js_MakeUnitString(cx, c);
}

inline JSString *JSString::getUnitString(JSContext *cx, JSString *str, size_t index) {
    JS_ASSERT(index < str->length());
    jschar c = str->chars()[index];
    if (c >= UNIT_STRING_LIMIT)
        return js_NewDependentString(cx, str, index, 1);
    return getUnitString(cx, c);
}

JS_END_EXTERN_C

#endif 
