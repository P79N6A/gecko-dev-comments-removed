





































#ifndef jsgcstats_h___
#define jsgcstats_h___

#include "mozilla/Util.h"

#if !defined JS_DUMP_CONSERVATIVE_GC_ROOTS && defined DEBUG
# define JS_DUMP_CONSERVATIVE_GC_ROOTS 1
#endif

namespace js {
namespace gc {




enum ConservativeGCTest
{
    CGCT_VALID,
    CGCT_LOWBITSET, 
    CGCT_NOTARENA,  
    CGCT_OTHERCOMPARTMENT,  
    CGCT_NOTCHUNK,  
    CGCT_FREEARENA, 
    CGCT_NOTLIVE,   
    CGCT_END
};

struct ConservativeGCStats
{
    uint32  counter[gc::CGCT_END];  

    uint32  unaligned;              
 

    void add(const ConservativeGCStats &another) {
        for (size_t i = 0; i < mozilla::ArrayLength(counter); ++i)
            counter[i] += another.counter[i];
    }

    void dump(FILE *fp);
};

} 

} 

#endif 
