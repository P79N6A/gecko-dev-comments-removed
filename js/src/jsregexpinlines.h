






































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
      : refCount(1), source(source), compiled(), parenCount(), flags(flags) {}
    bool compileHelper(JSContext *cx, UString &pattern);
    bool compile(JSContext *cx);
    static const uint32 allFlags = JSREG_FOLD | JSREG_GLOB | JSREG_MULTILINE | JSREG_STICKY;
    void handlePCREError(JSContext *cx, int error);
    void handleYarrError(JSContext *cx, int error);

  public:
    ~RegExp() {
#if !ENABLE_YARR_JIT
        if (compiled)
            jsRegExpFree(compiled);
#endif
    }

    static bool hasMetaChars(const jschar *chars, size_t length);

    



    static bool parseFlags(JSContext *cx, JSString *flagStr, uint32 &flagsOut);

    






    bool execute(JSContext *cx, JSString *input, size_t *lastIndex, bool test, Value *rval);

    
    static RegExp *create(JSContext *cx, JSString *source, uint32 flags);
    static RegExp *createFlagged(JSContext *cx, JSString *source, JSString *flags);
    





    static JSObject *createObject(JSContext *cx, const jschar *chars, size_t length, uint32 flags);
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
RegExp::createObject(JSContext *cx, const jschar *chars, size_t length, uint32 flags)
{
    JS_ASSERT((flags & allFlags) == flags);
    JSString *str = js_NewStringCopyN(cx, chars, length);
    if (!str)
        return NULL;
    AutoValueRooter tvr(cx, StringValue(str));
    uint32 staticsFlags = cx->regExpStatics.getFlags();
    JS_ASSERT((staticsFlags & allFlags) == staticsFlags);
    RegExp *re = RegExp::create(cx, str, flags | staticsFlags);
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

inline bool
RegExp::compileHelper(JSContext *cx, UString &pattern)
{
#if ENABLE_YARR_JIT
    bool fellBack = false;
    int error = 0;
    jitCompileRegex(*cx->runtime->regExpAllocator, compiled, pattern, parenCount, error, fellBack, ignoreCase(), multiline());
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
RegExp::hasMetaChars(const jschar *chars, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        jschar c = chars[i];
        switch (c) {
          
          case '^': case '$': case '\\': case '.': case '*': case '+':
          case '?': case '(': case ')': case '[': case ']': case '{':
          case '}': case '|':
            return true;
          default:;
        }
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



inline void
RegExpStatics::clone(const RegExpStatics &other)
{
    JS_ASSERT(this != &other);
    clear();
    input = other.input;
    flags = other.flags;
    JS_ASSERT((flags & allFlags) == flags);
    matchPairs.append(other.matchPairs);
}

inline bool
RegExpStatics::createDependent(size_t start, size_t end, Value *out) const 
{
    JS_ASSERT(start <= end);
    JS_ASSERT(end <= input->length());
    JSString *str = js_NewDependentString(cx, input, start, end - start);
    if (!str)
        return false;
    *out = StringValue(str);
    return true;
}

inline bool
RegExpStatics::createInput(Value *out) const
{
    *out = input ? StringValue(input) : Valueify(JS_GetEmptyStringValue(cx));
    return true;
}

inline bool
RegExpStatics::makeMatch(size_t checkValidIndex, size_t pairNum, Value *out) const
{
    if (checkValidIndex / 2 >= pairCount() || matchPairs[checkValidIndex] < 0) {
        *out = Valueify(JS_GetEmptyStringValue(cx));
        return true;
    }
    return createDependent(get(pairNum, 0), get(pairNum, 1), out);
}

inline bool
RegExpStatics::createLastParen(Value *out) const
{
    if (pairCount() <= 1) {
        *out = Valueify(JS_GetEmptyStringValue(cx));
        return true;
    }
    size_t num = pairCount() - 1;
    int start = get(num, 0);
    int end = get(num, 1);
    if (start == -1) {
        JS_ASSERT(end == -1);
        *out = Valueify(JS_GetEmptyStringValue(cx));
        return true;
    }
    JS_ASSERT(start >= 0 && end >= 0);
    return createDependent(start, end, out);
}

inline bool
RegExpStatics::createLeftContext(Value *out) const
{
    if (!pairCount()) {
        *out = Valueify(JS_GetEmptyStringValue(cx));
        return true;
    }
    if (matchPairs[0] < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(0, matchPairs[0], out);
}

inline bool
RegExpStatics::createRightContext(Value *out) const
{
    if (!pairCount()) {
        *out = Valueify(JS_GetEmptyStringValue(cx));
        return true;
    }
    if (matchPairs[1] < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(matchPairs[1], input->length(), out);
}

inline void
RegExpStatics::getParen(size_t num, JSSubString *out) const
{
    out->chars = input->chars() + get(num + 1, 0);
    out->length = getParenLength(num);
}

inline void
RegExpStatics::getLastMatch(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    JS_ASSERT(input);
    out->chars = input->chars() + get(0, 0);
    JS_ASSERT(get(0, 1) >= get(0, 0));
    out->length = get(0, 1) - get(0, 0);
}

inline void
RegExpStatics::getLastParen(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    size_t num = pairCount() - 1;
    out->chars = input->chars() + get(num, 0);
    JS_ASSERT(get(num, 1) >= get(num, 0));
    out->length = get(num, 1) - get(num, 0);
}

inline void
RegExpStatics::getLeftContext(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = input->chars();
    out->length = get(0, 0);
}

inline void
RegExpStatics::getRightContext(JSSubString *out) const
{
    if (!pairCount()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = input->chars() + get(0, 1);
    JS_ASSERT(get(0, 1) <= int(input->length()));
    out->length = input->length() - get(0, 1);
}

}

#endif 
