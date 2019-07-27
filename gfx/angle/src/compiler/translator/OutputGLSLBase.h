





#ifndef CROSSCOMPILERGLSL_OUTPUTGLSLBASE_H_
#define CROSSCOMPILERGLSL_OUTPUTGLSLBASE_H_

#include <set>

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/LoopInfo.h"
#include "compiler/translator/ParseContext.h"

class TOutputGLSLBase : public TIntermTraverser
{
  public:
    TOutputGLSLBase(TInfoSinkBase &objSink,
                    ShArrayIndexClampingStrategy clampingStrategy,
                    ShHashFunction64 hashFunction,
                    NameMap &nameMap,
                    TSymbolTable& symbolTable,
                    int shaderVersion);

  protected:
    TInfoSinkBase &objSink() { return mObjSink; }
    void writeTriplet(Visit visit, const char *preStr, const char *inStr, const char *postStr);
    void writeVariableType(const TType &type);
    virtual bool writeVariablePrecision(TPrecision precision) = 0;
    void writeFunctionParameters(const TIntermSequence &args);
    const ConstantUnion *writeConstantUnion(const TType &type, const ConstantUnion *pConstUnion);
    TString getTypeName(const TType &type);

    virtual void visitSymbol(TIntermSymbol *node);
    virtual void visitConstantUnion(TIntermConstantUnion *node);
    virtual bool visitBinary(Visit visit, TIntermBinary *node);
    virtual bool visitUnary(Visit visit, TIntermUnary *node);
    virtual bool visitSelection(Visit visit, TIntermSelection *node);
    virtual bool visitAggregate(Visit visit, TIntermAggregate *node);
    virtual bool visitLoop(Visit visit, TIntermLoop *node);
    virtual bool visitBranch(Visit visit, TIntermBranch *node);

    void visitCodeBlock(TIntermNode *node);

    
    
    TString hashName(const TString &name);
    
    TString hashVariableName(const TString &name);
    
    TString hashFunctionName(const TString &mangled_name);
    
    virtual TString translateTextureFunction(TString &name) { return name; }

  private:
    bool structDeclared(const TStructure *structure) const;
    void declareStruct(const TStructure *structure);

    void writeBuiltInFunctionTriplet(Visit visit, const char *preStr, bool useEmulatedFunction);

    TInfoSinkBase &mObjSink;
    bool mDeclaringVariables;

    
    std::set<int> mDeclaredStructs;

    
    TLoopStack mLoopUnrollStack;

    ShArrayIndexClampingStrategy mClampingStrategy;

    
    ShHashFunction64 mHashFunction;

    NameMap &mNameMap;

    TSymbolTable &mSymbolTable;

    const int mShaderVersion;
};

#endif  
