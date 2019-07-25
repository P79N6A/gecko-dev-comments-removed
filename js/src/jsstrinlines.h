






































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

inline void
JSString::finalize(JSContext *cx, unsigned thingKind) {
    if (JS_LIKELY(thingKind == js::gc::FINALIZE_STRING)) {
        JS_ASSERT(!JSString::isStatic(this));
        JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
        if (isDependent()) {
            JS_ASSERT(dependentBase());
            JS_RUNTIME_UNMETER(cx->runtime, liveDependentStrings);
        } else if (isFlat()) {
            



            cx->free(flatChars());
        } else if (isTopNode()) {
            cx->free(topNodeBuffer());
        }
    } else {
        unsigned type = thingKind - js::gc::FINALIZE_EXTERNAL_STRING0;
        JS_ASSERT(type < JS_ARRAY_LENGTH(str_finalizers));
        JS_ASSERT(!isStatic(this));
        JS_ASSERT(isFlat());
        JS_RUNTIME_UNMETER(cx->runtime, liveStrings);

        
        jschar *chars = flatChars();
        if (!chars)
            return;
        JSStringFinalizeOp finalizer = str_finalizers[type];
        if (finalizer)
            finalizer(cx, this);
    }
}

inline void
JSShortString::finalize(JSContext *cx, unsigned thingKind)
{
    JS_ASSERT(js::gc::FINALIZE_SHORT_STRING == thingKind);
    JS_ASSERT(!JSString::isStatic(header()));
    JS_ASSERT(header()->isFlat());
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
}

inline
JSRopeBuilder::JSRopeBuilder(JSContext *cx)
  : cx(cx), mStr(cx->runtime->emptyString) {}

#endif 
