





#include "compiler/ValidateLimitations.h"
#include "compiler/InfoSink.h"
#include "compiler/ParseHelper.h"

namespace {
bool IsLoopIndex(const TIntermSymbol* symbol, const TLoopStack& stack) {
    for (TLoopStack::const_iterator i = stack.begin(); i != stack.end(); ++i) {
        if (i->index.id == symbol->getId())
            return true;
    }
    return false;
}

void MarkLoopForUnroll(const TIntermSymbol* symbol, TLoopStack& stack) {
    for (TLoopStack::iterator i = stack.begin(); i != stack.end(); ++i) {
        if (i->index.id == symbol->getId()) {
            ASSERT(i->loop != NULL);
            i->loop->setUnrollFlag(true);
            return;
        }
    }
    UNREACHABLE();
}










class ValidateConstIndexExpr : public TIntermTraverser {
public:
    ValidateConstIndexExpr(const TLoopStack& stack)
        : mValid(true), mLoopStack(stack) {}

    
    bool isValid() const { return mValid; }

    virtual void visitSymbol(TIntermSymbol* symbol) {
        
        
        if (mValid) {
            mValid = (symbol->getQualifier() == EvqConst) ||
                     IsLoopIndex(symbol, mLoopStack);
        }
    }
    virtual void visitConstantUnion(TIntermConstantUnion*) {}
    virtual bool visitBinary(Visit, TIntermBinary*) { return true; }
    virtual bool visitUnary(Visit, TIntermUnary*) { return true; }
    virtual bool visitSelection(Visit, TIntermSelection*) { return true; }
    virtual bool visitAggregate(Visit, TIntermAggregate*) { return true; }
    virtual bool visitLoop(Visit, TIntermLoop*) { return true; }
    virtual bool visitBranch(Visit, TIntermBranch*) { return true; }

private:
    bool mValid;
    const TLoopStack& mLoopStack;
};




class ValidateLoopIndexExpr : public TIntermTraverser {
public:
    ValidateLoopIndexExpr(TLoopStack& stack)
        : mUsesFloatLoopIndex(false),
          mUsesIntLoopIndex(false),
          mLoopStack(stack) {}

    bool usesFloatLoopIndex() const { return mUsesFloatLoopIndex; }
    bool usesIntLoopIndex() const { return mUsesIntLoopIndex; }

    virtual void visitSymbol(TIntermSymbol* symbol) {
        if (IsLoopIndex(symbol, mLoopStack)) {
            switch (symbol->getBasicType()) {
              case EbtFloat:
                mUsesFloatLoopIndex = true;
                break;
              case EbtInt:
                mUsesIntLoopIndex = true;
                MarkLoopForUnroll(symbol, mLoopStack);
                break;
              default:
                UNREACHABLE();
            }
        }
    }
    virtual void visitConstantUnion(TIntermConstantUnion*) {}
    virtual bool visitBinary(Visit, TIntermBinary*) { return true; }
    virtual bool visitUnary(Visit, TIntermUnary*) { return true; }
    virtual bool visitSelection(Visit, TIntermSelection*) { return true; }
    virtual bool visitAggregate(Visit, TIntermAggregate*) { return true; }
    virtual bool visitLoop(Visit, TIntermLoop*) { return true; }
    virtual bool visitBranch(Visit, TIntermBranch*) { return true; }

private:
    bool mUsesFloatLoopIndex;
    bool mUsesIntLoopIndex;
    TLoopStack& mLoopStack;
};
}  

ValidateLimitations::ValidateLimitations(ShShaderType shaderType,
                                         TInfoSinkBase& sink)
    : mShaderType(shaderType),
      mSink(sink),
      mNumErrors(0)
{
}

void ValidateLimitations::visitSymbol(TIntermSymbol*)
{
}

void ValidateLimitations::visitConstantUnion(TIntermConstantUnion*)
{
}

bool ValidateLimitations::visitBinary(Visit, TIntermBinary* node)
{
    
    validateOperation(node, node->getLeft());

    
    switch (node->getOp()) {
      case EOpIndexDirect:
        validateIndexing(node);
        break;
      case EOpIndexIndirect:
#if defined(__APPLE__)
        
        
        
        
        if ((node->getLeft() != NULL) && (node->getRight() != NULL) &&
            (node->getLeft()->getAsSymbolNode())) {
            TIntermSymbol* symbol = node->getLeft()->getAsSymbolNode();
            if (IsSampler(symbol->getBasicType()) && symbol->isArray()) {
                ValidateLoopIndexExpr validate(mLoopStack);
                node->getRight()->traverse(&validate);
                if (validate.usesFloatLoopIndex()) {
                    error(node->getLine(),
                          "sampler array index is float loop index",
                          "for");
                }
            }
        }
#endif
        validateIndexing(node);
        break;
      default: break;
    }
    return true;
}

bool ValidateLimitations::visitUnary(Visit, TIntermUnary* node)
{
    
    validateOperation(node, node->getOperand());

    return true;
}

bool ValidateLimitations::visitSelection(Visit, TIntermSelection*)
{
    return true;
}

bool ValidateLimitations::visitAggregate(Visit, TIntermAggregate* node)
{
    switch (node->getOp()) {
      case EOpFunctionCall:
        validateFunctionCall(node);
        break;
      default:
        break;
    }
    return true;
}

bool ValidateLimitations::visitLoop(Visit, TIntermLoop* node)
{
    if (!validateLoopType(node))
        return false;

    TLoopInfo info;
    memset(&info, 0, sizeof(TLoopInfo));
    info.loop = node;
    if (!validateForLoopHeader(node, &info))
        return false;

    TIntermNode* body = node->getBody();
    if (body != NULL) {
        mLoopStack.push_back(info);
        body->traverse(this);
        mLoopStack.pop_back();
    }

    
    return false;
}

bool ValidateLimitations::visitBranch(Visit, TIntermBranch*)
{
    return true;
}

void ValidateLimitations::error(TSourceLoc loc,
                                const char *reason, const char* token)
{
    mSink.prefix(EPrefixError);
    mSink.location(loc);
    mSink << "'" << token << "' : " << reason << "\n";
    ++mNumErrors;
}

bool ValidateLimitations::withinLoopBody() const
{
    return !mLoopStack.empty();
}

bool ValidateLimitations::isLoopIndex(const TIntermSymbol* symbol) const
{
    return IsLoopIndex(symbol, mLoopStack);
}

bool ValidateLimitations::validateLoopType(TIntermLoop* node) {
    TLoopType type = node->getType();
    if (type == ELoopFor)
        return true;

    
    error(node->getLine(),
          "This type of loop is not allowed",
          type == ELoopWhile ? "while" : "do");
    return false;
}

bool ValidateLimitations::validateForLoopHeader(TIntermLoop* node,
                                                TLoopInfo* info)
{
    ASSERT(node->getType() == ELoopFor);

    
    
    
    
    if (!validateForLoopInit(node, info))
        return false;
    if (!validateForLoopCond(node, info))
        return false;
    if (!validateForLoopExpr(node, info))
        return false;

    return true;
}

bool ValidateLimitations::validateForLoopInit(TIntermLoop* node,
                                              TLoopInfo* info)
{
    TIntermNode* init = node->getInit();
    if (init == NULL) {
        error(node->getLine(), "Missing init declaration", "for");
        return false;
    }

    
    
    
    
    TIntermAggregate* decl = init->getAsAggregate();
    if ((decl == NULL) || (decl->getOp() != EOpDeclaration)) {
        error(init->getLine(), "Invalid init declaration", "for");
        return false;
    }
    
    TIntermSequence& declSeq = decl->getSequence();
    if (declSeq.size() != 1) {
        error(decl->getLine(), "Invalid init declaration", "for");
        return false;
    }
    TIntermBinary* declInit = declSeq[0]->getAsBinaryNode();
    if ((declInit == NULL) || (declInit->getOp() != EOpInitialize)) {
        error(decl->getLine(), "Invalid init declaration", "for");
        return false;
    }
    TIntermSymbol* symbol = declInit->getLeft()->getAsSymbolNode();
    if (symbol == NULL) {
        error(declInit->getLine(), "Invalid init declaration", "for");
        return false;
    }
    
    TBasicType type = symbol->getBasicType();
    if ((type != EbtInt) && (type != EbtFloat)) {
        error(symbol->getLine(),
              "Invalid type for loop index", getBasicString(type));
        return false;
    }
    
    if (!isConstExpr(declInit->getRight())) {
        error(declInit->getLine(),
              "Loop index cannot be initialized with non-constant expression",
              symbol->getSymbol().c_str());
        return false;
    }

    info->index.id = symbol->getId();
    return true;
}

bool ValidateLimitations::validateForLoopCond(TIntermLoop* node,
                                              TLoopInfo* info)
{
    TIntermNode* cond = node->getCondition();
    if (cond == NULL) {
        error(node->getLine(), "Missing condition", "for");
        return false;
    }
    
    
    
    
    TIntermBinary* binOp = cond->getAsBinaryNode();
    if (binOp == NULL) {
        error(node->getLine(), "Invalid condition", "for");
        return false;
    }
    
    TIntermSymbol* symbol = binOp->getLeft()->getAsSymbolNode();
    if (symbol == NULL) {
        error(binOp->getLine(), "Invalid condition", "for");
        return false;
    }
    if (symbol->getId() != info->index.id) {
        error(symbol->getLine(),
              "Expected loop index", symbol->getSymbol().c_str());
        return false;
    }
    
    switch (binOp->getOp()) {
      case EOpEqual:
      case EOpNotEqual:
      case EOpLessThan:
      case EOpGreaterThan:
      case EOpLessThanEqual:
      case EOpGreaterThanEqual:
        break;
      default:
        error(binOp->getLine(),
              "Invalid relational operator",
              getOperatorString(binOp->getOp()));
        break;
    }
    
    if (!isConstExpr(binOp->getRight())) {
        error(binOp->getLine(),
              "Loop index cannot be compared with non-constant expression",
              symbol->getSymbol().c_str());
        return false;
    }

    return true;
}

bool ValidateLimitations::validateForLoopExpr(TIntermLoop* node,
                                              TLoopInfo* info)
{
    TIntermNode* expr = node->getExpression();
    if (expr == NULL) {
        error(node->getLine(), "Missing expression", "for");
        return false;
    }

    
    
    
    
    
    
    
    
    
    TIntermUnary* unOp = expr->getAsUnaryNode();
    TIntermBinary* binOp = unOp ? NULL : expr->getAsBinaryNode();

    TOperator op = EOpNull;
    TIntermSymbol* symbol = NULL;
    if (unOp != NULL) {
        op = unOp->getOp();
        symbol = unOp->getOperand()->getAsSymbolNode();
    } else if (binOp != NULL) {
        op = binOp->getOp();
        symbol = binOp->getLeft()->getAsSymbolNode();
    }

    
    if (symbol == NULL) {
        error(expr->getLine(), "Invalid expression", "for");
        return false;
    }
    if (symbol->getId() != info->index.id) {
        error(symbol->getLine(),
              "Expected loop index", symbol->getSymbol().c_str());
        return false;
    }

    
    switch (op) {
        case EOpPostIncrement:
        case EOpPostDecrement:
        case EOpPreIncrement:
        case EOpPreDecrement:
            ASSERT((unOp != NULL) && (binOp == NULL));
            break;
        case EOpAddAssign:
        case EOpSubAssign:
            ASSERT((unOp == NULL) && (binOp != NULL));
            break;
        default:
            error(expr->getLine(), "Invalid operator", getOperatorString(op));
            return false;
    }

    
    if (binOp != NULL) {
        if (!isConstExpr(binOp->getRight())) {
            error(binOp->getLine(),
                  "Loop index cannot be modified by non-constant expression",
                  symbol->getSymbol().c_str());
            return false;
        }
    }

    return true;
}

bool ValidateLimitations::validateFunctionCall(TIntermAggregate* node)
{
    ASSERT(node->getOp() == EOpFunctionCall);

    
    if (!withinLoopBody())
        return true;

    
    typedef std::vector<int> ParamIndex;
    ParamIndex pIndex;
    TIntermSequence& params = node->getSequence();
    for (TIntermSequence::size_type i = 0; i < params.size(); ++i) {
        TIntermSymbol* symbol = params[i]->getAsSymbolNode();
        if (symbol && isLoopIndex(symbol))
            pIndex.push_back(i);
    }
    
    
    if (pIndex.empty())
        return true;

    bool valid = true;
    TSymbolTable& symbolTable = GlobalParseContext->symbolTable;
    TSymbol* symbol = symbolTable.find(node->getName());
    ASSERT(symbol && symbol->isFunction());
    TFunction* function = static_cast<TFunction*>(symbol);
    for (ParamIndex::const_iterator i = pIndex.begin();
         i != pIndex.end(); ++i) {
        const TParameter& param = function->getParam(*i);
        TQualifier qual = param.type->getQualifier();
        if ((qual == EvqOut) || (qual == EvqInOut)) {
            error(params[*i]->getLine(),
                  "Loop index cannot be used as argument to a function out or inout parameter",
                  params[*i]->getAsSymbolNode()->getSymbol().c_str());
            valid = false;
        }
    }

    return valid;
}

bool ValidateLimitations::validateOperation(TIntermOperator* node,
                                            TIntermNode* operand) {
    
    if (!withinLoopBody() || !node->modifiesState())
        return true;

    const TIntermSymbol* symbol = operand->getAsSymbolNode();
    if (symbol && isLoopIndex(symbol)) {
        error(node->getLine(),
              "Loop index cannot be statically assigned to within the body of the loop",
              symbol->getSymbol().c_str());
    }
    return true;
}

bool ValidateLimitations::isConstExpr(TIntermNode* node)
{
    ASSERT(node != NULL);
    return node->getAsConstantUnion() != NULL;
}

bool ValidateLimitations::isConstIndexExpr(TIntermNode* node)
{
    ASSERT(node != NULL);

    ValidateConstIndexExpr validate(mLoopStack);
    node->traverse(&validate);
    return validate.isValid();
}

bool ValidateLimitations::validateIndexing(TIntermBinary* node)
{
    ASSERT((node->getOp() == EOpIndexDirect) ||
           (node->getOp() == EOpIndexIndirect));

    bool valid = true;
    TIntermTyped* index = node->getRight();
    
    if (!index->isScalar() || (index->getBasicType() != EbtInt)) {
        error(index->getLine(),
              "Index expression must have integral type",
              index->getCompleteString().c_str());
        valid = false;
    }
    
    
    TIntermTyped* operand = node->getLeft();
    bool skip = (mShaderType == SH_VERTEX_SHADER) &&
                (operand->getQualifier() == EvqUniform);
    if (!skip && !isConstIndexExpr(index)) {
        error(index->getLine(), "Index expression must be constant", "[]");
        valid = false;
    }
    return valid;
}

