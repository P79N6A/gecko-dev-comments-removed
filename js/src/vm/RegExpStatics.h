







































#ifndef RegExpStatics_h__
#define RegExpStatics_h__

#include "jscntxt.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/Vector.h"

#include "vm/MatchPairs.h"

namespace js {

class RegExpStatics
{
    typedef Vector<int, 20, SystemAllocPolicy> Pairs;
    Pairs                   matchPairs;
    
    HeapPtr<JSLinearString> matchPairsInput;
    
    HeapPtr<JSString>       pendingInput;
    RegExpFlag              flags;
    RegExpStatics           *bufferLink;
    bool                    copied;

    bool createDependent(JSContext *cx, size_t start, size_t end, Value *out) const;

    inline void copyTo(RegExpStatics &dst);

    inline void aboutToWrite();

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

    inline void restore();

    void checkInvariants() {
#if DEBUG
        if (pairCount() == 0) {
            JS_ASSERT(!matchPairsInput);
            return;
        }

        
        JS_ASSERT(matchPairsInput);
        size_t mpiLen = matchPairsInput->length();

        
        JS_ASSERT(pairIsPresent(0));
        JS_ASSERT(get(0, 1) >= 0);

        
        for (size_t i = 0; i < pairCount(); ++i) {
            if (!pairIsPresent(i))
                continue;
            int start = get(i, 0);
            int limit = get(i, 1);
            JS_ASSERT(mpiLen >= size_t(limit) && limit >= start && start >= 0);
        }
#endif
    }

    



    void checkParenNum(size_t pairNum) const {
        JS_ASSERT(1 <= pairNum);
        JS_ASSERT(pairNum < pairCount());
    }

    
    size_t getParenLength(size_t pairNum) const {
        checkParenNum(pairNum);
        JS_ASSERT(pairIsPresent(pairNum));
        return get(pairNum, 1) - get(pairNum, 0);
    }

    int get(size_t pairNum, bool which) const {
        JS_ASSERT(pairNum < pairCount());
        return matchPairs[2 * pairNum + which];
    }

    




    bool makeMatch(JSContext *cx, size_t checkValidIndex, size_t pairNum, Value *out) const;

    void markFlagsSet(JSContext *cx);

    struct InitBuffer {};
    explicit RegExpStatics(InitBuffer) : bufferLink(NULL), copied(false) {}

    friend class PreserveRegExpStatics;

  public:
    inline RegExpStatics();

    static JSObject *create(JSContext *cx, GlobalObject *parent);

    

    inline bool updateFromMatchPairs(JSContext *cx, JSLinearString *input, MatchPairs *newPairs);
    inline void setMultiline(JSContext *cx, bool enabled);

    inline void clear();

    
    inline void reset(JSContext *cx, JSString *newInput, bool newMultiline);

    inline void setPendingInput(JSString *newInput);

    

    






  private:
    size_t pairCount() const {
        JS_ASSERT(matchPairs.length() % 2 == 0);
        return matchPairs.length() / 2;
    }

  public:
    size_t parenCount() const {
        size_t pc = pairCount();
        JS_ASSERT(pc);
        return pc - 1;
    }

    JSString *getPendingInput() const { return pendingInput; }
    RegExpFlag getFlags() const { return flags; }
    bool multiline() const { return flags & MultilineFlag; }

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
        JS_ASSERT_IF(get(0, 1) == -1, get(1, 1) == -1);
        return get(0, 1) - get(0, 0) > 0;
    }

    void mark(JSTracer *trc) {
        if (pendingInput)
            MarkString(trc, &pendingInput, "res->pendingInput");
        if (matchPairsInput)
            MarkString(trc, &matchPairsInput, "res->matchPairsInput");
    }

    bool pairIsPresent(size_t pairNum) const {
        return get(pairNum, 0) >= 0;
    }

    

    bool createPendingInput(JSContext *cx, Value *out) const;
    bool createLastMatch(JSContext *cx, Value *out) const { return makeMatch(cx, 0, 0, out); }
    bool createLastParen(JSContext *cx, Value *out) const;
    bool createLeftContext(JSContext *cx, Value *out) const;
    bool createRightContext(JSContext *cx, Value *out) const;

    
    bool createParen(JSContext *cx, size_t pairNum, Value *out) const {
        JS_ASSERT(pairNum >= 1);
        if (pairNum >= pairCount()) {
            out->setString(cx->runtime->emptyString);
            return true;
        }
        return makeMatch(cx, pairNum * 2, pairNum, out);
    }

    

    void getParen(size_t pairNum, JSSubString *out) const;
    void getLastMatch(JSSubString *out) const;
    void getLastParen(JSSubString *out) const;
    void getLeftContext(JSSubString *out) const;
    void getRightContext(JSSubString *out) const;

    class StackRoot
    {
        Root<JSLinearString*> matchPairsInputRoot;
        RootString pendingInputRoot;

      public:
        StackRoot(JSContext *cx, RegExpStatics *buffer)
          : matchPairsInputRoot(cx, (JSLinearString**) &buffer->matchPairsInput),
            pendingInputRoot(cx, (JSString**) &buffer->pendingInput)
        {}
    };
};

class PreserveRegExpStatics
{
    RegExpStatics * const original;
    RegExpStatics buffer;
    RegExpStatics::StackRoot bufferRoot;

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

size_t SizeOfRegExpStaticsData(const JSObject *obj, JSMallocSizeOfFun mallocSizeOf);

} 

#endif
