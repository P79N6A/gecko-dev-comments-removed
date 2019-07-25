














#ifndef __INTERMEDIATE_H
#define __INTERMEDIATE_H

#include "compiler/Common.h"
#include "compiler/Types.h"
#include "compiler/ConstantUnion.h"




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

    EOpConvIntToBool,
    EOpConvFloatToBool,
    EOpConvBoolToFloat,
    EOpConvIntToFloat,
    EOpConvFloatToInt,
    EOpConvBoolToInt,

    
    
    

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
    EOpDivAssign,
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
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    TIntermNode() : line(0) {}

    TSourceLoc getLine() const { return line; }
    void setLine(TSourceLoc l) { line = l; }

    virtual void traverse(TIntermTraverser*) = 0;
    virtual TIntermTyped* getAsTyped() { return 0; }
    virtual TIntermConstantUnion* getAsConstantUnion() { return 0; }
    virtual TIntermAggregate* getAsAggregate() { return 0; }
    virtual TIntermBinary* getAsBinaryNode() { return 0; }
    virtual TIntermUnary* getAsUnaryNode() { return 0; }
    virtual TIntermSelection* getAsSelectionNode() { return 0; }
    virtual TIntermSymbol* getAsSymbolNode() { return 0; }
    virtual TIntermLoop* getAsLoopNode() { return 0; }
    virtual ~TIntermNode() { }

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

    void setType(const TType& t) { type = t; }
    const TType& getType() const { return type; }
    TType* getTypePointer() { return &type; }

    TBasicType getBasicType() const { return type.getBasicType(); }
    TQualifier getQualifier() const { return type.getQualifier(); }
    TPrecision getPrecision() const { return type.getPrecision(); }
    int getNominalSize() const { return type.getNominalSize(); }
    
    bool isMatrix() const { return type.isMatrix(); }
    bool isArray()  const { return type.isArray(); }
    bool isVector() const { return type.isVector(); }
    bool isScalar() const { return type.isScalar(); }
    const char* getBasicString() const { return type.getBasicString(); }
    const char* getQualifierString() const { return type.getQualifierString(); }
    TString getCompleteString() const { return type.getCompleteString(); }

protected:
    TType type;
};




enum TLoopType {
    ELoopFor,
    ELoopWhile,
    ELoopDoWhile,
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

    TLoopType getType() const { return type; }
    TIntermNode* getInit() { return init; }
    TIntermTyped* getCondition() { return cond; }
    TIntermTyped* getExpression() { return expr; }
    TIntermNode* getBody() { return body; }

    void setUnrollFlag(bool flag) { unrollFlag = flag; }
    bool getUnrollFlag() { return unrollFlag; }

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

    TOperator getFlowOp() { return flowOp; }
    TIntermTyped* getExpression() { return expression; }

protected:
    TOperator flowOp;
    TIntermTyped* expression;  
};




class TIntermSymbol : public TIntermTyped {
public:
    
    
    
    TIntermSymbol(int i, const TString& sym, const TType& t) : 
            TIntermTyped(t), id(i)  { symbol = sym; originalSymbol = sym; } 

    int getId() const { return id; }
    const TString& getSymbol() const { return symbol; }

    void setId(int newId) { id = newId; }
    void setSymbol(const TString& sym) { symbol = sym; }

    const TString& getOriginalSymbol() const { return originalSymbol; }

    virtual void traverse(TIntermTraverser*);
    virtual TIntermSymbol* getAsSymbolNode() { return this; }

protected:
    int id;
    TString symbol;
    TString originalSymbol;
};

class TIntermConstantUnion : public TIntermTyped {
public:
    TIntermConstantUnion(ConstantUnion *unionPointer, const TType& t) : TIntermTyped(t), unionArrayPointer(unionPointer) { }

    ConstantUnion* getUnionArrayPointer() const { return unionArrayPointer; }
    void setUnionArrayPointer(ConstantUnion *c) { unionArrayPointer = c; }

    virtual TIntermConstantUnion* getAsConstantUnion()  { return this; }
    virtual void traverse(TIntermTraverser*);

    TIntermTyped* fold(TOperator, TIntermTyped*, TInfoSink&);

protected:
    ConstantUnion *unionArrayPointer;
};




class TIntermOperator : public TIntermTyped {
public:
    TOperator getOp() const { return op; }
    void setOp(TOperator o) { op = o; }

    bool modifiesState() const;
    bool isConstructor() const;

protected:
    TIntermOperator(TOperator o) : TIntermTyped(TType(EbtFloat, EbpUndefined)), op(o) {}
    TIntermOperator(TOperator o, TType& t) : TIntermTyped(t), op(o) {}   
    TOperator op;
};




class TIntermBinary : public TIntermOperator {
public:
    TIntermBinary(TOperator o) : TIntermOperator(o) {}

    virtual TIntermBinary* getAsBinaryNode() { return this; }
    virtual void traverse(TIntermTraverser*);

    void setLeft(TIntermTyped* n) { left = n; }
    void setRight(TIntermTyped* n) { right = n; }
    TIntermTyped* getLeft() const { return left; }
    TIntermTyped* getRight() const { return right; }
    bool promote(TInfoSink&);

protected:
    TIntermTyped* left;
    TIntermTyped* right;
};




class TIntermUnary : public TIntermOperator {
public:
    TIntermUnary(TOperator o, TType& t) : TIntermOperator(o, t), operand(0) {}
    TIntermUnary(TOperator o) : TIntermOperator(o), operand(0) {}

    virtual void traverse(TIntermTraverser*);
    virtual TIntermUnary* getAsUnaryNode() { return this; }

    void setOperand(TIntermTyped* o) { operand = o; }
    TIntermTyped* getOperand() { return operand; }    
    bool promote(TInfoSink&);

protected:
    TIntermTyped* operand;
};

typedef TVector<TIntermNode*> TIntermSequence;
typedef TVector<int> TQualifierList;
typedef TMap<TString, TString> TPragmaTable;



class TIntermAggregate : public TIntermOperator {
public:
    TIntermAggregate() : TIntermOperator(EOpNull), userDefined(false), pragmaTable(0), endLine(0) { }
    TIntermAggregate(TOperator o) : TIntermOperator(o), pragmaTable(0) { }
    ~TIntermAggregate() { delete pragmaTable; }

    virtual TIntermAggregate* getAsAggregate() { return this; }
    virtual void traverse(TIntermTraverser*);

    TIntermSequence& getSequence() { return sequence; }

    void setName(const TString& n) { name = n; }
    const TString& getName() const { return name; }

    void setUserDefined() { userDefined = true; }
    bool isUserDefined() { return userDefined; }

    void setOptimize(bool o) { optimize = o; }
    bool getOptimize() { return optimize; }
    void setDebug(bool d) { debug = d; }
    bool getDebug() { return debug; }
    void addToPragmaTable(const TPragmaTable& pTable);
    const TPragmaTable& getPragmaTable() const { return *pragmaTable; }
    void setEndLine(TSourceLoc line) { endLine = line; }
    TSourceLoc getEndLine() const { return endLine; }

protected:
    TIntermAggregate(const TIntermAggregate&); 
    TIntermAggregate& operator=(const TIntermAggregate&); 
    TIntermSequence sequence;
    TString name;
    bool userDefined; 

    bool optimize;
    bool debug;
    TPragmaTable *pragmaTable;
    TSourceLoc endLine;
};




class TIntermSelection : public TIntermTyped {
public:
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB) :
            TIntermTyped(TType(EbtVoid, EbpUndefined)), condition(cond), trueBlock(trueB), falseBlock(falseB) {}
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB, const TType& type) :
            TIntermTyped(type), condition(cond), trueBlock(trueB), falseBlock(falseB) {}

    virtual void traverse(TIntermTraverser*);

    bool usesTernaryOperator() const { return getBasicType() != EbtVoid; }
    TIntermNode* getCondition() const { return condition; }
    TIntermNode* getTrueBlock() const { return trueBlock; }
    TIntermNode* getFalseBlock() const { return falseBlock; }
    TIntermSelection* getAsSelectionNode() { return this; }

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
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    TIntermTraverser(bool preVisit = true, bool inVisit = false, bool postVisit = false, bool rightToLeft = false) : 
            preVisit(preVisit),
            inVisit(inVisit),
            postVisit(postVisit),
            rightToLeft(rightToLeft),
            depth(0) {}

    virtual void visitSymbol(TIntermSymbol*) {}
    virtual void visitConstantUnion(TIntermConstantUnion*) {}
    virtual bool visitBinary(Visit visit, TIntermBinary*) {return true;}
    virtual bool visitUnary(Visit visit, TIntermUnary*) {return true;}
    virtual bool visitSelection(Visit visit, TIntermSelection*) {return true;}
    virtual bool visitAggregate(Visit visit, TIntermAggregate*) {return true;}
    virtual bool visitLoop(Visit visit, TIntermLoop*) {return true;}
    virtual bool visitBranch(Visit visit, TIntermBranch*) {return true;}

    void incrementDepth() {depth++;}
    void decrementDepth() {depth--;}

    const bool preVisit;
    const bool inVisit;
    const bool postVisit;
    const bool rightToLeft;

protected:
    int depth;
};

#endif 
