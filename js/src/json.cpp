






#include "mozilla/FloatingPoint.h"

#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsinterp.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsobj.h"
#include "json.h"
#include "jsonparser.h"
#include "jsprf.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jsxml.h"

#include "frontend/TokenStream.h"
#include "vm/StringBuffer.h"

#include "jsatominlines.h"
#include "jsboolinlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::Maybe;

Class js::JSONClass = {
    js_JSON_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_JSON),
    JS_PropertyStub,        
    JS_PropertyStub,        
    JS_PropertyStub,        
    JS_StrictPropertyStub,  
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};


JSBool
js_json_parse(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    JSString *str = (argc >= 1) ? ToString(cx, args[0]) : cx->names().undefined;
    if (!str)
        return false;

    JSStableString *stable = str->ensureStable(cx);
    if (!stable)
        return false;

    JS::Anchor<JSString *> anchor(stable);

    RootedValue reviver(cx, (argc >= 2) ? args[1] : UndefinedValue());

    
    return ParseJSONWithReviver(cx, stable->chars(), stable->length(), reviver, args.rval());
}


JSBool
js_json_stringify(JSContext *cx, unsigned argc, Value *vp)
{
    RootedObject replacer(cx, (argc >= 2 && vp[3].isObject())
                              ? &vp[3].toObject()
                              : NULL);
    RootedValue value(cx, (argc >= 1) ? vp[2] : UndefinedValue());
    RootedValue space(cx, (argc >= 3) ? vp[4] : UndefinedValue());

    StringBuffer sb(cx);
    if (!js_Stringify(cx, &value, replacer, space, sb))
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
        } else {
            JS_ASSERT(c < ' ');
            if (!sb.append("\\u00"))
                return false;
            JS_ASSERT((c >> 4) < 10);
            uint8_t x = c >> 4, y = c % 16;
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
                     HandleObject replacer, const AutoIdVector &propertyList)
      : sb(sb),
        gap(gap),
        replacer(cx, replacer),
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
    RootedObject replacer;
    const AutoIdVector &propertyList;
    uint32_t depth;
    HashSet<JSObject *> objectStack;
};

static JSBool Str(JSContext *cx, const Value &v, StringifyContext *scx);

static JSBool
WriteIndent(JSContext *cx, StringifyContext *scx, uint32_t limit)
{
    if (!scx->gap.empty()) {
        if (!scx->sb.append('\n'))
            return JS_FALSE;
        for (uint32_t i = 0; i < limit; i++) {
            if (!scx->sb.append(scx->gap.begin(), scx->gap.end()))
                return JS_FALSE;
        }
    }

    return JS_TRUE;
}

class CycleDetector
{
  public:
    CycleDetector(JSContext *cx, StringifyContext *scx, JSObject *obj)
      : objectStack(scx->objectStack), obj(cx, obj) {
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
    RootedObject obj;
};

template<typename KeyType>
class KeyStringifier {
};

template<>
class KeyStringifier<uint32_t> {
  public:
    static JSString *toString(JSContext *cx, uint32_t index) {
        return IndexToString(cx, index);
    }
};

template<>
class KeyStringifier<HandleId> {
  public:
    static JSString *toString(JSContext *cx, HandleId id) {
        return IdToString(cx, id);
    }
};





template<typename KeyType>
static bool
PreprocessValue(JSContext *cx, HandleObject holder, KeyType key, MutableHandleValue vp, StringifyContext *scx)
{
    RootedString keyStr(cx);

    
    if (vp.get().isObject()) {
        RootedValue toJSON(cx);
        RootedId id(cx, NameToId(cx->names().toJSON));
        Rooted<JSObject*> obj(cx, &vp.get().toObject());
        if (!GetMethod(cx, obj, id, 0, &toJSON))
            return false;

        if (js_IsCallable(toJSON)) {
            keyStr = KeyStringifier<KeyType>::toString(cx, key);
            if (!keyStr)
                return false;

            InvokeArgsGuard args;
            if (!cx->stack.pushInvokeArgs(cx, 1, &args))
                return false;

            args.setCallee(toJSON);
            args.setThis(vp);
            args[0] = StringValue(keyStr);

            if (!Invoke(cx, args))
                return false;
            vp.set(args.rval());
        }
    }

    
    if (scx->replacer && scx->replacer->isCallable()) {
        if (!keyStr) {
            keyStr = KeyStringifier<KeyType>::toString(cx, key);
            if (!keyStr)
                return false;
        }

        InvokeArgsGuard args;
        if (!cx->stack.pushInvokeArgs(cx, 2, &args))
            return false;

        args.setCallee(ObjectValue(*scx->replacer));
        args.setThis(ObjectValue(*holder));
        args[0] = StringValue(keyStr);
        args[1] = vp;

        if (!Invoke(cx, args))
            return false;
        vp.set(args.rval());
    }

    
    if (vp.get().isObject()) {
        JSObject &obj = vp.get().toObject();
        if (ObjectClassIs(obj, ESClass_Number, cx)) {
            double d;
            if (!ToNumber(cx, vp, &d))
                return false;
            vp.set(NumberValue(d));
        } else if (ObjectClassIs(obj, ESClass_String, cx)) {
            JSString *str = ToStringSlow(cx, vp);
            if (!str)
                return false;
            vp.set(StringValue(str));
        } else if (ObjectClassIs(obj, ESClass_Boolean, cx)) {
            if (!BooleanGetPrimitiveValue(cx, obj, vp.address()))
                return false;
            JS_ASSERT(vp.get().isBoolean());
        }
    }

    return true;
}








static inline bool
IsFilteredValue(const Value &v)
{
    return v.isUndefined() || js_IsCallable(v) || VALUE_IS_XML(v);
}


static JSBool
JO(JSContext *cx, HandleObject obj, StringifyContext *scx)
{
    









    
    CycleDetector detect(cx, scx, obj);
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
    RootedId id(cx);
    for (size_t i = 0, len = propertyList.length(); i < len; i++) {
        






        id = propertyList[i];
        RootedValue outputValue(cx);
        if (!JSObject::getGeneric(cx, obj, obj, id, &outputValue))
            return false;
        if (!PreprocessValue(cx, obj, HandleId(id), &outputValue, scx))
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
JA(JSContext *cx, HandleObject obj, StringifyContext *scx)
{
    









    
    CycleDetector detect(cx, scx, obj);
    if (!detect.init(cx))
        return JS_FALSE;

    if (!scx->sb.append('['))
        return JS_FALSE;

    
    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    
    if (length != 0) {
        
        if (!WriteIndent(cx, scx, scx->depth))
            return JS_FALSE;

        
        RootedValue outputValue(cx);
        for (uint32_t i = 0; i < length; i++) {
            





            if (!JSObject::getElement(cx, obj, obj, i, &outputValue))
                return JS_FALSE;
            if (!PreprocessValue(cx, obj, i, &outputValue, scx))
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
            if (!MOZ_DOUBLE_IS_FINITE(v.toDouble()))
                return scx->sb.append("null");
        }

        StringBuffer sb(cx);
        if (!NumberValueToStringBuffer(cx, v, sb))
            return false;

        return scx->sb.append(sb.begin(), sb.length());
    }

    
    JS_ASSERT(v.isObject());
    RootedObject obj(cx, &v.toObject());

    scx->depth++;
    JSBool ok;
    if (ObjectClassIs(v.toObject(), ESClass_Array, cx))
        ok = JA(cx, obj, scx);
    else
        ok = JO(cx, obj, scx);
    scx->depth--;

    return ok;
}


JSBool
js_Stringify(JSContext *cx, MutableHandleValue vp, JSObject *replacer_, Value space_,
             StringBuffer &sb)
{
    RootedObject replacer(cx, replacer_);
    RootedValue spaceRoot(cx, space_);
    Value &space = spaceRoot.get();

    
    AutoIdVector propertyList(cx);
    if (replacer) {
        if (replacer->isCallable()) {
            
        } else if (ObjectClassIs(*replacer, ESClass_Array, cx)) {
            



























            
            uint32_t len;
            JS_ALWAYS_TRUE(GetLengthProperty(cx, replacer, &len));
            if (replacer->isArray() && !replacer->isIndexed())
                len = Min(len, replacer->getDenseInitializedLength());

            HashSet<jsid, JsidHasher> idSet(cx);
            if (!idSet.init(len))
                return false;

            
            uint32_t i = 0;

            
            RootedValue v(cx);
            for (; i < len; i++) {
                
                if (!JSObject::getElement(cx, replacer, replacer, i, &v))
                    return false;

                jsid id;
                if (v.isNumber()) {
                    
                    int32_t n;
                    if (v.isNumber() && ValueFitsInInt32(v, &n) && INT_FITS_IN_JSID(n)) {
                        id = INT_TO_JSID(n);
                    } else {
                        if (!ValueToId(cx, v, &id))
                            return false;
                    }
                } else if (v.isString() ||
                           (v.isObject() &&
                            (ObjectClassIs(v.toObject(), ESClass_String, cx) ||
                             ObjectClassIs(v.toObject(), ESClass_Number, cx))))
                {
                    
                    if (!ValueToId(cx, v, &id))
                        return false;
                } else {
                    continue;
                }

                
                HashSet<jsid, JsidHasher>::AddPtr p = idSet.lookupForAdd(id);
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
        if (ObjectClassIs(spaceObj, ESClass_Number, cx)) {
            double d;
            if (!ToNumber(cx, space, &d))
                return false;
            space = NumberValue(d);
        } else if (ObjectClassIs(spaceObj, ESClass_String, cx)) {
            JSString *str = ToStringSlow(cx, space);
            if (!str)
                return false;
            space = StringValue(str);
        }
    }

    StringBuffer gap(cx);

    if (space.isNumber()) {
        
        double d;
        JS_ALWAYS_TRUE(ToInteger(cx, space, &d));
        d = Min(10.0, d);
        if (d >= 1 && !gap.appendN(' ', uint32_t(d)))
            return false;
    } else if (space.isString()) {
        
        JSLinearString *str = space.toString()->ensureLinear(cx);
        if (!str)
            return false;
        JS::Anchor<JSString *> anchor(str);
        size_t len = Min(size_t(10), space.toString()->length());
        if (!gap.append(str->chars(), len))
            return false;
    } else {
        
        JS_ASSERT(gap.empty());
    }

    
    RootedObject wrapper(cx, NewBuiltinClassInstance(cx, &ObjectClass));
    if (!wrapper)
        return false;

    
    RootedId emptyId(cx, NameToId(cx->names().empty));
    if (!DefineNativeProperty(cx, wrapper, emptyId, vp, JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_ENUMERATE, 0, 0))
    {
        return false;
    }

    
    StringifyContext scx(cx, sb, gap, replacer, propertyList);
    if (!scx.init())
        return false;

    if (!PreprocessValue(cx, wrapper, HandleId(emptyId), vp, &scx))
        return false;
    if (IsFilteredValue(vp))
        return true;

    return Str(cx, vp, &scx);
}


static bool
Walk(JSContext *cx, HandleObject holder, HandleId name, HandleValue reviver, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    
    RootedValue val(cx);
    if (!JSObject::getGeneric(cx, holder, holder, name, &val))
        return false;

    
    if (val.isObject()) {
        RootedObject obj(cx, &val.toObject());

        
        JS_ASSERT(!obj->isProxy());
        if (obj->isArray()) {
            
            uint32_t length = obj->getArrayLength();

            
            RootedId id(cx);
            RootedValue newElement(cx);
            for (uint32_t i = 0; i < length; i++) {
                if (!IndexToId(cx, i, id.address()))
                    return false;

                
                if (!Walk(cx, obj, id, reviver, &newElement))
                    return false;

                if (newElement.isUndefined()) {
                    
                    if (!JSObject::deleteByValue(cx, obj, IdToValue(id), &newElement, false))
                        return false;
                } else {
                    
                    if (!DefineNativeProperty(cx, obj, id, newElement, JS_PropertyStub,
                                              JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0))
                    {
                        return false;
                    }
                }
            }
        } else {
            
            AutoIdVector keys(cx);
            if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, &keys))
                return false;

            
            RootedId id(cx);
            RootedValue newElement(cx);
            for (size_t i = 0, len = keys.length(); i < len; i++) {
                
                id = keys[i];
                if (!Walk(cx, obj, id, reviver, &newElement))
                    return false;

                if (newElement.isUndefined()) {
                    
                    if (!JSObject::deleteByValue(cx, obj, IdToValue(id), &newElement, false))
                        return false;
                } else {
                    
                    JS_ASSERT(obj->isNative());
                    if (!DefineNativeProperty(cx, obj, id, newElement, JS_PropertyStub,
                                              JS_StrictPropertyStub, JSPROP_ENUMERATE, 0, 0))
                    {
                        return false;
                    }
                }
            }
        }
    }

    
    RootedString key(cx, IdToString(cx, name));
    if (!key)
        return false;

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 2, &args))
        return false;

    args.setCallee(reviver);
    args.setThis(ObjectValue(*holder));
    args[0] = StringValue(key);
    args[1] = val;

    if (!Invoke(cx, args))
        return false;
    vp.set(args.rval());
    return true;
}

static bool
Revive(JSContext *cx, HandleValue reviver, MutableHandleValue vp)
{
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &ObjectClass));
    if (!obj)
        return false;

    if (!JSObject::defineProperty(cx, obj, cx->names().empty, vp))
        return false;

    Rooted<jsid> id(cx, NameToId(cx->names().empty));
    return Walk(cx, obj, id, reviver, vp);
}

JSBool
js::ParseJSONWithReviver(JSContext *cx, StableCharPtr chars, size_t length, HandleValue reviver,
                         MutableHandleValue vp, DecodingMode decodingMode )
{
    
    JSONParser parser(cx, chars, length,
                      decodingMode == STRICT ? JSONParser::StrictJSON : JSONParser::LegacyJSON);
    if (!parser.parse(vp))
        return false;

    
    if (js_IsCallable(reviver))
        return Revive(cx, reviver, vp);
    return true;
}

#if JS_HAS_TOSOURCE
static JSBool
json_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    vp->setString(cx->names().JSON);
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
js_InitJSONClass(JSContext *cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->asGlobal());

    




    if (!global->getOrCreateBooleanPrototype(cx))
        return NULL;

    RootedObject JSON(cx, NewObjectWithClassProto(cx, &JSONClass, NULL, global));
    if (!JSON || !JSObject::setSingletonType(cx, JSON))
        return NULL;

    if (!JS_DefineProperty(cx, global, js_JSON_str, OBJECT_TO_JSVAL(JSON),
                           JS_PropertyStub, JS_StrictPropertyStub, 0))
        return NULL;

    if (!JS_DefineFunctions(cx, JSON, json_static_methods))
        return NULL;

    MarkStandardClassInitializedNoProto(global, &JSONClass);

    return JSON;
}
