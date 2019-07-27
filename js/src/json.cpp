





#include "json.h"

#include "mozilla/FloatingPoint.h"
#include "mozilla/Range.h"

#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsonparser.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsutil.h"

#include "vm/Interpreter.h"
#include "vm/StringBuffer.h"

#include "jsatominlines.h"
#include "jsboolinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::IsFinite;
using mozilla::Maybe;
using mozilla::RangedPtr;

const Class js::JSONClass = {
    js_JSON_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_JSON),
    JS_PropertyStub,        
    JS_DeletePropertyStub,  
    JS_PropertyStub,        
    JS_StrictPropertyStub,  
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static inline bool IsQuoteSpecialCharacter(char16_t c)
{
    JS_STATIC_ASSERT('\b' < ' ');
    JS_STATIC_ASSERT('\f' < ' ');
    JS_STATIC_ASSERT('\n' < ' ');
    JS_STATIC_ASSERT('\r' < ' ');
    JS_STATIC_ASSERT('\t' < ' ');
    return c == '"' || c == '\\' || c < ' ';
}


template <typename CharT>
static bool
Quote(StringBuffer &sb, JSLinearString *str)
{
    size_t len = str->length();

    
    if (!sb.append('"'))
        return false;

    
    JS::AutoCheckCannotGC nogc;
    const RangedPtr<const CharT> buf(str->chars<CharT>(nogc), len);
    for (size_t i = 0; i < len; ++i) {
        
        size_t mark = i;
        do {
            if (IsQuoteSpecialCharacter(buf[i]))
                break;
        } while (++i < len);
        if (i > mark) {
            if (!sb.appendSubstring(str, mark, i - mark))
                return false;
            if (i == len)
                break;
        }

        char16_t c = buf[i];
        if (c == '"' || c == '\\') {
            if (!sb.append('\\') || !sb.append(c))
                return false;
        } else if (c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t') {
           char16_t abbrev = (c == '\b')
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
            if (!sb.append(Latin1Char('0' + x)) ||
                !sb.append(Latin1Char(y < 10 ? '0' + y : 'a' + (y - 10))))
            {
                return false;
            }
        }
    }

    
    return sb.append('"');
}

static bool
Quote(JSContext *cx, StringBuffer &sb, JSString *str)
{
    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return false;

    return linear->hasLatin1Chars()
           ? Quote<Latin1Char>(sb, linear)
           : Quote<char16_t>(sb, linear);
}

namespace {

class StringifyContext
{
  public:
    StringifyContext(JSContext *cx, StringBuffer &sb, const StringBuffer &gap,
                     HandleObject replacer, const AutoIdVector &propertyList)
      : sb(sb),
        gap(gap),
        replacer(cx, replacer),
        propertyList(propertyList),
        depth(0)
    {}

    StringBuffer &sb;
    const StringBuffer &gap;
    RootedObject replacer;
    const AutoIdVector &propertyList;
    uint32_t depth;
};

} 

static bool Str(JSContext *cx, const Value &v, StringifyContext *scx);

static bool
WriteIndent(JSContext *cx, StringifyContext *scx, uint32_t limit)
{
    if (!scx->gap.empty()) {
        if (!scx->sb.append('\n'))
            return false;

        if (scx->gap.isUnderlyingBufferLatin1()) {
            for (uint32_t i = 0; i < limit; i++) {
                if (!scx->sb.append(scx->gap.rawLatin1Begin(), scx->gap.rawLatin1End()))
                    return false;
            }
        } else {
            for (uint32_t i = 0; i < limit; i++) {
                if (!scx->sb.append(scx->gap.rawTwoByteBegin(), scx->gap.rawTwoByteEnd()))
                    return false;
            }
        }
    }

    return true;
}

namespace {

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

} 





template<typename KeyType>
static bool
PreprocessValue(JSContext *cx, HandleObject holder, KeyType key, MutableHandleValue vp, StringifyContext *scx)
{
    RootedString keyStr(cx);

    
    if (vp.isObject()) {
        RootedValue toJSON(cx);
        RootedObject obj(cx, &vp.toObject());
        if (!JSObject::getProperty(cx, obj, obj, cx->names().toJSON, &toJSON))
            return false;

        if (IsCallable(toJSON)) {
            keyStr = KeyStringifier<KeyType>::toString(cx, key);
            if (!keyStr)
                return false;

            InvokeArgs args(cx);
            if (!args.init(1))
                return false;

            args.setCallee(toJSON);
            args.setThis(vp);
            args[0].setString(keyStr);

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

        InvokeArgs args(cx);
        if (!args.init(2))
            return false;

        args.setCallee(ObjectValue(*scx->replacer));
        args.setThis(ObjectValue(*holder));
        args[0].setString(keyStr);
        args[1].set(vp);

        if (!Invoke(cx, args))
            return false;
        vp.set(args.rval());
    }

    
    if (vp.get().isObject()) {
        RootedObject obj(cx, &vp.get().toObject());
        if (ObjectClassIs(obj, ESClass_Number, cx)) {
            double d;
            if (!ToNumber(cx, vp, &d))
                return false;
            vp.set(NumberValue(d));
        } else if (ObjectClassIs(obj, ESClass_String, cx)) {
            JSString *str = ToStringSlow<CanGC>(cx, vp);
            if (!str)
                return false;
            vp.set(StringValue(str));
        } else if (ObjectClassIs(obj, ESClass_Boolean, cx)) {
            vp.setBoolean(BooleanGetPrimitiveValue(obj));
        }
    }

    return true;
}








static inline bool
IsFilteredValue(const Value &v)
{
    return v.isUndefined() || v.isSymbol() || IsCallable(v);
}


static bool
JO(JSContext *cx, HandleObject obj, StringifyContext *scx)
{
    









    
    AutoCycleDetector detect(cx, obj);
    if (!detect.init())
        return false;
    if (detect.foundCycle()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_JSON_CYCLIC_VALUE,
                             js_object_str);
        return false;
    }

    if (!scx->sb.append('{'))
        return false;

    
    Maybe<AutoIdVector> ids;
    const AutoIdVector *props;
    if (scx->replacer && !scx->replacer->isCallable()) {
        JS_ASSERT(JS_IsArrayObject(cx, scx->replacer));
        props = &scx->propertyList;
    } else {
        JS_ASSERT_IF(scx->replacer, scx->propertyList.length() == 0);
        ids.emplace(cx);
        if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, ids.ptr()))
            return false;
        props = ids.ptr();
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


static bool
JA(JSContext *cx, HandleObject obj, StringifyContext *scx)
{
    









    
    AutoCycleDetector detect(cx, obj);
    if (!detect.init())
        return false;
    if (detect.foundCycle()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_JSON_CYCLIC_VALUE,
                             js_object_str);
        return false;
    }

    if (!scx->sb.append('['))
        return false;

    
    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    
    if (length != 0) {
        
        if (!WriteIndent(cx, scx, scx->depth))
            return false;

        
        RootedValue outputValue(cx);
        for (uint32_t i = 0; i < length; i++) {
            





            if (!JSObject::getElement(cx, obj, obj, i, &outputValue))
                return false;
            if (!PreprocessValue(cx, obj, i, &outputValue, scx))
                return false;
            if (IsFilteredValue(outputValue)) {
                if (!scx->sb.append("null"))
                    return false;
            } else {
                if (!Str(cx, outputValue, scx))
                    return false;
            }

            
            if (i < length - 1) {
                if (!scx->sb.append(','))
                    return false;
                if (!WriteIndent(cx, scx, scx->depth))
                    return false;
            }
        }

        
        if (!WriteIndent(cx, scx, scx->depth - 1))
            return false;
    }

    return scx->sb.append(']');
}

static bool
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
            if (!IsFinite(v.toDouble()))
                return scx->sb.append("null");
        }

        return NumberValueToStringBuffer(cx, v, scx->sb);
    }

    
    JS_ASSERT(v.isObject());
    RootedObject obj(cx, &v.toObject());

    scx->depth++;
    bool ok;
    if (ObjectClassIs(obj, ESClass_Array, cx))
        ok = JA(cx, obj, scx);
    else
        ok = JO(cx, obj, scx);
    scx->depth--;

    return ok;
}


bool
js_Stringify(JSContext *cx, MutableHandleValue vp, JSObject *replacer_, Value space_,
             StringBuffer &sb)
{
    RootedObject replacer(cx, replacer_);
    RootedValue space(cx, space_);

    
    AutoIdVector propertyList(cx);
    if (replacer) {
        if (replacer->isCallable()) {
            
        } else if (ObjectClassIs(replacer, ESClass_Array, cx)) {
            



























            
            uint32_t len;
            JS_ALWAYS_TRUE(GetLengthProperty(cx, replacer, &len));
            if (replacer->is<ArrayObject>() && !replacer->isIndexed())
                len = Min(len, replacer->getDenseInitializedLength());

            
            
            
            
            const uint32_t MaxInitialSize = 1024;
            HashSet<jsid, JsidHasher> idSet(cx);
            if (!idSet.init(Min(len, MaxInitialSize)))
                return false;

            
            uint32_t i = 0;

            
            RootedValue v(cx);
            for (; i < len; i++) {
                if (!CheckForInterrupt(cx))
                    return false;

                
                if (!JSObject::getElement(cx, replacer, replacer, i, &v))
                    return false;

                RootedId id(cx);
                if (v.isNumber()) {
                    
                    int32_t n;
                    if (v.isNumber() && ValueFitsInInt32(v, &n) && INT_FITS_IN_JSID(n)) {
                        id = INT_TO_JSID(n);
                    } else {
                        if (!ValueToId<CanGC>(cx, v, &id))
                            return false;
                    }
                } else if (v.isString() ||
                           IsObjectWithClass(v, ESClass_String, cx) ||
                           IsObjectWithClass(v, ESClass_Number, cx))
                {
                    
                    if (!ValueToId<CanGC>(cx, v, &id))
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
            replacer = nullptr;
        }
    }

    
    if (space.isObject()) {
        RootedObject spaceObj(cx, &space.toObject());
        if (ObjectClassIs(spaceObj, ESClass_Number, cx)) {
            double d;
            if (!ToNumber(cx, space, &d))
                return false;
            space = NumberValue(d);
        } else if (ObjectClassIs(spaceObj, ESClass_String, cx)) {
            JSString *str = ToStringSlow<CanGC>(cx, space);
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
        size_t len = Min(size_t(10), str->length());
        if (!gap.appendSubstring(str, 0, len))
            return false;
    } else {
        
        JS_ASSERT(gap.empty());
    }

    
    RootedObject wrapper(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
    if (!wrapper)
        return false;

    
    RootedId emptyId(cx, NameToId(cx->names().empty));
    if (!DefineNativeProperty(cx, wrapper, emptyId, vp, JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_ENUMERATE))
    {
        return false;
    }

    
    StringifyContext scx(cx, sb, gap, replacer, propertyList);
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

        if (ObjectClassIs(obj, ESClass_Array, cx)) {
            
            uint32_t length;
            if (!GetLengthProperty(cx, obj, &length))
                return false;

            
            RootedId id(cx);
            RootedValue newElement(cx);
            for (uint32_t i = 0; i < length; i++) {
                if (!IndexToId(cx, i, &id))
                    return false;

                
                if (!Walk(cx, obj, id, reviver, &newElement))
                    return false;

                if (newElement.isUndefined()) {
                    
                    bool succeeded;
                    if (!JSObject::deleteGeneric(cx, obj, id, &succeeded))
                        return false;
                } else {
                    
                    
                    
                    if (!JSObject::defineGeneric(cx, obj, id, newElement))
                        return false;
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
                    
                    bool succeeded;
                    if (!JSObject::deleteGeneric(cx, obj, id, &succeeded))
                        return false;
                } else {
                    
                    
                    
                    if (!JSObject::defineGeneric(cx, obj, id, newElement))
                        return false;
                }
            }
        }
    }

    
    RootedString key(cx, IdToString(cx, name));
    if (!key)
        return false;

    InvokeArgs args(cx);
    if (!args.init(2))
        return false;

    args.setCallee(reviver);
    args.setThis(ObjectValue(*holder));
    args[0].setString(key);
    args[1].set(val);

    if (!Invoke(cx, args))
        return false;
    vp.set(args.rval());
    return true;
}

static bool
Revive(JSContext *cx, HandleValue reviver, MutableHandleValue vp)
{
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
    if (!obj)
        return false;

    if (!JSObject::defineProperty(cx, obj, cx->names().empty, vp))
        return false;

    Rooted<jsid> id(cx, NameToId(cx->names().empty));
    return Walk(cx, obj, id, reviver, vp);
}

template <typename CharT>
bool
js::ParseJSONWithReviver(JSContext *cx, const mozilla::Range<const CharT> chars, HandleValue reviver,
                         MutableHandleValue vp)
{
    
    JSONParser<CharT> parser(cx, chars);
    if (!parser.parse(vp))
        return false;

    
    if (IsCallable(reviver))
        return Revive(cx, reviver, vp);
    return true;
}

template bool
js::ParseJSONWithReviver(JSContext *cx, const mozilla::Range<const Latin1Char> chars,
                         HandleValue reviver, MutableHandleValue vp);

template bool
js::ParseJSONWithReviver(JSContext *cx, const mozilla::Range<const char16_t> chars, HandleValue reviver,
                         MutableHandleValue vp);

#if JS_HAS_TOSOURCE
static bool
json_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setString(cx->names().JSON);
    return true;
}
#endif


static bool
json_parse(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    JSString *str = (args.length() >= 1)
                    ? ToString<CanGC>(cx, args[0])
                    : cx->names().undefined;
    if (!str)
        return false;

    JSFlatString *flat = str->ensureFlat(cx);
    if (!flat)
        return false;

    AutoStableStringChars flatChars(cx);
    if (!flatChars.init(cx, flat))
        return false;

    RootedValue reviver(cx, args.get(1));

    
    return flatChars.isLatin1()
           ? ParseJSONWithReviver(cx, flatChars.latin1Range(), reviver, args.rval())
           : ParseJSONWithReviver(cx, flatChars.twoByteRange(), reviver, args.rval());
}


bool
json_stringify(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject replacer(cx, args.get(1).isObject() ? &args[1].toObject() : nullptr);
    RootedValue value(cx, args.get(0));
    RootedValue space(cx, args.get(2));

    StringBuffer sb(cx);
    if (!js_Stringify(cx, &value, replacer, space, sb))
        return false;

    
    
    
    if (!sb.empty()) {
        JSString *str = sb.finishString();
        if (!str)
            return false;
        args.rval().setString(str);
    } else {
        args.rval().setUndefined();
    }

    return true;
}

static const JSFunctionSpec json_static_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  json_toSource,      0, 0),
#endif
    JS_FN("parse",          json_parse,         2, 0),
    JS_FN("stringify",      json_stringify,     3, 0),
    JS_FS_END
};

JSObject *
js_InitJSONClass(JSContext *cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    




    if (!GlobalObject::getOrCreateBooleanPrototype(cx, global))
        return nullptr;

    RootedObject proto(cx, obj->as<GlobalObject>().getOrCreateObjectPrototype(cx));
    RootedObject JSON(cx, NewObjectWithClassProto(cx, &JSONClass, proto, global, SingletonObject));
    if (!JSON)
        return nullptr;

    if (!JS_DefineProperty(cx, global, js_JSON_str, JSON, 0,
                           JS_PropertyStub, JS_StrictPropertyStub))
        return nullptr;

    if (!JS_DefineFunctions(cx, JSON, json_static_methods))
        return nullptr;

    global->setConstructor(JSProto_JSON, ObjectValue(*JSON));

    return JSON;
}
