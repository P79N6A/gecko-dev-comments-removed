






































#ifndef jsregexp_h___
#define jsregexp_h___



#include <stddef.h>
#include "jsprvtd.h"
#include "jsstr.h"
#include "jscntxt.h"
#include "jsvector.h"

#ifdef JS_THREADSAFE
#include "jsdhash.h"
#endif

extern js::Class js_RegExpClass;

namespace js {

class RegExpStatics
{
    typedef Vector<int, 20, SystemAllocPolicy> MatchPairs;
    MatchPairs      matchPairs;
    
    JSString        *matchPairsInput;
    
    JSString        *pendingInput;
    uintN           flags;
    RegExpStatics   *bufferLink;
    bool            copied;

    bool createDependent(JSContext *cx, size_t start, size_t end, Value *out) const;

    size_t pairCount() const {
        JS_ASSERT(matchPairs.length() % 2 == 0);
        return matchPairs.length() / 2;
    }

    size_t pairCountCrash() const {
        JS_CRASH_UNLESS(matchPairs.length() % 2 == 0);
        return pairCount();
    }

    void copyTo(RegExpStatics &dst) {
        dst.matchPairs.clear();
        
        JS_ALWAYS_TRUE(dst.matchPairs.append(matchPairs));
        dst.matchPairsInput = matchPairsInput;
        dst.pendingInput = pendingInput;
        dst.flags = flags;
    }

    void aboutToWrite() {
        if (bufferLink && !bufferLink->copied) {
            copyTo(*bufferLink);
            bufferLink->copied = true;
        }
    }

    bool save(JSContext *cx, RegExpStatics *buffer) {
        JS_ASSERT(!buffer->copied && !buffer->bufferLink);
        buffer->bufferLink = bufferLink;
        bufferLink = buffer;
        if (!buffer->matchPairs.reserve(matchPairs.length())) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        return true;
    }

    void restore() {
        if (bufferLink->copied)
            bufferLink->copyTo(*this);
        bufferLink = bufferLink->bufferLink;
    }

    void checkInvariants() {
#if DEBUG
        if (pairCount() == 0) {
            JS_ASSERT(!matchPairsInput);
            return;
        }

        
        JS_ASSERT(matchPairsInput);
        size_t mpiLen = matchPairsInput->length();

        JS_ASSERT(pairIsPresent(0));

        
        for (size_t i = 0; i < pairCount(); ++i) {
            if (!pairIsPresent(i))
                continue;
            int start = get(i, 0);
            int limit = get(i, 1);
            JS_ASSERT(mpiLen >= size_t(limit) && limit >= start && start >= 0);
        }
#endif
    }

    int get(size_t pairNum, bool which) const {
        JS_ASSERT(pairNum < pairCount());
        return matchPairs[2 * pairNum + which];
    }

    int getCrash(size_t pairNum, bool which) const {
        JS_CRASH_UNLESS(pairNum < pairCountCrash());
        return get(pairNum, which);
    }

    




    bool makeMatch(JSContext *cx, size_t checkValidIndex, size_t pairNum, Value *out) const;

    static const uintN allFlags = JSREG_FOLD | JSREG_GLOB | JSREG_STICKY | JSREG_MULTILINE;

    struct InitBuffer {};
    explicit RegExpStatics(InitBuffer) : bufferLink(NULL), copied(false) {}

    friend class PreserveRegExpStatics;

  public:
    RegExpStatics() : bufferLink(NULL), copied(false) { clear(); }

    static RegExpStatics *extractFrom(JSObject *global);

    

    



    bool updateFromMatch(JSContext *cx, JSString *input, int *buf, size_t matchItemCount) {
        aboutToWrite();
        pendingInput = input;

        if (!matchPairs.resizeUninitialized(matchItemCount)) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        for (size_t i = 0; i < matchItemCount; ++i)
            matchPairs[i] = buf[i];

        matchPairsInput = input;
        return true;
    }

    void setMultiline(bool enabled) {
        aboutToWrite();
        if (enabled)
            flags = flags | JSREG_MULTILINE;
        else
            flags = flags & ~JSREG_MULTILINE;
    }

    void clear() {
        aboutToWrite();
        flags = 0;
        pendingInput = NULL;
        matchPairsInput = NULL;
        matchPairs.clear();
    }

    bool pairIsPresent(size_t pairNum) { return get(0, 0) != -1; }

    
    void reset(JSString *newInput, bool newMultiline) {
        aboutToWrite();
        clear();
        pendingInput = newInput;
        setMultiline(newMultiline);
        checkInvariants();
    }

    void setPendingInput(JSString *newInput) {
        aboutToWrite();
        pendingInput = newInput;
    }

    

    JSString *getPendingInput() const { return pendingInput; }
    uintN getFlags() const { return flags; }
    bool multiline() const { return flags & JSREG_MULTILINE; }

    size_t matchStart() const {
        int start = get(0, 0);
        JS_ASSERT(start >= 0);
        return size_t(start);
    }

    size_t matchLimit() const {
        int limit = get(0, 1);
        JS_ASSERT(size_t(limit) >= matchStart() && limit >= 0);
        return size_t(limit);
    }

    bool matched() const {
        JS_ASSERT(pairCount() > 0);
        return get(0, 1) - get(0, 0) > 0;
    }

    size_t getParenCount() const {
        JS_ASSERT(pairCount() > 0);
        return pairCount() - 1;
    }

    void mark(JSTracer *trc) const {
        if (pendingInput)
            JS_CALL_STRING_TRACER(trc, pendingInput, "res->pendingInput");
        if (matchPairsInput)
            JS_CALL_STRING_TRACER(trc, matchPairsInput, "res->matchPairsInput");
    }

    size_t getParenLength(size_t parenNum) const {
        if (pairCountCrash() <= parenNum + 1)
            return 0;
        return getCrash(parenNum + 1, 1) - getCrash(parenNum + 1, 0);
    }

    

    bool createPendingInput(JSContext *cx, Value *out) const;
    bool createLastMatch(JSContext *cx, Value *out) const { return makeMatch(cx, 0, 0, out); }
    bool createLastParen(JSContext *cx, Value *out) const;
    bool createLeftContext(JSContext *cx, Value *out) const;
    bool createRightContext(JSContext *cx, Value *out) const;

    bool createParen(JSContext *cx, size_t parenNum, Value *out) const {
        return makeMatch(cx, (parenNum + 1) * 2, parenNum + 1, out);
    }

    

    void getParen(size_t num, JSSubString *out) const;
    void getLastMatch(JSSubString *out) const;
    void getLastParen(JSSubString *out) const;
    void getLeftContext(JSSubString *out) const;
    void getRightContext(JSSubString *out) const;
};

class PreserveRegExpStatics
{
    RegExpStatics *const original;
    RegExpStatics buffer;

  public:
    explicit PreserveRegExpStatics(RegExpStatics *original)
     : original(original),
       buffer(RegExpStatics::InitBuffer())
    {}

    bool init(JSContext *cx) {
        return original->save(cx, &buffer);
    }

    ~PreserveRegExpStatics() {
        original->restore();
    }
};

}

static inline bool
VALUE_IS_REGEXP(JSContext *cx, js::Value v)
{
    return !v.isPrimitive() && v.toObject().isRegExp();
}

inline const js::Value &
JSObject::getRegExpLastIndex() const
{
    JS_ASSERT(isRegExp());
    return getSlot(JSSLOT_REGEXP_LAST_INDEX);
}

inline void
JSObject::setRegExpLastIndex(const js::Value &v)
{
    JS_ASSERT(isRegExp());
    setSlot(JSSLOT_REGEXP_LAST_INDEX, v);
}

inline void
JSObject::setRegExpLastIndex(jsdouble d)
{
    JS_ASSERT(isRegExp());
    setSlot(JSSLOT_REGEXP_LAST_INDEX, js::NumberValue(d));
}

inline void
JSObject::zeroRegExpLastIndex()
{
    JS_ASSERT(isRegExp());
    getSlotRef(JSSLOT_REGEXP_LAST_INDEX).setInt32(0);
}

namespace js { class AutoStringRooter; }

inline bool
JSObject::isRegExp() const
{
    return getClass() == &js_RegExpClass;
}

extern JS_FRIEND_API(JSBool)
js_ObjectIsRegExp(JSObject *obj);

extern JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj);




extern JSBool
js_regexp_toString(JSContext *cx, JSObject *obj, js::Value *vp);

extern JS_FRIEND_API(JSObject *) JS_FASTCALL
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *proto);





extern JS_FRIEND_API(void)
js_SaveAndClearRegExpStatics(JSContext *cx, js::RegExpStatics *res, js::AutoStringRooter *tvr);


extern JS_FRIEND_API(void)
js_RestoreRegExpStatics(JSContext *cx, js::RegExpStatics *res);

extern JSBool
js_XDRRegExpObject(JSXDRState *xdr, JSObject **objp);

extern JSBool
js_regexp_exec(JSContext *cx, uintN argc, js::Value *vp);
extern JSBool
js_regexp_test(JSContext *cx, uintN argc, js::Value *vp);

#endif 
