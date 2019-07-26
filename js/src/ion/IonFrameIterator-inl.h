







































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

    SnapshotIterator s(si_);
    
    s.skip(); 
    s.skip(); 

    unsigned i = 0;
    for (; i < start; i++)
        s.skip();
    for (; i < end; i++) {
        
        
        
        Value v = s.maybeRead();
        if (!op(i, &v))
            return false;
    }
    return true;
}

} 
} 

#endif 
