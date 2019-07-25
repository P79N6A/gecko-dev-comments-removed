





#ifndef COMPILER_MAP_LONG_VARIABLE_NAMES_H_
#define COMPILER_MAP_LONG_VARIABLE_NAMES_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"


#define MAX_IDENTIFIER_NAME_SIZE 32



class MapLongVariableNames : public TIntermTraverser {
public:
    MapLongVariableNames(TMap<TString, TString>& varyingLongNameMap);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitLoop(Visit, TIntermLoop*);

private:
    TString mapVaryingLongName(const TString& name);

    TMap<TString, TString>& mVaryingLongNameMap;
};

#endif  
