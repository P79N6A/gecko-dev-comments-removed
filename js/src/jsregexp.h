






































#ifndef jsregexp_h___
#define jsregexp_h___



#include <stddef.h>
#include "jspubtd.h"
#include "jsstr.h"

#ifdef JS_THREADSAFE
#include "jsdhash.h"
#endif


namespace js {
class RegExp;
class RegExpStatics;
class AutoValueRooter;
}

JS_BEGIN_EXTERN_C

extern JSClass js_RegExpClass;

static inline bool
VALUE_IS_REGEXP(JSContext *cx, jsval v)
{
    return !JSVAL_IS_PRIMITIVE(v) && JSVAL_TO_OBJECT(v)->isRegExp();
}

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
js_regexp_toString(JSContext *cx, JSObject *obj, jsval *vp);

extern JS_FRIEND_API(JSObject *) JS_FASTCALL
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);





extern JS_FRIEND_API(void)
js_SaveAndClearRegExpStatics(JSContext *cx, js::RegExpStatics *statics, js::AutoValueRooter *tvr);


extern JS_FRIEND_API(void)
js_RestoreRegExpStatics(JSContext *cx, js::RegExpStatics *statics, js::AutoValueRooter *tvr);

extern JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

JS_END_EXTERN_C

#endif 
