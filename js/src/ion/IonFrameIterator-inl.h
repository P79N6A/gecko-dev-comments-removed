







































#ifndef jsion_frame_iterator_inl_h__
#define jsion_frame_iterator_inl_h__

#include "ion/IonFrameIterator.h"

namespace js {
namespace ion {

template <class Op>
inline bool
InlineFrameIterator::forEachCanonicalActualArg(Op op, unsigned start, unsigned count)
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

    s.skip(); 
    s.skip(); 

    unsigned i = 0;
    for (; i < start; i++)
        s.skip();
    for (; i < formalEnd; i++) {
        
        
        
        Value v = s.maybeRead();
        if (!op(i, &v))
            return false;
    }
    if (formalEnd != end) {
        Value *argv = frame_->argv() + 1;
        for (; i < end; i++) {
            if (!op(i, &argv[i]))
                return false;
        }
    }
    return true;
}

} 
} 

#endif 
