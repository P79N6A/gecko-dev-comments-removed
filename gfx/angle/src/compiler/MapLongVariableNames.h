





#ifndef COMPILER_MAP_LONG_VARIABLE_NAMES_H_
#define COMPILER_MAP_LONG_VARIABLE_NAMES_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"


#define MAX_IDENTIFIER_NAME_SIZE 32



class MapLongVariableNames : public TIntermTraverser {
public:
    MapLongVariableNames() { }

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);
};

#endif  
