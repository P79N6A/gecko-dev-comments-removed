






































#ifndef jsregexpinlines_h___
#define jsregexpinlines_h___

#include "jscntxt.h"
#include "jsobj.h"
#include "jsregexp.h"
#include "jsscope.h"

#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "methodjit/MethodJIT.h"
#include "assembler/wtf/Platform.h"
#include "yarr/BumpPointerAllocator.h"

#include "yarr/Yarr.h"
#if ENABLE_YARR_JIT
#include "yarr/YarrJIT.h"
#endif

namespace js {





extern Class regexp_statics_class;

static inline JSObject *
regexp_statics_construct(JSContext *cx, GlobalObject *parent)
{
    JSObject *obj = NewObject<WithProto::Given>(cx, &regexp_statics_class, NULL, parent);
    if (!obj)
        return NULL;
    RegExpStatics *res = cx->new_<RegExpStatics>();
    if (!res)
        return NULL;
    obj->setPrivate(static_cast<void *>(res));
    return obj;
}
















class RegExp
{
#if ENABLE_YARR_JIT
    
    JSC::Yarr::YarrCodeBlock    codeBlock;
#endif
    JSC::Yarr::BytecodePattern  *byteCode;
    JSLinearString              *source;
    size_t                      refCount;
    unsigned                    parenCount; 
    uint32                      flags;
#ifdef DEBUG
  public:
    JSCompartment               *compartment;

  private:
#endif

    RegExp(JSLinearString *source, uint32 flags, JSCompartment *compartment)
      :
#if ENABLE_YARR_JIT
        codeBlock(),
#endif
        byteCode(NULL), source(source), refCount(1), parenCount(0), flags(flags)
#ifdef DEBUG
        , compartment(compartment)
#endif
    { }

    JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR;

    ~RegExp() {
#if ENABLE_YARR_JIT
        codeBlock.release();
#endif
        
        if (byteCode)
            delete byteCode;
    }

    bool compileHelper(JSContext *cx, JSLinearString &pattern);
    bool compile(JSContext *cx);
    static const uint32 allFlags = JSREG_FOLD | JSREG_GLOB | JSREG_MULTILINE | JSREG_STICKY;
    void reportYarrError(JSContext *cx, JSC::Yarr::ErrorCode error);
    static inline bool initArena(JSContext *cx);
    static inline void checkMatchPairs(JSString *input, int *buf, size_t matchItemCount);
    static JSObject *createResult(JSContext *cx, JSString *input, int *buf, size_t matchItemCount);
    inline bool executeInternal(JSContext *cx, RegExpStatics *res, JSString *input,
                                size_t *lastIndex, bool test, Value *rval);

  public:
    static inline bool isMetaChar(jschar c);
    static inline bool hasMetaChars(const jschar *chars, size_t length);

    





    static bool parseFlags(JSContext *cx, JSString *flagStr, uintN *flagsOut);

    






    bool execute(JSContext *cx, RegExpStatics *res, JSString *input, size_t *lastIndex, bool test,
                 Value *rval) {
        JS_ASSERT(res);
        return executeInternal(cx, res, input, lastIndex, test, rval);
    }

    bool executeNoStatics(JSContext *cx, JSString *input, size_t *lastIndex, bool test,
                          Value *rval) {
        return executeInternal(cx, NULL, input, lastIndex, test, rval);
    }

    

    static AlreadyIncRefed<RegExp> create(JSContext *cx, JSString *source, uint32 flags);

    
    static AlreadyIncRefed<RegExp> createFlagged(JSContext *cx, JSString *source, JSString *flags);

    





    static JSObject *createObject(JSContext *cx, RegExpStatics *res, const jschar *chars,
                                  size_t length, uint32 flags);
    static JSObject *createObjectNoStatics(JSContext *cx, const jschar *chars, size_t length,
                                           uint32 flags);
    static RegExp *extractFrom(JSObject *obj);

    

    void incref(JSContext *cx);
    void decref(JSContext *cx);

    

    JSLinearString *getSource() const { return source; }
    size_t getParenCount() const { return parenCount; }
    bool ignoreCase() const { return flags & JSREG_FOLD; }
    bool global() const { return flags & JSREG_GLOB; }
    bool multiline() const { return flags & JSREG_MULTILINE; }
    bool sticky() const { return flags & JSREG_STICKY; }

    const uint32 &getFlags() const {
        JS_ASSERT((flags & allFlags) == flags);
        return flags;
    }
};

class RegExpMatchBuilder
{
    JSContext   * const cx;
    JSObject    * const array;

  public:
    RegExpMatchBuilder(JSContext *cx, JSObject *array) : cx(cx), array(array) {}

    bool append(int index, JSString *str) {
        JS_ASSERT(str);
        return append(INT_TO_JSID(index), StringValue(str));
    }

    bool append(jsid id, Value val) {
        return !!js_DefineProperty(cx, array, id, &val, js::PropertyStub, js::StrictPropertyStub,
                                   JSPROP_ENUMERATE);
    }

    bool appendIndex(int index) {
        return append(ATOM_TO_JSID(cx->runtime->atomState.indexAtom), Int32Value(index));
    }

    
    bool appendInput(JSString *str) {
        JS_ASSERT(str);
        return append(ATOM_TO_JSID(cx->runtime->atomState.inputAtom), StringValue(str));
    }
};



inline bool
RegExp::initArena(JSContext *cx)
{
    if (cx->regExpPool.first.next)
        return true;

    






    int64 *timestamp;
    JS_ARENA_ALLOCATE_CAST(timestamp, int64 *, &cx->regExpPool, sizeof *timestamp);
    if (!timestamp)
        return false;
    *timestamp = JS_Now();
    return true;
}

inline void
RegExp::checkMatchPairs(JSString *input, int *buf, size_t matchItemCount)
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
RegExp::createResult(JSContext *cx, JSString *input, int *buf, size_t matchItemCount)
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
            if (!(captured && builder.append(i / 2, captured)))
                return NULL;
        } else {
            
            JS_ASSERT(i != 0); 
            if (!builder.append(INT_TO_JSID(i / 2), UndefinedValue()))
                return NULL;
        }
    }

    if (!builder.appendIndex(buf[0]) ||
        !builder.appendInput(input))
        return NULL;

    return array;
}

inline bool
RegExp::executeInternal(JSContext *cx, RegExpStatics *res, JSString *inputstr,
                        size_t *lastIndex, bool test, Value *rval)
{
    const size_t pairCount = parenCount + 1;
    const size_t bufCount = pairCount * 3; 
    const size_t matchItemCount = pairCount * 2;

    if (!initArena(cx))
        return false;

    AutoArenaAllocator aaa(&cx->regExpPool);
    int *buf = aaa.alloc<int>(bufCount);
    if (!buf)
        return false;

    



    for (int *it = buf; it != buf + matchItemCount; ++it)
        *it = -1;

    JSLinearString *input = inputstr->ensureLinear(cx);
    if (!input)
        return false;

    size_t len = input->length();
    const jschar *chars = input->chars();

    



    size_t inputOffset = 0;

    if (sticky()) {
        
        chars += *lastIndex;
        len -= *lastIndex;
        inputOffset = *lastIndex;
    }

    int result;
#if ENABLE_YARR_JIT
    if (!codeBlock.isFallBack())
        result = JSC::Yarr::execute(codeBlock, chars, *lastIndex - inputOffset, len, buf);
    else
        result = JSC::Yarr::interpret(byteCode, chars, *lastIndex - inputOffset, len, buf);
#else
    result = JSC::Yarr::interpret(byteCode, chars, *lastIndex - inputOffset, len, buf);
#endif
    if (result == -1) {
        *rval = NullValue();
        return true;
    }

    



    if (JS_UNLIKELY(inputOffset)) {
        for (size_t i = 0; i < matchItemCount; ++i)
            buf[i] = buf[i] < 0 ? -1 : buf[i] + inputOffset;
    }

    
    checkMatchPairs(input, buf, matchItemCount);

    if (res)
        res->updateFromMatch(cx, input, buf, matchItemCount);

    *lastIndex = buf[1];

    if (test) {
        *rval = BooleanValue(true);
        return true;
    }

    JSObject *array = createResult(cx, input, buf, matchItemCount);
    if (!array)
        return false;

    *rval = ObjectValue(*array);
    return true;
}

inline AlreadyIncRefed<RegExp>
RegExp::create(JSContext *cx, JSString *source, uint32 flags)
{
    typedef AlreadyIncRefed<RegExp> RetType;
    JSLinearString *flatSource = source->ensureLinear(cx);
    if (!flatSource)
        return RetType(NULL);
    RegExp *self = cx->new_<RegExp>(flatSource, flags, cx->compartment);
    if (!self)
        return RetType(NULL);
    if (!self->compile(cx)) {
        Foreground::delete_(self);
        return RetType(NULL);
    }
    return RetType(self);
}

inline JSObject *
RegExp::createObject(JSContext *cx, RegExpStatics *res, const jschar *chars, size_t length,
                     uint32 flags)
{
    uint32 staticsFlags = res->getFlags();
    return createObjectNoStatics(cx, chars, length, flags | staticsFlags);
}

inline JSObject *
RegExp::createObjectNoStatics(JSContext *cx, const jschar *chars, size_t length, uint32 flags)
{
    JS_ASSERT((flags & allFlags) == flags);
    JSString *str = js_NewStringCopyN(cx, chars, length);
    if (!str)
        return NULL;
    




    JS::Anchor<JSString *> anchor(str);
    AlreadyIncRefed<RegExp> re = RegExp::create(cx, str, flags);
    if (!re)
        return NULL;
    JSObject *obj = NewBuiltinClassInstance(cx, &js_RegExpClass);
    if (!obj || !obj->initRegExp(cx, re.get())) {
        re->decref(cx);
        return NULL;
    }
    return obj;
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
RegExp::compileHelper(JSContext *cx, JSLinearString &pattern)
{
    JSC::Yarr::ErrorCode yarrError;
    JSC::Yarr::YarrPattern yarrPattern(pattern, ignoreCase(), multiline(), &yarrError);
    if (yarrError) {
        reportYarrError(cx, yarrError);
        return false;
    }
    parenCount = yarrPattern.m_numSubpatterns;

#if ENABLE_YARR_JIT && defined(JS_METHODJIT)
    if (EnableYarrJIT(cx) && !yarrPattern.m_containsBackreferences) {
        JSC::Yarr::JSGlobalData globalData(cx->compartment->jaegerCompartment->execAlloc());
        JSC::Yarr::jitCompile(yarrPattern, &globalData, codeBlock);
        if (!codeBlock.isFallBack())
            return true;
    }
#endif

    codeBlock.setFallBack(true);
    byteCode = JSC::Yarr::byteCompile(yarrPattern, cx->compartment->regExpAllocator).get();

    return true;
}

inline bool
RegExp::compile(JSContext *cx)
{
    
    if (!source->ensureLinear(cx))
        return false;

    if (!sticky())
        return compileHelper(cx, *source);

    



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
    return compileHelper(cx, *fakeySource);
}

inline bool
RegExp::isMetaChar(jschar c)
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
RegExp::hasMetaChars(const jschar *chars, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (isMetaChar(chars[i]))
            return true;
    }
    return false;
}

inline void
RegExp::incref(JSContext *cx)
{
#ifdef DEBUG
    assertSameCompartment(cx, compartment);
#endif
    ++refCount;
}

inline void
RegExp::decref(JSContext *cx)
{
#ifdef DEBUG
    assertSameCompartment(cx, compartment);
#endif
    if (--refCount == 0)
        cx->delete_(this);
}

inline RegExp *
RegExp::extractFrom(JSObject *obj)
{
    JS_ASSERT_IF(obj, obj->isRegExp());
    RegExp *re = static_cast<RegExp *>(obj->getPrivate());
#ifdef DEBUG
    if (re)
        CompartmentChecker::check(obj->getCompartment(), re->compartment);
#endif
    return re;
}



inline RegExpStatics *
RegExpStatics::extractFrom(js::GlobalObject *globalObj)
{
    Value resVal = globalObj->getRegExpStatics();
    RegExpStatics *res = static_cast<RegExpStatics *>(resVal.toObject().getPrivate());
    return res;
}

inline bool
RegExpStatics::createDependent(JSContext *cx, size_t start, size_t end, Value *out) const 
{
    JS_ASSERT(start <= end);
    JS_ASSERT(end <= matchPairsInput->length());
    JSString *str = js_NewDependentString(cx, matchPairsInput, start, end - start);
    if (!str)
        return false;
    *out = StringValue(str);
    return true;
}

inline bool
RegExpStatics::createPendingInput(JSContext *cx, Value *out) const
{
    out->setString(pendingInput ? pendingInput : cx->runtime->emptyString);
    return true;
}

inline bool
RegExpStatics::makeMatch(JSContext *cx, size_t checkValidIndex, size_t pairNum, Value *out) const
{
    if (checkValidIndex / 2 >= pairCount() || matchPairs[checkValidIndex] < 0) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    return createDependent(cx, get(pairNum, 0), get(pairNum, 1), out);
}

inline bool
RegExpStatics::createLastParen(JSContext *cx, Value *out) const
{
    if (pairCount() <= 1) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    size_t num = pairCount() - 1;
    int start = get(num, 0);
    int end = get(num, 1);
    if (start == -1) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    JS_ASSERT(start >= 0 && end >= 0);
    JS_ASSERT(end >= start);
    return createDependent(cx, start, end, out);
}

inline bool
RegExpStatics::createLeftContext(JSContext *cx, Value *out) const
{
    if (!pairCount()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    if (matchPairs[0] < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(cx, 0, matchPairs[0], out);
}

inline bool
RegExpStatics::createRightContext(JSContext *cx, Value *out) const
{
    if (!pairCount()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    if (matchPairs[1] < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(cx, matchPairs[1], matchPairsInput->length(), out);
}

inline void
RegExpStatics::getParen(size_t pairNum, JSSubString *out) const
{
    checkParenNum(pairNum);
    if (!pairIsPresent(pairNum)) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars() + get(pairNum, 0);
    out->length = getParenLength(pairNum);
}

inline void
RegExpStatics::getLastMatch(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    JS_ASSERT(matchPairsInput);
    out->chars = matchPairsInput->chars() + get(0, 0);
    JS_ASSERT(get(0, 1) >= get(0, 0));
    out->length = get(0, 1) - get(0, 0);
}

inline void
RegExpStatics::getLastParen(JSSubString *out) const
{
    size_t pc = pairCount();
    
    if (pc <= 1) {
        *out = js_EmptySubString;
        return;
    }
    getParen(pc - 1, out);
}

inline void
RegExpStatics::getLeftContext(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars();
    out->length = get(0, 0);
}

inline void
RegExpStatics::getRightContext(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars() + get(0, 1);
    JS_ASSERT(get(0, 1) <= int(matchPairsInput->length()));
    out->length = matchPairsInput->length() - get(0, 1);
}

} 

inline bool
JSObject::initRegExp(JSContext *cx, js::RegExp *re)
{
    JS_ASSERT(isRegExp());

    



    if (nativeEmpty()) {
        const js::Shape **shapep = &cx->compartment->initialRegExpShape;
        if (!*shapep) {
            *shapep = assignInitialRegExpShape(cx);
            if (!*shapep)
                return false;
        }
        setLastProperty(*shapep);
        JS_ASSERT(!nativeEmpty());
    }

    JS_ASSERT(nativeLookup(ATOM_TO_JSID(cx->runtime->atomState.lastIndexAtom))->slot ==
              JSObject::JSSLOT_REGEXP_LAST_INDEX);
    JS_ASSERT(nativeLookup(ATOM_TO_JSID(cx->runtime->atomState.sourceAtom))->slot ==
              JSObject::JSSLOT_REGEXP_SOURCE);
    JS_ASSERT(nativeLookup(ATOM_TO_JSID(cx->runtime->atomState.globalAtom))->slot ==
              JSObject::JSSLOT_REGEXP_GLOBAL);
    JS_ASSERT(nativeLookup(ATOM_TO_JSID(cx->runtime->atomState.ignoreCaseAtom))->slot ==
              JSObject::JSSLOT_REGEXP_IGNORE_CASE);
    JS_ASSERT(nativeLookup(ATOM_TO_JSID(cx->runtime->atomState.multilineAtom))->slot ==
              JSObject::JSSLOT_REGEXP_MULTILINE);
    JS_ASSERT(nativeLookup(ATOM_TO_JSID(cx->runtime->atomState.stickyAtom))->slot ==
              JSObject::JSSLOT_REGEXP_STICKY);

    setPrivate(re);
    zeroRegExpLastIndex();
    setRegExpSource(re->getSource());
    setRegExpGlobal(re->global());
    setRegExpIgnoreCase(re->ignoreCase());
    setRegExpMultiline(re->multiline());
    setRegExpSticky(re->sticky());
    return true;
}

#endif 
