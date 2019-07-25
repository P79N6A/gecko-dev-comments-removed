






































#ifndef jsregexp_h___
#define jsregexp_h___



#include <stddef.h>
#include "jsprvtd.h"
#include "jsstr.h"

#ifdef JS_THREADSAFE
#include "jsdhash.h"
#endif

extern js::Class js_RegExpClass;

static inline bool
VALUE_IS_REGEXP(JSContext *cx, js::Value v)
{
    return !v.isPrimitive() && v.toObject().isRegExp();
}

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

extern JS_FRIEND_API(JSObject *) JS_FASTCALL
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);





extern JS_FRIEND_API(void)
js_SaveAndClearRegExpStatics(JSContext *cx, js::RegExpStatics *res, js::AutoStringRooter *tvr);


extern JS_FRIEND_API(void)
js_RestoreRegExpStatics(JSContext *cx, js::RegExpStatics *res);

extern JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

#endif 
