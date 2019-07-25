





#include "GLSLANG/ShaderLang.h"
#include "compiler/intermediate.h"

class TInfoSinkBase;

struct TLoopInfo {
    struct TIndex {
        int id;  
    } index;
    TIntermLoop* loop;
};
typedef TVector<TLoopInfo> TLoopStack;



class ValidateLimitations : public TIntermTraverser {
public:
    ValidateLimitations(ShShaderType shaderType, TInfoSinkBase& sink);

    int numErrors() const { return mNumErrors; }

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit, TIntermBinary*);
    virtual bool visitUnary(Visit, TIntermUnary*);
    virtual bool visitSelection(Visit, TIntermSelection*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);
    virtual bool visitLoop(Visit, TIntermLoop*);
    virtual bool visitBranch(Visit, TIntermBranch*);

private:
    void error(TSourceLoc loc, const char *reason, const char* token);

    bool withinLoopBody() const;
    bool isLoopIndex(const TIntermSymbol* symbol) const;
    bool validateLoopType(TIntermLoop* node);
    bool validateForLoopHeader(TIntermLoop* node, TLoopInfo* info);
    bool validateForLoopInit(TIntermLoop* node, TLoopInfo* info);
    bool validateForLoopCond(TIntermLoop* node, TLoopInfo* info);
    bool validateForLoopExpr(TIntermLoop* node, TLoopInfo* info);
    
    
    bool validateFunctionCall(TIntermAggregate* node);
    bool validateOperation(TIntermOperator* node, TIntermNode* operand);

    
    
    bool isConstExpr(TIntermNode* node);
    bool isConstIndexExpr(TIntermNode* node);
    bool validateIndexing(TIntermBinary* node);

    ShShaderType mShaderType;
    TInfoSinkBase& mSink;
    int mNumErrors;
    TLoopStack mLoopStack;
};

