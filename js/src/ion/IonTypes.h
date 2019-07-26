






#ifndef jsion_types_h_
#define jsion_types_h_

#include <jstypes.h>

namespace js {
namespace ion {

typedef uint32_t SnapshotOffset;
typedef uint32_t BailoutId;

static const SnapshotOffset INVALID_SNAPSHOT_OFFSET = uint32_t(-1);



enum BailoutKind
{
    
    
    Bailout_Normal,

    
    
    Bailout_ArgumentCheck,

    
    
    Bailout_TypeBarrier,

    
    Bailout_Monitor,

    
    Bailout_RecompileCheck,

    
    Bailout_BoundsCheck,

    
    Bailout_ShapeGuard,

    
    Bailout_CachedShapeGuard
};

inline const char *
BailoutKindString(BailoutKind kind) {
    switch(kind) {
      case Bailout_Normal:
        return "Bailout_Normal";
      case Bailout_ArgumentCheck:
        return "Bailout_ArgumentCheck";
      case Bailout_TypeBarrier:
        return "Bailout_TypeBarrier";
      case Bailout_Monitor:
        return "Bailout_Monitor";
      case Bailout_RecompileCheck:
        return "Bailout_RecompileCheck";
      case Bailout_BoundsCheck:
        return "Bailout_BoundsCheck";
      case Bailout_ShapeGuard:
        return "Bailout_ShapeGuard";
      case Bailout_CachedShapeGuard:
        return "Bailout_CachedShapeGuard";
      default:
        JS_NOT_REACHED("Invalid BailoutKind");
    }
    return "INVALID_BAILOUT_KIND";
}




enum MIRType
{
    MIRType_Undefined,
    MIRType_Null,
    MIRType_Boolean,
    MIRType_Int32,
    MIRType_Double,
    MIRType_String,
    MIRType_Object,
    MIRType_Magic,
    MIRType_Value,
    MIRType_None,         
    MIRType_Slots,        
    MIRType_Elements,     
    MIRType_StackFrame,   
    MIRType_Shape,        
    MIRType_ForkJoinSlice 
};

#ifdef DEBUG

#define TRACK_SNAPSHOTS 1
#endif

} 
} 

#endif 

