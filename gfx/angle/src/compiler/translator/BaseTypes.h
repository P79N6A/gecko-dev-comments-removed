





#ifndef _BASICTYPES_INCLUDED_
#define _BASICTYPES_INCLUDED_

#include <assert.h>




enum TPrecision
{
    
    EbpUndefined,
    EbpLow,
    EbpMedium,
    EbpHigh
};

inline const char* getPrecisionString(TPrecision p)
{
    switch(p)
    {
    case EbpHigh:		return "highp";		break;
    case EbpMedium:		return "mediump";	break;
    case EbpLow:		return "lowp";		break;
    default:			return "mediump";   break;   
    }
}




enum TBasicType
{
    EbtVoid,
    EbtFloat,
    EbtInt,
    EbtUInt,
    EbtBool,
    EbtGVec4,              
    EbtGuardSamplerBegin,  
    EbtSampler2D,
    EbtSampler3D,
    EbtSamplerCube,
    EbtSampler2DArray,
    EbtSamplerExternalOES,  
    EbtSampler2DRect,       
    EbtISampler2D,
    EbtISampler3D,
    EbtISamplerCube,
    EbtISampler2DArray,
    EbtUSampler2D,
    EbtUSampler3D,
    EbtUSamplerCube,
    EbtUSampler2DArray,
    EbtSampler2DShadow,
    EbtSamplerCubeShadow,
    EbtSampler2DArrayShadow,
    EbtGuardSamplerEnd,    
    EbtGSampler2D,         
    EbtGSampler3D,         
    EbtGSamplerCube,       
    EbtGSampler2DArray,    
    EbtStruct,
    EbtInterfaceBlock,
    EbtAddress,            
};

const char* getBasicString(TBasicType t);

inline bool IsSampler(TBasicType type)
{
    return type > EbtGuardSamplerBegin && type < EbtGuardSamplerEnd;
}

inline bool IsIntegerSampler(TBasicType type)
{
    switch (type)
    {
      case EbtISampler2D:
      case EbtISampler3D:
      case EbtISamplerCube:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSampler3D:
      case EbtUSamplerCube:
      case EbtUSampler2DArray:
        return true;
      case EbtSampler2D:
      case EbtSampler3D:
      case EbtSamplerCube:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
      case EbtSampler2DArrayShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSampler2D(TBasicType type)
{
    switch (type)
    {
      case EbtSampler2D:
      case EbtISampler2D:
      case EbtUSampler2D:
      case EbtSampler2DArray:
      case EbtISampler2DArray:
      case EbtUSampler2DArray:
      case EbtSampler2DRect:
      case EbtSamplerExternalOES:
      case EbtSampler2DShadow:
      case EbtSampler2DArrayShadow:
        return true;
      case EbtSampler3D:
      case EbtISampler3D:
      case EbtUSampler3D:
      case EbtISamplerCube:
      case EbtUSamplerCube:
      case EbtSamplerCube:
      case EbtSamplerCubeShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSamplerCube(TBasicType type)
{
    switch (type)
    {
      case EbtSamplerCube:
      case EbtISamplerCube:
      case EbtUSamplerCube:
      case EbtSamplerCubeShadow:
        return true;
      case EbtSampler2D:
      case EbtSampler3D:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
      case EbtISampler2D:
      case EbtISampler3D:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSampler3D:
      case EbtUSampler2DArray:
      case EbtSampler2DShadow:
      case EbtSampler2DArrayShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSampler3D(TBasicType type)
{
    switch (type)
    {
      case EbtSampler3D:
      case EbtISampler3D:
      case EbtUSampler3D:
        return true;
      case EbtSampler2D:
      case EbtSamplerCube:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
      case EbtISampler2D:
      case EbtISamplerCube:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSamplerCube:
      case EbtUSampler2DArray:
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
      case EbtSampler2DArrayShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsSamplerArray(TBasicType type)
{
    switch (type)
    {
      case EbtSampler2DArray:
      case EbtISampler2DArray:
      case EbtUSampler2DArray:
      case EbtSampler2DArrayShadow:
        return true;
      case EbtSampler2D:
      case EbtISampler2D:
      case EbtUSampler2D:
      case EbtSampler2DRect:
      case EbtSamplerExternalOES:
      case EbtSampler3D:
      case EbtISampler3D:
      case EbtUSampler3D:
      case EbtISamplerCube:
      case EbtUSamplerCube:
      case EbtSamplerCube:
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool IsShadowSampler(TBasicType type)
{
    switch (type)
    {
      case EbtSampler2DShadow:
      case EbtSamplerCubeShadow:
      case EbtSampler2DArrayShadow:
        return true;
      case EbtISampler2D:
      case EbtISampler3D:
      case EbtISamplerCube:
      case EbtISampler2DArray:
      case EbtUSampler2D:
      case EbtUSampler3D:
      case EbtUSamplerCube:
      case EbtUSampler2DArray:
      case EbtSampler2D:
      case EbtSampler3D:
      case EbtSamplerCube:
      case EbtSamplerExternalOES:
      case EbtSampler2DRect:
      case EbtSampler2DArray:
        return false;
      default:
        assert(!IsSampler(type));
    }

    return false;
}

inline bool SupportsPrecision(TBasicType type)
{
    return type == EbtFloat || type == EbtInt || type == EbtUInt || IsSampler(type);
}







enum TQualifier
{
    EvqTemporary,     
    EvqGlobal,        
    EvqInternal,      
    EvqConst,         
    EvqAttribute,     
    EvqVaryingIn,     
    EvqVaryingOut,    
    EvqInvariantVaryingIn,     
    EvqInvariantVaryingOut,    
    EvqUniform,       

    EvqVertexIn,      
    EvqFragmentOut,   
    EvqVertexOut,     
    EvqFragmentIn,    

    
    EvqIn,
    EvqOut,
    EvqInOut,
    EvqConstReadOnly,

    
    EvqPosition,
    EvqPointSize,

    
    EvqFragCoord,
    EvqFrontFacing,
    EvqPointCoord,

    
    EvqFragColor,
    EvqFragData,
    EvqFragDepth,

    
    EvqSmooth,        
    EvqFlat,          
    EvqSmoothOut = EvqSmooth,
    EvqFlatOut = EvqFlat,
    EvqCentroidOut,   
    EvqSmoothIn,
    EvqFlatIn,
    EvqCentroidIn,    

    
    EvqLast
};

enum TLayoutMatrixPacking
{
    EmpUnspecified,
    EmpRowMajor,
    EmpColumnMajor
};

enum TLayoutBlockStorage
{
    EbsUnspecified,
    EbsShared,
    EbsPacked,
    EbsStd140
};

struct TLayoutQualifier
{
    int location;
    TLayoutMatrixPacking matrixPacking;
    TLayoutBlockStorage blockStorage;

    static TLayoutQualifier create()
    {
        TLayoutQualifier layoutQualifier;

        layoutQualifier.location = -1;
        layoutQualifier.matrixPacking = EmpUnspecified;
        layoutQualifier.blockStorage = EbsUnspecified;

        return layoutQualifier;
    }

    bool isEmpty() const
    {
        return location == -1 && matrixPacking == EmpUnspecified && blockStorage == EbsUnspecified;
    }
};




inline const char* getQualifierString(TQualifier q)
{
    switch(q)
    {
    case EvqTemporary:      return "Temporary";      break;
    case EvqGlobal:         return "Global";         break;
    case EvqConst:          return "const";          break;
    case EvqConstReadOnly:  return "const";          break;
    case EvqAttribute:      return "attribute";      break;
    case EvqVaryingIn:      return "varying";        break;
    case EvqVaryingOut:     return "varying";        break;
    case EvqInvariantVaryingIn: return "invariant varying";	break;
    case EvqInvariantVaryingOut:return "invariant varying";	break;
    case EvqUniform:        return "uniform";        break;
    case EvqVertexIn:       return "in";             break;
    case EvqFragmentOut:    return "out";            break;
    case EvqVertexOut:      return "out";            break;
    case EvqFragmentIn:     return "in";             break;
    case EvqIn:             return "in";             break;
    case EvqOut:            return "out";            break;
    case EvqInOut:          return "inout";          break;
    case EvqPosition:       return "Position";       break;
    case EvqPointSize:      return "PointSize";      break;
    case EvqFragCoord:      return "FragCoord";      break;
    case EvqFrontFacing:    return "FrontFacing";    break;
    case EvqFragColor:      return "FragColor";      break;
    case EvqFragData:       return "FragData";       break;
    case EvqFragDepth:      return "FragDepth";      break;
    case EvqSmoothOut:      return "smooth out";     break;
    case EvqCentroidOut:    return "centroid out";   break;
    case EvqFlatOut:        return "flat out";       break;
    case EvqSmoothIn:       return "smooth in";      break;
    case EvqCentroidIn:     return "centroid in";    break;
    case EvqFlatIn:         return "flat in";        break;
    default:                return "unknown qualifier";
    }
}

inline const char* getMatrixPackingString(TLayoutMatrixPacking mpq)
{
    switch (mpq)
    {
    case EmpUnspecified:    return "mp_unspecified";
    case EmpRowMajor:       return "row_major";
    case EmpColumnMajor:    return "column_major";
    default:                return "unknown matrix packing";
    }
}

inline const char* getBlockStorageString(TLayoutBlockStorage bsq)
{
    switch (bsq)
    {
    case EbsUnspecified:    return "bs_unspecified";
    case EbsShared:         return "shared";
    case EbsPacked:         return "packed";
    case EbsStd140:         return "std140";
    default:                return "unknown block storage";
    }
}

inline const char* getInterpolationString(TQualifier q)
{
    switch(q)
    {
    case EvqSmoothOut:      return "smooth";   break;
    case EvqCentroidOut:    return "centroid"; break;
    case EvqFlatOut:        return "flat";     break;
    case EvqSmoothIn:       return "smooth";   break;
    case EvqCentroidIn:     return "centroid"; break;
    case EvqFlatIn:         return "flat";     break;
    default:                return "unknown interpolation";
    }
}

#endif 
