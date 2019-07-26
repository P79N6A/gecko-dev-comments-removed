






#ifndef jsion_frame_iterator_inl_h__
#define jsion_frame_iterator_inl_h__

#include "ion/IonFrameIterator.h"

namespace js {
namespace ion {

template <class Op>
inline bool
SnapshotIterator::readFrameArgs(Op op, const Value *argv, Value *scopeChain, Value *thisv,
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
    for (; i < start; i++)
        skip();
    for (; i < formalEnd; i++) {
        
        
        
        Value v = maybeRead();
        op(v);
    }
    if (iterEnd >= formalEnd) {
        for (; i < iterEnd; i++)
            op(argv[i]);
    }
    return true;
}

template <class Op>
inline bool
InlineFrameIterator::forEachCanonicalActualArg(Op op, unsigned start, unsigned count) const
{
    unsigned nactual = numActualArgs();
    if (count == unsigned(-1))
        count = nactual - start;

    unsigned end = start + count;
    JS_ASSERT(start <= end && end <= nactual);

    unsigned nformal = callee()->nargs;
    unsigned formalEnd = end;
    if (!more() && end > nformal) {
        formalEnd = nformal;
    } else {
        
        
        
        
        JS_ASSERT(end <= nformal);
    }

    SnapshotIterator s(si_);
    Value *argv = frame_->actualArgs();
    return s.readFrameArgs(op, argv, NULL, NULL, start, formalEnd, end);
}

} 
} 

#endif 
