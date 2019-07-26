






#ifndef RegExpStatics_h__
#define RegExpStatics_h__

#include "mozilla/GuardObjects.h"

#include "jscntxt.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "js/Vector.h"

#include "vm/MatchPairs.h"

namespace js {

class RegExpStatics
{
    
    VectorMatchPairs        matches;
    HeapPtr<JSLinearString> matchesInput;

    
    HeapPtr<RegExpObject>   regexp;
    size_t                  lastIndex;

    
    HeapPtr<JSString>       pendingInput;
    RegExpFlag              flags;

    



    bool                    pendingLazyEvaluation;

    
    RegExpStatics           *bufferLink;
    bool                    copied;

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
    inline RegExpStatics();

    static JSObject *create(JSContext *cx, GlobalObject *parent);

    

    inline void updateLazily(JSContext *cx, JSLinearString *input,
                             RegExpObject *regexp, size_t lastIndex);
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
        if (regexp)
            gc::MarkObject(trc, &regexp, "res->regexp");
        if (pendingInput)
            MarkString(trc, &pendingInput, "res->pendingInput");
        if (matchesInput)
            MarkString(trc, &matchesInput, "res->matchesInput");
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

size_t SizeOfRegExpStaticsData(const JSObject *obj, JSMallocSizeOfFun mallocSizeOf);

} 

#endif
