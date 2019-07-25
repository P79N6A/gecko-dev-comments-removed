














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
    virtual TSourceLoc getLine() const { return line; }
    virtual void setLine(TSourceLoc l) { line = l; }
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
    virtual TIntermTyped* getAsTyped()         { return this; }
    virtual void setType(const TType& t) { type = t; }
    virtual const TType& getType() const { return type; }
    virtual TType* getTypePointer() { return &type; }

    virtual TBasicType getBasicType() const { return type.getBasicType(); }
    virtual TQualifier getQualifier() const { return type.getQualifier(); }
    virtual TPrecision getPrecision() const { return type.getPrecision(); }
    virtual int getNominalSize() const { return type.getNominalSize(); }
    virtual int getSize() const { return type.getInstanceSize(); }
    virtual bool isMatrix() const { return type.isMatrix(); }
    virtual bool isArray()  const { return type.isArray(); }
    virtual bool isVector() const { return type.isVector(); }
    virtual bool isScalar() const { return type.isScalar(); }
    const char* getBasicString()      const { return type.getBasicString(); }
    const char* getQualifierString()  const { return type.getQualifierString(); }
    TString getCompleteString() const { return type.getCompleteString(); }

protected:
    TType type;
};




class TIntermLoop : public TIntermNode {
public:
    TIntermLoop(TIntermNode *init, TIntermNode* aBody, TIntermTyped* aTest, TIntermTyped* aTerminal, bool testFirst) : 
            init(init),
            body(aBody),
            test(aTest),
            terminal(aTerminal),
            first(testFirst) { }
    virtual TIntermLoop* getAsLoopNode() { return this; }
    virtual void traverse(TIntermTraverser*);
    TIntermNode *getInit() { return init; }
    TIntermNode *getBody() { return body; }
    TIntermTyped *getTest() { return test; }
    TIntermTyped *getTerminal() { return terminal; }
    bool testFirst() { return first; }
protected:
    TIntermNode *init;
    TIntermNode *body;       
    TIntermTyped *test;      
    TIntermTyped *terminal;  
    bool first;              
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
            TIntermTyped(t), id(i)  { symbol = sym;} 
    virtual int getId() const { return id; }
    virtual const TString& getSymbol() const { return symbol;  }
    virtual void traverse(TIntermTraverser*);
    virtual TIntermSymbol* getAsSymbolNode() { return this; }
protected:
    int id;
    TString symbol;
};

class TIntermConstantUnion : public TIntermTyped {
public:
    TIntermConstantUnion(ConstantUnion *unionPointer, const TType& t) : TIntermTyped(t), unionArrayPointer(unionPointer) { }
    ConstantUnion* getUnionArrayPointer() const { return unionArrayPointer; }
    void setUnionArrayPointer(ConstantUnion *c) { unionArrayPointer = c; }
    virtual TIntermConstantUnion* getAsConstantUnion()  { return this; }
    virtual void traverse(TIntermTraverser* );
    virtual TIntermTyped* fold(TOperator, TIntermTyped*, TInfoSink&);
protected:
    ConstantUnion *unionArrayPointer;
};




class TIntermOperator : public TIntermTyped {
public:
    TOperator getOp() const { return op; }
    bool modifiesState() const;
    bool isConstructor() const;
    virtual bool promote(TInfoSink&) { return true; }
protected:
    TIntermOperator(TOperator o) : TIntermTyped(TType(EbtFloat, EbpUndefined)), op(o) {}
    TIntermOperator(TOperator o, TType& t) : TIntermTyped(t), op(o) {}   
    TOperator op;
};




class TIntermBinary : public TIntermOperator {
public:
    TIntermBinary(TOperator o) : TIntermOperator(o) {}
    virtual void traverse(TIntermTraverser*);
    virtual void setLeft(TIntermTyped* n) { left = n; }
    virtual void setRight(TIntermTyped* n) { right = n; }
    virtual TIntermTyped* getLeft() const { return left; }
    virtual TIntermTyped* getRight() const { return right; }
    virtual TIntermBinary* getAsBinaryNode() { return this; }
    virtual bool promote(TInfoSink&);
protected:
    TIntermTyped* left;
    TIntermTyped* right;
};




class TIntermUnary : public TIntermOperator {
public:
    TIntermUnary(TOperator o, TType& t) : TIntermOperator(o, t), operand(0) {}
    TIntermUnary(TOperator o) : TIntermOperator(o), operand(0) {}
    virtual void traverse(TIntermTraverser*);
    virtual void setOperand(TIntermTyped* o) { operand = o; }
    virtual TIntermTyped* getOperand() { return operand; }
    virtual TIntermUnary* getAsUnaryNode() { return this; }
    virtual bool promote(TInfoSink&);
protected:
    TIntermTyped* operand;
};

typedef TVector<TIntermNode*> TIntermSequence;
typedef TVector<int> TQualifierList;



class TIntermAggregate : public TIntermOperator {
public:
    TIntermAggregate() : TIntermOperator(EOpNull), userDefined(false), pragmaTable(0) { }
    TIntermAggregate(TOperator o) : TIntermOperator(o), pragmaTable(0) { }
    ~TIntermAggregate() { delete pragmaTable; }
    virtual TIntermAggregate* getAsAggregate() { return this; }
    virtual void setOperator(TOperator o) { op = o; }
    virtual TIntermSequence& getSequence() { return sequence; }
    virtual void setName(const TString& n) { name = n; }
    virtual const TString& getName() const { return name; }
    virtual void traverse(TIntermTraverser*);
    virtual void setUserDefined() { userDefined = true; }
    virtual bool isUserDefined() { return userDefined; }
    virtual TQualifierList& getQualifier() { return qualifier; }
    void setOptimize(bool o) { optimize = o; }
    void setDebug(bool d) { debug = d; }
    bool getOptimize() { return optimize; }
    bool getDebug() { return debug; }
    void addToPragmaTable(const TPragmaTable& pTable);
    const TPragmaTable& getPragmaTable() const { return *pragmaTable; }
protected:
    TIntermAggregate(const TIntermAggregate&); 
    TIntermAggregate& operator=(const TIntermAggregate&); 
    TIntermSequence sequence;
    TQualifierList qualifier;
    TString name;
    bool userDefined; 
    bool optimize;
    bool debug;
    TPragmaTable *pragmaTable;
};




class TIntermSelection : public TIntermTyped {
public:
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB) :
            TIntermTyped(TType(EbtVoid, EbpUndefined)), condition(cond), trueBlock(trueB), falseBlock(falseB) {}
    TIntermSelection(TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB, const TType& type) :
            TIntermTyped(type), condition(cond), trueBlock(trueB), falseBlock(falseB) {}
    virtual void traverse(TIntermTraverser*);
    bool usesTernaryOperator() const { return getBasicType() != EbtVoid; }
    virtual TIntermNode* getCondition() const { return condition; }
    virtual TIntermNode* getTrueBlock() const { return trueBlock; }
    virtual TIntermNode* getFalseBlock() const { return falseBlock; }
    virtual TIntermSelection* getAsSelectionNode() { return this; }
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
