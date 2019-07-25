







































#ifndef RegExpObject_inl_h___
#define RegExpObject_inl_h___

#include "RegExpObject.h"
#include "RegExpStatics.h"

#include "jsobjinlines.h"
#include "jsstrinlines.h"

inline js::RegExpObject *
JSObject::asRegExp()
{
    JS_ASSERT(isRegExp());
    js::RegExpObject *reobj = static_cast<js::RegExpObject *>(this);
    JS_ASSERT(reobj->getPrivate());
    return reobj;
}

namespace js {







class PreInitRegExpObject
{
    RegExpObject    *reobj;
    DebugOnly<bool> gotResult;

  public:
    explicit PreInitRegExpObject(JSObject *obj) {
        JS_ASSERT(obj->isRegExp());
        reobj = static_cast<RegExpObject *>(obj);
        gotResult = false;
    }

    ~PreInitRegExpObject() {
        JS_ASSERT(gotResult);
    }

    RegExpObject *get() { return reobj; }

    void succeed() {
        JS_ASSERT(!gotResult);
        JS_ASSERT(reobj->getPrivate());
        gotResult = true;
    }

    void fail() {
        JS_ASSERT(!gotResult);
        gotResult = true;
    }
};

inline bool
ValueIsRegExp(const Value &v)
{
    return !v.isPrimitive() && v.toObject().isRegExp();
}

inline bool
IsRegExpMetaChar(jschar c)
{
    switch (c) {
      
      case '^': case '$': case '\\': case '.': case '*': case '+':
      case '?': case '(': case ')': case '[': case ']': case '{':
      case '}': case '|':
        return true;
      default:
        return false;
    }
}

inline bool
HasRegExpMetaChars(const jschar *chars, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (IsRegExpMetaChar(chars[i]))
            return true;
    }
    return false;
}

inline bool
ResetRegExpObject(JSContext *cx, RegExpObject *reobj, JSString *str, RegExpFlag flags)
{
    AlreadyIncRefed<RegExpPrivate> rep = RegExpPrivate::create(cx, str, flags, NULL);
    if (!rep)
        return false;

    return reobj->reset(cx, rep);
}

inline bool
ResetRegExpObject(JSContext *cx, RegExpObject *reobj, AlreadyIncRefed<RegExpPrivate> rep)
{
    return reobj->reset(cx, rep);
}

inline RegExpObject *
RegExpObject::create(JSContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
                     RegExpFlag flags, TokenStream *ts)
{
    RegExpFlag staticsFlags = res->getFlags();
    return createNoStatics(cx, chars, length, RegExpFlag(flags | staticsFlags), ts);
}

inline RegExpObject *
RegExpObject::createNoStatics(JSContext *cx, const jschar *chars, size_t length,
                              RegExpFlag flags, TokenStream *ts)
{
    JSString *str = js_NewStringCopyN(cx, chars, length);
    if (!str)
        return NULL;

    





    JS::Anchor<JSString *> anchor(str);
    AlreadyIncRefed<RegExpPrivate> rep = RegExpPrivate::create(cx, str, flags, ts);
    if (!rep)
        return NULL;

    JSObject *obj = NewBuiltinClassInstance(cx, &RegExpClass);
    if (!obj)
        return NULL;

    PreInitRegExpObject pireo(obj);
    RegExpObject *reobj = pireo.get();
    if (!ResetRegExpObject(cx, reobj, rep)) {
        rep->decref(cx);
        pireo.fail();
        return NULL;
    }

    pireo.succeed();
    return reobj;
}

inline bool
RegExpObject::reset(JSContext *cx, AlreadyIncRefed<RegExpPrivate> rep)
{
    if (nativeEmpty()) {
        const js::Shape **shapep = &cx->compartment->initialRegExpShape;
        if (!*shapep) {
            *shapep = assignInitialShape(cx);
            if (!*shapep)
                return false;
        }
        setLastProperty(*shapep);
        JS_ASSERT(!nativeEmpty());
    }

    DebugOnly<JSAtomState *> atomState = &cx->runtime->atomState;
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(atomState->lastIndexAtom))->slot == LAST_INDEX_SLOT);
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(atomState->sourceAtom))->slot == SOURCE_SLOT);
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(atomState->globalAtom))->slot == GLOBAL_FLAG_SLOT);
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(atomState->ignoreCaseAtom))->slot ==
                                 IGNORE_CASE_FLAG_SLOT);
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(atomState->multilineAtom))->slot ==
                                 MULTILINE_FLAG_SLOT);
    JS_ASSERT(nativeLookup(cx, ATOM_TO_JSID(atomState->stickyAtom))->slot == STICKY_FLAG_SLOT);

    setPrivate(rep.get());
    zeroLastIndex();
    setSource(rep->getSource());
    setGlobal(rep->global());
    setIgnoreCase(rep->ignoreCase());
    setMultiline(rep->multiline());
    setSticky(rep->sticky());
    return true;
}



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

    bool append(uint32 index, Value v) {
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

inline void
RegExpPrivate::checkMatchPairs(JSString *input, int *buf, size_t matchItemCount)
{
#if DEBUG
    size_t inputLength = input->length();
    for (size_t i = 0; i < matchItemCount; i += 2) {
        int start = buf[i];
        int limit = buf[i + 1];
        JS_ASSERT(limit >= start); 
        if (start == -1)
            continue;
        JS_ASSERT(start >= 0);
        JS_ASSERT(size_t(limit) <= inputLength);
    }
#endif
}

inline JSObject *
RegExpPrivate::createResult(JSContext *cx, JSString *input, int *buf, size_t matchItemCount)
{
    




    JSObject *array = NewSlowEmptyArray(cx);
    if (!array)
        return NULL;

    RegExpMatchBuilder builder(cx, array);
    for (size_t i = 0; i < matchItemCount; i += 2) {
        int start = buf[i];
        int end = buf[i + 1];

        JSString *captured;
        if (start >= 0) {
            JS_ASSERT(start <= end);
            JS_ASSERT(unsigned(end) <= input->length());
            captured = js_NewDependentString(cx, input, start, end - start);
            if (!captured || !builder.append(i / 2, StringValue(captured)))
                return NULL;
        } else {
            
            JS_ASSERT(i != 0); 
            if (!builder.append(i / 2, UndefinedValue()))
                return NULL;
        }
    }

    if (!builder.setIndex(buf[0]) || !builder.setInput(input))
        return NULL;

    return array;
}

inline AlreadyIncRefed<RegExpPrivate>
RegExpPrivate::create(JSContext *cx, JSString *source, RegExpFlag flags, TokenStream *ts)
{
    typedef AlreadyIncRefed<RegExpPrivate> RetType;

    JSLinearString *flatSource = source->ensureLinear(cx);
    if (!flatSource)
        return RetType(NULL);

    RegExpPrivate *self = cx->new_<RegExpPrivate>(flatSource, flags, cx->compartment);
    if (!self)
        return RetType(NULL);

    if (!self->compile(cx, ts)) {
        Foreground::delete_(self);
        return RetType(NULL);
    }

    return RetType(self);
}




static inline bool
EnableYarrJIT(JSContext *cx)
{
#if defined ANDROID && defined(JS_TRACER) && defined(JS_METHODJIT)
    return cx->traceJitEnabled || cx->methodJitEnabled;
#else
    return true;
#endif
}

inline bool
RegExpPrivate::compileHelper(JSContext *cx, JSLinearString &pattern, TokenStream *ts)
{
#if ENABLE_YARR_JIT
    JSC::Yarr::ErrorCode yarrError;
    JSC::Yarr::YarrPattern yarrPattern(pattern, ignoreCase(), multiline(), &yarrError);
    if (yarrError) {
        reportYarrError(cx, ts, yarrError);
        return false;
    }
    parenCount = yarrPattern.m_numSubpatterns;

#if defined(JS_METHODJIT)
    if (EnableYarrJIT(cx) && !yarrPattern.m_containsBackreferences) {
        bool ok = cx->compartment->ensureJaegerCompartmentExists(cx);
        if (!ok)
            return false;
        JSC::Yarr::JSGlobalData globalData(cx->compartment->jaegerCompartment()->execAlloc());
        JSC::Yarr::jitCompile(yarrPattern, &globalData, codeBlock);
        if (!codeBlock.isFallBack())
            return true;
    }
#endif

    codeBlock.setFallBack(true);
    byteCode = JSC::Yarr::byteCompile(yarrPattern, cx->compartment->regExpAllocator).get();
    return true;
#else
    int error = 0;
    compiled = jsRegExpCompile(pattern.chars(), pattern.length(),
        ignoreCase() ? JSRegExpIgnoreCase : JSRegExpDoNotIgnoreCase,
        multiline() ? JSRegExpMultiline : JSRegExpSingleLine,
        &parenCount, &error);
    if (error) {
        reportPCREError(cx, error);
        return false;
    }
    return true;
#endif
}

inline bool
RegExpPrivate::compile(JSContext *cx, TokenStream *ts)
{
    
    if (!source->ensureLinear(cx))
        return false;

    if (!sticky())
        return compileHelper(cx, *source, ts);

    



    static const jschar prefix[] = {'^', '(', '?', ':'};
    static const jschar postfix[] = {')'};

    StringBuffer sb(cx);
    if (!sb.reserve(JS_ARRAY_LENGTH(prefix) + source->length() + JS_ARRAY_LENGTH(postfix)))
        return false;
    sb.infallibleAppend(prefix, JS_ARRAY_LENGTH(prefix));
    sb.infallibleAppend(source->chars(), source->length());
    sb.infallibleAppend(postfix, JS_ARRAY_LENGTH(postfix));

    JSLinearString *fakeySource = sb.finishString();
    if (!fakeySource)
        return false;
    return compileHelper(cx, *fakeySource, ts);
}

inline void
RegExpPrivate::incref(JSContext *cx)
{
#ifdef DEBUG
    assertSameCompartment(cx, compartment);
#endif
    ++refCount;
}

inline void
RegExpPrivate::decref(JSContext *cx)
{
#ifdef DEBUG
    assertSameCompartment(cx, compartment);
#endif
    if (--refCount == 0)
        cx->delete_(this);
}

inline RegExpPrivate *
RegExpObject::getPrivate() const
{
    RegExpPrivate *rep = static_cast<RegExpPrivate *>(JSObject::getPrivate());
#ifdef DEBUG
    if (rep)
        CompartmentChecker::check(compartment(), rep->compartment);
#endif
    return rep;
}

} 

#endif
