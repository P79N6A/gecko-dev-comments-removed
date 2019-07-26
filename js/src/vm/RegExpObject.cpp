





#include "vm/RegExpObject.h"

#include "mozilla/MemoryReporting.h"

#include "jsstr.h"

#include "frontend/TokenStream.h"
#ifndef JS_YARR
#include "irregexp/RegExpParser.h"
#endif
#include "vm/MatchPairs.h"
#include "vm/RegExpStatics.h"
#include "vm/StringBuffer.h"
#include "vm/TraceLogging.h"
#include "vm/Xdr.h"
#ifdef JS_YARR
#include "yarr/YarrSyntaxChecker.h"
#endif

#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using namespace js;

using mozilla::DebugOnly;
using mozilla::Maybe;
using js::frontend::TokenStream;

JS_STATIC_ASSERT(IgnoreCaseFlag == JSREG_FOLD);
JS_STATIC_ASSERT(GlobalFlag == JSREG_GLOB);
JS_STATIC_ASSERT(MultilineFlag == JSREG_MULTILINE);
JS_STATIC_ASSERT(StickyFlag == JSREG_STICKY);



RegExpObjectBuilder::RegExpObjectBuilder(ExclusiveContext *cx, RegExpObject *reobj)
  : cx(cx), reobj_(cx, reobj)
{}

bool
RegExpObjectBuilder::getOrCreate()
{
    if (reobj_)
        return true;

    
    
    JSObject *obj = NewBuiltinClassInstance(cx, &RegExpObject::class_, TenuredObject);
    if (!obj)
        return false;
    obj->initPrivate(nullptr);

    reobj_ = &obj->as<RegExpObject>();
    return true;
}

bool
RegExpObjectBuilder::getOrCreateClone(HandleTypeObject type)
{
    JS_ASSERT(!reobj_);
    JS_ASSERT(type->clasp() == &RegExpObject::class_);

    JSObject *parent = type->proto().toObject()->getParent();

    
    
    JSObject *clone = NewObjectWithType(cx->asJSContext(), type, parent, TenuredObject);
    if (!clone)
        return false;
    clone->initPrivate(nullptr);

    reobj_ = &clone->as<RegExpObject>();
    return true;
}

RegExpObject *
RegExpObjectBuilder::build(HandleAtom source, RegExpShared &shared)
{
    if (!getOrCreate())
        return nullptr;

    if (!reobj_->init(cx, source, shared.getFlags()))
        return nullptr;

    reobj_->setShared(shared);
    return reobj_;
}

RegExpObject *
RegExpObjectBuilder::build(HandleAtom source, RegExpFlag flags)
{
    if (!getOrCreate())
        return nullptr;

    return reobj_->init(cx, source, flags) ? reobj_.get() : nullptr;
}

RegExpObject *
RegExpObjectBuilder::clone(Handle<RegExpObject *> other)
{
    RootedTypeObject type(cx, other->type());
    if (!getOrCreateClone(type))
        return nullptr;

    




    RegExpStatics *res = other->getProto()->getParent()->as<GlobalObject>().getRegExpStatics(cx);
    if (!res)
        return nullptr;

    RegExpFlag origFlags = other->getFlags();
    RegExpFlag staticsFlags = res->getFlags();
    if ((origFlags & staticsFlags) != staticsFlags) {
        RegExpFlag newFlags = RegExpFlag(origFlags | staticsFlags);
        Rooted<JSAtom *> source(cx, other->getSource());
        return build(source, newFlags);
    }

    RegExpGuard g(cx);
    if (!other->getShared(cx->asJSContext(), &g))
        return nullptr;

    Rooted<JSAtom *> source(cx, other->getSource());
    return build(source, *g);
}



bool
MatchPairs::initArray(size_t pairCount)
{
    JS_ASSERT(pairCount > 0);

    
    if (!allocOrExpandArray(pairCount))
        return false;

    
    for (size_t i = 0; i < pairCount; i++) {
        pairs_[i].start = -1;
        pairs_[i].limit = -1;
    }

    return true;
}

bool
MatchPairs::initArrayFrom(MatchPairs &copyFrom)
{
    JS_ASSERT(copyFrom.pairCount() > 0);

    if (!allocOrExpandArray(copyFrom.pairCount()))
        return false;

    for (size_t i = 0; i < pairCount_; i++) {
        JS_ASSERT(copyFrom[i].check());
        pairs_[i].start = copyFrom[i].start;
        pairs_[i].limit = copyFrom[i].limit;
    }

    return true;
}

void
MatchPairs::displace(size_t disp)
{
    if (disp == 0)
        return;

    for (size_t i = 0; i < pairCount_; i++) {
        JS_ASSERT(pairs_[i].check());
        pairs_[i].start += (pairs_[i].start < 0) ? 0 : disp;
        pairs_[i].limit += (pairs_[i].limit < 0) ? 0 : disp;
    }
}

bool
ScopedMatchPairs::allocOrExpandArray(size_t pairCount)
{
    
    if (pairCount_) {
        JS_ASSERT(pairs_);
        JS_ASSERT(pairCount_ == pairCount);
        return true;
    }

    JS_ASSERT(!pairs_);
    pairs_ = (MatchPair *)lifoScope_.alloc().alloc(sizeof(MatchPair) * pairCount);
    if (!pairs_)
        return false;

    pairCount_ = pairCount;
    return true;
}

bool
VectorMatchPairs::allocOrExpandArray(size_t pairCount)
{
    if (!vec_.resizeUninitialized(sizeof(MatchPair) * pairCount))
        return false;

    pairs_ = &vec_[0];
    pairCount_ = pairCount;
    return true;
}



static inline void
MaybeTraceRegExpShared(JSContext *cx, RegExpShared *shared)
{
    Zone *zone = cx->zone();
    if (zone->needsBarrier())
        shared->trace(zone->barrierTracer());
}

bool
RegExpObject::getShared(JSContext *cx, RegExpGuard *g)
{
    if (RegExpShared *shared = maybeShared()) {
        
        
        MaybeTraceRegExpShared(cx, shared);

        g->init(*shared);
        return true;
    }

    return createShared(cx, g);
}

 void
RegExpObject::trace(JSTracer *trc, JSObject *obj)
{
    RegExpShared *shared = obj->as<RegExpObject>().maybeShared();
    if (!shared)
        return;

    
    
    
    
    
    
    
    
    if (trc->runtime()->isHeapBusy() &&
        IS_GC_MARKING_TRACER(trc) &&
        !obj->tenuredZone()->isPreservingCode())
    {
        obj->setPrivate(nullptr);
    } else {
        shared->trace(trc);
    }
}

const Class RegExpObject::class_ = {
    js_RegExp_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(RegExpObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_RegExp),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,        
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    RegExpObject::trace
};

RegExpObject *
RegExpObject::create(ExclusiveContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
                     RegExpFlag flags, TokenStream *tokenStream, LifoAlloc &alloc)
{
    RegExpFlag staticsFlags = res->getFlags();
    return createNoStatics(cx, chars, length, RegExpFlag(flags | staticsFlags), tokenStream, alloc);
}

RegExpObject *
RegExpObject::createNoStatics(ExclusiveContext *cx, const jschar *chars, size_t length, RegExpFlag flags,
                              TokenStream *tokenStream, LifoAlloc &alloc)
{
    RootedAtom source(cx, AtomizeChars(cx, chars, length));
    if (!source)
        return nullptr;

    return createNoStatics(cx, source, flags, tokenStream, alloc);
}

RegExpObject *
RegExpObject::createNoStatics(ExclusiveContext *cx, HandleAtom source, RegExpFlag flags,
                              TokenStream *tokenStream, LifoAlloc &alloc)
{
#ifdef JS_YARR
    if (!RegExpShared::checkSyntax(cx, tokenStream, source))
        return nullptr;
#else 
    Maybe<CompileOptions> dummyOptions;
    Maybe<TokenStream> dummyTokenStream;
    if (!tokenStream) {
        dummyOptions.construct(cx->asJSContext());
        dummyTokenStream.construct(cx, dummyOptions.ref(),
                                   (const jschar *) nullptr, 0,
                                   (frontend::StrictModeGetter *) nullptr);
        tokenStream = dummyTokenStream.addr();
    }

    if (!irregexp::ParsePatternSyntax(*tokenStream, alloc, source->chars(), source->length()))
        return nullptr;
#endif

    RegExpObjectBuilder builder(cx);
    return builder.build(source, flags);
}

bool
RegExpObject::createShared(JSContext *cx, RegExpGuard *g)
{
    Rooted<RegExpObject*> self(cx, this);

    JS_ASSERT(!maybeShared());
    if (!cx->compartment()->regExps.get(cx, getSource(), getFlags(), g))
        return false;

    self->setShared(**g);
    return true;
}

Shape *
RegExpObject::assignInitialShape(ExclusiveContext *cx, Handle<RegExpObject*> self)
{
    JS_ASSERT(self->nativeEmpty());

    JS_STATIC_ASSERT(LAST_INDEX_SLOT == 0);
    JS_STATIC_ASSERT(SOURCE_SLOT == LAST_INDEX_SLOT + 1);
    JS_STATIC_ASSERT(GLOBAL_FLAG_SLOT == SOURCE_SLOT + 1);
    JS_STATIC_ASSERT(IGNORE_CASE_FLAG_SLOT == GLOBAL_FLAG_SLOT + 1);
    JS_STATIC_ASSERT(MULTILINE_FLAG_SLOT == IGNORE_CASE_FLAG_SLOT + 1);
    JS_STATIC_ASSERT(STICKY_FLAG_SLOT == MULTILINE_FLAG_SLOT + 1);

    
    if (!self->addDataProperty(cx, cx->names().lastIndex, LAST_INDEX_SLOT, JSPROP_PERMANENT))
        return nullptr;

    
    unsigned attrs = JSPROP_PERMANENT | JSPROP_READONLY;
    if (!self->addDataProperty(cx, cx->names().source, SOURCE_SLOT, attrs))
        return nullptr;
    if (!self->addDataProperty(cx, cx->names().global, GLOBAL_FLAG_SLOT, attrs))
        return nullptr;
    if (!self->addDataProperty(cx, cx->names().ignoreCase, IGNORE_CASE_FLAG_SLOT, attrs))
        return nullptr;
    if (!self->addDataProperty(cx, cx->names().multiline, MULTILINE_FLAG_SLOT, attrs))
        return nullptr;
    return self->addDataProperty(cx, cx->names().sticky, STICKY_FLAG_SLOT, attrs);
}

bool
RegExpObject::init(ExclusiveContext *cx, HandleAtom source, RegExpFlag flags)
{
    Rooted<RegExpObject *> self(cx, this);

    if (!EmptyShape::ensureInitialCustomShape<RegExpObject>(cx, self))
        return false;

    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().lastIndex))->slot() ==
              LAST_INDEX_SLOT);
    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().source))->slot() ==
              SOURCE_SLOT);
    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().global))->slot() ==
              GLOBAL_FLAG_SLOT);
    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().ignoreCase))->slot() ==
              IGNORE_CASE_FLAG_SLOT);
    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().multiline))->slot() ==
              MULTILINE_FLAG_SLOT);
    JS_ASSERT(self->nativeLookup(cx, NameToId(cx->names().sticky))->slot() ==
              STICKY_FLAG_SLOT);

    



    self->JSObject::setPrivate(nullptr);

    self->zeroLastIndex();
    self->setSource(source);
    self->setGlobal(flags & GlobalFlag);
    self->setIgnoreCase(flags & IgnoreCaseFlag);
    self->setMultiline(flags & MultilineFlag);
    self->setSticky(flags & StickyFlag);
    return true;
}

JSFlatString *
RegExpObject::toString(JSContext *cx) const
{
    JSAtom *src = getSource();
    StringBuffer sb(cx);
    if (size_t len = src->length()) {
        if (!sb.reserve(len + 2))
            return nullptr;
        sb.infallibleAppend('/');
        sb.infallibleAppend(src->chars(), len);
        sb.infallibleAppend('/');
    } else {
        if (!sb.append("/(?:)/"))
            return nullptr;
    }
    if (global() && !sb.append('g'))
        return nullptr;
    if (ignoreCase() && !sb.append('i'))
        return nullptr;
    if (multiline() && !sb.append('m'))
        return nullptr;
    if (sticky() && !sb.append('y'))
        return nullptr;

    return sb.finishString();
}



RegExpShared::RegExpShared(JSCompartment *comp, JSAtom *source, RegExpFlag flags)
  : comp(comp), source(source), flags(flags), parenCount(0), canStringMatch(false), marked_(false)
{
#ifdef JS_YARR
    bytecode = nullptr;
#else
    byteCode = nullptr;
#endif
}

RegExpShared::~RegExpShared()
{
#ifdef JS_YARR
#ifdef JS_ION
    codeBlock.release();
#endif
    js_delete<BytecodePattern>(bytecode);
#else 
    js_free(byteCode);
#endif 

    for (size_t i = 0; i < tables.length(); i++)
        js_delete(tables[i]);
}

void
RegExpShared::trace(JSTracer *trc)
{
    if (IS_GC_MARKING_TRACER(trc))
        marked_ = true;

    if (source)
        MarkString(trc, &source, "RegExpShared source");

#if !defined(JS_YARR) && defined(JS_ION)
    if (jitCode)
        MarkJitCode(trc, &jitCode, "RegExpShared code");
#endif
}

#ifdef JS_YARR

void
RegExpShared::reportYarrError(ExclusiveContext *cx, TokenStream *ts, ErrorCode error)
{
    switch (error) {
      case JSC::Yarr::NoError:
        MOZ_ASSUME_UNREACHABLE("Called reportYarrError with value for no error");
#define COMPILE_EMSG(__code, __msg)                                                              \
      case JSC::Yarr::__code:                                                                    \
        if (ts)                                                                                  \
            ts->reportError(__msg);                                                              \
        else                                                                                     \
            JS_ReportErrorFlagsAndNumberUC(cx->asJSContext(),                                    \
                                           JSREPORT_ERROR, js_GetErrorMessage, nullptr, __msg);     \
        return
      COMPILE_EMSG(PatternTooLarge, JSMSG_REGEXP_TOO_COMPLEX);
      COMPILE_EMSG(QuantifierOutOfOrder, JSMSG_BAD_QUANTIFIER);
      COMPILE_EMSG(QuantifierWithoutAtom, JSMSG_BAD_QUANTIFIER);
      COMPILE_EMSG(MissingParentheses, JSMSG_MISSING_PAREN);
      COMPILE_EMSG(ParenthesesUnmatched, JSMSG_UNMATCHED_RIGHT_PAREN);
      COMPILE_EMSG(ParenthesesTypeInvalid, JSMSG_BAD_QUANTIFIER); 
      COMPILE_EMSG(CharacterClassUnmatched, JSMSG_BAD_CLASS_RANGE);
      COMPILE_EMSG(CharacterClassInvalidRange, JSMSG_BAD_CLASS_RANGE);
      COMPILE_EMSG(CharacterClassOutOfOrder, JSMSG_BAD_CLASS_RANGE);
      COMPILE_EMSG(QuantifierTooLarge, JSMSG_BAD_QUANTIFIER);
      COMPILE_EMSG(EscapeUnterminated, JSMSG_TRAILING_SLASH);
      COMPILE_EMSG(RuntimeError, JSMSG_REGEXP_RUNTIME_ERROR);
#undef COMPILE_EMSG
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown Yarr error code");
    }
}

bool
RegExpShared::checkSyntax(ExclusiveContext *cx, TokenStream *tokenStream, JSLinearString *source)
{
    ErrorCode error = JSC::Yarr::checkSyntax(*source);
    if (error == JSC::Yarr::NoError)
        return true;

    reportYarrError(cx, tokenStream, error);
    return false;
}

#endif 

bool
RegExpShared::compile(JSContext *cx, bool matchOnly, const jschar *sampleChars, size_t sampleLength)
{
    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    AutoTraceLog logCompile(logger, TraceLogger::IrregexpCompile);

    if (!sticky()) {
        RootedAtom pattern(cx, source);
        return compile(cx, pattern, matchOnly, sampleChars, sampleLength);
    }

    



    static const jschar prefix[] = {'^', '(', '?', ':'};
    static const jschar postfix[] = {')'};

    using mozilla::ArrayLength;
    StringBuffer sb(cx);
    if (!sb.reserve(ArrayLength(prefix) + source->length() + ArrayLength(postfix)))
        return false;
    sb.infallibleAppend(prefix, ArrayLength(prefix));
    sb.infallibleAppend(source->chars(), source->length());
    sb.infallibleAppend(postfix, ArrayLength(postfix));

    RootedAtom fakeySource(cx, sb.finishAtom());
    if (!fakeySource)
        return false;

    return compile(cx, fakeySource, matchOnly, sampleChars, sampleLength);
}

bool
RegExpShared::compile(JSContext *cx, HandleAtom pattern, bool matchOnly, const jschar *sampleChars, size_t sampleLength)
{
    if (cx->compartment() != comp)
        MOZ_CRASH();

    if (!ignoreCase() && !StringHasRegExpMetaChars(pattern->chars(), pattern->length())) {
        canStringMatch = true;
        parenCount = 0;
        return true;
    }

#ifdef JS_YARR

    ErrorCode yarrError;
    YarrPattern yarrPattern(*pattern, ignoreCase(), multiline(), &yarrError);
    if (yarrError) {
        reportYarrError(cx, nullptr, yarrError);
        return false;
    }
    this->parenCount = yarrPattern.m_numSubpatterns;

#ifdef JS_ION
    if (isJITRuntimeEnabled(cx) && !yarrPattern.m_containsBackreferences) {
        JSC::ExecutableAllocator *execAlloc = cx->runtime()->getExecAlloc(cx);
        if (!execAlloc)
            return false;

        JSGlobalData globalData(execAlloc);
        YarrJITCompileMode compileMode = matchOnly ? JSC::Yarr::MatchOnly
                                                   : JSC::Yarr::IncludeSubpatterns;

        jitCompile(yarrPattern, JSC::Yarr::Char16, &globalData, codeBlock, compileMode);

        
        if (!codeBlock.isFallBack())
            return true;
    }
    codeBlock.setFallBack(true);
#endif

    WTF::BumpPointerAllocator *bumpAlloc = cx->runtime()->getBumpPointerAllocator(cx);
    if (!bumpAlloc) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    bytecode = byteCompile(yarrPattern, bumpAlloc).get();

#else 

    JS_ASSERT(!matchOnly);

    CompileOptions options(cx);
    TokenStream dummyTokenStream(cx, options, nullptr, 0, nullptr);

    LifoAllocScope scope(&cx->tempLifoAlloc());

    
    irregexp::RegExpCompileData data;
    if (!irregexp::ParsePattern(dummyTokenStream, cx->tempLifoAlloc(),
                                pattern->chars(), pattern->length(), multiline(), &data))
    {
        return false;
    }

    this->parenCount = data.capture_count;

    irregexp::RegExpCode code = irregexp::CompilePattern(cx, this, &data, sampleChars, sampleLength,
                                                         false ,
                                                         ignoreCase());
    if (code.empty())
        return false;

#ifdef JS_ION
    JS_ASSERT(!code.jitCode || !code.byteCode);
    jitCode = code.jitCode;

    if (jitCode && jitCode->tenuredZone() != comp->zone())
        MOZ_CRASH();
#endif

    byteCode = code.byteCode;

#endif 

    return true;
}

bool
RegExpShared::compileIfNecessary(JSContext *cx, const jschar *sampleChars, size_t sampleLength)
{
    if (isCompiled(false) || canStringMatch)
        return true;
    return compile(cx, false, sampleChars, sampleLength);
}

#ifdef JS_YARR
bool
RegExpShared::compileMatchOnlyIfNecessary(JSContext *cx)
{
    if (isCompiled(true) || canStringMatch)
        return true;
    return compile(cx, true, nullptr, 0);
}
#endif 

RegExpRunStatus
RegExpShared::execute(JSContext *cx, const jschar *chars, size_t length,
                      size_t *lastIndex, MatchPairs &matches)
{
    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());

    
    if (!compileIfNecessary(cx, chars, length))
        return RegExpRunStatus_Error;

    
    if (!matches.initArray(pairCount()))
        return RegExpRunStatus_Error;

    



    size_t origLength = length;
    size_t start = *lastIndex;
    size_t displacement = 0;

    if (sticky()) {
        displacement = start;
        chars += displacement;
        length -= displacement;
        start = 0;
    }

#ifndef JS_YARR
    
    irregexp::RegExpStackScope stackScope(cx->runtime());
#endif

    if (canStringMatch) {
        int res = StringFindPattern(chars+start, length-start, source->chars(), source->length());
        if (res == -1)
            return RegExpRunStatus_Success_NotFound;

        matches[0].start = res + start;
        matches[0].limit = res + start + source->length();

        matches.displace(displacement);
        matches.checkAgainst(origLength);
        *lastIndex = matches[0].limit;
        return RegExpRunStatus_Success;
    }

#ifdef JS_YARR

    unsigned result;

    
    JS_STATIC_ASSERT(sizeof(int32_t) == sizeof(int));
    JS_STATIC_ASSERT(sizeof(int32_t) == sizeof(unsigned));

#ifdef JS_ION
    if (codeBlock.isFallBack()) {
        AutoTraceLog logInterpret(logger, TraceLogger::YarrInterpret);
        result = JSC::Yarr::interpret(cx, bytecode, chars, length, start, (unsigned *) matches.pairsRaw());
    } else {
        AutoTraceLog logJIT(logger, TraceLogger::YarrJIT);
        result = codeBlock.execute(chars, start, length, (int *) matches.pairsRaw()).start;
    }
#else 
    {
        AutoTraceLog logInterpret(logger, TraceLogger::YarrInterpret);
        result = JSC::Yarr::interpret(cx, bytecode, chars, length, start, (unsigned *) matches.pairsRaw());
    }
#endif 

    if (result == JSC::Yarr::offsetError) {
        reportYarrError(cx, nullptr, JSC::Yarr::RuntimeError);
        return RegExpRunStatus_Error;
    }

    if (result == JSC::Yarr::offsetNoMatch)
        return RegExpRunStatus_Success_NotFound;

#else 

    if (hasByteCode()) {
        AutoTraceLog logInterpreter(logger, TraceLogger::IrregexpExecute);
        RegExpRunStatus result =
            irregexp::InterpretCode(cx, byteCode, chars, start, length, &matches);
        if (result == RegExpRunStatus_Success) {
            matches.displace(displacement);
            matches.checkAgainst(origLength);
            *lastIndex = matches[0].limit;
        }
        return result;
    }

#ifdef JS_ION
    while (true) {
        RegExpRunStatus result;
        {
            AutoTraceLog logJIT(logger, TraceLogger::IrregexpExecute);
            result = irregexp::ExecuteCode(cx, jitCode, chars, start, length, &matches);
        }

        if (result == RegExpRunStatus_Error) {
            
            
            
            bool interrupted;
            {
                JSRuntime::AutoLockForInterrupt lock(cx->runtime());
                interrupted = cx->runtime()->interrupt;
            }

            if (interrupted) {
                if (!InvokeInterruptCallback(cx))
                    return RegExpRunStatus_Error;
                continue;
            }

            js_ReportOverRecursed(cx);
            return RegExpRunStatus_Error;
        }

        if (result == RegExpRunStatus_Success_NotFound)
            return RegExpRunStatus_Success_NotFound;

        JS_ASSERT(result == RegExpRunStatus_Success);
        break;
    }
#else 
    MOZ_CRASH();
#endif 

#endif 

    matches.displace(displacement);
    matches.checkAgainst(origLength);
    *lastIndex = matches[0].limit;
    return RegExpRunStatus_Success;
}

#ifdef JS_YARR

RegExpRunStatus
RegExpShared::executeMatchOnly(JSContext *cx, const jschar *chars, size_t length,
                               size_t *lastIndex, MatchPair &match)
{
    TraceLogger *logger = js::TraceLoggerForMainThread(cx->runtime());

    
    if (!compileMatchOnlyIfNecessary(cx))
        return RegExpRunStatus_Error;

#ifdef DEBUG
    const size_t origLength = length;
#endif
    size_t start = *lastIndex;
    size_t displacement = 0;

    if (sticky()) {
        displacement = start;
        chars += displacement;
        length -= displacement;
        start = 0;
    }

    if (canStringMatch) {
        int res = StringFindPattern(chars+start, length-start, source->chars(), source->length());
        if (res == -1)
            return RegExpRunStatus_Success_NotFound;

        match = MatchPair(res + start, res + start + source->length());
        match.displace(displacement);
        *lastIndex = match.limit;
        return RegExpRunStatus_Success;
    }

#ifdef JS_ION
    if (!codeBlock.isFallBack()) {
        AutoTraceLog logJIT(logger, TraceLogger::YarrJIT);
        MatchResult result = codeBlock.execute(chars, start, length);
        if (!result)
            return RegExpRunStatus_Success_NotFound;

        match = MatchPair(result.start, result.end);
        match.displace(displacement);
        *lastIndex = match.limit;
        return RegExpRunStatus_Success;
    }
#endif

    




    JS_ASSERT(hasBytecode());
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
    if (!matches.initArray(pairCount()))
        return RegExpRunStatus_Error;

    unsigned result;
    {
        AutoTraceLog logInterpret(logger, TraceLogger::YarrInterpret);
        result = JSC::Yarr::interpret(cx, bytecode, chars, length, start, (unsigned *) matches.pairsRaw());
    }

    if (result == JSC::Yarr::offsetError) {
        reportYarrError(cx, nullptr, JSC::Yarr::RuntimeError);
        return RegExpRunStatus_Error;
    }

    if (result == JSC::Yarr::offsetNoMatch)
        return RegExpRunStatus_Success_NotFound;

    match = MatchPair(result, matches[0].limit);
    match.displace(displacement);

#ifdef DEBUG
    matches.displace(displacement);
    matches.checkAgainst(origLength);
#endif

    *lastIndex = match.limit;
    return RegExpRunStatus_Success;
}

#endif 

size_t
RegExpShared::sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    size_t n = mallocSizeOf(this);

#ifndef JS_YARR
    if (byteCode)
        n += mallocSizeOf(byteCode);
#endif

    n += tables.sizeOfExcludingThis(mallocSizeOf);
    for (size_t i = 0; i < tables.length(); i++)
        n += mallocSizeOf(tables[i]);

    return n;
}



RegExpCompartment::RegExpCompartment(JSRuntime *rt)
  : set_(rt), matchResultTemplateObject_(nullptr)
{}

RegExpCompartment::~RegExpCompartment()
{
    
    
    if (set_.initialized()) {
        for (Set::Enum e(set_); !e.empty(); e.popFront()) {
            RegExpShared *shared = e.front();
            js_delete(shared);
        }
    }
}

JSObject *
RegExpCompartment::createMatchResultTemplateObject(JSContext *cx)
{
    JS_ASSERT(!matchResultTemplateObject_);

    
    RootedObject templateObject(cx, NewDenseUnallocatedArray(cx, 0, nullptr, TenuredObject));
    if (!templateObject)
        return matchResultTemplateObject_; 

    
    RootedValue index(cx, Int32Value(0));
    if (!baseops::DefineProperty(cx, templateObject, cx->names().index, index,
                                 JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE))
        return matchResultTemplateObject_; 

    
    RootedValue inputVal(cx, StringValue(cx->runtime()->emptyString));
    if (!baseops::DefineProperty(cx, templateObject, cx->names().input, inputVal,
                                 JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE))
        return matchResultTemplateObject_; 

    
    DebugOnly<Shape *> shape = templateObject->lastProperty();
    JS_ASSERT(shape->previous()->slot() == 0 &&
              shape->previous()->propidRef() == NameToId(cx->names().index));
    JS_ASSERT(shape->slot() == 1 &&
              shape->propidRef() == NameToId(cx->names().input));

    matchResultTemplateObject_.set(templateObject);

    return matchResultTemplateObject_;
}

bool
RegExpCompartment::init(JSContext *cx)
{
    if (!set_.init(0)) {
        if (cx)
            js_ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

void
RegExpCompartment::sweep(JSRuntime *rt)
{
    for (Set::Enum e(set_); !e.empty(); e.popFront()) {
        RegExpShared *shared = e.front();

        if (this != &shared->compartment()->regExps)
            MOZ_CRASH();

        
        
        
        
        
        
        
        
        
        bool keep = shared->marked() && !IsStringAboutToBeFinalized(shared->source.unsafeGet());
#if !defined(JS_YARR) && defined(JS_ION)
        if (keep && shared->jitCode)
            keep = !IsJitCodeAboutToBeFinalized(shared->jitCode.unsafeGet());
#endif
        if (keep) {
            shared->clearMarked();
        } else {
            js_delete(shared);
            e.removeFront();
        }
    }

    if (matchResultTemplateObject_ &&
        IsObjectAboutToBeFinalized(matchResultTemplateObject_.unsafeGet()))
    {
        matchResultTemplateObject_.set(nullptr);
    }
}

bool
RegExpCompartment::get(JSContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g)
{
    if (this != &cx->compartment()->regExps)
        MOZ_CRASH();

    Key key(source, flags);
    Set::AddPtr p = set_.lookupForAdd(key);
    if (p) {
        
        
        MaybeTraceRegExpShared(cx, *p);

        if ((*p)->compartment() != cx->compartment())
            MOZ_CRASH();

        g->init(**p);
        return true;
    }

    ScopedJSDeletePtr<RegExpShared> shared(cx->new_<RegExpShared>(cx->compartment(), source, flags));
    if (!shared)
        return false;

    if (!set_.add(p, shared)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    
    MaybeTraceRegExpShared(cx, shared);

    g->init(*shared.forget());
    return true;
}

bool
RegExpCompartment::get(JSContext *cx, HandleAtom atom, JSString *opt, RegExpGuard *g)
{
    RegExpFlag flags = RegExpFlag(0);
    if (opt && !ParseRegExpFlags(cx, opt, &flags))
        return false;

    return get(cx, atom, flags, g);
}

size_t
RegExpCompartment::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    size_t n = 0;
    n += set_.sizeOfExcludingThis(mallocSizeOf);
    for (Set::Enum e(set_); !e.empty(); e.popFront()) {
        RegExpShared *shared = e.front();
        n += shared->sizeOfIncludingThis(mallocSizeOf);
    }
    return n;
}



JSObject *
js::CloneRegExpObject(JSContext *cx, JSObject *obj_)
{
    RegExpObjectBuilder builder(cx);
    Rooted<RegExpObject*> regex(cx, &obj_->as<RegExpObject>());
    JSObject *res = builder.clone(regex);
    JS_ASSERT_IF(res, res->type() == regex->type());
    return res;
}

bool
js::ParseRegExpFlags(JSContext *cx, JSString *flagStr, RegExpFlag *flagsOut)
{
    size_t n = flagStr->length();
    const jschar *s = flagStr->getChars(cx);
    if (!s)
        return false;

    *flagsOut = RegExpFlag(0);
    for (size_t i = 0; i < n; i++) {
#define HANDLE_FLAG(name_)                                                    \
        JS_BEGIN_MACRO                                                        \
            if (*flagsOut & (name_))                                          \
                goto bad_flag;                                                \
            *flagsOut = RegExpFlag(*flagsOut | (name_));                      \
        JS_END_MACRO
        switch (s[i]) {
          case 'i': HANDLE_FLAG(IgnoreCaseFlag); break;
          case 'g': HANDLE_FLAG(GlobalFlag); break;
          case 'm': HANDLE_FLAG(MultilineFlag); break;
          case 'y': HANDLE_FLAG(StickyFlag); break;
          default:
          bad_flag:
          {
            char charBuf[2];
            charBuf[0] = char(s[i]);
            charBuf[1] = '\0';
            JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, nullptr,
                                         JSMSG_BAD_REGEXP_FLAG, charBuf);
            return false;
          }
        }
#undef HANDLE_FLAG
    }
    return true;
}

template<XDRMode mode>
bool
js::XDRScriptRegExpObject(XDRState<mode> *xdr, HeapPtrObject *objp)
{
    

    RootedAtom source(xdr->cx());
    uint32_t flagsword = 0;

    if (mode == XDR_ENCODE) {
        JS_ASSERT(objp);
        RegExpObject &reobj = (*objp)->as<RegExpObject>();
        source = reobj.getSource();
        flagsword = reobj.getFlags();
    }
    if (!XDRAtom(xdr, &source) || !xdr->codeUint32(&flagsword))
        return false;
    if (mode == XDR_DECODE) {
        RegExpFlag flags = RegExpFlag(flagsword);
        RegExpObject *reobj = RegExpObject::createNoStatics(xdr->cx(), source, flags, nullptr,
                                                            xdr->cx()->tempLifoAlloc());
        if (!reobj)
            return false;

        objp->init(reobj);
    }
    return true;
}

template bool
js::XDRScriptRegExpObject(XDRState<XDR_ENCODE> *xdr, HeapPtrObject *objp);

template bool
js::XDRScriptRegExpObject(XDRState<XDR_DECODE> *xdr, HeapPtrObject *objp);

JSObject *
js::CloneScriptRegExpObject(JSContext *cx, RegExpObject &reobj)
{
    

    RootedAtom source(cx, reobj.getSource());
    return RegExpObject::createNoStatics(cx, source, reobj.getFlags(), nullptr, cx->tempLifoAlloc());
}
