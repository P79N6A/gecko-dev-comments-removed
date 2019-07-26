






#include "frontend/TokenStream.h"
#include "vm/MatchPairs.h"
#include "vm/RegExpStatics.h"
#include "vm/StringBuffer.h"
#include "vm/Xdr.h"

#include "jsobjinlines.h"

#include "vm/RegExpObject-inl.h"
#include "vm/RegExpStatics-inl.h"

using namespace js;
using js::frontend::TokenStream;

JS_STATIC_ASSERT(IgnoreCaseFlag == JSREG_FOLD);
JS_STATIC_ASSERT(GlobalFlag == JSREG_GLOB);
JS_STATIC_ASSERT(MultilineFlag == JSREG_MULTILINE);
JS_STATIC_ASSERT(StickyFlag == JSREG_STICKY);



RegExpObjectBuilder::RegExpObjectBuilder(JSContext *cx, RegExpObject *reobj)
  : cx(cx), reobj_(cx, reobj)
{}

bool
RegExpObjectBuilder::getOrCreate()
{
    if (reobj_)
        return true;

    JSObject *obj = NewBuiltinClassInstance(cx, &RegExpClass);
    if (!obj)
        return false;
    obj->initPrivate(NULL);

    reobj_ = &obj->asRegExp();
    return true;
}

bool
RegExpObjectBuilder::getOrCreateClone(RegExpObject *proto)
{
    JS_ASSERT(!reobj_);

    JSObject *clone = NewObjectWithGivenProto(cx, &RegExpClass, proto, proto->getParent());
    if (!clone)
        return false;
    clone->initPrivate(NULL);

    reobj_ = &clone->asRegExp();
    return true;
}

RegExpObject *
RegExpObjectBuilder::build(HandleAtom source, RegExpShared &shared)
{
    if (!getOrCreate())
        return NULL;

    if (!reobj_->init(cx, source, shared.getFlags()))
        return NULL;

    reobj_->setShared(cx, shared);
    return reobj_;
}

RegExpObject *
RegExpObjectBuilder::build(HandleAtom source, RegExpFlag flags)
{
    if (!getOrCreate())
        return NULL;

    return reobj_->init(cx, source, flags) ? reobj_.get() : NULL;
}

RegExpObject *
RegExpObjectBuilder::clone(Handle<RegExpObject *> other, Handle<RegExpObject *> proto)
{
    if (!getOrCreateClone(proto))
        return NULL;

    




    RegExpStatics *res = proto->getParent()->asGlobal().getRegExpStatics();
    RegExpFlag origFlags = other->getFlags();
    RegExpFlag staticsFlags = res->getFlags();
    if ((origFlags & staticsFlags) != staticsFlags) {
        RegExpFlag newFlags = RegExpFlag(origFlags | staticsFlags);
        Rooted<JSAtom *> source(cx, other->getSource());
        return build(source, newFlags);
    }

    RegExpGuard g;
    if (!other->getShared(cx, &g))
        return NULL;

    Rooted<JSAtom *> source(cx, other->getSource());
    return build(source, *g);
}



MatchPairs *
MatchPairs::create(LifoAlloc &alloc, size_t pairCount, size_t backingPairCount)
{
    void *mem = alloc.alloc(calculateSize(backingPairCount));
    if (!mem)
        return NULL;

    return new (mem) MatchPairs(pairCount);
}

inline void
MatchPairs::checkAgainst(size_t inputLength)
{
#if DEBUG
    for (size_t i = 0; i < pairCount(); ++i) {
        MatchPair p = pair(i);
        p.check();
        if (p.isUndefined())
            continue;
        JS_ASSERT(size_t(p.limit) <= inputLength);
    }
#endif
}



static void
regexp_trace(JSTracer *trc, RawObject obj)
{
     




    if (trc->runtime->isHeapBusy() && IS_GC_MARKING_TRACER(trc))
        obj->setPrivate(NULL);
}

Class js::RegExpClass = {
    js_RegExp_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(RegExpObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_RegExp),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,        
    JS_ResolveStub,
    JS_ConvertStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    regexp_trace
};

RegExpObject *
RegExpObject::create(JSContext *cx, RegExpStatics *res, StableCharPtr chars, size_t length,
                     RegExpFlag flags, TokenStream *tokenStream)
{
    RegExpFlag staticsFlags = res->getFlags();
    return createNoStatics(cx, chars, length, RegExpFlag(flags | staticsFlags), tokenStream);
}

RegExpObject *
RegExpObject::createNoStatics(JSContext *cx, StableCharPtr chars, size_t length, RegExpFlag flags,
                              TokenStream *tokenStream)
{
    RootedAtom source(cx, AtomizeChars(cx, chars.get(), length));
    if (!source)
        return NULL;

    return createNoStatics(cx, source, flags, tokenStream);
}

RegExpObject *
RegExpObject::createNoStatics(JSContext *cx, HandleAtom source, RegExpFlag flags,
                              TokenStream *tokenStream)
{
    if (!RegExpShared::checkSyntax(cx, tokenStream, source))
        return NULL;

    RegExpObjectBuilder builder(cx);
    return builder.build(source, flags);
}

bool
RegExpObject::createShared(JSContext *cx, RegExpGuard *g)
{
    Rooted<RegExpObject*> self(cx, this);

    JS_ASSERT(!maybeShared());
    if (!cx->compartment->regExps.get(cx, getSource(), getFlags(), g))
        return false;

    self->setShared(cx, **g);
    return true;
}

UnrootedShape
RegExpObject::assignInitialShape(JSContext *cx)
{
    JS_ASSERT(isRegExp());
    JS_ASSERT(nativeEmpty());

    JS_STATIC_ASSERT(LAST_INDEX_SLOT == 0);
    JS_STATIC_ASSERT(SOURCE_SLOT == LAST_INDEX_SLOT + 1);
    JS_STATIC_ASSERT(GLOBAL_FLAG_SLOT == SOURCE_SLOT + 1);
    JS_STATIC_ASSERT(IGNORE_CASE_FLAG_SLOT == GLOBAL_FLAG_SLOT + 1);
    JS_STATIC_ASSERT(MULTILINE_FLAG_SLOT == IGNORE_CASE_FLAG_SLOT + 1);
    JS_STATIC_ASSERT(STICKY_FLAG_SLOT == MULTILINE_FLAG_SLOT + 1);

    RootedObject self(cx, this);

    
    if (!addDataProperty(cx, NameToId(cx->names().lastIndex), LAST_INDEX_SLOT, JSPROP_PERMANENT))
        return UnrootedShape(NULL);

    
    unsigned attrs = JSPROP_PERMANENT | JSPROP_READONLY;
    if (!self->addDataProperty(cx, NameToId(cx->names().source), SOURCE_SLOT, attrs))
        return UnrootedShape(NULL);
    if (!self->addDataProperty(cx, NameToId(cx->names().global), GLOBAL_FLAG_SLOT, attrs))
        return UnrootedShape(NULL);
    if (!self->addDataProperty(cx, NameToId(cx->names().ignoreCase), IGNORE_CASE_FLAG_SLOT, attrs))
        return UnrootedShape(NULL);
    if (!self->addDataProperty(cx, NameToId(cx->names().multiline), MULTILINE_FLAG_SLOT, attrs))
        return UnrootedShape(NULL);
    return self->addDataProperty(cx, NameToId(cx->names().sticky), STICKY_FLAG_SLOT, attrs);
}

inline bool
RegExpObject::init(JSContext *cx, HandleAtom source, RegExpFlag flags)
{
    Rooted<RegExpObject *> self(cx, this);

    if (nativeEmpty()) {
        if (isDelegate()) {
            if (!assignInitialShape(cx))
                return false;
        } else {
            RootedShape shape(cx, assignInitialShape(cx));
            if (!shape)
                return false;
            RootedObject proto(cx, self->getProto());
            EmptyShape::insertInitialShape(cx, shape, proto);
        }
        JS_ASSERT(!self->nativeEmpty());
    }

    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().lastIndex))->slot() ==
              LAST_INDEX_SLOT);
    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().source))->slot() ==
              SOURCE_SLOT);
    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().global))->slot() ==
              GLOBAL_FLAG_SLOT);
    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().ignoreCase))->slot() ==
              IGNORE_CASE_FLAG_SLOT);
    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().multiline))->slot() ==
              MULTILINE_FLAG_SLOT);
    JS_ASSERT(self->nativeLookupNoAllocation(NameToId(cx->names().sticky))->slot() ==
              STICKY_FLAG_SLOT);

    



    self->JSObject::setPrivate(NULL);

    self->zeroLastIndex();
    self->setSource(source);
    self->setGlobal(flags & GlobalFlag);
    self->setIgnoreCase(flags & IgnoreCaseFlag);
    self->setMultiline(flags & MultilineFlag);
    self->setSticky(flags & StickyFlag);
    return true;
}

RegExpRunStatus
RegExpObject::execute(JSContext *cx, StableCharPtr chars, size_t length, size_t *lastIndex,
                      MatchPairs **output)
{
    RegExpGuard g;
    if (!getShared(cx, &g))
        return RegExpRunStatus_Error;
    return g->execute(cx, chars, length, lastIndex, output);
}

JSFlatString *
RegExpObject::toString(JSContext *cx) const
{
    JSAtom *src = getSource();
    StringBuffer sb(cx);
    if (size_t len = src->length()) {
        if (!sb.reserve(len + 2))
            return NULL;
        sb.infallibleAppend('/');
        sb.infallibleAppend(src->chars(), len);
        sb.infallibleAppend('/');
    } else {
        if (!sb.append("/(?:)/"))
            return NULL;
    }
    if (global() && !sb.append('g'))
        return NULL;
    if (ignoreCase() && !sb.append('i'))
        return NULL;
    if (multiline() && !sb.append('m'))
        return NULL;
    if (sticky() && !sb.append('y'))
        return NULL;

    return sb.finishString();
}



RegExpShared::RegExpShared(JSRuntime *rt, JSAtom *source, RegExpFlag flags)
  : source(source), flags(flags), parenCount(0),
#if ENABLE_YARR_JIT
    codeBlock(),
#endif
    bytecode(NULL), activeUseCount(0), gcNumberWhenUsed(rt->gcNumber)
{}

RegExpShared::~RegExpShared()
{
#if ENABLE_YARR_JIT
    codeBlock.release();
#endif
    if (bytecode)
        js_delete<BytecodePattern>(bytecode);
}

void
RegExpShared::reportYarrError(JSContext *cx, TokenStream *ts, ErrorCode error)
{
    switch (error) {
      case JSC::Yarr::NoError:
        JS_NOT_REACHED("Called reportYarrError with value for no error");
        return;
#define COMPILE_EMSG(__code, __msg)                                                              \
      case JSC::Yarr::__code:                                                                    \
        if (ts)                                                                                  \
            ts->reportError(__msg);                                                              \
        else                                                                                     \
            JS_ReportErrorFlagsAndNumberUC(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL, __msg); \
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
#undef COMPILE_EMSG
      default:
        JS_NOT_REACHED("Unknown Yarr error code");
    }
}

bool
RegExpShared::checkSyntax(JSContext *cx, TokenStream *tokenStream, JSLinearString *source)
{
    ErrorCode error = JSC::Yarr::checkSyntax(*source);
    if (error == JSC::Yarr::NoError)
        return true;

    reportYarrError(cx, tokenStream, error);
    return false;
}

bool
RegExpShared::compile(JSContext *cx)
{
    if (!sticky())
        return compile(cx, *source);

    



    static const jschar prefix[] = {'^', '(', '?', ':'};
    static const jschar postfix[] = {')'};

    using mozilla::ArrayLength;
    StringBuffer sb(cx);
    if (!sb.reserve(ArrayLength(prefix) + source->length() + ArrayLength(postfix)))
        return false;
    sb.infallibleAppend(prefix, ArrayLength(prefix));
    sb.infallibleAppend(source->chars(), source->length());
    sb.infallibleAppend(postfix, ArrayLength(postfix));

    JSAtom *fakeySource = sb.finishAtom();
    if (!fakeySource)
        return false;

    return compile(cx, *fakeySource);
}

bool
RegExpShared::compile(JSContext *cx, JSLinearString &pattern)
{
    
    ErrorCode yarrError;
    YarrPattern yarrPattern(pattern, ignoreCase(), multiline(), &yarrError);
    if (yarrError) {
        reportYarrError(cx, NULL, yarrError);
        return false;
    }
    this->parenCount = yarrPattern.m_numSubpatterns;

#if ENABLE_YARR_JIT
    if (isJITRuntimeEnabled(cx) && !yarrPattern.m_containsBackreferences) {
        JSC::ExecutableAllocator *execAlloc = cx->runtime->getExecAlloc(cx);
        if (!execAlloc)
            return false;

        JSGlobalData globalData(execAlloc);
        YarrJITCompileMode compileMode = JSC::Yarr::IncludeSubpatterns;

        jitCompile(yarrPattern, JSC::Yarr::Char16, &globalData, codeBlock, compileMode);

        
        if (!codeBlock.isFallBack())
            return true;
    }
    codeBlock.setFallBack(true);
#endif

    WTF::BumpPointerAllocator *bumpAlloc = cx->runtime->getBumpPointerAllocator(cx);
    if (!bumpAlloc) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    bytecode = byteCompile(yarrPattern, bumpAlloc).get();
    return true;
}

bool
RegExpShared::compileIfNecessary(JSContext *cx)
{
    if (hasCode() || hasBytecode())
        return true;
    return compile(cx);
}

RegExpRunStatus
RegExpShared::execute(JSContext *cx, StableCharPtr chars, size_t length, size_t *lastIndex,
                      MatchPairs **output)
{
    
    if (!compileIfNecessary(cx))
        return RegExpRunStatus_Error;

    const size_t origLength = length;
    size_t backingPairCount = pairCount() * 2;

    LifoAlloc &alloc = cx->tempLifoAlloc();
    MatchPairs *matchPairs = MatchPairs::create(alloc, pairCount(), backingPairCount);
    if (!matchPairs)
        return RegExpRunStatus_Error;

    



    size_t start = *lastIndex;
    size_t displacement = 0;

    if (sticky()) {
        displacement = *lastIndex;
        chars += displacement;
        length -= displacement;
        start = 0;
    }

    unsigned *outputBuf = (unsigned *)matchPairs->buffer();
    unsigned result;

#if ENABLE_YARR_JIT
    if (codeBlock.isFallBack())
        result = JSC::Yarr::interpret(bytecode, chars.get(), length, start, outputBuf);
    else
        result = codeBlock.execute(chars.get(), start, length, (int *)outputBuf).start;
#else
    result = JSC::Yarr::interpret(bytecode, chars.get(), length, start, outputBuf);
#endif

    *output = matchPairs;

    if (result == JSC::Yarr::offsetNoMatch)
        return RegExpRunStatus_Success_NotFound;

    matchPairs->displace(displacement);
    matchPairs->checkAgainst(origLength);
    *lastIndex = matchPairs->pair(0).limit;
    return RegExpRunStatus_Success;
}



RegExpCompartment::RegExpCompartment(JSRuntime *rt)
  : map_(rt), inUse_(rt)
{}

RegExpCompartment::~RegExpCompartment()
{
    JS_ASSERT(map_.empty());
    JS_ASSERT(inUse_.empty());
}

bool
RegExpCompartment::init(JSContext *cx)
{
    if (!map_.init() || !inUse_.init()) {
        if (cx)
            js_ReportOutOfMemory(cx);
        return false;
    }

    return true;
}


void
RegExpCompartment::sweep(JSRuntime *rt)
{
#ifdef DEBUG
    for (Map::Range r = map_.all(); !r.empty(); r.popFront())
        JS_ASSERT(inUse_.has(r.front().value));
#endif

    map_.clear();

    for (PendingSet::Enum e(inUse_); !e.empty(); e.popFront()) {
        RegExpShared *shared = e.front();
        if (shared->activeUseCount == 0 && shared->gcNumberWhenUsed < rt->gcStartNumber) {
            js_delete(shared);
            e.removeFront();
        }
    }
}

inline bool
RegExpCompartment::get(JSContext *cx, JSAtom *source, RegExpFlag flags, RegExpGuard *g)
{
    Key key(source, flags);
    Map::AddPtr p = map_.lookupForAdd(key);
    if (p) {
        g->init(*p->value);
        return true;
    }

    ScopedDeletePtr<RegExpShared> shared(cx->new_<RegExpShared>(cx->runtime, source, flags));
    if (!shared)
        return false;

    
    if (!map_.add(p, key, shared)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    
    if (!inUse_.put(shared)) {
        map_.remove(key);
        js_ReportOutOfMemory(cx);
        return false;
    }

    
    g->init(*shared.forget());
    return true;
}

bool
RegExpCompartment::get(JSContext *cx, JSAtom *atom, JSString *opt, RegExpGuard *g)
{
    RegExpFlag flags = RegExpFlag(0);
    if (opt && !ParseRegExpFlags(cx, opt, &flags))
        return false;

    return get(cx, atom, flags, g);
}

size_t
RegExpCompartment::sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf)
{
    return map_.sizeOfExcludingThis(mallocSizeOf);
}



JSObject *
js::CloneRegExpObject(JSContext *cx, JSObject *obj_, JSObject *proto_)
{
    RegExpObjectBuilder builder(cx);
    Rooted<RegExpObject*> regex(cx, &obj_->asRegExp());
    Rooted<RegExpObject*> proto(cx, &proto_->asRegExp());
    return builder.clone(regex, proto);
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
            JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
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
        RegExpObject &reobj = (*objp)->asRegExp();
        source = reobj.getSource();
        flagsword = reobj.getFlags();
    }
    if (!XDRAtom(xdr, &source) || !xdr->codeUint32(&flagsword))
        return false;
    if (mode == XDR_DECODE) {
        RegExpFlag flags = RegExpFlag(flagsword);
        Rooted<RegExpObject*> reobj(xdr->cx(), RegExpObject::createNoStatics(xdr->cx(), source, flags, NULL));
        if (!reobj)
            return false;

        if (!JSObject::clearParent(xdr->cx(), reobj))
            return false;
        if (!JSObject::clearType(xdr->cx(), reobj))
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
    Rooted<RegExpObject*> clone(cx, RegExpObject::createNoStatics(cx, source, reobj.getFlags(), NULL));
    if (!clone)
        return NULL;
    if (!JSObject::clearParent(cx, clone))
        return NULL;
    if (!JSObject::clearType(cx, clone))
        return NULL;
    return clone;
}
