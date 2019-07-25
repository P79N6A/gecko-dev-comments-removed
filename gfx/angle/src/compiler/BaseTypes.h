





#ifndef _BASICTYPES_INCLUDED_
#define _BASICTYPES_INCLUDED_




enum TPrecision
{
    
    EbpUndefined,
    EbpLow,
    EbpMedium,
    EbpHigh,
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
    EbtBool,
    EbtGuardSamplerBegin,  
    EbtSampler2D,
    EbtSamplerCube,
    EbtSamplerExternalOES,  
    EbtSampler2DRect,       
    EbtGuardSamplerEnd,    
    EbtStruct,
    EbtAddress,            
};

inline const char* getBasicString(TBasicType t)
{
    switch (t)
    {
    case EbtVoid:              return "void";              break;
    case EbtFloat:             return "float";             break;
    case EbtInt:               return "int";               break;
    case EbtBool:              return "bool";              break;
    case EbtSampler2D:         return "sampler2D";         break;
    case EbtSamplerCube:       return "samplerCube";       break;
    case EbtSamplerExternalOES: return "samplerExternalOES"; break;
    case EbtSampler2DRect:     return "sampler2DRect";     break;
    case EbtStruct:            return "structure";         break;
    default:                   return "unknown type";
    }
}

inline bool IsSampler(TBasicType type)
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
