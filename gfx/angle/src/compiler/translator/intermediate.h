














#ifndef __INTERMEDIATE_H
#define __INTERMEDIATE_H

#include "GLSLANG/ShaderLang.h"

#include <algorithm>
#include <queue>
#include "compiler/translator/Common.h"
#include "compiler/translator/Types.h"
#include "compiler/translator/ConstantUnion.h"




enum TOperator {
    EOpNull,            
    EOpSequence,        
    EOpFunctionCall,    
    EOpFunction,        
    EOpParameters,      

    EOpDeclaration,
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

extern const char* getOperatorString(TOperator op);

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




class TIntermNode {
public:
    POOL_ALLOCATOR_NEW_DELETE();
    TIntermNode() {
        
        
        line.first_file = line.last_file = 0;
        line.first_line = line.last_line = 0;
    }
    virtual ~TIntermNode() { }

    const TSourceLoc& getLine() const { return line; }
    void setLine(const TSourceLoc& l) { line = l; }

    virtual void traverse(TIntermTraverser*) = 0;
    virtual TIntermTyped* getAsTyped() { return 0; }
    virtual TIntermConstantUnion* getAsConstantUnion() { return 0; }
    virtual TIntermAggregate* getAsAggregate() { return 0; }
    virtual TIntermBinary* getAsBinaryNode() { return 0; }
    virtual TIntermUnary* getAsUnaryNode() { return 0; }
    virtual TIntermSelection* getAsSelectionNode() { return 0; }
    virtual TIntermSymbol* getAsSymbolNode() { return 0; }
    virtual TIntermLoop* getAsLoopNode() { return 0; }

    
    
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement) = 0;

    
    
    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const = 0;

protected:
    TSourceLoc line;
};




struct TIntermNodePair {
    TIntermNode* node1;
    TIntermNode* node2;
};




class TIntermTyped : public TIntermNode {
public:
    TIntermTyped(const TType& t) : type(t)  { }
    virtual TIntermTyped* getAsTyped() { return this; }

    virtual bool hasSideEffects() const = 0;

    void setType(const TType& t) { type = t; }
    const TType& getType() const { return type; }
    TType* getTypePointer() { return &type; }

    TBasicType getBasicType() const { return type.getBasicType(); }
    TQualifier getQualifier() const { return type.getQualifier(); }
    TPrecision getPrecision() const { return type.getPrecision(); }
    int getCols() const { return type.getCols(); }
    int getRows() const { return type.getRows(); }
    int getNominalSize() const { return type.getNominalSize(); }
    int getSecondarySize() const { return type.getSecondarySize(); }
    
    bool isInterfaceBlock() const { return type.isInterfaceBlock(); }
    bool isMatrix() const { return type.isMatrix(); }
    bool isArray()  const { return type.isArray(); }
    bool isVector() const { return type.isVector(); }
    bool isScalar() const { return type.isScalar(); }
    bool isScalarInt() const { return type.isScalarInt(); }
    const char* getBasicString() const { return type.getBasicString(); }
    const char* getQualifierString() const { return type.getQualifierString(); }
    TString getCompleteString() const { return type.getCompleteString(); }

    int getArraySize() const { return type.getArraySize(); }

protected:
    TType type;
};




enum TLoopType {
    ELoopFor,
    ELoopWhile,
    ELoopDoWhile
};

class TIntermLoop : public TIntermNode {
public:
    TIntermLoop(TLoopType aType,
                TIntermNode *aInit, TIntermTyped* aCond, TIntermTyped* aExpr,
                TIntermNode* aBody) :
            type(aType),
            init(aInit),
            cond(aCond),
            expr(aExpr),
            body(aBody),
            unrollFlag(false) { }

    virtual TIntermLoop* getAsLoopNode() { return this; }
    virtual void traverse(TIntermTraverser*);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    TLoopType getType() const { return type; }
    TIntermNode* getInit() { return init; }
    TIntermTyped* getCondition() { return cond; }
    TIntermTyped* getExpression() { return expr; }
    TIntermNode* getBody() { return body; }

    void setUnrollFlag(bool flag) { unrollFlag = flag; }
    bool getUnrollFlag() { return unrollFlag; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const;

protected:
    TLoopType type;
    TIntermNode* init;  
    TIntermTyped* cond; 
    TIntermTyped* expr; 
    TIntermNode* body;  

    bool unrollFlag; 
};




class TIntermBranch : public TIntermNode {
public:
    TIntermBranch(TOperator op, TIntermTyped* e) :
            flowOp(op),
            expression(e) { }

    virtual void traverse(TIntermTraverser*);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    TOperator getFlowOp() { return flowOp; }
    TIntermTyped* getExpression() { return expression; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const;

protected:
    TOperator flowOp;
    TIntermTyped* expression;  
};




class TIntermSymbol : public TIntermTyped {
public:
    
    
    
    TIntermSymbol(int i, const TString& sym, const TType& t) : 
        TIntermTyped(t), id(i)  { symbol = sym; }

    virtual bool hasSideEffects() const { return false; }

    int getId() const { return id; }
    const TString& getSymbol() const { return symbol; }

    void setId(int newId) { id = newId; }

    virtual void traverse(TIntermTraverser*);
    virtual TIntermSymbol* getAsSymbolNode() { return this; }
    virtual bool replaceChildNode(TIntermNode *, TIntermNode *) { return false; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const {}

protected:
    int id;
    TString symbol;
};

class TIntermConstantUnion : public TIntermTyped {
public:
    TIntermConstantUnion(ConstantUnion *unionPointer, const TType& t) : TIntermTyped(t), unionArrayPointer(unionPointer) { }

    virtual bool hasSideEffects() const { return false; }

    ConstantUnion* getUnionArrayPointer() const { return unionArrayPointer; }
    
    int getIConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getIConst() : 0; }
    unsigned int getUConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getUConst() : 0; }
    float getFConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getFConst() : 0.0f; }
    bool getBConst(size_t index) const { return unionArrayPointer ? unionArrayPointer[index].getBConst() : false; }

    virtual TIntermConstantUnion* getAsConstantUnion()  { return this; }
    virtual void traverse(TIntermTraverser*);
    virtual bool replaceChildNode(TIntermNode *, TIntermNode *) { return false; }

    TIntermTyped* fold(TOperator, TIntermTyped*, TInfoSink&);

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const {}

protected:
    ConstantUnion *unionArrayPointer;
};




class TIntermOperator : public TIntermTyped {
public:
    TOperator getOp() const { return op; }
    void setOp(TOperator o) { op = o; }

    bool isAssignment() const;
    bool isConstructor() const;

    virtual bool hasSideEffects() const { return isAssignment(); }

protected:
    TIntermOperator(TOperator o) : TIntermTyped(TType(EbtFloat, EbpUndefined)), op(o) {}
    TIntermOperator(TOperator o, const TType& t) : TIntermTyped(t), op(o) {}
    TOperator op;
};




class TIntermBinary : public TIntermOperator {
public:
    TIntermBinary(TOperator o) : TIntermOperator(o), addIndexClamp(false) {}

    virtual TIntermBinary* getAsBinaryNode() { return this; }
    virtual void traverse(TIntermTraverser*);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    virtual bool hasSideEffects() const { return (isAssignment() || left->hasSideEffects() || right->hasSideEffects()); }

    void setLeft(TIntermTyped* n) { left = n; }
    void setRight(TIntermTyped* n) { right = n; }
    TIntermTyped* getLeft() const { return left; }
    TIntermTyped* getRight() const { return right; }
    bool promote(TInfoSink&);

    void setAddIndexClamp() { addIndexClamp = true; }
    bool getAddIndexClamp() { return addIndexClamp; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const;

protected:
    TIntermTyped* left;
    TIntermTyped* right;

    
    bool addIndexClamp;
};




class TIntermUnary : public TIntermOperator {
public:
    TIntermUnary(TOperator o, const TType& t) : TIntermOperator(o, t), operand(0), useEmulatedFunction(false) {}
    TIntermUnary(TOperator o) : TIntermOperator(o), operand(0), useEmulatedFunction(false) {}

    virtual void traverse(TIntermTraverser*);
    virtual TIntermUnary* getAsUnaryNode() { return this; }
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    virtual bool hasSideEffects() const { return (isAssignment() || operand->hasSideEffects()); }

    void setOperand(TIntermTyped* o) { operand = o; }
    TIntermTyped* getOperand() { return operand; }    
    bool promote(TInfoSink&);

    void setUseEmulatedFunction() { useEmulatedFunction = true; }
    bool getUseEmulatedFunction() { return useEmulatedFunction; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const;

protected:
    TIntermTyped* operand;

    
    
    bool useEmulatedFunction;
};

typedef TVector<TIntermNode*> TIntermSequence;
typedef TVector<int> TQualifierList;




class TIntermAggregate : public TIntermOperator {
public:
    TIntermAggregate() : TIntermOperator(EOpNull), userDefined(false), useEmulatedFunction(false) { }
    TIntermAggregate(TOperator o) : TIntermOperator(o), useEmulatedFunction(false) { }
    ~TIntermAggregate() { }

    virtual TIntermAggregate* getAsAggregate() { return this; }
    virtual void traverse(TIntermTraverser*);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    
    virtual bool hasSideEffects() const { return true; }

    TIntermSequence& getSequence() { return sequence; }

    void setName(const TString& n) { name = n; }
    const TString& getName() const { return name; }

    void setUserDefined() { userDefined = true; }
    bool isUserDefined() const { return userDefined; }

    void setOptimize(bool o) { optimize = o; }
    bool getOptimize() { return optimize; }
    void setDebug(bool d) { debug = d; }
    bool getDebug() { return debug; }

    void setUseEmulatedFunction() { useEmulatedFunction = true; }
    bool getUseEmulatedFunction() { return useEmulatedFunction; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const;

protected:
    TIntermAggregate(const TIntermAggregate&); 
    TIntermAggregate& operator=(const TIntermAggregate&); 
    TIntermSequence sequence;
    TString name;
    bool userDefined; 

    bool optimize;
    bool debug;

    
    
    bool useEmulatedFunction;
};




class TIntermSelection : public TIntermTyped {
public:
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB) :
            TIntermTyped(TType(EbtVoid, EbpUndefined)), condition(cond), trueBlock(trueB), falseBlock(falseB) {}
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB, const TType& type) :
            TIntermTyped(type), condition(cond), trueBlock(trueB), falseBlock(falseB) {}

    virtual void traverse(TIntermTraverser*);
    virtual bool replaceChildNode(
        TIntermNode *original, TIntermNode *replacement);

    
    virtual bool hasSideEffects() const { return true; }

    bool usesTernaryOperator() const { return getBasicType() != EbtVoid; }
    TIntermNode* getCondition() const { return condition; }
    TIntermNode* getTrueBlock() const { return trueBlock; }
    TIntermNode* getFalseBlock() const { return falseBlock; }
    TIntermSelection* getAsSelectionNode() { return this; }

    virtual void enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const;

protected:
    TIntermTyped* condition;
    TIntermNode* trueBlock;
    TIntermNode* falseBlock;
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
    TIntermTraverser(bool preVisit = true, bool inVisit = false, bool postVisit = false, bool rightToLeft = false) : 
            preVisit(preVisit),
            inVisit(inVisit),
            postVisit(postVisit),
            rightToLeft(rightToLeft),
            depth(0),
            maxDepth(0) {}
    virtual ~TIntermTraverser() {}

    virtual void visitSymbol(TIntermSymbol*) {}
    virtual void visitConstantUnion(TIntermConstantUnion*) {}
    virtual bool visitBinary(Visit visit, TIntermBinary*) {return true;}
    virtual bool visitUnary(Visit visit, TIntermUnary*) {return true;}
    virtual bool visitSelection(Visit visit, TIntermSelection*) {return true;}
    virtual bool visitAggregate(Visit visit, TIntermAggregate*) {return true;}
    virtual bool visitLoop(Visit visit, TIntermLoop*) {return true;}
    virtual bool visitBranch(Visit visit, TIntermBranch*) {return true;}

    int getMaxDepth() const {return maxDepth;}

    void incrementDepth(TIntermNode *current)
    {
        depth++;
        maxDepth = std::max(maxDepth, depth);
        path.push_back(current);
    }

    void decrementDepth()
    {
        depth--;
        path.pop_back();
    }

    TIntermNode *getParentNode()
    {
        return path.size() == 0 ? NULL : path.back();
    }

    
    
    static TString hash(const TString& name, ShHashFunction64 hashFunction);

    const bool preVisit;
    const bool inVisit;
    const bool postVisit;
    const bool rightToLeft;

protected:
    int depth;
    int maxDepth;

    
    TVector<TIntermNode *> path;
};





class TMaxDepthTraverser : public TIntermTraverser
{
public:
    POOL_ALLOCATOR_NEW_DELETE();
    TMaxDepthTraverser(int depthLimit)
      : TIntermTraverser(true, true, false, false),
        depthLimit(depthLimit)
    {}

    virtual bool visitBinary(Visit visit, TIntermBinary*) { return depthCheck(); }
    virtual bool visitUnary(Visit visit, TIntermUnary*) { return depthCheck(); }
    virtual bool visitSelection(Visit visit, TIntermSelection*) { return depthCheck(); }
    virtual bool visitAggregate(Visit visit, TIntermAggregate*) { return depthCheck(); }
    virtual bool visitLoop(Visit visit, TIntermLoop*) { return depthCheck(); }
    virtual bool visitBranch(Visit visit, TIntermBranch*) { return depthCheck(); }

protected:
    int depthLimit;

    bool depthCheck() const { return maxDepth < depthLimit; }
};

#endif 
