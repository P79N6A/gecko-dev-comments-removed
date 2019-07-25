








































#include <string.h>
#include "jsapi.h"
#include "jsarena.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsonparser.h"
#include "jsprf.h"
#include "jsscan.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jsxml.h"
#include "jsvector.h"

#include "json.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;

Class js_JSONClass = {
    js_JSON_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_JSON),
    PropertyStub,        
    PropertyStub,        
    PropertyStub,        
    StrictPropertyStub,  
    EnumerateStub,
    ResolveStub,
    ConvertStub
};


JSBool
js_json_parse(JSContext *cx, uintN argc, Value *vp)
{
    
    JSLinearString *linear;
    if (argc >= 1) {
        JSString *str = js_ValueToString(cx, vp[2]);
        if (!str)
            return false;
        linear = str->ensureLinear(cx);
        if (!linear)
            return false;
    } else {
        linear = cx->runtime->atomState.typeAtoms[JSTYPE_VOID];
    }
    JS::Anchor<JSString *> anchor(linear);

    Value reviver = (argc >= 2) ? vp[3] : UndefinedValue();

    
    return ParseJSONWithReviver(cx, linear->chars(), linear->length(), reviver, vp);
}


JSBool
js_json_stringify(JSContext *cx, uintN argc, Value *vp)
{
    *vp = (argc >= 1) ? vp[2] : UndefinedValue();
    JSObject *replacer = (argc >= 2 && vp[3].isObject())
                         ? &vp[3].toObject()
                         : NULL;
    Value space = (argc >= 3) ? vp[4] : UndefinedValue();

    StringBuffer sb(cx);
    if (!js_Stringify(cx, vp, replacer, space, sb))
        return false;

    
    
    
    if (!sb.empty()) {
        JSString *str = sb.finishString();
        if (!str)
            return false;
        vp->setString(str);
    } else {
        vp->setUndefined();
    }

    return true;
}

JSBool
js_TryJSON(JSContext *cx, Value *vp)
{
    if (!vp->isObject())
        return true;

    JSObject *obj = &vp->toObject();
    Value fval;
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.toJSONAtom);
    if (!js_GetMethod(cx, obj, id, JSGET_NO_METHOD_BARRIER, &fval))
        return false;
    if (js_IsCallable(fval)) {
        if (!ExternalInvoke(cx, ObjectValue(*obj), fval, 0, NULL, vp))
            return false;
    }
    return true;
}


static inline bool IsQuoteSpecialCharacter(jschar c)
{
    JS_STATIC_ASSERT('\b' < ' ');
    JS_STATIC_ASSERT('\f' < ' ');
    JS_STATIC_ASSERT('\n' < ' ');
    JS_STATIC_ASSERT('\r' < ' ');
    JS_STATIC_ASSERT('\t' < ' ');
    return c == '"' || c == '\\' || c < ' ';
}


static bool
Quote(JSContext *cx, StringBuffer &sb, JSString *str)
{
    JS::Anchor<JSString *> anchor(str);
    size_t len = str->length();
    const jschar *buf = str->getChars(cx);
    if (!buf)
        return false;

    
    if (!sb.append('"'))
        return false;

    
    for (size_t i = 0; i < len; ++i) {
        
        size_t mark = i;
        do {
            if (IsQuoteSpecialCharacter(buf[i]))
                break;
        } while (++i < len);
        if (i > mark) {
            if (!sb.append(&buf[mark], i - mark))
                return false;
            if (i == len)
                break;
        }

        jschar c = buf[i];
        if (c == '"' || c == '\\') {
            if (!sb.append('\\') || !sb.append(c))
                return false;
        } else if (c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t') {
           jschar abbrev = (c == '\b')
                         ? 'b'
                         : (c == '\f')
                         ? 'f'
                         : (c == '\n')
                         ? 'n'
                         : (c == '\r')
                         ? 'r'
                         : 't';
           if (!sb.append('\\') || !sb.append(abbrev))
               return false;
           mark = i + 1;
        } else {
            JS_ASSERT(c < ' ');
            if (!sb.append("\\u00"))
                return false;
            JS_ASSERT((c >> 4) < 10);
            uint8 x = c >> 4, y = c % 16;
            if (!sb.append('0' + x) || !sb.append(y < 10 ? '0' + y : 'a' + (y - 10)))
                return false;
        }
    }

    
    return sb.append('"');
}

class StringifyContext
{
  public:
    StringifyContext(JSContext *cx, StringBuffer &sb, const StringBuffer &gap,
                     JSObject *replacer, const AutoIdVector &propertyList)
      : sb(sb),
        gap(gap),
        replacer(replacer),
        propertyList(propertyList),
        depth(0),
        objectStack(cx)
    {}

    bool init() {
        return objectStack.init(16);
    }

#ifdef DEBUG
    ~StringifyContext() { JS_ASSERT(objectStack.empty()); }
#endif

    StringBuffer &sb;
    const StringBuffer &gap;
    JSObject * const replacer;
    const AutoIdVector &propertyList;
    uint32 depth;
    HashSet<JSObject *> objectStack;
};

static JSBool Str(JSContext *cx, const Value &v, StringifyContext *scx);

static JSBool
WriteIndent(JSContext *cx, StringifyContext *scx, uint32 limit)
{
    if (!scx->gap.empty()) {
        if (!scx->sb.append('\n'))
            return JS_FALSE;
        for (uint32 i = 0; i < limit; i++) {
            if (!scx->sb.append(scx->gap.begin(), scx->gap.end()))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
}

class CycleDetector
{
  public:
    CycleDetector(StringifyContext *scx, JSObject *obj)
      : objectStack(scx->objectStack), obj(obj) {
    }

    bool init(JSContext *cx) {
        HashSet<JSObject *>::AddPtr ptr = objectStack.lookupForAdd(obj);
        if (ptr) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CYCLIC_VALUE, js_object_str);
            return false;
        }
        return objectStack.add(ptr, obj);
    }

    ~CycleDetector() {
        objectStack.remove(obj);
    }

  private:
    HashSet<JSObject *> &objectStack;
    JSObject *const obj;
};





static bool
PreprocessValue(JSContext *cx, JSObject *holder, jsid key, Value *vp, StringifyContext *scx)
{
    JSString *keyStr = NULL;

    
    if (vp->isObject()) {
        Value toJSON;
        jsid id = ATOM_TO_JSID(cx->runtime->atomState.toJSONAtom);
        if (!js_GetMethod(cx, &vp->toObject(), id, JSGET_NO_METHOD_BARRIER, &toJSON))
            return false;

        if (js_IsCallable(toJSON)) {
            keyStr = IdToString(cx, key);
            if (!keyStr)
                return false;

            LeaveTrace(cx);
            InvokeArgsGuard args;
            if (!cx->stack.pushInvokeArgs(cx, 1, &args))
                return false;

            args.calleev() = toJSON;
            args.thisv() = *vp;
            args[0] = StringValue(keyStr);

            if (!Invoke(cx, args))
                return false;
            *vp = args.rval();
        }
    }

    
    if (scx->replacer && scx->replacer->isCallable()) {
        if (!keyStr) {
            keyStr = IdToString(cx, key);
            if (!keyStr)
                return false;
        }

        LeaveTrace(cx);
        InvokeArgsGuard args;
        if (!cx->stack.pushInvokeArgs(cx, 2, &args))
            return false;

        args.calleev() = ObjectValue(*scx->replacer);
        args.thisv() = ObjectValue(*holder);
        args[0] = StringValue(keyStr);
        args[1] = *vp;

        if (!Invoke(cx, args))
            return false;
        *vp = args.rval();
    }

    
    if (vp->isObject()) {
        JSObject *obj = &vp->toObject();
        Class *clasp = obj->getClass();
        if (clasp == &js_NumberClass) {
            double d;
            if (!ValueToNumber(cx, *vp, &d))
                return false;
            vp->setNumber(d);
        } else if (clasp == &js_StringClass) {
            JSString *str = js_ValueToString(cx, *vp);
            if (!str)
                return false;
            vp->setString(str);
        } else if (clasp == &js_BooleanClass) {
            *vp = obj->getPrimitiveThis();
            JS_ASSERT(vp->isBoolean());
        }
    }

    return true;
}








static inline bool
IsFilteredValue(const Value &v)
{
    return v.isUndefined() || js_IsCallable(v) || (v.isObject() && v.toObject().isXML());
}


static JSBool
JO(JSContext *cx, JSObject *obj, StringifyContext *scx)
{
    









    
    CycleDetector detect(scx, obj);
    if (!detect.init(cx))
        return JS_FALSE;

    if (!scx->sb.append('{'))
        return JS_FALSE;

    
    Maybe<AutoIdVector> ids;
    const AutoIdVector *props;
    if (scx->replacer && !scx->replacer->isCallable()) {
        JS_ASSERT(JS_IsArrayObject(cx, scx->replacer));
        props = &scx->propertyList;
    } else {
        JS_ASSERT_IF(scx->replacer, scx->propertyList.length() == 0);
        ids.construct(cx);
        if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, ids.addr()))
            return false;
        props = ids.addr();
    }

    
    const AutoIdVector &propertyList = *props;

    
    bool wroteMember = false;
    for (size_t i = 0, len = propertyList.length(); i < len; i++) {
        






        const jsid &id = propertyList[i];
        Value outputValue;
        if (!obj->getProperty(cx, id, &outputValue))
            return false;
        if (!PreprocessValue(cx, obj, id, &outputValue, scx))
            return false;
        if (IsFilteredValue(outputValue))
            continue;

        
        if (wroteMember && !scx->sb.append(','))
            return false;
        wroteMember = true;

        if (!WriteIndent(cx, scx, scx->depth))
            return false;

        JSString *s = IdToString(cx, id);
        if (!s)
            return false;

        if (!Quote(cx, scx->sb, s) ||
            !scx->sb.append(':') ||
            !(scx->gap.empty() || scx->sb.append(' ')) ||
            !Str(cx, outputValue, scx))
        {
            return false;
        }
    }

    if (wroteMember && !WriteIndent(cx, scx, scx->depth - 1))
        return false;

    return scx->sb.append('}');
}


static JSBool
JA(JSContext *cx, JSObject *obj, StringifyContext *scx)
{
    









    
    CycleDetector detect(scx, obj);
    if (!detect.init(cx))
        return JS_FALSE;

    if (!scx->sb.append('['))
        return JS_FALSE;

    
    jsuint length;
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    
    if (length != 0) {
        
        if (!WriteIndent(cx, scx, scx->depth))
            return JS_FALSE;

        
        Value outputValue;
        for (jsuint i = 0; i < length; i++) {
            jsid id = INT_TO_JSID(i);

            





            if (!obj->getProperty(cx, id, &outputValue))
                return JS_FALSE;
            if (!PreprocessValue(cx, obj, id, &outputValue, scx))
                return JS_FALSE;
            if (IsFilteredValue(outputValue)) {
                if (!scx->sb.append("null"))
                    return JS_FALSE;
            } else {
                if (!Str(cx, outputValue, scx))
                    return JS_FALSE;
            }

            
            if (i < length - 1) {
                if (!scx->sb.append(','))
                    return JS_FALSE;
                if (!WriteIndent(cx, scx, scx->depth))
                    return JS_FALSE;
            }
        }

        
        if (!WriteIndent(cx, scx, scx->depth - 1))
            return JS_FALSE;
    }

    return scx->sb.append(']');
}

static JSBool
Str(JSContext *cx, const Value &v, StringifyContext *scx)
{
    
    JS_ASSERT(!IsFilteredValue(v));

    JS_CHECK_RECURSION(cx, return false);

    












    
    if (v.isString())
        return Quote(cx, scx->sb, v.toString());

    
    if (v.isNull())
        return scx->sb.append("null");

    
    if (v.isBoolean())
        return v.toBoolean() ? scx->sb.append("true") : scx->sb.append("false");

    
    if (v.isNumber()) {
        if (v.isDouble()) {
            if (!JSDOUBLE_IS_FINITE(v.toDouble()))
                return scx->sb.append("null");
        }

        StringBuffer sb(cx);
        if (!NumberValueToStringBuffer(cx, v, sb))
            return false;

        return scx->sb.append(sb.begin(), sb.length());
    }

    
    JS_ASSERT(v.isObject());
    JSBool ok;

    scx->depth++;
    ok = (JS_IsArrayObject(cx, &v.toObject()) ? JA : JO)(cx, &v.toObject(), scx);
    scx->depth--;

    return ok;
}


JSBool
js_Stringify(JSContext *cx, Value *vp, JSObject *replacer, Value space, StringBuffer &sb)
{
    
    AutoIdVector propertyList(cx);
    if (replacer) {
        if (replacer->isCallable()) {
            
        } else if (JS_IsArrayObject(cx, replacer)) {
            



























            
            jsuint len;
            JS_ALWAYS_TRUE(js_GetLengthProperty(cx, replacer, &len));
            if (replacer->isDenseArray())
                len = JS_MIN(len, replacer->getDenseArrayCapacity());

            HashSet<jsid> idSet(cx);
            if (!idSet.init(len))
                return false;

            
            jsuint i = 0;

            
            for (; i < len; i++) {
                
                Value v;
                if (!replacer->getProperty(cx, INT_TO_JSID(i), &v))
                    return false;

                jsid id;
                if (v.isNumber()) {
                    
                    int32_t n;
                    if (v.isNumber() && ValueFitsInInt32(v, &n) && INT_FITS_IN_JSID(n)) {
                        id = INT_TO_JSID(n);
                    } else {
                        if (!js_ValueToStringId(cx, v, &id))
                            return false;
                        id = js_CheckForStringIndex(id);
                    }
                } else if (v.isString() ||
                           (v.isObject() && (v.toObject().isString() || v.toObject().isNumber())))
                {
                    
                    if (!js_ValueToStringId(cx, v, &id))
                        return false;
                    id = js_CheckForStringIndex(id);
                } else {
                    continue;
                }

                
                HashSet<jsid>::AddPtr p = idSet.lookupForAdd(id);
                if (!p) {
                    
                    if (!idSet.add(p, id) || !propertyList.append(id))
                        return false;
                }
            }
        } else {
            replacer = NULL;
        }
    }

    
    if (space.isObject()) {
        JSObject &spaceObj = space.toObject();
        if (spaceObj.isNumber()) {
            jsdouble d;
            if (!ValueToNumber(cx, space, &d))
                return false;
            space = NumberValue(d);
        } else if (spaceObj.isString()) {
            JSString *str = js_ValueToString(cx, space);
            if (!str)
                return false;
            space = StringValue(str);
        }
    }

    StringBuffer gap(cx);

    if (space.isNumber()) {
        
        jsdouble d;
        JS_ALWAYS_TRUE(ToInteger(cx, space, &d));
        d = JS_MIN(10, d);
        if (d >= 1 && !gap.appendN(' ', uint32(d)))
            return false;
    } else if (space.isString()) {
        
        JSLinearString *str = space.toString()->ensureLinear(cx);
        if (!str)
            return false;
        JS::Anchor<JSString *> anchor(str);
        size_t len = JS_MIN(10, space.toString()->length());
        if (!gap.append(str->chars(), len))
            return false;
    } else {
        
        JS_ASSERT(gap.empty());
    }

    
    JSObject *wrapper = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!wrapper)
        return false;

    
    jsid emptyId = ATOM_TO_JSID(cx->runtime->atomState.emptyAtom);
    if (!DefineNativeProperty(cx, wrapper, emptyId, *vp, PropertyStub, StrictPropertyStub,
                              JSPROP_ENUMERATE, 0, 0))
    {
        return false;
    }

    
    StringifyContext scx(cx, sb, gap, replacer, propertyList);
    if (!scx.init())
        return false;

    if (!PreprocessValue(cx, wrapper, emptyId, vp, &scx))
        return false;
    if (IsFilteredValue(*vp))
        return true;

    return Str(cx, *vp, &scx);
}


static bool
Walk(JSContext *cx, JSObject *holder, jsid name, const Value &reviver, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);

    
    Value val;
    if (!holder->getProperty(cx, name, &val))
        return false;

    
    if (val.isObject()) {
        JSObject *obj = &val.toObject();

        if (obj->isArray()) {
            
            jsuint length = obj->getArrayLength();

            
            for (jsuint i = 0; i < length; i++) {
                jsid id;
                if (!IndexToId(cx, i, &id))
                    return false;

                
                Value newElement;
                if (!Walk(cx, obj, id, reviver, &newElement))
                    return false;

                










                if (newElement.isUndefined()) {
                    
                    JS_ALWAYS_TRUE(array_deleteProperty(cx, obj, id, &newElement, false));
                } else {
                    
                    JS_ALWAYS_TRUE(array_defineProperty(cx, obj, id, &newElement, PropertyStub,
                                                        StrictPropertyStub, JSPROP_ENUMERATE));
                }
            }
        } else {
            
            AutoIdVector keys(cx);
            if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, &keys))
                return false;

            
            for (size_t i = 0, len = keys.length(); i < len; i++) {
                
                Value newElement;
                jsid id = keys[i];
                if (!Walk(cx, obj, id, reviver, &newElement))
                    return false;

                if (newElement.isUndefined()) {
                    
                    if (!js_DeleteProperty(cx, obj, id, &newElement, false))
                        return false;
                } else {
                    
                    JS_ASSERT(obj->isNative());
                    if (!DefineNativeProperty(cx, obj, id, newElement, PropertyStub,
                                              StrictPropertyStub, JSPROP_ENUMERATE, 0, 0))
                    {
                        return false;
                    }
                }
            }
        }
    }

    
    JSString *key = IdToString(cx, name);
    if (!key)
        return false;

    LeaveTrace(cx);
    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 2, &args))
        return false;

    args.calleev() = reviver;
    args.thisv() = ObjectValue(*holder);
    args[0] = StringValue(key);
    args[1] = val;

    if (!Invoke(cx, args))
        return false;
    *vp = args.rval();
    return true;
}

static bool
Revive(JSContext *cx, const Value &reviver, Value *vp)
{

    JSObject *obj = NewBuiltinClassInstance(cx, &js_ObjectClass);
    if (!obj)
        return false;

    AutoObjectRooter tvr(cx, obj);
    if (!obj->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.emptyAtom),
                             *vp, NULL, NULL, JSPROP_ENUMERATE)) {
        return false;
    }

    return Walk(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.emptyAtom), reviver, vp);
}

namespace js {

JSBool
ParseJSONWithReviver(JSContext *cx, const jschar *chars, size_t length, const Value &reviver,
                     Value *vp, DecodingMode decodingMode )
{
    
    JSONParser parser(cx, chars, length,
                      decodingMode == STRICT ? JSONParser::StrictJSON : JSONParser::LegacyJSON);
    if (!parser.parse(vp))
        return false;

    
    if (js_IsCallable(reviver))
        return Revive(cx, reviver, vp);
    return true;
}

} 

#if JS_HAS_TOSOURCE
static JSBool
json_toSource(JSContext *cx, uintN argc, Value *vp)
{
    vp->setString(CLASS_ATOM(cx, JSON));
    return JS_TRUE;
}
#endif

static JSFunctionSpec json_static_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  json_toSource,      0, 0),
#endif
    JS_FN("parse",          js_json_parse,      2, 0),
    JS_FN("stringify",      js_json_stringify,  3, 0),
    JS_FS_END
};

JSObject *
js_InitJSONClass(JSContext *cx, JSObject *obj)
{
    JSObject *JSON;

    JSON = NewNonFunction<WithProto::Class>(cx, &js_JSONClass, NULL, obj);
    if (!JSON)
        return NULL;
    if (!JS_DefineProperty(cx, obj, js_JSON_str, OBJECT_TO_JSVAL(JSON),
                           JS_PropertyStub, JS_StrictPropertyStub, 0))
        return NULL;

    if (!JS_DefineFunctions(cx, JSON, json_static_methods))
        return NULL;

    MarkStandardClassInitializedNoProto(obj, &js_JSONClass);

    return JSON;
}
