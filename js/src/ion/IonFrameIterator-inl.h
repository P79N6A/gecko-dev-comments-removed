






#ifndef jsion_frame_iterator_inl_h__
#define jsion_frame_iterator_inl_h__

#include "ion/IonFrameIterator.h"
#include "ion/Bailouts.h"
#include "ion/Ion.h"

namespace js {
namespace ion {

template <class Op>
inline void
SnapshotIterator::readFrameArgs(Op &op, const Value *argv, Value *scopeChain, Value *thisv,
                                unsigned start, unsigned formalEnd, unsigned iterEnd)
{
    if (scopeChain)
        *scopeChain = read();
    else
        skip();

    if (thisv)
        *thisv = read();
    else
        skip();

    unsigned i = 0;
    if (formalEnd < start)
        i = start;

    for (; i < start; i++)
        skip();
    for (; i < formalEnd && i < iterEnd; i++) {
        
        
        
        Value v = maybeRead();
        op(v);
    }
    if (iterEnd >= formalEnd) {
        for (; i < iterEnd; i++)
            op(argv[i]);
    }
}

template <AllowGC allowGC>
inline
InlineFrameIteratorMaybeGC<allowGC>::InlineFrameIteratorMaybeGC(
                                JSContext *cx, const IonFrameIterator *iter)
  : callee_(cx),
    script_(cx)
{
    resetOn(iter);
}

template <AllowGC allowGC>
inline
InlineFrameIteratorMaybeGC<allowGC>::InlineFrameIteratorMaybeGC(
        JSContext *cx,
        const InlineFrameIteratorMaybeGC<allowGC> *iter)
  : frame_(iter ? iter->frame_ : NULL),
    framesRead_(0),
    callee_(cx),
    script_(cx)
{
    if (frame_) {
        start_ = SnapshotIterator(*frame_);
        
        
        framesRead_ = iter->framesRead_ - 1;
        findNextFrame();
    }
}

template <AllowGC allowGC>
template <class Op>
inline void
InlineFrameIteratorMaybeGC<allowGC>::forEachCanonicalActualArg(
                JSContext *cx, Op op, unsigned start, unsigned count) const
{
    unsigned nactual = numActualArgs();
    if (count == unsigned(-1))
        count = nactual - start;

    unsigned end = start + count;
    unsigned nformal = callee()->nargs;

    JS_ASSERT(start <= end && end <= nactual);

    if (more()) {
        
        
        
        
        

        
        unsigned formal_end = (end < nformal) ? end : nformal;
        SnapshotIterator s(si_);
        s.readFrameArgs(op, NULL, NULL, NULL, start, nformal, formal_end);

        
        
        InlineFrameIteratorMaybeGC it(cx, this);
        SnapshotIterator parent_s((++it).snapshotIterator());

        
        
        JS_ASSERT(parent_s.slots() >= nactual + 2);
        unsigned skip = parent_s.slots() - nactual - 2;
        for (unsigned j = 0; j < skip; j++)
            parent_s.skip();

        
        parent_s.readFrameArgs(op, NULL, NULL, NULL, nformal, nactual, end);
    } else {
        SnapshotIterator s(si_);
        Value *argv = frame_->actualArgs();
        s.readFrameArgs(op, argv, NULL, NULL, start, nformal, end);
    }
}
 
template <AllowGC allowGC>
inline JSObject *
InlineFrameIteratorMaybeGC<allowGC>::scopeChain() const
{
    SnapshotIterator s(si_);

    
    Value v = s.read();
    if (v.isObject()) {
        JS_ASSERT_IF(script()->hasAnalysis(), script()->analysis()->usesScopeChain());
        return &v.toObject();
    }

    return callee()->environment();
}

template <AllowGC allowGC>
inline JSObject *
InlineFrameIteratorMaybeGC<allowGC>::thisObject() const
{
    
    SnapshotIterator s(si_);

    
    s.skip();

    
    
    Value v = s.read();
    JS_ASSERT(v.isObject());
    return &v.toObject();
}

template <AllowGC allowGC>
inline unsigned
InlineFrameIteratorMaybeGC<allowGC>::numActualArgs() const
{
    
    
    
    
    
    if (more())
        return numActualArgs_;

    return frame_->numActualArgs();
}

template <AllowGC allowGC>
inline
InlineFrameIteratorMaybeGC<allowGC>::InlineFrameIteratorMaybeGC(
                                                JSContext *cx, const IonBailoutIterator *iter)
  : frame_(iter),
    framesRead_(0),
    callee_(cx),
    script_(cx)
{
    if (iter) {
        start_ = SnapshotIterator(*iter);
        findNextFrame();
    }
}

template <AllowGC allowGC>
inline InlineFrameIteratorMaybeGC<allowGC> &
InlineFrameIteratorMaybeGC<allowGC>::operator++()
{
    findNextFrame();
    return *this;
}

} 
} 

#endif 
