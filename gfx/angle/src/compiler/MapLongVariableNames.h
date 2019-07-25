





#ifndef COMPILER_MAP_LONG_VARIABLE_NAMES_H_
#define COMPILER_MAP_LONG_VARIABLE_NAMES_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"


#define MAX_SHORTENED_IDENTIFIER_SIZE 32



class MapLongVariableNames : public TIntermTraverser {
public:
    MapLongVariableNames(std::map<std::string, std::string>& varyingLongNameMap);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitLoop(Visit, TIntermLoop*);

private:
    TString mapVaryingLongName(const TString& name);

    std::map<std::string, std::string>& mVaryingLongNameMap;
};

#endif  
