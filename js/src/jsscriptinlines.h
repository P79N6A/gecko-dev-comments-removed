







































#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

inline JSObject *JSScript::getRegExp(size_t index) {
    JSObjectArray *arr = regexps();
    JS_ASSERT((uint32) index < arr->length);
    JSObject *obj = arr->vector[index];
    JS_ASSERT(STOBJ_GET_CLASS(obj) == &js_RegExpClass);
    return obj;
}

#endif 
