







































#ifndef RegExpObject_inl_h___
#define RegExpObject_inl_h___

#include "mozilla/Util.h"

#include "RegExpObject.h"
#include "RegExpStatics.h"

#include "jsobjinlines.h"
#include "jsstrinlines.h"
#include "RegExpStatics-inl.h"

inline js::RegExpObject *
JSObject::asRegExp()
{
    JS_ASSERT(isRegExp());
    return static_cast<js::RegExpObject *>(this);
}

namespace js {

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

inline void
RegExpObject::setPrivate(RegExpPrivate *rep)
{
    JSObject::setPrivate(rep);
}

inline RegExpObject *
RegExpObject::create(JSContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
                     RegExpFlag flags, TokenStream *tokenStream)
{
    RegExpFlag staticsFlags = res->getFlags();
    return createNoStatics(cx, chars, length, RegExpFlag(flags | staticsFlags), tokenStream);
}

inline RegExpObject *
RegExpObject::createNoStatics(JSContext *cx, const jschar *chars, size_t length,
                              RegExpFlag flags, TokenStream *tokenStream)
{
    JSAtom *source = js_AtomizeChars(cx, chars, length);
    if (!source)
        return NULL;

    return createNoStatics(cx, source, flags, tokenStream);
}

inline RegExpObject *
RegExpObject::createNoStatics(JSContext *cx, JSAtom *source, RegExpFlag flags,
                              TokenStream *tokenStream)
{
    if (!RegExpPrivateCode::checkSyntax(cx, tokenStream, source))
        return NULL;

    RegExpObjectBuilder builder(cx);
    return builder.build(source, flags);
}

inline void
RegExpObject::purge(JSContext *cx)
{
    if (RegExpPrivate *rep = getPrivate()) {
        rep->decref(cx);
        setPrivate(NULL);
    }
}

inline void
RegExpObject::finalize(JSContext *cx)
{
    purge(cx);
#ifdef DEBUG
    setPrivate((RegExpPrivate *) 0x1); 
#endif
}

inline bool
RegExpObject::init(JSContext *cx, JSLinearString *source, RegExpFlag flags)
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

    JS_ASSERT(!getPrivate());
    zeroLastIndex();
    setSource(source);
    setGlobal(flags & GlobalFlag);
    setIgnoreCase(flags & IgnoreCaseFlag);
    setMultiline(flags & MultilineFlag);
    setSticky(flags & StickyFlag);
    return true;
}



inline AlreadyIncRefed<RegExpPrivate>
RegExpPrivate::create(JSContext *cx, JSLinearString *source, RegExpFlag flags, TokenStream *ts)
{
    typedef AlreadyIncRefed<RegExpPrivate> RetType;

    




    bool cacheable = source->isAtom();

    






    JSRuntime *rt = cx->runtime;
    RegExpPrivateCache *cache = NULL; 
    RegExpPrivateCache::AddPtr addPtr;
    if (cacheable) {
        cache = cx->threadData()->getOrCreateRegExpPrivateCache(rt);
        if (!cache) {
            js_ReportOutOfMemory(cx);
            return RetType(NULL);
        }

        addPtr = cache->lookupForAdd(&source->asAtom());
        if (addPtr) {
            RegExpPrivate *cached = addPtr->value;
            if (cached->getFlags() == flags) {
                cached->incref(cx);
                return RetType(cached);
            }
            
        }
    }

    JSLinearString *flatSource = source->ensureLinear(cx);
    if (!flatSource)
        return RetType(NULL);

    RegExpPrivate *self = cx->new_<RegExpPrivate>(flatSource, flags);
    if (!self)
        return RetType(NULL);

    if (!self->compile(cx, ts)) {
        Foreground::delete_(self);
        return RetType(NULL);
    }

    if (cacheable) {
        if (addPtr) {
            JS_ASSERT(addPtr->key == &self->getSource()->asAtom());
            addPtr->value = self;
        } else {
            if (!cache->add(addPtr, &source->asAtom(), self)) {
                js_ReportOutOfMemory(cx);
                return RetType(NULL);
            }
        }
    }

    return RetType(self);
}


inline bool
RegExpPrivateCode::isJITRuntimeEnabled(JSContext *cx)
{
#if defined(ANDROID) && defined(JS_TRACER) && defined(JS_METHODJIT)
    return cx->traceJitEnabled || cx->methodJitEnabled;
#else
    return true;
#endif
}

inline bool
RegExpPrivateCode::compile(JSContext *cx, JSLinearString &pattern, TokenStream *ts,
                           uintN *parenCount, RegExpFlag flags)
{
#if ENABLE_YARR_JIT
    
    ErrorCode yarrError;
    YarrPattern yarrPattern(pattern, bool(flags & IgnoreCaseFlag), bool(flags & MultilineFlag),
                            &yarrError);
    if (yarrError) {
        reportYarrError(cx, ts, yarrError);
        return false;
    }
    *parenCount = yarrPattern.m_numSubpatterns;

    





#ifdef JS_METHODJIT
    if (isJITRuntimeEnabled(cx) && !yarrPattern.m_containsBackreferences) {
        if (!cx->compartment->ensureJaegerCompartmentExists(cx))
            return false;

        JSGlobalData globalData(cx->compartment->jaegerCompartment()->execAlloc());
        jitCompile(yarrPattern, &globalData, codeBlock);
        if (!codeBlock.isFallBack())
            return true;
    }
#endif

    codeBlock.setFallBack(true);
    byteCode = byteCompile(yarrPattern, cx->compartment->regExpAllocator).get();
    return true;
#else 
    int error = 0;
    compiled = jsRegExpCompile(pattern.chars(), pattern.length(),
                  ignoreCase() ? JSRegExpIgnoreCase : JSRegExpDoNotIgnoreCase,
                  multiline() ? JSRegExpMultiline : JSRegExpSingleLine,
                  parenCount, &error);
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
        return code.compile(cx, *source, ts, &parenCount, getFlags());

    



    static const jschar prefix[] = {'^', '(', '?', ':'};
    static const jschar postfix[] = {')'};

    using mozilla::ArrayLength;
    StringBuffer sb(cx);
    if (!sb.reserve(ArrayLength(prefix) + source->length() + ArrayLength(postfix)))
        return false;
    sb.infallibleAppend(prefix, ArrayLength(prefix));
    sb.infallibleAppend(source->chars(), source->length());
    sb.infallibleAppend(postfix, ArrayLength(postfix));

    JSLinearString *fakeySource = sb.finishString();
    if (!fakeySource)
        return false;
    return code.compile(cx, *fakeySource, ts, &parenCount, getFlags());
}

inline RegExpRunStatus
RegExpPrivateCode::execute(JSContext *cx, const jschar *chars, size_t length, size_t start,
                           int *output, size_t outputCount)
{
    int result;
#if ENABLE_YARR_JIT
    (void) cx; 
    if (codeBlock.isFallBack())
        result = JSC::Yarr::interpret(byteCode, chars, start, length, output);
    else
        result = JSC::Yarr::execute(codeBlock, chars, start, length, output);
#else
    result = jsRegExpExecute(cx, compiled, chars, length, start, output, outputCount);
#endif

    if (result == -1)
        return RegExpRunStatus_Success_NotFound;

#if !ENABLE_YARR_JIT
    if (result < 0) {
        reportPCREError(cx, result);
        return RegExpRunStatus_Error;
    }
#endif

    JS_ASSERT(result >= 0);
    return RegExpRunStatus_Success;
}

inline void
RegExpPrivate::incref(JSContext *cx)
{
    ++refCount;
}

inline void
RegExpPrivate::decref(JSContext *cx)
{
    if (--refCount != 0)
        return;

    RegExpPrivateCache *cache;
    if (source->isAtom() && (cache = cx->threadData()->getRegExpPrivateCache())) {
        RegExpPrivateCache::Ptr ptr = cache->lookup(&source->asAtom());
        if (ptr && ptr->value == this)
            cache->remove(ptr);
    }

    cx->delete_(this);
}

} 

#endif
