






































#ifndef jsatom_inlines_h___
#define jsatom_inlines_h___

#include "jsatom.h"
#include "jsnum.h"




inline JSBool
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

inline JSBool
js_Int32ToId(JSContext* cx, int32 index, jsid* id)
{
    if (INT_FITS_IN_JSVAL(index)) {
        *id = INT_TO_JSID(index);
        JS_ASSERT(INT_JSID_TO_JSVAL(*id) == INT_TO_JSVAL(index));
        return JS_TRUE;
    }
    JSString* str = js_NumberToString(cx, index);
    if (!str)
        return JS_FALSE;
    return js_ValueToStringId(cx, STRING_TO_JSVAL(str), id);
}

#endif 
