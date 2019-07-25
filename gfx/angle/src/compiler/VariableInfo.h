





#ifndef COMPILER_VARIABLE_INFO_H_
#define COMPILER_VARIABLE_INFO_H_

#include "GLSLANG/ShaderLang.h"
#include "compiler/intermediate.h"



struct TVariableInfo {
    TPersistString name;
    TPersistString mappedName;
    ShDataType type;
    int size;
};
typedef std::vector<TVariableInfo> TVariableInfoList;


class CollectAttribsUniforms : public TIntermTraverser {
public:
    CollectAttribsUniforms(TVariableInfoList& attribs,
                           TVariableInfoList& uniforms);

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);

private:
    TVariableInfoList& mAttribs;
    TVariableInfoList& mUniforms;
};

#endif  
