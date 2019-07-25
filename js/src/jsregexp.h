






































#ifndef jsregexp_h___
#define jsregexp_h___



#include <stddef.h>
#include "jspubtd.h"
#include "jsstr.h"

#ifdef JS_THREADSAFE
#include "jsdhash.h"
#endif

inline const js::Value &
JSObject::getRegExpLastIndex() const
{
    JS_ASSERT(isRegExp());
    return fslots[JSSLOT_REGEXP_LAST_INDEX];
}

inline void
JSObject::setRegExpLastIndex(const js::Value &v)
{
    JS_ASSERT(isRegExp());
    fslots[JSSLOT_REGEXP_LAST_INDEX] = v;
}

inline void
JSObject::setRegExpLastIndex(jsdouble d)
{
    JS_ASSERT(isRegExp());
    fslots[JSSLOT_REGEXP_LAST_INDEX] = js::NumberValue(d);
}

inline void
JSObject::zeroRegExpLastIndex()
{
    JS_ASSERT(isRegExp());
    fslots[JSSLOT_REGEXP_LAST_INDEX].setInt32(0);
}

namespace js { class AutoStringRooter; }

extern JS_FRIEND_API(void)
js_SaveAndClearRegExpStatics(JSContext *cx, JSRegExpStatics *statics,
                             js::AutoStringRooter *tvr);

extern JS_FRIEND_API(void)
js_RestoreRegExpStatics(JSContext *cx, JSRegExpStatics *statics,
                        js::AutoStringRooter *tvr);









typedef struct RECharSet {
    JSPackedBool    converted;
    JSPackedBool    sense;
    uint16          length;
    union {
        uint8       *bits;
        struct {
            size_t  startIndex;
            size_t  length;
        } src;
    } u;
} RECharSet;

typedef struct RENode RENode;

struct JSRegExp {
    jsrefcount   nrefs;         
    uint16       flags;         
    size_t       parenCount;    
    size_t       classCount;    
    RECharSet    *classList;    
    JSString     *source;       
    jsbytecode   program[1];    
};

extern JSRegExp *
js_NewRegExp(JSContext *cx, js::TokenStream *ts,
             JSString *str, uintN flags, JSBool flat);

extern JSRegExp *
js_NewRegExpOpt(JSContext *cx, JSString *str, JSString *opt, JSBool flat);

#define HOLD_REGEXP(cx, re) JS_ATOMIC_INCREMENT(&(re)->nrefs)
#define DROP_REGEXP(cx, re) js_DestroyRegExp(cx, re)

extern void
js_DestroyRegExp(JSContext *cx, JSRegExp *re);






extern JSBool
js_ExecuteRegExp(JSContext *cx, JSRegExp *re, JSString *str, size_t *indexp,
                 JSBool test, js::Value *rval);

extern void
js_InitRegExpStatics(JSContext *cx);

extern void
js_TraceRegExpStatics(JSTracer *trc, JSContext *acx);

extern void
js_FreeRegExpStatics(JSContext *cx);

#define VALUE_IS_REGEXP(cx, v)                                                \
    ((v).isObject() && v.toObject().isRegExp())

extern js::Class js_RegExpClass;

inline bool
JSObject::isRegExp() const
{
    return getClass() == &js_RegExpClass;
}

extern JS_FRIEND_API(JSBool)
js_ObjectIsRegExp(JSObject *obj);

extern JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj);




extern JSBool
js_regexp_toString(JSContext *cx, JSObject *obj, js::Value *vp);




extern JSObject *
js_NewRegExpObject(JSContext *cx, js::TokenStream *ts,
                   const jschar *chars, size_t length, uintN flags);

extern JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

extern JS_FRIEND_API(JSObject *) JS_FASTCALL
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);


extern bool
js_ContainsRegExpMetaChars(const jschar *chars, size_t length);

#endif 
