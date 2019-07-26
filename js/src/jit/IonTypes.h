





#ifndef jit_IonTypes_h
#define jit_IonTypes_h

#include "jstypes.h"

#include "js/Value.h"

namespace js {
namespace jit {

typedef uint32_t SnapshotOffset;
typedef uint32_t BailoutId;




static const uint32_t MAX_BUFFER_SIZE = (1 << 30) - 1;


static const uint32_t SNAPSHOT_MAX_NARGS = 127;

static const SnapshotOffset INVALID_SNAPSHOT_OFFSET = uint32_t(-1);



enum BailoutKind
{
    
    
    Bailout_Normal,

    
    
    Bailout_ArgumentCheck,

    
    Bailout_BoundsCheck,

    
    Bailout_ShapeGuard,

    
    Bailout_BaselineInfo
};

static const uint32_t BAILOUT_KIND_BITS = 3;
static const uint32_t BAILOUT_RESUME_BITS = 1;

inline const char *
BailoutKindString(BailoutKind kind)
{
    switch (kind) {
      case Bailout_Normal:
        return "Bailout_Normal";
      case Bailout_ArgumentCheck:
        return "Bailout_ArgumentCheck";
      case Bailout_BoundsCheck:
        return "Bailout_BoundsCheck";
      case Bailout_ShapeGuard:
        return "Bailout_ShapeGuard";
      case Bailout_BaselineInfo:
        return "Bailout_BaselineInfo";
      default:
        MOZ_ASSUME_UNREACHABLE("Invalid BailoutKind");
    }
}

static const uint32_t ELEMENT_TYPE_BITS = 4;
static const uint32_t ELEMENT_TYPE_SHIFT = 0;
static const uint32_t ELEMENT_TYPE_MASK = (1 << ELEMENT_TYPE_BITS) - 1;
static const uint32_t VECTOR_SCALE_BITS = 2;
static const uint32_t VECTOR_SCALE_SHIFT = ELEMENT_TYPE_BITS + ELEMENT_TYPE_SHIFT;
static const uint32_t VECTOR_SCALE_MASK = (1 << VECTOR_SCALE_BITS) - 1;




enum MIRType
{
    MIRType_Undefined,
    MIRType_Null,
    MIRType_Boolean,
    MIRType_Int32,
    MIRType_Double,
    MIRType_Float32,
    MIRType_String,
    MIRType_Object,
    MIRType_Magic,
    MIRType_Value,
    MIRType_None,          
    MIRType_Slots,         
    MIRType_Elements,      
    MIRType_Pointer,       
    MIRType_Shape,         
    MIRType_ForkJoinContext, 
    MIRType_Last = MIRType_ForkJoinContext,
    MIRType_Float32x4 = MIRType_Float32 | (2 << VECTOR_SCALE_SHIFT),
    MIRType_Int32x4   = MIRType_Int32   | (2 << VECTOR_SCALE_SHIFT),
    MIRType_Doublex2  = MIRType_Double  | (1 << VECTOR_SCALE_SHIFT)
};

static inline MIRType
ElementType(MIRType type)
{
    JS_STATIC_ASSERT(MIRType_Last <= ELEMENT_TYPE_MASK);
    return static_cast<MIRType>((type >> ELEMENT_TYPE_SHIFT) & ELEMENT_TYPE_MASK);
}

static inline uint32_t
VectorSize(MIRType type)
{
    return 1 << ((type >> VECTOR_SCALE_SHIFT) & VECTOR_SCALE_MASK);
}

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
        MOZ_ASSUME_UNREACHABLE("unexpected jsval type");
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
    case MIRType_Float32: 
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
    case MIRType_Float32:
      return "Float32";
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
    case MIRType_ForkJoinContext:
      return "ForkJoinContext";
    default:
      MOZ_ASSUME_UNREACHABLE("Unknown MIRType.");
  }
}

static inline bool
IsNumberType(MIRType type)
{
    return type == MIRType_Int32 || type == MIRType_Double || type == MIRType_Float32;
}

static inline bool
IsFloatType(MIRType type)
{
    return type == MIRType_Int32 || type == MIRType_Float32;
}

static inline bool
IsFloatingPointType(MIRType type)
{
    return type == MIRType_Double || type == MIRType_Float32;
}

static inline bool
IsNullOrUndefined(MIRType type)
{
    return type == MIRType_Null || type == MIRType_Undefined;
}

#ifdef DEBUG

#define TRACK_SNAPSHOTS 1



#  if defined(JS_ION)
#    define CHECK_OSIPOINT_REGISTERS 1
#  endif
#endif

enum {
    ArgType_General = 0x1,
    ArgType_Double  = 0x2,
    ArgType_Float32 = 0x3,

    RetType_Shift   = 0x0,
    ArgType_Shift   = 0x2,
    ArgType_Mask    = 0x3
};

enum ABIFunctionType
{
    
    
    Args_General0 = ArgType_General << RetType_Shift,
    Args_General1 = Args_General0 | (ArgType_General << (ArgType_Shift * 1)),
    Args_General2 = Args_General1 | (ArgType_General << (ArgType_Shift * 2)),
    Args_General3 = Args_General2 | (ArgType_General << (ArgType_Shift * 3)),
    Args_General4 = Args_General3 | (ArgType_General << (ArgType_Shift * 4)),
    Args_General5 = Args_General4 | (ArgType_General << (ArgType_Shift * 5)),
    Args_General6 = Args_General5 | (ArgType_General << (ArgType_Shift * 6)),
    Args_General7 = Args_General6 | (ArgType_General << (ArgType_Shift * 7)),
    Args_General8 = Args_General7 | (ArgType_General << (ArgType_Shift * 8)),

    
    Args_Double_None = ArgType_Double << RetType_Shift,

    
    Args_Int_Double = Args_General0 | (ArgType_Double << ArgType_Shift),

    
    Args_Float32_Float32 = (ArgType_Float32 << RetType_Shift) | (ArgType_Float32 << ArgType_Shift),

    
    Args_Double_Double = Args_Double_None | (ArgType_Double << ArgType_Shift),

    
    Args_Double_Int = Args_Double_None | (ArgType_General << ArgType_Shift),

    
    Args_Double_DoubleInt = Args_Double_None |
        (ArgType_General << (ArgType_Shift * 1)) |
        (ArgType_Double << (ArgType_Shift * 2)),

    
    Args_Double_DoubleDouble = Args_Double_Double | (ArgType_Double << (ArgType_Shift * 2)),

    
    Args_Double_IntDouble = Args_Double_None |
        (ArgType_Double << (ArgType_Shift * 1)) |
        (ArgType_General << (ArgType_Shift * 2)),

    
    Args_Int_IntDouble = Args_General0 |
        (ArgType_Double << (ArgType_Shift * 1)) |
        (ArgType_General << (ArgType_Shift * 2))
};

} 
} 

#endif 
