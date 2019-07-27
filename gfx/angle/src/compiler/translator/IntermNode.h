














#ifndef COMPILER_TRANSLATOR_INTERMEDIATE_H_
#define COMPILER_TRANSLATOR_INTERMEDIATE_H_

#include "GLSLANG/ShaderLang.h"

#include <algorithm>
#include <queue>

#include "compiler/translator/Common.h"
#include "compiler/translator/Types.h"
#include "compiler/translator/ConstantUnion.h"




enum TOperator
{
    EOpNull,            
    EOpSequence,        
    EOpFunctionCall,
    EOpFunction,        
    EOpParameters,      

    EOpDeclaration,
    EOpInvariantDeclaration, 
    EOpPrototype,

    
    
    

    EOpNegative,
    EOpLogicalNot,
    EOpVectorLogicalNot,

    EOpPostIncrement,
    EOpPostDecrement,
    EOpPreIncrement,
    EOpPreDecrement,

    
    
    

    EOpAdd,
    EOpSub,
    EOpMul,
    EOpDiv,
    EOpEqual,
    EOpNotEqual,
    EOpVectorEqual,
    EOpVectorNotEqual,
    EOpLessThan,
    EOpGreaterThan,
    EOpLessThanEqual,
    EOpGreaterThanEqual,
    EOpComma,

    EOpVectorTimesScalar,
    EOpVectorTimesMatrix,
    EOpMatrixTimesVector,
    EOpMatrixTimesScalar,

    EOpLogicalOr,
    EOpLogicalXor,
    EOpLogicalAnd,

    EOpIndexDirect,
    EOpIndexIndirect,
    EOpIndexDirectStruct,
    EOpIndexDirectInterfaceBlock,

    EOpVectorSwizzle,

    
    
    

    EOpRadians,
    EOpDegrees,
    EOpSin,
    EOpCos,
    EOpTan,
    EOpAsin,
    EOpAcos,
    EOpAtan,

    EOpPow,
    EOpExp,
    EOpLog,
    EOpExp2,
    EOpLog2,
    EOpSqrt,
    EOpInverseSqrt,

    EOpAbs,
    EOpSign,
    EOpFloor,
    EOpCeil,
    EOpFract,
    EOpMod,
    EOpMin,
    EOpMax,
    EOpClamp,
    EOpMix,
    EOpStep,
    EOpSmoothStep,

    EOpLength,
    EOpDistance,
    EOpDot,
    EOpCross,
    EOpNormalize,
    EOpFaceForward,
    EOpReflect,
    EOpRefract,

    EOpDFdx,            
    EOpDFdy,            
    EOpFwidth,          

    EOpMatrixTimesMatrix,

    EOpAny,
    EOpAll,

    
    
    

    EOpKill,            
    EOpReturn,
    EOpBreak,
    EOpContinue,

    
    
    

    EOpConstructInt,
    EOpConstructUInt,
    EOpConstructBool,
    EOpConstructFloat,
    EOpConstructVec2,
    EOpConstructVec3,
    EOpConstructVec4,
    EOpConstructBVec2,
    EOpConstructBVec3,
    EOpConstructBVec4,
    EOpConstructIVec2,
    EOpConstructIVec3,
    EOpConstructIVec4,
    EOpConstructUVec2,
    EOpConstructUVec3,
    EOpConstructUVec4,
    EOpConstructMat2,
    EOpConstructMat3,
    EOpConstructMat4,
    EOpConstructStruct,

    
    
    

    EOpAssign,
    EOpInitialize,
    EOpAddAssign,
    EOpSubAssign,
    EOpMulAssign,
    EOpVectorTimesMatrixAssign,
    EOpVectorTimesScalarAssign,
    EOpMatrixTimesScalarAssign,
    EOpMatrixTimesMatrixAssign,
    EOpDivAssign
};

class TIntermTraverser;
class TIntermAggregate;
class TIntermBinary;
class TIntermUnary;
class TIntermConstantUnion;
class TIntermSelection;
class TIntermTyped;
class TIntermSymbol;
class TIntermLoop;
class TInfoSink;
class TIntermRaw;




class TIntermNode
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TIntermNode()
    {
        
        
        mLine.first_file = mLine.last_file = 0;
        mLine.first_line = mLine.last_line = 0;
    }
    virtual ~TIntermNode() { }

    const TSourceLoc &getLine() const { return mLine; }
    void setLine(const TSourceLoc &l) { mLine = l; }

    virtual void traverse(TIntermTraverser *) = 0;
    virtual TIntermTyped *getAsTyped() { return 0; }
    virtual TIntermConstantUnion *getAsConstantUnion() { return 0; }
    virtual TIntermAggregate *getAsAggregate() { return 0; }
    virtual TIntermBinary *getAsBinaryNode() { return 0; }
    virtual TIntermUnary *getAsUnaryNode() { return 0; }
    virtual TIntermSelection *getAsSelectionNode() { return 0; }
    virtual TIntermSymbol *getAsSymbolNode() { return 0; }
    virtual TIntermLoop *getAsLoopNode() { return 0; }
    virtual TIntermRaw *getAsRawNode() { return 0; }

    
    
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement) = 0;

    
    
    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const = 0;

  protected:
    TSourceLoc mLine;
};




struct TIntermNodePair
{
    TIntermNode *node1;
    TIntermNode *node2;
};




class TIntermTyped : public TIntermNode
{
  public:
    TIntermTyped(const TType &t) : mType(t)  { }
    virtual TIntermTyped *getAsTyped() { return this; }

    virtual bool hasSideEffects() const = 0;

    void setType(const TType &t) { mType = t; }
    const TType &getType() const { return mType; }
    TType *getTypePointer() { return &mType; }

    TBasicType getBasicType() const { return mType.getBasicType(); }
    TQualifier getQualifier() const { return mType.getQualifier(); }
    TPrecision getPrecision() const { return mType.getPrecision(); }
    int getCols() const { return mType.getCols(); }
    int getRows() const { return mType.getRows(); }
    int getNominalSize() const { return mType.getNominalSize(); }
    int getSecondarySize() const { return mType.getSecondarySize(); }

    bool isInterfaceBlock() const { return mType.isInterfaceBlock(); }
    bool isMatrix() const { return mType.isMatrix(); }
    bool isArray()  const { return mType.isArray(); }
    bool isVector() const { return mType.isVector(); }
    bool isScalar() const { return mType.isScalar(); }
    bool isScalarInt() const { return mType.isScalarInt(); }
    const char *getBasicString() const { return mType.getBasicString(); }
    const char *getQualifierString() const { return mType.getQualifierString(); }
    TString getCompleteString() const { return mType.getCompleteString(); }

    int getArraySize() const { return mType.getArraySize(); }

  protected:
    TType mType;
};




enum TLoopType
{
    ELoopFor,
    ELoopWhile,
    ELoopDoWhile
};

class TIntermLoop : public TIntermNode
{
  public:
    TIntermLoop(TLoopType type,
                TIntermNode *init, TIntermTyped *cond, TIntermTyped *expr,
                TIntermNode *body)
        : mType(type),
          mInit(init),
          mCond(cond),
          mExpr(expr),
          mBody(body),
          mUnrollFlag(false) { }

    virtual TIntermLoop *getAsLoopNode() { return this; }
    virtual void traverse(TIntermTraverser *);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    TLoopType getType() const { return mType; }
    TIntermNode *getInit() { return mInit; }
    TIntermTyped *getCondition() { return mCond; }
    TIntermTyped *getExpression() { return mExpr; }
    TIntermNode *getBody() { return mBody; }

    void setUnrollFlag(bool flag) { mUnrollFlag = flag; }
    bool getUnrollFlag() const { return mUnrollFlag; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const;

  protected:
    TLoopType mType;
    TIntermNode *mInit;  
    TIntermTyped *mCond; 
    TIntermTyped *mExpr; 
    TIntermNode *mBody;  

    bool mUnrollFlag; 
};




class TIntermBranch : public TIntermNode
{
  public:
    TIntermBranch(TOperator op, TIntermTyped *e)
        : mFlowOp(op),
          mExpression(e) { }

    virtual void traverse(TIntermTraverser *);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    TOperator getFlowOp() { return mFlowOp; }
    TIntermTyped* getExpression() { return mExpression; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const;

protected:
    TOperator mFlowOp;
    TIntermTyped *mExpression;  
};




class TIntermSymbol : public TIntermTyped
{
  public:
    
    
    
    TIntermSymbol(int id, const TString &symbol, const TType &type)
        : TIntermTyped(type),
          mId(id)
    {
        mSymbol = symbol;
    }

    virtual bool hasSideEffects() const { return false; }

    int getId() const { return mId; }
    const TString &getSymbol() const { return mSymbol; }

    void setId(int newId) { mId = newId; }

    virtual void traverse(TIntermTraverser *);
    virtual TIntermSymbol *getAsSymbolNode() { return this; }
    virtual bool replaceChildNode(TIntermNode *, TIntermNode *) { return false; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const {}

  protected:
    int mId;
    TString mSymbol;
};




class TIntermRaw : public TIntermTyped
{
  public:
    TIntermRaw(const TType &type, const TString &rawText)
        : TIntermTyped(type),
          mRawText(rawText) { }

    virtual bool hasSideEffects() const { return false; }

    TString getRawText() const { return mRawText; }

    virtual void traverse(TIntermTraverser *);

    virtual TIntermRaw *getAsRawNode() { return this; }
    virtual bool replaceChildNode(TIntermNode *, TIntermNode *) { return false; }
    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const {}

  protected:
    TString mRawText;
};

class TIntermConstantUnion : public TIntermTyped
{
  public:
    TIntermConstantUnion(ConstantUnion *unionPointer, const TType &type)
        : TIntermTyped(type),
          mUnionArrayPointer(unionPointer) { }

    virtual bool hasSideEffects() const { return false; }

    ConstantUnion *getUnionArrayPointer() const { return mUnionArrayPointer; }

    int getIConst(size_t index) const
    {
        return mUnionArrayPointer ? mUnionArrayPointer[index].getIConst() : 0;
    }
    unsigned int getUConst(size_t index) const
    {
        return mUnionArrayPointer ? mUnionArrayPointer[index].getUConst() : 0;
    }
    float getFConst(size_t index) const
    {
        return mUnionArrayPointer ? mUnionArrayPointer[index].getFConst() : 0.0f;
    }
    bool getBConst(size_t index) const
    {
        return mUnionArrayPointer ? mUnionArrayPointer[index].getBConst() : false;
    }

    virtual TIntermConstantUnion *getAsConstantUnion()  { return this; }
    virtual void traverse(TIntermTraverser *);
    virtual bool replaceChildNode(TIntermNode *, TIntermNode *) { return false; }

    TIntermTyped *fold(TOperator, TIntermTyped *, TInfoSink &);

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const {}

  protected:
    ConstantUnion *mUnionArrayPointer;
};




class TIntermOperator : public TIntermTyped
{
  public:
    TOperator getOp() const { return mOp; }
    void setOp(TOperator op) { mOp = op; }

    bool isAssignment() const;
    bool isConstructor() const;

    virtual bool hasSideEffects() const { return isAssignment(); }

  protected:
    TIntermOperator(TOperator op)
        : TIntermTyped(TType(EbtFloat, EbpUndefined)),
          mOp(op) {}
    TIntermOperator(TOperator op, const TType &type)
        : TIntermTyped(type),
          mOp(op) {}

    TOperator mOp;
};




class TIntermBinary : public TIntermOperator
{
  public:
    TIntermBinary(TOperator op)
        : TIntermOperator(op),
          mAddIndexClamp(false) {}

    virtual TIntermBinary *getAsBinaryNode() { return this; }
    virtual void traverse(TIntermTraverser *);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    virtual bool hasSideEffects() const
    {
        return isAssignment() || mLeft->hasSideEffects() || mRight->hasSideEffects();
    }

    void setLeft(TIntermTyped *node) { mLeft = node; }
    void setRight(TIntermTyped *node) { mRight = node; }
    TIntermTyped *getLeft() const { return mLeft; }
    TIntermTyped *getRight() const { return mRight; }
    bool promote(TInfoSink &);

    void setAddIndexClamp() { mAddIndexClamp = true; }
    bool getAddIndexClamp() { return mAddIndexClamp; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const;

  protected:
    TIntermTyped* mLeft;
    TIntermTyped* mRight;

    
    bool mAddIndexClamp;
};




class TIntermUnary : public TIntermOperator
{
  public:
    TIntermUnary(TOperator op, const TType &type)
        : TIntermOperator(op, type),
          mOperand(NULL),
          mUseEmulatedFunction(false) {}
    TIntermUnary(TOperator op)
        : TIntermOperator(op),
          mOperand(NULL),
          mUseEmulatedFunction(false) {}

    virtual void traverse(TIntermTraverser *);
    virtual TIntermUnary *getAsUnaryNode() { return this; }
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    virtual bool hasSideEffects() const
    {
        return isAssignment() || mOperand->hasSideEffects();
    }

    void setOperand(TIntermTyped *operand) { mOperand = operand; }
    TIntermTyped *getOperand() { return mOperand; }
    bool promote(TInfoSink &);

    void setUseEmulatedFunction() { mUseEmulatedFunction = true; }
    bool getUseEmulatedFunction() { return mUseEmulatedFunction; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const;

  protected:
    TIntermTyped *mOperand;

    
    
    bool mUseEmulatedFunction;
};

typedef TVector<TIntermNode *> TIntermSequence;
typedef TVector<int> TQualifierList;




class TIntermAggregate : public TIntermOperator
{
  public:
    TIntermAggregate()
        : TIntermOperator(EOpNull),
          mUserDefined(false),
          mUseEmulatedFunction(false) { }
    TIntermAggregate(TOperator op)
        : TIntermOperator(op),
          mUseEmulatedFunction(false) { }
    ~TIntermAggregate() { }

    virtual TIntermAggregate *getAsAggregate() { return this; }
    virtual void traverse(TIntermTraverser *);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    
    virtual bool hasSideEffects() const { return true; }

    TIntermSequence *getSequence() { return &mSequence; }

    void setName(const TString &name) { mName = name; }
    const TString &getName() const { return mName; }

    void setUserDefined() { mUserDefined = true; }
    bool isUserDefined() const { return mUserDefined; }

    void setOptimize(bool optimize) { mOptimize = optimize; }
    bool getOptimize() const { return mOptimize; }
    void setDebug(bool debug) { mDebug = debug; }
    bool getDebug() const { return mDebug; }

    void setUseEmulatedFunction() { mUseEmulatedFunction = true; }
    bool getUseEmulatedFunction() { return mUseEmulatedFunction; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const;

  protected:
    TIntermAggregate(const TIntermAggregate &); 
    TIntermAggregate &operator=(const TIntermAggregate &); 
    TIntermSequence mSequence;
    TString mName;
    bool mUserDefined; 

    bool mOptimize;
    bool mDebug;

    
    
    bool mUseEmulatedFunction;
};




class TIntermSelection : public TIntermTyped
{
  public:
    TIntermSelection(TIntermTyped *cond, TIntermNode *trueB, TIntermNode *falseB)
        : TIntermTyped(TType(EbtVoid, EbpUndefined)),
          mCondition(cond),
          mTrueBlock(trueB),
          mFalseBlock(falseB) {}
    TIntermSelection(TIntermTyped *cond, TIntermNode *trueB, TIntermNode *falseB,
                     const TType &type)
        : TIntermTyped(type),
          mCondition(cond),
          mTrueBlock(trueB),
          mFalseBlock(falseB) {}

    virtual void traverse(TIntermTraverser *);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    
    virtual bool hasSideEffects() const { return true; }

    bool usesTernaryOperator() const { return getBasicType() != EbtVoid; }
    TIntermNode *getCondition() const { return mCondition; }
    TIntermNode *getTrueBlock() const { return mTrueBlock; }
    TIntermNode *getFalseBlock() const { return mFalseBlock; }
    TIntermSelection *getAsSelectionNode() { return this; }

    virtual void enqueueChildren(std::queue<TIntermNode *> *nodeQueue) const;

protected:
    TIntermTyped *mCondition;
    TIntermNode *mTrueBlock;
    TIntermNode *mFalseBlock;
};

enum Visit
{
    PreVisit,
    InVisit,
    PostVisit
};









class TIntermTraverser
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    
    TIntermTraverser(bool preVisit = true, bool inVisit = false, bool postVisit = false,
                     bool rightToLeft = false)
        : preVisit(preVisit),
          inVisit(inVisit),
          postVisit(postVisit),
          rightToLeft(rightToLeft),
          mDepth(0),
          mMaxDepth(0) {}
    virtual ~TIntermTraverser() {}

    virtual void visitSymbol(TIntermSymbol *) {}
    virtual void visitRaw(TIntermRaw *) {}
    virtual void visitConstantUnion(TIntermConstantUnion *) {}
    virtual bool visitBinary(Visit, TIntermBinary *) { return true; }
    virtual bool visitUnary(Visit, TIntermUnary *) { return true; }
    virtual bool visitSelection(Visit, TIntermSelection *) { return true; }
    virtual bool visitAggregate(Visit, TIntermAggregate *) { return true; }
    virtual bool visitLoop(Visit, TIntermLoop *) { return true; }
    virtual bool visitBranch(Visit, TIntermBranch *) { return true; }

    int getMaxDepth() const { return mMaxDepth; }

    void incrementDepth(TIntermNode *current)
    {
        mDepth++;
        mMaxDepth = std::max(mMaxDepth, mDepth);
        mPath.push_back(current);
    }

    void decrementDepth()
    {
        mDepth--;
        mPath.pop_back();
    }

    TIntermNode *getParentNode()
    {
        return mPath.size() == 0 ? NULL : mPath.back();
    }

    
    
    static TString hash(const TString& name, ShHashFunction64 hashFunction);

    const bool preVisit;
    const bool inVisit;
    const bool postVisit;
    const bool rightToLeft;

  protected:
    int mDepth;
    int mMaxDepth;

    
    TVector<TIntermNode *> mPath;
};





class TMaxDepthTraverser : public TIntermTraverser
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TMaxDepthTraverser(int depthLimit)
        : TIntermTraverser(true, true, false, false),
          mDepthLimit(depthLimit) { }

    virtual bool visitBinary(Visit, TIntermBinary *) { return depthCheck(); }
    virtual bool visitUnary(Visit, TIntermUnary *) { return depthCheck(); }
    virtual bool visitSelection(Visit, TIntermSelection *) { return depthCheck(); }
    virtual bool visitAggregate(Visit, TIntermAggregate *) { return depthCheck(); }
    virtual bool visitLoop(Visit, TIntermLoop *) { return depthCheck(); }
    virtual bool visitBranch(Visit, TIntermBranch *) { return depthCheck(); }

protected:
    bool depthCheck() const { return mMaxDepth < mDepthLimit; }

    int mDepthLimit;
};

#endif  
