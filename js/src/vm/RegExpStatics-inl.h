






#ifndef RegExpStatics_inl_h__
#define RegExpStatics_inl_h__

#include "RegExpStatics.h"

#include "vm/RegExpObject-inl.h"
#include "vm/String-inl.h"

namespace js {

class RegExpStatics
{
    
    VectorMatchPairs        matches;
    HeapPtr<JSLinearString> matchesInput;

    
    RegExpHeapGuard         regexp;
    size_t                  lastIndex;

    
    HeapPtr<JSString>       pendingInput;
    RegExpFlag              flags;

    



    bool                    pendingLazyEvaluation;

    
    RegExpStatics           *bufferLink;
    bool                    copied;

  public:
    RegExpStatics() : bufferLink(NULL), copied(false) { clear(); }
    static JSObject *create(JSContext *cx, GlobalObject *parent);

  private:
    bool executeLazy(JSContext *cx);

    inline void aboutToWrite();
    inline void copyTo(RegExpStatics &dst);

    inline void restore();
    bool save(JSContext *cx, RegExpStatics *buffer) {
        JS_ASSERT(!buffer->copied && !buffer->bufferLink);
        buffer->bufferLink = bufferLink;
        bufferLink = buffer;
        if (!buffer->matches.allocOrExpandArray(matches.length())) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        return true;
    }

    inline void checkInvariants();

    




    bool makeMatch(JSContext *cx, size_t checkValidIndex, size_t pairNum, Value *out);
    bool createDependent(JSContext *cx, size_t start, size_t end, Value *out);

    void markFlagsSet(JSContext *cx);

    struct InitBuffer {};
    explicit RegExpStatics(InitBuffer) : bufferLink(NULL), copied(false) {}

    friend class PreserveRegExpStatics;

  public:
    
    inline void updateLazily(JSContext *cx, JSLinearString *input,
                             RegExpShared *shared, size_t lastIndex);
    inline bool updateFromMatchPairs(JSContext *cx, JSLinearString *input, MatchPairs &newPairs);
    inline void setMultiline(JSContext *cx, bool enabled);

    inline void clear();

    
    inline void reset(JSContext *cx, JSString *newInput, bool newMultiline);

    inline void setPendingInput(JSString *newInput);

  public:
    
    const MatchPairs &getMatches() const {
        
        JS_ASSERT(!pendingLazyEvaluation);
        return matches;
    }

    JSString *getPendingInput() const { return pendingInput; }

    RegExpFlag getFlags() const { return flags; }
    bool multiline() const { return flags & MultilineFlag; }

    
    bool matched() const {
        
        JS_ASSERT(!pendingLazyEvaluation);
        JS_ASSERT(matches.pairCount() > 0);
        return matches[0].limit - matches[0].start > 0;
    }

    void mark(JSTracer *trc) {
        



        if (pendingInput)
            MarkString(trc, &pendingInput, "res->pendingInput");
        if (matchesInput)
            MarkString(trc, &matchesInput, "res->matchesInput");
        if (regexp.initialized())
            regexp->trace(trc);
    }

    

    bool createPendingInput(JSContext *cx, Value *out);
    bool createLastMatch(JSContext *cx, Value *out);
    bool createLastParen(JSContext *cx, Value *out);
    bool createParen(JSContext *cx, size_t pairNum, Value *out);
    bool createLeftContext(JSContext *cx, Value *out);
    bool createRightContext(JSContext *cx, Value *out);

    

    void getParen(size_t pairNum, JSSubString *out) const;
    void getLastMatch(JSSubString *out) const;
    void getLastParen(JSSubString *out) const;
    void getLeftContext(JSSubString *out) const;
    void getRightContext(JSSubString *out) const;

    

    class AutoRooter : private AutoGCRooter
    {
      public:
        explicit AutoRooter(JSContext *cx, RegExpStatics *statics_
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
          : AutoGCRooter(cx, REGEXPSTATICS), statics(statics_), skip(cx, statics_)
        {
            MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        }

        friend void AutoGCRooter::trace(JSTracer *trc);
        void trace(JSTracer *trc);

      private:
        RegExpStatics *statics;
        SkipRoot skip;
        MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    };
};

class PreserveRegExpStatics
{
    RegExpStatics * const original;
    RegExpStatics buffer;
    RegExpStatics::AutoRooter bufferRoot;

  public:
    explicit PreserveRegExpStatics(JSContext *cx, RegExpStatics *original)
     : original(original),
       buffer(RegExpStatics::InitBuffer()),
       bufferRoot(cx, &buffer)
    {}

    bool init(JSContext *cx) {
        return original->save(cx, &buffer);
    }

    inline ~PreserveRegExpStatics();
};

inline js::RegExpStatics *
js::GlobalObject::getRegExpStatics() const
{
    JSObject &resObj = getSlot(REGEXP_STATICS).toObject();
    return static_cast<RegExpStatics *>(resObj.getPrivate());
}

inline size_t
SizeOfRegExpStaticsData(const JSObject *obj, JSMallocSizeOfFun mallocSizeOf)
{
    return mallocSizeOf(obj->getPrivate());
}

inline bool
RegExpStatics::createDependent(JSContext *cx, size_t start, size_t end, Value *out)
{
    
    JS_ASSERT(!pendingLazyEvaluation);

    JS_ASSERT(start <= end);
    JS_ASSERT(end <= matchesInput->length());
    JSString *str = js_NewDependentString(cx, matchesInput, start, end - start);
    if (!str)
        return false;
    *out = StringValue(str);
    return true;
}

inline bool
RegExpStatics::createPendingInput(JSContext *cx, Value *out)
{
    
    out->setString(pendingInput ? pendingInput.get() : cx->runtime->emptyString);
    return true;
}

inline bool
RegExpStatics::makeMatch(JSContext *cx, size_t checkValidIndex, size_t pairNum, Value *out)
{
    
    JS_ASSERT(!pendingLazyEvaluation);

    bool checkWhich  = checkValidIndex % 2;
    size_t checkPair = checkValidIndex / 2;

    if (matches.empty() || checkPair >= matches.pairCount() ||
        (checkWhich ? matches[checkPair].limit : matches[checkPair].start) < 0)
    {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    const MatchPair &pair = matches[pairNum];
    return createDependent(cx, pair.start, pair.limit, out);
}

inline bool
RegExpStatics::createLastMatch(JSContext *cx, Value *out)
{
    if (!executeLazy(cx))
        return false;
    return makeMatch(cx, 0, 0, out);
}

inline bool
RegExpStatics::createLastParen(JSContext *cx, Value *out)
{
    if (!executeLazy(cx))
        return false;

    if (matches.empty() || matches.pairCount() == 1) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    const MatchPair &pair = matches[matches.pairCount() - 1];
    if (pair.start == -1) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    JS_ASSERT(pair.start >= 0 && pair.limit >= 0);
    JS_ASSERT(pair.limit >= pair.start);
    return createDependent(cx, pair.start, pair.limit, out);
}

inline bool
RegExpStatics::createParen(JSContext *cx, size_t pairNum, Value *out)
{
    JS_ASSERT(pairNum >= 1);
    if (!executeLazy(cx))
        return false;

    if (matches.empty() || pairNum >= matches.pairCount()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    return makeMatch(cx, pairNum * 2, pairNum, out);
}

inline bool
RegExpStatics::createLeftContext(JSContext *cx, Value *out)
{
    if (!executeLazy(cx))
        return false;

    if (matches.empty()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    if (matches[0].start < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(cx, 0, matches[0].start, out);
}

inline bool
RegExpStatics::createRightContext(JSContext *cx, Value *out)
{
    if (!executeLazy(cx))
        return false;

    if (matches.empty()) {
        out->setString(cx->runtime->emptyString);
        return true;
    }
    if (matches[0].limit < 0) {
        *out = UndefinedValue();
        return true;
    }
    return createDependent(cx, matches[0].limit, matchesInput->length(), out);
}

inline void
RegExpStatics::getParen(size_t pairNum, JSSubString *out) const
{
    JS_ASSERT(!pendingLazyEvaluation);

    JS_ASSERT(pairNum >= 1 && pairNum < matches.pairCount());
    const MatchPair &pair = matches[pairNum];
    if (pair.isUndefined()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars  = matchesInput->chars() + pair.start;
    out->length = pair.length();
}

inline void
RegExpStatics::getLastMatch(JSSubString *out) const
{
    JS_ASSERT(!pendingLazyEvaluation);

    if (matches.empty()) {
        *out = js_EmptySubString;
        return;
    }
    JS_ASSERT(matchesInput);
    out->chars = matchesInput->chars() + matches[0].start;
    JS_ASSERT(matches[0].limit >= matches[0].start);
    out->length = matches[0].length();
}

inline void
RegExpStatics::getLastParen(JSSubString *out) const
{
    JS_ASSERT(!pendingLazyEvaluation);

    
    if (matches.empty() || matches.pairCount() == 1) {
        *out = js_EmptySubString;
        return;
    }
    getParen(matches.parenCount(), out);
}

inline void
RegExpStatics::getLeftContext(JSSubString *out) const
{
    JS_ASSERT(!pendingLazyEvaluation);

    if (matches.empty()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchesInput->chars();
    out->length = matches[0].start;
}

inline void
RegExpStatics::getRightContext(JSSubString *out) const
{
    JS_ASSERT(!pendingLazyEvaluation);

    if (matches.empty()) {
        *out = js_EmptySubString;
        return;
    }
    out->chars = matchesInput->chars() + matches[0].limit;
    JS_ASSERT(matches[0].limit <= int(matchesInput->length()));
    out->length = matchesInput->length() - matches[0].limit;
}

inline void
RegExpStatics::copyTo(RegExpStatics &dst)
{
    
    if (!pendingLazyEvaluation)
        dst.matches.initArrayFrom(matches);

    if (regexp.initialized())
        dst.regexp.init(*regexp);
    else
        dst.regexp.release();

    dst.matchesInput = matchesInput;
    dst.lastIndex = lastIndex;
    dst.pendingInput = pendingInput;
    dst.flags = flags;
    dst.pendingLazyEvaluation = pendingLazyEvaluation;

    JS_ASSERT_IF(pendingLazyEvaluation, regexp.initialized());
    JS_ASSERT_IF(pendingLazyEvaluation, matchesInput);
    JS_ASSERT(regexp.initialized() == dst.regexp.initialized());
}

inline void
RegExpStatics::aboutToWrite()
{
    if (bufferLink && !bufferLink->copied) {
        copyTo(*bufferLink);
        bufferLink->copied = true;
    }
}

inline void
RegExpStatics::restore()
{
    if (bufferLink->copied)
        bufferLink->copyTo(*this);
    bufferLink = bufferLink->bufferLink;
}

inline void
RegExpStatics::updateLazily(JSContext *cx, JSLinearString *input,
                            RegExpShared *shared, size_t lastIndex)
{
    JS_ASSERT(input && shared);
    aboutToWrite();

    BarrieredSetPair<JSString, JSLinearString>(cx->compartment,
                                               pendingInput, input,
                                               matchesInput, input);
    if (regexp.initialized())
        regexp.release();
    regexp.init(*shared);

    this->lastIndex = lastIndex;
    pendingLazyEvaluation = true;
}

inline bool
RegExpStatics::updateFromMatchPairs(JSContext *cx, JSLinearString *input, MatchPairs &newPairs)
{
    JS_ASSERT(input);
    aboutToWrite();

    
    pendingLazyEvaluation = false;
    this->regexp.release();
    this->lastIndex = size_t(-1);

    BarrieredSetPair<JSString, JSLinearString>(cx->compartment,
                                               pendingInput, input,
                                               matchesInput, input);

    if (!matches.initArrayFrom(newPairs)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

inline void
RegExpStatics::clear()
{
    aboutToWrite();

    matches.forgetArray();
    matchesInput = NULL;
    regexp.release();
    lastIndex = size_t(-1);
    pendingInput = NULL;
    flags = RegExpFlag(0);
    pendingLazyEvaluation = false;
}

inline void
RegExpStatics::setPendingInput(JSString *newInput)
{
    aboutToWrite();
    pendingInput = newInput;
}

PreserveRegExpStatics::~PreserveRegExpStatics()
{
    original->restore();
}

inline void
RegExpStatics::setMultiline(JSContext *cx, bool enabled)
{
    aboutToWrite();
    if (enabled) {
        flags = RegExpFlag(flags | MultilineFlag);
        markFlagsSet(cx);
    } else {
        flags = RegExpFlag(flags & ~MultilineFlag);
    }
}

inline void
RegExpStatics::markFlagsSet(JSContext *cx)
{
    







    JS_ASSERT(this == cx->global()->getRegExpStatics());

    types::MarkTypeObjectFlags(cx, cx->global(), types::OBJECT_FLAG_REGEXP_FLAGS_SET);
}

inline void
RegExpStatics::reset(JSContext *cx, JSString *newInput, bool newMultiline)
{
    aboutToWrite();
    clear();
    pendingInput = newInput;
    setMultiline(cx, newMultiline);
    checkInvariants();
}

inline void
RegExpStatics::checkInvariants()
{
#ifdef DEBUG
    if (pendingLazyEvaluation) {
        JS_ASSERT(regexp.initialized());
        JS_ASSERT(pendingInput);
        JS_ASSERT(lastIndex != size_t(-1));
        return;
    }

    if (matches.empty()) {
        JS_ASSERT(!matchesInput);
        return;
    }

    
    JS_ASSERT(matchesInput);
    size_t mpiLen = matchesInput->length();

    
    JS_ASSERT(!matches[0].isUndefined());
    JS_ASSERT(matches[0].limit >= 0);

    
    for (size_t i = 0; i < matches.pairCount(); i++) {
        if (matches[i].isUndefined())
            continue;
        const MatchPair &pair = matches[i];
        JS_ASSERT(mpiLen >= size_t(pair.limit) && pair.limit >= pair.start && pair.start >= 0);
    }
#endif 
}

} 

inline js::RegExpStatics *
JSContext::regExpStatics()
{
    return global()->getRegExpStatics();
}

#endif
