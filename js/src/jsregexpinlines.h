






































#ifndef jsregexpinlines_h___
#define jsregexpinlines_h___

#include "jsregexp.h"
#include "jscntxt.h"
#include "jsobjinlines.h"
#include "assembler/wtf/Platform.h"

#if ENABLE_YARR_JIT
#include "yarr/yarr/RegexJIT.h"
#else
#include "yarr/pcre/pcre.h"
#endif

namespace js {





extern Class regexp_statics_class;

static inline JSObject *
regexp_statics_construct(JSContext *cx, JSObject *parent)
{
    JSObject *obj = NewObject<WithProto::Given>(cx, &regexp_statics_class, NULL, parent);
    if (!obj)
        return NULL;
    RegExpStatics *res = cx->create<RegExpStatics>();
    if (!res)
        return NULL;
    obj->setPrivate(static_cast<void *>(res));
    return obj;
}


class RegExp
{
    jsrefcount                  refCount;
    JSString                    *source;
#if ENABLE_YARR_JIT
    JSC::Yarr::RegexCodeBlock   compiled;
#else
    JSRegExp                    *compiled;
#endif
    unsigned                    parenCount;
    uint32                      flags;

    RegExp(JSString *source, uint32 flags)
      : refCount(1), source(source), compiled(), parenCount(0), flags(flags) {}
    bool compileHelper(JSContext *cx, UString &pattern);
    bool compile(JSContext *cx);
    static const uint32 allFlags = JSREG_FOLD | JSREG_GLOB | JSREG_MULTILINE | JSREG_STICKY;
    void handlePCREError(JSContext *cx, int error);
    void handleYarrError(JSContext *cx, int error);
    static inline bool initArena(JSContext *cx);
    static inline void checkMatchPairs(JSString *input, int *buf, size_t matchItemCount);
    static JSObject *createResult(JSContext *cx, JSString *input, int *buf, size_t matchItemCount);
    inline bool executeInternal(JSContext *cx, RegExpStatics *res, JSString *input,
                                size_t *lastIndex, bool test, Value *rval);

  public:
    ~RegExp() {
#if !ENABLE_YARR_JIT
        if (compiled)
            jsRegExpFree(compiled);
#endif
    }

    static bool isMetaChar(jschar c);
    static bool hasMetaChars(const jschar *chars, size_t length);

    



    static bool parseFlags(JSContext *cx, JSString *flagStr, uint32 &flagsOut);

    






    bool execute(JSContext *cx, RegExpStatics *res, JSString *input, size_t *lastIndex, bool test,
                 Value *rval) {
        JS_ASSERT(res);
        return executeInternal(cx, res, input, lastIndex, test, rval);
    }

    bool executeNoStatics(JSContext *cx, JSString *input, size_t *lastIndex, bool test,
                          Value *rval) {
        return executeInternal(cx, NULL, input, lastIndex, test, rval);
    }

    
    static RegExp *create(JSContext *cx, JSString *source, uint32 flags);
    static RegExp *createFlagged(JSContext *cx, JSString *source, JSString *flags);
    





    static JSObject *createObject(JSContext *cx, RegExpStatics *res, const jschar *chars,
                                  size_t length, uint32 flags);
    static JSObject *createObjectNoStatics(JSContext *cx, const jschar *chars, size_t length,
                                           uint32 flags);
    static RegExp *extractFrom(JSObject *obj);
    static RegExp *clone(JSContext *cx, const RegExp &other);

    
    void incref(JSContext *cx) { JS_ATOMIC_INCREMENT(&refCount); }
    void decref(JSContext *cx);

    
    JSString *getSource() const { return source; }
    size_t getParenCount() const { return parenCount; }
    bool ignoreCase() const { return flags & JSREG_FOLD; }
    bool global() const { return flags & JSREG_GLOB; }
    bool multiline() const { return flags & JSREG_MULTILINE; }
    bool sticky() const { return flags & JSREG_STICKY; }

    const uint32 &getFlags() const { JS_ASSERT((flags & allFlags) == flags); return flags; }
    uint32 flagCount() const;
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
        return !!js_DefineProperty(cx, array, id, &val, js::PropertyStub, js::PropertyStub,
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
    int largestStartSeen = 0;
    for (size_t i = 0; i < matchItemCount; i += 2) {
        int start = buf[i];
        int limit = buf[i + 1];
        JS_ASSERT(limit >= start); 
        if (start == -1)
            continue;
        JS_ASSERT(start >= 0);
        JS_ASSERT(size_t(limit) <= inputLength);
        
        JS_ASSERT(start >= largestStartSeen);
        largestStartSeen = start;
    }
#endif
}

inline JSObject *
RegExp::createResult(JSContext *cx, JSString *input, int *buf, size_t matchItemCount)
{
    




    JSObject *array = js_NewSlowArrayObject(cx);
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
RegExp::executeInternal(JSContext *cx, RegExpStatics *res, JSString *input,
                        size_t *lastIndex, bool test, Value *rval)
{
#if !ENABLE_YARR_JIT
    JS_ASSERT(compiled);
#endif
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

    const jschar *chars = input->chars();
    size_t len = input->length();

    



    size_t inputOffset = 0;

    if (sticky()) {
        
        chars += *lastIndex;
        len -= *lastIndex;
        inputOffset = *lastIndex;
    }

#if ENABLE_YARR_JIT
    int result = JSC::Yarr::executeRegex(cx, compiled, chars, *lastIndex - inputOffset, len, buf,
                                         bufCount);
#else
    int result = jsRegExpExecute(cx, compiled, chars, len, *lastIndex - inputOffset, buf, 
                                 bufCount) < 0 ? -1 : buf[0];
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

inline RegExp *
RegExp::create(JSContext *cx, JSString *source, uint32 flags)
{
    RegExp *self;
    void *mem = cx->malloc(sizeof(*self));
    if (!mem)
        return NULL;
    self = new (mem) RegExp(source, flags);
    if (!self->compile(cx)) {
        cx->destroy<RegExp>(self);
        return NULL;
    }
    return self;
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
    RegExp *re = RegExp::create(cx, str, flags);
    if (!re)
        return NULL;
    JSObject *obj = NewBuiltinClassInstance(cx, &js_RegExpClass);
    if (!obj) {
        re->decref(cx);
        return NULL;
    }
    obj->setPrivate(re);
    obj->zeroRegExpLastIndex();
    return obj;
}

#ifdef ANDROID
static bool
YarrJITIsBroken(JSContext *cx)
{
#if defined(JS_TRACER) && defined(JS_METHODJIT)
    




    return !cx->traceJitEnabled && !cx->methodJitEnabled;
#else
    return false;
#endif
}
#endif  

inline bool
RegExp::compileHelper(JSContext *cx, UString &pattern)
{
#if ENABLE_YARR_JIT
    bool fellBack = false;
    int error = 0;
    jitCompileRegex(*cx->runtime->regExpAllocator, compiled, pattern, parenCount, error, fellBack, ignoreCase(), multiline()
#ifdef ANDROID
                    
                    , YarrJITIsBroken(cx)
#endif
);
    if (!error)
        return true;
    if (fellBack)
        handlePCREError(cx, error);
    else
        handleYarrError(cx, error);
    return false;
#else
    int error = 0;
    compiled = jsRegExpCompile(pattern.chars(), pattern.length(),
                               ignoreCase() ? JSRegExpIgnoreCase : JSRegExpDoNotIgnoreCase,
                               multiline() ? JSRegExpMultiline : JSRegExpSingleLine,
                               &parenCount, &error);
    if (!error)
        return true;
    handlePCREError(cx, error);
    return false;
#endif
}

inline bool
RegExp::compile(JSContext *cx)
{
    if (!sticky())
        return compileHelper(cx, *source);
    



    static const jschar prefix[] = {'^', '(', '?', ':'};
    static const jschar postfix[] = {')'};

    JSCharBuffer cb(cx);
    if (!cb.reserve(JS_ARRAY_LENGTH(prefix) + source->length() + JS_ARRAY_LENGTH(postfix)))
        return false;
    JS_ALWAYS_TRUE(cb.append(prefix, JS_ARRAY_LENGTH(prefix)));
    JS_ALWAYS_TRUE(cb.append(source->chars(), source->length()));
    JS_ALWAYS_TRUE(cb.append(postfix, JS_ARRAY_LENGTH(postfix)));

    JSString *fakeySource = js_NewStringFromCharBuffer(cx, cb);
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

inline uint32
RegExp::flagCount() const
{
    uint32 nflags = 0;
    for (uint32 tmpFlags = flags; tmpFlags != 0; tmpFlags &= tmpFlags - 1)
        nflags++;
    return nflags;
}

inline void
RegExp::decref(JSContext *cx)
{
    if (JS_ATOMIC_DECREMENT(&refCount) == 0)
        cx->destroy<RegExp>(this);
}

inline RegExp *
RegExp::extractFrom(JSObject *obj)
{
    JS_ASSERT_IF(obj, obj->isRegExp());
    return static_cast<RegExp *>(obj->getPrivate());
}

inline RegExp *
RegExp::clone(JSContext *cx, const RegExp &other)
{
    return create(cx, other.source, other.flags);
}



inline RegExpStatics *
RegExpStatics::extractFrom(JSObject *global)
{
    Value resVal = global->getReservedSlot(JSRESERVED_GLOBAL_REGEXP_STATICS);
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
    out->chars = matchPairsInput->chars() + getCrash(pairNum, 0);
    out->length = getParenLength(pairNum);
}

inline void
RegExpStatics::getLastMatch(JSSubString *out) const
{
    if (!pairCountCrash()) {
        *out = js_EmptySubString;
        return;
    }
    JS_CRASH_UNLESS(matchPairsInput);
    out->chars = matchPairsInput->chars() + getCrash(0, 0);
    JS_CRASH_UNLESS(getCrash(0, 1) >= getCrash(0, 0));
    out->length = get(0, 1) - get(0, 0);
}

inline void
RegExpStatics::getLastParen(JSSubString *out) const
{
    size_t pairCount = pairCountCrash();
    
    if (pairCount <= 1) {
        *out = js_EmptySubString;
        return;
    }
    getParen(pairCount - 1, out);
}

inline void
RegExpStatics::getLeftContext(JSSubString *out) const
{
    if (!pairCountCrash()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars();
    out->length = getCrash(0, 0);
}

inline void
RegExpStatics::getRightContext(JSSubString *out) const
{
    if (!pairCountCrash()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchPairsInput->chars() + getCrash(0, 1);
    JS_CRASH_UNLESS(get(0, 1) <= int(matchPairsInput->length()));
    out->length = matchPairsInput->length() - get(0, 1);
}

}

#endif 
