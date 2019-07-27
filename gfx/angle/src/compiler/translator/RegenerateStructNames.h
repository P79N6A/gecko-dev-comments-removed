





#ifndef COMPILER_TRANSLATOR_REGENERATE_STRUCT_NAMES_H_
#define COMPILER_TRANSLATOR_REGENERATE_STRUCT_NAMES_H_

#include "compiler/translator/Intermediate.h"
#include "compiler/translator/SymbolTable.h"

#include <set>

class RegenerateStructNames : public TIntermTraverser
{
  public:
    RegenerateStructNames(const TSymbolTable &symbolTable,
                          int shaderVersion)
        : mSymbolTable(symbolTable),
          mShaderVersion(shaderVersion),
          mScopeDepth(0) {}

  protected:
    virtual void visitSymbol(TIntermSymbol *);
    virtual bool visitAggregate(Visit, TIntermAggregate *);

  private:
    const TSymbolTable &mSymbolTable;
    int mShaderVersion;

    
    
    int mScopeDepth;

    
    std::set<int> mDeclaredGlobalStructs;
};

#endif  
