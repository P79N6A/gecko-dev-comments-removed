






































#ifndef jsatominlines_h___
#define jsatominlines_h___

#include "jsatom.h"
#include "jsnum.h"

inline bool
js_ValueToAtom(JSContext *cx, const js::Value &v, JSAtom **atomp)
{
    if (!v.isString()) {
        JSString *str = js_ValueToString(cx, v);
        if (!str)
            return false;
        JS::Anchor<JSString *> anchor(str);
        *atomp = js_AtomizeString(cx, str, 0);
        return !!*atomp;
    }

    JSString *str = v.toString();
    if (str->isAtom()) {
        *atomp = &str->asAtom();
        return true;
    }

    *atomp = js_AtomizeString(cx, str, 0);
    return !!*atomp;
}

inline bool
js_ValueToStringId(JSContext *cx, const js::Value &v, jsid *idp)
{
    JSAtom *atom;
    if (js_ValueToAtom(cx, v, &atom)) {
        *idp = ATOM_TO_JSID(atom);
        return true;
    }
    return false;
}

inline bool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, const js::Value &idval,
                         jsid *idp)
{
    JS_ASSERT(!idval.isInt32() || !INT_FITS_IN_JSID(idval.toInt32()));

#if JS_HAS_XML_SUPPORT
    extern bool js_InternNonIntElementIdSlow(JSContext *, JSObject *,
                                             const js::Value &, jsid *);
    if (idval.isObject())
        return js_InternNonIntElementIdSlow(cx, obj, idval, idp);
#endif

    return js_ValueToStringId(cx, idval, idp);
}

inline bool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, const js::Value &idval,
                         jsid *idp, js::Value *vp)
{
    JS_ASSERT(!idval.isInt32() || !INT_FITS_IN_JSID(idval.toInt32()));

#if JS_HAS_XML_SUPPORT
    extern bool js_InternNonIntElementIdSlow(JSContext *, JSObject *,
                                             const js::Value &,
                                             jsid *, js::Value *);
    if (idval.isObject())
        return js_InternNonIntElementIdSlow(cx, obj, idval, idp, vp);
#endif

    JSAtom *atom;
    if (js_ValueToAtom(cx, idval, &atom)) {
        *idp = ATOM_TO_JSID(atom);
        vp->setString(atom);
        return true;
    }
    return false;
}

inline bool
js_Int32ToId(JSContext* cx, int32 index, jsid* id)
{
    if (INT_FITS_IN_JSID(index)) {
        *id = INT_TO_JSID(index);
        return true;
    }

    JSString* str = js_NumberToString(cx, index);
    if (!str)
        return false;

    return js_ValueToStringId(cx, js::StringValue(str), id);
}

#endif 
