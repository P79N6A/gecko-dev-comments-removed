






#ifndef jsion_frame_iterator_inl_h__
#define jsion_frame_iterator_inl_h__

#include "ion/IonFrameIterator.h"

namespace js {
namespace ion {

template <class Op>
inline void
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

template <class Op>
inline void
InlineFrameIterator::forEachCanonicalActualArg(JSContext *cx, Op op, unsigned start, unsigned count) const
{
    unsigned nactual = numActualArgs();
    if (count == unsigned(-1))
        count = nactual - start;

    unsigned end = start + count;
    unsigned nformal = callee()->nargs;

    JS_ASSERT(start <= end && end <= nactual);

    if (more()) {
        
        
        
        

        InlineFrameIterator it(cx, this);
        ++it;
        SnapshotIterator s(it.snapshotIterator());

        
        
        JS_ASSERT(s.slots() >= nactual + 2);
        unsigned skip = s.slots() - nactual - 2;
        for (unsigned j = 0; j < skip; j++)
            s.skip();

        s.readFrameArgs(op, NULL, NULL, NULL, start, nactual, end);
    } else {
        SnapshotIterator s(si_);
        Value *argv = frame_->actualArgs();
        s.readFrameArgs(op, argv, NULL, NULL, start, nformal, end);
    }

}

} 
} 

#endif 
