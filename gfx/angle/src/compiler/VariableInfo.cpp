





#include "compiler/VariableInfo.h"

static TString arrayBrackets(int index)
{
    TStringStream stream;
    stream << "[" << index << "]";
    return stream.str();
}


static ShDataType getVariableDataType(const TType& type)
{
    switch (type.getBasicType()) {
      case EbtFloat:
          if (type.isMatrix()) {
              switch (type.getNominalSize()) {
                case 2: return SH_FLOAT_MAT2;
                case 3: return SH_FLOAT_MAT3;
                case 4: return SH_FLOAT_MAT4;
                default: UNREACHABLE();
              }
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_FLOAT_VEC2;
                case 3: return SH_FLOAT_VEC3;
                case 4: return SH_FLOAT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_FLOAT;
          }
      case EbtInt:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_INT_VEC2;
                case 3: return SH_INT_VEC3;
                case 4: return SH_INT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_INT;
          }
      case EbtBool:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_BOOL_VEC2;
                case 3: return SH_BOOL_VEC3;
                case 4: return SH_BOOL_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_BOOL;
          }
      case EbtSampler2D: return SH_SAMPLER_2D;
      case EbtSamplerCube: return SH_SAMPLER_CUBE;
      default: UNREACHABLE();
    }
    return SH_NONE;
}

static void getBuiltInVariableInfo(const TType& type,
                                   const TString& name,
                                   TVariableInfoList& infoList);
static void getUserDefinedVariableInfo(const TType& type,
                                       const TString& name,
                                       TVariableInfoList& infoList);


static void getVariableInfo(const TType& type,
                            const TString& name,
                            TVariableInfoList& infoList)
{
    if (type.getBasicType() == EbtStruct) {
        if (type.isArray()) {
            for (int i = 0; i < type.getArraySize(); ++i) {
                TString lname = name + arrayBrackets(i);
                getUserDefinedVariableInfo(type, lname, infoList);
            }
        } else {
            getUserDefinedVariableInfo(type, name, infoList);
        }
    } else {
        getBuiltInVariableInfo(type, name, infoList);
    }
}

void getBuiltInVariableInfo(const TType& type,
                            const TString& name,
                            TVariableInfoList& infoList)
{
    ASSERT(type.getBasicType() != EbtStruct);

    TVariableInfo varInfo;
    if (type.isArray()) {
        varInfo.name = (name + "[0]").c_str();
        varInfo.size = type.getArraySize();
    } else {
        varInfo.name = name.c_str();
        varInfo.size = 1;
    }
    varInfo.type = getVariableDataType(type);
    infoList.push_back(varInfo);
}

void getUserDefinedVariableInfo(const TType& type,
                                const TString& name,
                                TVariableInfoList& infoList)
{
    ASSERT(type.getBasicType() == EbtStruct);

    TString lname = name + ".";
    const TTypeList* structure = type.getStruct();
    for (size_t i = 0; i < structure->size(); ++i) {
        const TType* fieldType = (*structure)[i].type;
        getVariableInfo(*fieldType,
                        lname + fieldType->getFieldName(),
                        infoList);
    }
}

CollectAttribsUniforms::CollectAttribsUniforms(TVariableInfoList& attribs,
                                               TVariableInfoList& uniforms)
    : mAttribs(attribs),
      mUniforms(uniforms)
{
}


void CollectAttribsUniforms::visitSymbol(TIntermSymbol*)
{
}

void CollectAttribsUniforms::visitConstantUnion(TIntermConstantUnion*)
{
}

bool CollectAttribsUniforms::visitBinary(Visit, TIntermBinary*)
{
    return false;
}

bool CollectAttribsUniforms::visitUnary(Visit, TIntermUnary*)
{
    return false;
}

bool CollectAttribsUniforms::visitSelection(Visit, TIntermSelection*)
{
    return false;
}

bool CollectAttribsUniforms::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = false;

    switch (node->getOp())
    {
    case EOpSequence:
        
        visitChildren = true;
        break;
    case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if (qualifier == EvqAttribute || qualifier == EvqUniform)
        {
            TVariableInfoList& infoList = qualifier == EvqAttribute ?
                mAttribs : mUniforms;
            for (TIntermSequence::const_iterator i = sequence.begin();
                 i != sequence.end(); ++i)
            {
                const TIntermSymbol* variable = (*i)->getAsSymbolNode();
                
                
                
                
                
                ASSERT(variable != NULL);
                getVariableInfo(variable->getType(), variable->getSymbol(),
                                infoList);
            }
        }
        break;
    }
    default: break;
    }

    return visitChildren;
}

bool CollectAttribsUniforms::visitLoop(Visit, TIntermLoop*)
{
    return false;
}

bool CollectAttribsUniforms::visitBranch(Visit, TIntermBranch*)
{
    return false;
}

