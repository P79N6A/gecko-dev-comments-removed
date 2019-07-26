






#ifndef jsion_types_h_
#define jsion_types_h_

#include <jstypes.h>

namespace js {
namespace ion {

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef int8_t int8;

typedef uint32 SnapshotOffset;
typedef uint32 BailoutId;

static const SnapshotOffset INVALID_SNAPSHOT_OFFSET = uint32(-1);



enum BailoutKind
{
    
    
    Bailout_Normal,

    
    
    Bailout_ArgumentCheck,

    
    
    Bailout_TypeBarrier,

    
    Bailout_Monitor,

    
    Bailout_RecompileCheck,

    
    Bailout_BoundsCheck,

    
    Bailout_Invalidate
};

#ifdef DEBUG

#define TRACK_SNAPSHOTS 1
#endif

} 
} 

#endif 

