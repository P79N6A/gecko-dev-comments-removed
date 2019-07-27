





#include "compiler/translator/IntermNode.h"
#include "compiler/translator/LoopInfo.h"

class TInfoSinkBase;



class ValidateLimitations : public TIntermTraverser
{
  public:
    ValidateLimitations(sh::GLenum shaderType, TInfoSinkBase &sink);

    int numErrors() const { return mNumErrors; }

    virtual bool visitBinary(Visit, TIntermBinary *);
    virtual bool visitUnary(Visit, TIntermUnary *);
    virtual bool visitAggregate(Visit, TIntermAggregate *);
    virtual bool visitLoop(Visit, TIntermLoop *);

  private:
    void error(TSourceLoc loc, const char *reason, const char *token);

    bool withinLoopBody() const;
    bool isLoopIndex(TIntermSymbol *symbol);
    bool validateLoopType(TIntermLoop *node);

    bool validateForLoopHeader(TIntermLoop *node);
    
    int validateForLoopInit(TIntermLoop *node);
    bool validateForLoopCond(TIntermLoop *node, int indexSymbolId);
    bool validateForLoopExpr(TIntermLoop *node, int indexSymbolId);

    
    
    bool validateFunctionCall(TIntermAggregate *node);
    bool validateOperation(TIntermOperator *node, TIntermNode *operand);

    
    
    bool isConstExpr(TIntermNode *node);
    bool isConstIndexExpr(TIntermNode *node);
    bool validateIndexing(TIntermBinary *node);

    sh::GLenum mShaderType;
    TInfoSinkBase &mSink;
    int mNumErrors;
    TLoopStack mLoopStack;
};

