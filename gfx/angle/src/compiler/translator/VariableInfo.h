





#ifndef COMPILER_VARIABLE_INFO_H_
#define COMPILER_VARIABLE_INFO_H_

#include "compiler/translator/intermediate.h"



struct TVariableInfo {
    TVariableInfo(ShDataType type, int size);
    TVariableInfo();

    TPersistString name;
    TPersistString mappedName;
    ShDataType type;
    int size;
    bool isArray;
    TPrecision precision;
    bool staticUse;
};
typedef std::vector<TVariableInfo> TVariableInfoList;


class CollectVariables : public TIntermTraverser {
public:
    CollectVariables(TVariableInfoList& attribs,
                     TVariableInfoList& uniforms,
                     TVariableInfoList& varyings,
                     ShHashFunction64 hashFunction);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);

private:
    TVariableInfoList& mAttribs;
    TVariableInfoList& mUniforms;
    TVariableInfoList& mVaryings;

    bool mPointCoordAdded;
    bool mFrontFacingAdded;
    bool mFragCoordAdded;

    ShHashFunction64 mHashFunction;
};

#endif  
