





#ifndef _BASICTYPES_INCLUDED_
#define _BASICTYPES_INCLUDED_

#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define __inline inline
#endif




enum TPrecision
{
    
    EbpUndefined,
    EbpLow,
    EbpMedium,
    EbpHigh,
};

__inline const char* getPrecisionString(TPrecision p)
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
    EbtBool,
    EbtGuardSamplerBegin,  
    EbtSampler2D,
    EbtSamplerCube,
    EbtGuardSamplerEnd,    
    EbtStruct,
    EbtAddress,            
};

__inline bool IsSampler(TBasicType type)
{
    return type > EbtGuardSamplerBegin && type < EbtGuardSamplerEnd;
}







enum TQualifier
{
    EvqTemporary,     
    EvqGlobal,        
    EvqConst,         
    EvqAttribute,     
    EvqVaryingIn,     
    EvqVaryingOut,    
    EvqInvariantVaryingIn,     
    EvqInvariantVaryingOut,    
    EvqUniform,       

    
    EvqInput,
    EvqOutput,

    
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

    
    EvqLast,
};




__inline const char* getQualifierString(TQualifier q)
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
    case EvqIn:             return "in";             break;
    case EvqOut:            return "out";            break;
    case EvqInOut:          return "inout";          break;
    case EvqInput:          return "input";          break;
    case EvqOutput:         return "output";         break;
    case EvqPosition:       return "Position";       break;
    case EvqPointSize:      return "PointSize";      break;
    case EvqFragCoord:      return "FragCoord";      break;
    case EvqFrontFacing:    return "FrontFacing";    break;
    case EvqFragColor:      return "FragColor";      break;
    case EvqFragData:       return "FragData";      break;
    default:                return "unknown qualifier";
    }
}

#endif 
