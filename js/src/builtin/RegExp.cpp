







































#include "jscntxt.h"

#include "builtin/RegExp.h"

#include "vm/MethodGuard-inl.h"
#include "vm/RegExpObject-inl.h"
#include "vm/RegExpStatics-inl.h"

using namespace js;
using namespace js::types;

class RegExpMatchBuilder
{
    JSContext   * const cx;
    JSObject    * const array;

    bool setProperty(JSAtom *name, Value v) {
        return !!js_DefineProperty(cx, array, ATOM_TO_JSID(name), &v,
                                   JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE);
    }

  public:
    RegExpMatchBuilder(JSContext *cx, JSObject *array) : cx(cx), array(array) {}

    bool append(uint32_t index, Value v) {
        JS_ASSERT(!array->getOps()->getElement);
        return !!js_DefineElement(cx, array, index, &v, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_ENUMERATE);
    }

    bool setIndex(int index) {
        return setProperty(cx->runtime->atomState.indexAtom, Int32Value(index));
    }

    bool setInput(JSString *str) {
        JS_ASSERT(str);
        return setProperty(cx->runtime->atomState.inputAtom, StringValue(str));
    }
};

static bool
CreateRegExpMatchResult(JSContext *cx, JSString *input, const jschar *chars, size_t length,
                        MatchPairs *matchPairs, Value *rval)
{
    








    JSObject *array = NewSlowEmptyArray(cx);
    if (!array)
        return false;

    if (!input) {
        input = js_NewStringCopyN(cx, chars, length);
        if (!input)
            return false;
    }

    RegExpMatchBuilder builder(cx, array);

    for (size_t i = 0; i < matchPairs->pairCount(); ++i) {
        MatchPair pair = matchPairs->pair(i);

        JSString *captured;
        if (pair.isUndefined()) {
            JS_ASSERT(i != 0); 
            if (!builder.append(i, UndefinedValue()))
                return false;
        } else {
            captured = js_NewDependentString(cx, input, pair.start, pair.length());
            if (!captured || !builder.append(i, StringValue(captured)))
                return false;
        }
    }

    if (!builder.setIndex(matchPairs->pair(0).start) || !builder.setInput(input))
        return false;

    *rval = ObjectValue(*array);
    return true;
}

template <class T>
bool
ExecuteRegExpImpl(JSContext *cx, RegExpStatics *res, T &re, JSLinearString *input,
                  const jschar *chars, size_t length,
                  size_t *lastIndex, RegExpExecType type, Value *rval)
{
    LifoAllocScope allocScope(&cx->tempLifoAlloc());
    MatchPairs *matchPairs = NULL;
    RegExpRunStatus status = re.execute(cx, chars, length, lastIndex, &matchPairs);

    switch (status) {
      case RegExpRunStatus_Error:
        return false;
      case RegExpRunStatus_Success_NotFound:
        *rval = NullValue();
        return true;
      default:
        JS_ASSERT(status == RegExpRunStatus_Success);
        JS_ASSERT(matchPairs);
    }

    if (res)
        res->updateFromMatchPairs(cx, input, matchPairs);

    *lastIndex = matchPairs->pair(0).limit;

    if (type == RegExpTest) {
        *rval = BooleanValue(true);
        return true;
    }

    return CreateRegExpMatchResult(cx, input, chars, length, matchPairs, rval);
}

bool
js::ExecuteRegExp(JSContext *cx, RegExpStatics *res, RegExpShared &shared, JSLinearString *input,
                  const jschar *chars, size_t length,
                  size_t *lastIndex, RegExpExecType type, Value *rval)
{
    return ExecuteRegExpImpl(cx, res, shared, input, chars, length, lastIndex, type, rval);
}

bool
js::ExecuteRegExp(JSContext *cx, RegExpStatics *res, RegExpObject &reobj, JSLinearString *input,
                  const jschar *chars, size_t length,
                  size_t *lastIndex, RegExpExecType type, Value *rval)
{
    return ExecuteRegExpImpl(cx, res, reobj, input, chars, length, lastIndex, type, rval);
}


static JSAtom *
EscapeNakedForwardSlashes(JSContext *cx, JSAtom *unescaped)
{
    size_t oldLen = unescaped->length();
    const jschar *oldChars = unescaped->chars();

    JS::Anchor<JSString *> anchor(unescaped);

    
    StringBuffer sb(cx);

    for (const jschar *it = oldChars; it < oldChars + oldLen; ++it) {
        if (*it == '/' && (it == oldChars || it[-1] != '\\')) {
            
            if (sb.empty()) {
                
                if (!sb.reserve(oldLen + 1))
                    return NULL;
                sb.infallibleAppend(oldChars, size_t(it - oldChars));
            }
            if (!sb.append('\\'))
                return NULL;
        }

        if (!sb.empty() && !sb.append(*it))
            return NULL;
    }

    return sb.empty() ? unescaped : sb.finishAtom();
}












static bool
CompileRegExpObject(JSContext *cx, RegExpObjectBuilder &builder, CallArgs args)
{
    if (args.length() == 0) {
        RegExpStatics *res = cx->regExpStatics();
        RegExpObject *reobj = builder.build(cx->runtime->emptyString, res->getFlags());
        if (!reobj)
            return false;
        args.rval() = ObjectValue(*reobj);
        return true;
    }

    Value sourceValue = args[0];

    



    if (IsObjectWithClass(sourceValue, ESClass_RegExp, cx)) {
        




        JSObject &sourceObj = sourceValue.toObject();

        if (args.length() >= 2 && !args[1].isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NEWREGEXP_FLAGGED);
            return false;
        }

        



        RegExpFlag flags;
        {
            RegExpGuard g;
            if (!RegExpToShared(cx, sourceObj, &g))
                return false;

            flags = g->getFlags();
        }

        



        Value v;
        if (!sourceObj.getProperty(cx, cx->runtime->atomState.sourceAtom, &v))
            return false;

        RegExpObject *reobj = builder.build(&v.toString()->asAtom(), flags);
        if (!reobj)
            return false;

        args.rval() = ObjectValue(*reobj);
        return true;
    }

    JSAtom *source;
    if (sourceValue.isUndefined()) {
        source = cx->runtime->emptyString;
    } else {
        
        JSString *str = ToString(cx, sourceValue);
        if (!str)
            return false;

        source = js_AtomizeString(cx, str);
        if (!source)
            return false;
    }

    RegExpFlag flags = RegExpFlag(0);
    if (args.length() > 1 && !args[1].isUndefined()) {
        JSString *flagStr = ToString(cx, args[1]);
        if (!flagStr)
            return false;
        args[1].setString(flagStr);
        if (!ParseRegExpFlags(cx, flagStr, &flags))
            return false;
    }

    JSAtom *escapedSourceStr = EscapeNakedForwardSlashes(cx, source);
    if (!escapedSourceStr)
        return false;

    if (!js::detail::RegExpCode::checkSyntax(cx, NULL, escapedSourceStr))
        return false;

    RegExpStatics *res = cx->regExpStatics();
    RegExpObject *reobj = builder.build(escapedSourceStr, RegExpFlag(flags | res->getFlags()));
    if (!reobj)
        return NULL;

    args.rval() = ObjectValue(*reobj);
    return true;
}

static JSBool
regexp_compile(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, regexp_compile, &RegExpClass, &ok);
    if (!obj)
        return ok;

    RegExpObjectBuilder builder(cx, &obj->asRegExp());
    return CompileRegExpObject(cx, builder, args);
}

static JSBool
regexp_construct(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!IsConstructing(args)) {
        




        if (args.length() >= 1 && IsObjectWithClass(args[0], ESClass_RegExp, cx) &&
            (args.length() == 1 || args[1].isUndefined()))
        {
            args.rval() = args[0];
            return true;
        }
    }

    RegExpObjectBuilder builder(cx);
    return CompileRegExpObject(cx, builder, args);
}

static JSBool
regexp_toString(JSContext *cx, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, regexp_toString, &RegExpClass, &ok);
    if (!obj)
        return ok;

    JSString *str = obj->asRegExp().toString(cx);
    if (!str)
        return false;

    *vp = StringValue(str);
    return true;
}

static JSFunctionSpec regexp_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  regexp_toString,    0,0),
#endif
    JS_FN(js_toString_str,  regexp_toString,    0,0),
    JS_FN("compile",        regexp_compile,     2,0),
    JS_FN("exec",           regexp_exec,        1,0),
    JS_FN("test",           regexp_test,        1,0),
    JS_FS_END
};














#define DEFINE_STATIC_GETTER(name, code)                                        \
    static JSBool                                                               \
    name(JSContext *cx, JSObject *obj, jsid id, jsval *vp)                      \
    {                                                                           \
        RegExpStatics *res = cx->regExpStatics();                               \
        code;                                                                   \
    }

DEFINE_STATIC_GETTER(static_input_getter,        return res->createPendingInput(cx, vp))
DEFINE_STATIC_GETTER(static_multiline_getter,    *vp = BOOLEAN_TO_JSVAL(res->multiline());
                                                 return true)
DEFINE_STATIC_GETTER(static_lastMatch_getter,    return res->createLastMatch(cx, vp))
DEFINE_STATIC_GETTER(static_lastParen_getter,    return res->createLastParen(cx, vp))
DEFINE_STATIC_GETTER(static_leftContext_getter,  return res->createLeftContext(cx, vp))
DEFINE_STATIC_GETTER(static_rightContext_getter, return res->createRightContext(cx, vp))

DEFINE_STATIC_GETTER(static_paren1_getter,       return res->createParen(cx, 1, vp))
DEFINE_STATIC_GETTER(static_paren2_getter,       return res->createParen(cx, 2, vp))
DEFINE_STATIC_GETTER(static_paren3_getter,       return res->createParen(cx, 3, vp))
DEFINE_STATIC_GETTER(static_paren4_getter,       return res->createParen(cx, 4, vp))
DEFINE_STATIC_GETTER(static_paren5_getter,       return res->createParen(cx, 5, vp))
DEFINE_STATIC_GETTER(static_paren6_getter,       return res->createParen(cx, 6, vp))
DEFINE_STATIC_GETTER(static_paren7_getter,       return res->createParen(cx, 7, vp))
DEFINE_STATIC_GETTER(static_paren8_getter,       return res->createParen(cx, 8, vp))
DEFINE_STATIC_GETTER(static_paren9_getter,       return res->createParen(cx, 9, vp))

#define DEFINE_STATIC_SETTER(name, code)                                        \
    static JSBool                                                               \
    name(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)       \
    {                                                                           \
        RegExpStatics *res = cx->regExpStatics();                               \
        code;                                                                   \
        return true;                                                            \
    }

DEFINE_STATIC_SETTER(static_input_setter,
                     if (!JSVAL_IS_STRING(*vp) && !JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp))
                         return false;
                     res->setPendingInput(JSVAL_TO_STRING(*vp)))
DEFINE_STATIC_SETTER(static_multiline_setter,
                     if (!JSVAL_IS_BOOLEAN(*vp) && !JS_ConvertValue(cx, *vp, JSTYPE_BOOLEAN, vp))
                         return false;
                     res->setMultiline(cx, !!JSVAL_TO_BOOLEAN(*vp)))

const uint8_t REGEXP_STATIC_PROP_ATTRS    = JSPROP_PERMANENT | JSPROP_SHARED | JSPROP_ENUMERATE;
const uint8_t RO_REGEXP_STATIC_PROP_ATTRS = REGEXP_STATIC_PROP_ATTRS | JSPROP_READONLY;

const uint8_t HIDDEN_PROP_ATTRS = JSPROP_PERMANENT | JSPROP_SHARED;
const uint8_t RO_HIDDEN_PROP_ATTRS = HIDDEN_PROP_ATTRS | JSPROP_READONLY;

static JSPropertySpec regexp_static_props[] = {
    {"input",        0, REGEXP_STATIC_PROP_ATTRS,    static_input_getter, static_input_setter},
    {"multiline",    0, REGEXP_STATIC_PROP_ATTRS,    static_multiline_getter,
                                                     static_multiline_setter},
    {"lastMatch",    0, RO_REGEXP_STATIC_PROP_ATTRS, static_lastMatch_getter,    NULL},
    {"lastParen",    0, RO_REGEXP_STATIC_PROP_ATTRS, static_lastParen_getter,    NULL},
    {"leftContext",  0, RO_REGEXP_STATIC_PROP_ATTRS, static_leftContext_getter,  NULL},
    {"rightContext", 0, RO_REGEXP_STATIC_PROP_ATTRS, static_rightContext_getter, NULL},
    {"$1",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren1_getter,       NULL},
    {"$2",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren2_getter,       NULL},
    {"$3",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren3_getter,       NULL},
    {"$4",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren4_getter,       NULL},
    {"$5",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren5_getter,       NULL},
    {"$6",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren6_getter,       NULL},
    {"$7",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren7_getter,       NULL},
    {"$8",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren8_getter,       NULL},
    {"$9",           0, RO_REGEXP_STATIC_PROP_ATTRS, static_paren9_getter,       NULL},

    {"$_",           0, HIDDEN_PROP_ATTRS,    static_input_getter, static_input_setter},
    {"$*",           0, HIDDEN_PROP_ATTRS,    static_multiline_getter, static_multiline_setter},
    {"$&",           0, RO_HIDDEN_PROP_ATTRS, static_lastMatch_getter, NULL},
    {"$+",           0, RO_HIDDEN_PROP_ATTRS, static_lastParen_getter, NULL},
    {"$`",           0, RO_HIDDEN_PROP_ATTRS, static_leftContext_getter, NULL},
    {"$'",           0, RO_HIDDEN_PROP_ATTRS, static_rightContext_getter, NULL},
    {0,0,0,0,0}
};

JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *proto = global->createBlankPrototype(cx, &RegExpClass);
    if (!proto)
        return NULL;
    proto->setPrivate(NULL);

    RegExpObject *reproto = &proto->asRegExp();
    RegExpObjectBuilder builder(cx, reproto);
    if (!builder.build(cx->runtime->emptyString, RegExpFlag(0)))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, proto, NULL, regexp_methods))
        return NULL;

    JSFunction *ctor = global->createConstructor(cx, regexp_construct, &RegExpClass,
                                                 CLASS_ATOM(cx, RegExp), 2);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return NULL;

    
    if (!JS_DefineProperties(cx, ctor, regexp_static_props))
        return NULL;

    
    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;
    AddTypeProperty(cx, type, "source", Type::StringType());
    AddTypeProperty(cx, type, "global", Type::BooleanType());
    AddTypeProperty(cx, type, "ignoreCase", Type::BooleanType());
    AddTypeProperty(cx, type, "multiline", Type::BooleanType());
    AddTypeProperty(cx, type, "sticky", Type::BooleanType());
    AddTypeProperty(cx, type, "lastIndex", Type::Int32Type());

    if (!DefineConstructorAndPrototype(cx, global, JSProto_RegExp, ctor, proto))
        return NULL;

    return proto;
}


static const jschar GreedyStarChars[] = {'.', '*'};

static inline bool
StartsWithGreedyStar(JSAtom *source)
{
    return false;

#if 0
    if (source->length() < 3)
        return false;

    const jschar *chars = source->chars();
    return chars[0] == GreedyStarChars[0] &&
           chars[1] == GreedyStarChars[1] &&
           chars[2] != '?';
#endif
}

static inline bool
GetSharedForGreedyStar(JSContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g)
{
    if (cx->compartment->regExps.lookupHack(source, flags, cx, g))
        return true;

    JSAtom *hackedSource = js_AtomizeChars(cx, source->chars() + ArrayLength(GreedyStarChars),
                                           source->length() - ArrayLength(GreedyStarChars));
    if (!hackedSource)
        return false;

    return cx->compartment->regExps.getHack(cx, source, hackedSource, flags, g);
}







static bool
ExecuteRegExp(JSContext *cx, Native native, uintN argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, native, &RegExpClass, &ok);
    if (!obj)
        return ok;

    RegExpObject &reobj = obj->asRegExp();

    RegExpGuard re;
    if (StartsWithGreedyStar(reobj.getSource())) {
        if (!GetSharedForGreedyStar(cx, reobj.getSource(), reobj.getFlags(), &re))
            return false;
    } else {
        if (!reobj.getShared(cx, &re))
            return false;
    }

    RegExpStatics *res = cx->regExpStatics();

    
    JSString *input = ToString(cx, (args.length() > 0) ? args[0] : UndefinedValue());
    if (!input)
        return false;

    
    JSLinearString *linearInput = input->ensureLinear(cx);
    if (!linearInput)
        return false;
    const jschar *chars = linearInput->chars();
    size_t length = input->length();

    
    const Value &lastIndex = reobj.getLastIndex();

    
    double i;
    if (!ToInteger(cx, lastIndex, &i))
        return false;

    
    if (!re->global() && !re->sticky())
        i = 0;

    
    if (i < 0 || i > length) {
        reobj.zeroLastIndex();
        args.rval() = NullValue();
        return true;
    }

    
    RegExpExecType execType = (native == regexp_test) ? RegExpTest : RegExpExec;
    size_t lastIndexInt(i);
    if (!ExecuteRegExp(cx, res, *re, linearInput, chars, length, &lastIndexInt, execType,
                       &args.rval())) {
        return false;
    }

    
    if (re->global() || (!args.rval().isNull() && re->sticky())) {
        if (args.rval().isNull())
            reobj.zeroLastIndex();
        else
            reobj.setLastIndex(lastIndexInt);
    }

    return true;
}


JSBool
js::regexp_exec(JSContext *cx, uintN argc, Value *vp)
{
    return ExecuteRegExp(cx, regexp_exec, argc, vp);
}


JSBool
js::regexp_test(JSContext *cx, uintN argc, Value *vp)
{
    if (!ExecuteRegExp(cx, regexp_test, argc, vp))
        return false;
    if (!vp->isTrue())
        vp->setBoolean(false);
    return true;
}
