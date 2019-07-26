





#ifndef jsion_types_h_
#define jsion_types_h_

#include "js/Value.h"
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

    
    Bailout_BoundsCheck,

    
    Bailout_ShapeGuard,

    
    Bailout_CachedShapeGuard
};

#ifdef DEBUG
inline const char *
BailoutKindString(BailoutKind kind)
{
    switch (kind) {
      case Bailout_Normal:
        return "Bailout_Normal";
      case Bailout_ArgumentCheck:
        return "Bailout_ArgumentCheck";
      case Bailout_TypeBarrier:
        return "Bailout_TypeBarrier";
      case Bailout_Monitor:
        return "Bailout_Monitor";
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
#endif




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
    MIRType_Pointer,      
    MIRType_Shape,        
    MIRType_ForkJoinSlice 
};

static inline MIRType
MIRTypeFromValueType(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_DOUBLE:
        return MIRType_Double;
      case JSVAL_TYPE_INT32:
        return MIRType_Int32;
      case JSVAL_TYPE_UNDEFINED:
        return MIRType_Undefined;
      case JSVAL_TYPE_STRING:
        return MIRType_String;
      case JSVAL_TYPE_BOOLEAN:
        return MIRType_Boolean;
      case JSVAL_TYPE_NULL:
        return MIRType_Null;
      case JSVAL_TYPE_OBJECT:
        return MIRType_Object;
      case JSVAL_TYPE_MAGIC:
        return MIRType_Magic;
      case JSVAL_TYPE_UNKNOWN:
        return MIRType_Value;
      default:
        JS_NOT_REACHED("unexpected jsval type");
        return MIRType_None;
    }
}

static inline JSValueType
ValueTypeFromMIRType(MIRType type)
{
  switch (type) {
    case MIRType_Undefined:
      return JSVAL_TYPE_UNDEFINED;
    case MIRType_Null:
      return JSVAL_TYPE_NULL;
    case MIRType_Boolean:
      return JSVAL_TYPE_BOOLEAN;
    case MIRType_Int32:
      return JSVAL_TYPE_INT32;
    case MIRType_Double:
      return JSVAL_TYPE_DOUBLE;
    case MIRType_String:
      return JSVAL_TYPE_STRING;
    case MIRType_Magic:
      return JSVAL_TYPE_MAGIC;
    default:
      JS_ASSERT(type == MIRType_Object);
      return JSVAL_TYPE_OBJECT;
  }
}

static inline JSValueTag
MIRTypeToTag(MIRType type)
{
    return JSVAL_TYPE_TO_TAG(ValueTypeFromMIRType(type));
}

static inline const char *
StringFromMIRType(MIRType type)
{
  switch (type) {
    case MIRType_Undefined:
      return "Undefined";
    case MIRType_Null:
      return "Null";
    case MIRType_Boolean:
      return "Bool";
    case MIRType_Int32:
      return "Int32";
    case MIRType_Double:
      return "Double";
    case MIRType_String:
      return "String";
    case MIRType_Object:
      return "Object";
    case MIRType_Magic:
      return "Magic";
    case MIRType_Value:
      return "Value";
    case MIRType_None:
      return "None";
    case MIRType_Slots:
      return "Slots";
    case MIRType_Elements:
      return "Elements";
    case MIRType_Pointer:
      return "Pointer";
    case MIRType_ForkJoinSlice:
      return "ForkJoinSlice";
    default:
      JS_NOT_REACHED("Unknown MIRType.");
      return "";
  }
}

static inline bool
IsNumberType(MIRType type)
{
    return type == MIRType_Int32 || type == MIRType_Double;
}

static inline bool
IsNullOrUndefined(MIRType type)
{
    return type == MIRType_Null || type == MIRType_Undefined;
}

#ifdef DEBUG

#define TRACK_SNAPSHOTS 1
#endif

} 
} 

#endif 

