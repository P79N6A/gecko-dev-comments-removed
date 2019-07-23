






































#ifndef jsatom_inlines_h___
#define jsatom_inlines_h___

#include "jsatom.h"
#include "jsnum.h"

JS_BEGIN_EXTERN_C




static inline JSBool
js_ValueToStringId(JSContext *cx, jsval v, jsid *idp)
{
    JSString *str;
    JSAtom *atom;

    






    if (JSVAL_IS_STRING(v)) {
        str = JSVAL_TO_STRING(v);
        if (str->isAtomized()) {
            cx->weakRoots.lastAtom = v;
            *idp = ATOM_TO_JSID((JSAtom *) v);
            return JS_TRUE;
        }
    } else {
        str = js_ValueToString(cx, v);
        if (!str)
            return JS_FALSE;
    }
    atom = js_AtomizeString(cx, str, 0);
    if (!atom)
        return JS_FALSE;
    *idp = ATOM_TO_JSID(atom);
    return JS_TRUE;
}

static inline JSBool
js_Int32ToId(JSContext* cx, int32 index, jsid* id)
{
    if (index <= JSVAL_INT_MAX) {
        *id = INT_TO_JSID(index);
        return JS_TRUE;
    }
    JSString* str = js_NumberToString(cx, index);
    if (!str)
        return JS_FALSE;
    return js_ValueToStringId(cx, STRING_TO_JSVAL(str), id);
}

JS_END_EXTERN_C

#endif 
