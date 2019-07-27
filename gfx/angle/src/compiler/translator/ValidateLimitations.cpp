





#include "compiler/translator/ValidateLimitations.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/InitializeParseContext.h"
#include "compiler/translator/ParseContext.h"
#include "angle_gl.h"

namespace
{










class ValidateConstIndexExpr : public TIntermTraverser
{
  public:
    ValidateConstIndexExpr(TLoopStack& stack)
        : mValid(true), mLoopStack(stack) {}

    
    bool isValid() const { return mValid; }

    virtual void visitSymbol(TIntermSymbol *symbol)
    {
        
        
        if (mValid)
        {
            mValid = (symbol->getQualifier() == EvqConst) ||
                     (mLoopStack.findLoop(symbol));
        }
    }

  private:
    bool mValid;
    TLoopStack& mLoopStack;
};

const char *GetOperatorString(TOperator op)
{
    switch (op)
    {
      case EOpInitialize: return "=";
      case EOpAssign: return "=";
      case EOpAddAssign: return "+=";
      case EOpSubAssign: return "-=";
      case EOpDivAssign: return "/=";

      
      case EOpMulAssign:
      case EOpVectorTimesMatrixAssign:
      case EOpVectorTimesScalarAssign:
      case EOpMatrixTimesScalarAssign:
      case EOpMatrixTimesMatrixAssign: return "*=";

      
      case EOpIndexDirect:
      case EOpIndexIndirect: return "[]";

      case EOpIndexDirectStruct:
      case EOpIndexDirectInterfaceBlock: return ".";
      case EOpVectorSwizzle: return ".";
      case EOpAdd: return "+";
      case EOpSub: return "-";
      case EOpMul: return "*";
      case EOpDiv: return "/";
      case EOpMod: UNIMPLEMENTED(); break;
      case EOpEqual: return "==";
      case EOpNotEqual: return "!=";
      case EOpLessThan: return "<";
      case EOpGreaterThan: return ">";
      case EOpLessThanEqual: return "<=";
      case EOpGreaterThanEqual: return ">=";

      
      case EOpVectorTimesScalar:
      case EOpVectorTimesMatrix:
      case EOpMatrixTimesVector:
      case EOpMatrixTimesScalar:
      case EOpMatrixTimesMatrix: return "*";

      case EOpLogicalOr: return "||";
      case EOpLogicalXor: return "^^";
      case EOpLogicalAnd: return "&&";
      case EOpNegative: return "-";
      case EOpVectorLogicalNot: return "not";
      case EOpLogicalNot: return "!";
      case EOpPostIncrement: return "++";
      case EOpPostDecrement: return "--";
      case EOpPreIncrement: return "++";
      case EOpPreDecrement: return "--";

      case EOpRadians: return "radians";
      case EOpDegrees: return "degrees";
      case EOpSin: return "sin";
      case EOpCos: return "cos";
      case EOpTan: return "tan";
      case EOpAsin: return "asin";
      case EOpAcos: return "acos";
      case EOpAtan: return "atan";
      case EOpExp: return "exp";
      case EOpLog: return "log";
      case EOpExp2: return "exp2";
      case EOpLog2: return "log2";
      case EOpSqrt: return "sqrt";
      case EOpInverseSqrt: return "inversesqrt";
      case EOpAbs: return "abs";
      case EOpSign: return "sign";
      case EOpFloor: return "floor";
      case EOpCeil: return "ceil";
      case EOpFract: return "fract";
      case EOpLength: return "length";
      case EOpNormalize: return "normalize";
      case EOpDFdx: return "dFdx";
      case EOpDFdy: return "dFdy";
      case EOpFwidth: return "fwidth";
      case EOpAny: return "any";
      case EOpAll: return "all";

      default: break;
    }
    return "";
}

}  

ValidateLimitations::ValidateLimitations(sh::GLenum shaderType,
                                         TInfoSinkBase &sink)
    : mShaderType(shaderType),
      mSink(sink),
      mNumErrors(0)
{
}

bool ValidateLimitations::visitBinary(Visit, TIntermBinary *node)
{
    
    validateOperation(node, node->getLeft());

    
    switch (node->getOp())
    {
      case EOpIndexDirect:
      case EOpIndexIndirect:
        validateIndexing(node);
        break;
      default:
        break;
    }
    return true;
}

bool ValidateLimitations::visitUnary(Visit, TIntermUnary *node)
{
    
    validateOperation(node, node->getOperand());

    return true;
}

bool ValidateLimitations::visitAggregate(Visit, TIntermAggregate *node)
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

bool ValidateLimitations::visitLoop(Visit, TIntermLoop *node)
{
    if (!validateLoopType(node))
        return false;

    if (!validateForLoopHeader(node))
        return false;

    TIntermNode *body = node->getBody();
    if (body != NULL)
    {
        mLoopStack.push(node);
        body->traverse(this);
        mLoopStack.pop();
    }

    
    return false;
}

void ValidateLimitations::error(TSourceLoc loc,
                                const char *reason, const char *token)
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

bool ValidateLimitations::isLoopIndex(TIntermSymbol *symbol)
{
    return mLoopStack.findLoop(symbol) != NULL;
}

bool ValidateLimitations::validateLoopType(TIntermLoop *node)
{
    TLoopType type = node->getType();
    if (type == ELoopFor)
        return true;

    
    error(node->getLine(),
          "This type of loop is not allowed",
          type == ELoopWhile ? "while" : "do");
    return false;
}

bool ValidateLimitations::validateForLoopHeader(TIntermLoop *node)
{
    ASSERT(node->getType() == ELoopFor);

    
    
    
    
    int indexSymbolId = validateForLoopInit(node);
    if (indexSymbolId < 0)
        return false;
    if (!validateForLoopCond(node, indexSymbolId))
        return false;
    if (!validateForLoopExpr(node, indexSymbolId))
        return false;

    return true;
}

int ValidateLimitations::validateForLoopInit(TIntermLoop *node)
{
    TIntermNode *init = node->getInit();
    if (init == NULL)
    {
        error(node->getLine(), "Missing init declaration", "for");
        return -1;
    }

    
    
    
    
    TIntermAggregate *decl = init->getAsAggregate();
    if ((decl == NULL) || (decl->getOp() != EOpDeclaration))
    {
        error(init->getLine(), "Invalid init declaration", "for");
        return -1;
    }
    
    TIntermSequence *declSeq = decl->getSequence();
    if (declSeq->size() != 1)
    {
        error(decl->getLine(), "Invalid init declaration", "for");
        return -1;
    }
    TIntermBinary *declInit = (*declSeq)[0]->getAsBinaryNode();
    if ((declInit == NULL) || (declInit->getOp() != EOpInitialize))
    {
        error(decl->getLine(), "Invalid init declaration", "for");
        return -1;
    }
    TIntermSymbol *symbol = declInit->getLeft()->getAsSymbolNode();
    if (symbol == NULL)
    {
        error(declInit->getLine(), "Invalid init declaration", "for");
        return -1;
    }
    
    TBasicType type = symbol->getBasicType();
    if ((type != EbtInt) && (type != EbtUInt) && (type != EbtFloat)) {
        error(symbol->getLine(),
              "Invalid type for loop index", getBasicString(type));
        return -1;
    }
    
    if (!isConstExpr(declInit->getRight()))
    {
        error(declInit->getLine(),
              "Loop index cannot be initialized with non-constant expression",
              symbol->getSymbol().c_str());
        return -1;
    }

    return symbol->getId();
}

bool ValidateLimitations::validateForLoopCond(TIntermLoop *node,
                                              int indexSymbolId)
{
    TIntermNode *cond = node->getCondition();
    if (cond == NULL)
    {
        error(node->getLine(), "Missing condition", "for");
        return false;
    }
    
    
    
    
    TIntermBinary *binOp = cond->getAsBinaryNode();
    if (binOp == NULL)
    {
        error(node->getLine(), "Invalid condition", "for");
        return false;
    }
    
    TIntermSymbol *symbol = binOp->getLeft()->getAsSymbolNode();
    if (symbol == NULL)
    {
        error(binOp->getLine(), "Invalid condition", "for");
        return false;
    }
    if (symbol->getId() != indexSymbolId)
    {
        error(symbol->getLine(),
              "Expected loop index", symbol->getSymbol().c_str());
        return false;
    }
    
    switch (binOp->getOp())
    {
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
              GetOperatorString(binOp->getOp()));
        break;
    }
    
    if (!isConstExpr(binOp->getRight()))
    {
        error(binOp->getLine(),
              "Loop index cannot be compared with non-constant expression",
              symbol->getSymbol().c_str());
        return false;
    }

    return true;
}

bool ValidateLimitations::validateForLoopExpr(TIntermLoop *node,
                                              int indexSymbolId)
{
    TIntermNode *expr = node->getExpression();
    if (expr == NULL)
    {
        error(node->getLine(), "Missing expression", "for");
        return false;
    }

    
    
    
    
    
    
    
    
    
    TIntermUnary *unOp = expr->getAsUnaryNode();
    TIntermBinary *binOp = unOp ? NULL : expr->getAsBinaryNode();

    TOperator op = EOpNull;
    TIntermSymbol *symbol = NULL;
    if (unOp != NULL)
    {
        op = unOp->getOp();
        symbol = unOp->getOperand()->getAsSymbolNode();
    }
    else if (binOp != NULL)
    {
        op = binOp->getOp();
        symbol = binOp->getLeft()->getAsSymbolNode();
    }

    
    if (symbol == NULL)
    {
        error(expr->getLine(), "Invalid expression", "for");
        return false;
    }
    if (symbol->getId() != indexSymbolId)
    {
        error(symbol->getLine(),
              "Expected loop index", symbol->getSymbol().c_str());
        return false;
    }

    
    switch (op)
    {
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
        error(expr->getLine(), "Invalid operator", GetOperatorString(op));
        return false;
    }

    
    if (binOp != NULL)
    {
        if (!isConstExpr(binOp->getRight()))
        {
            error(binOp->getLine(),
                  "Loop index cannot be modified by non-constant expression",
                  symbol->getSymbol().c_str());
            return false;
        }
    }

    return true;
}

bool ValidateLimitations::validateFunctionCall(TIntermAggregate *node)
{
    ASSERT(node->getOp() == EOpFunctionCall);

    
    if (!withinLoopBody())
        return true;

    
    typedef std::vector<size_t> ParamIndex;
    ParamIndex pIndex;
    TIntermSequence *params = node->getSequence();
    for (TIntermSequence::size_type i = 0; i < params->size(); ++i)
    {
        TIntermSymbol *symbol = (*params)[i]->getAsSymbolNode();
        if (symbol && isLoopIndex(symbol))
            pIndex.push_back(i);
    }
    
    
    if (pIndex.empty())
        return true;

    bool valid = true;
    TSymbolTable& symbolTable = GetGlobalParseContext()->symbolTable;
    TSymbol* symbol = symbolTable.find(node->getName(), GetGlobalParseContext()->shaderVersion);
    ASSERT(symbol && symbol->isFunction());
    TFunction *function = static_cast<TFunction *>(symbol);
    for (ParamIndex::const_iterator i = pIndex.begin();
         i != pIndex.end(); ++i)
    {
        const TParameter &param = function->getParam(*i);
        TQualifier qual = param.type->getQualifier();
        if ((qual == EvqOut) || (qual == EvqInOut))
        {
            error((*params)[*i]->getLine(),
                  "Loop index cannot be used as argument to a function out or inout parameter",
                  (*params)[*i]->getAsSymbolNode()->getSymbol().c_str());
            valid = false;
        }
    }

    return valid;
}

bool ValidateLimitations::validateOperation(TIntermOperator *node,
                                            TIntermNode* operand)
{
    
    if (!withinLoopBody() || !node->isAssignment())
        return true;

    TIntermSymbol *symbol = operand->getAsSymbolNode();
    if (symbol && isLoopIndex(symbol))
    {
        error(node->getLine(),
              "Loop index cannot be statically assigned to within the body of the loop",
              symbol->getSymbol().c_str());
    }
    return true;
}

bool ValidateLimitations::isConstExpr(TIntermNode *node)
{
    ASSERT(node != NULL);
    return node->getAsConstantUnion() != NULL;
}

bool ValidateLimitations::isConstIndexExpr(TIntermNode *node)
{
    ASSERT(node != NULL);

    ValidateConstIndexExpr validate(mLoopStack);
    node->traverse(&validate);
    return validate.isValid();
}

bool ValidateLimitations::validateIndexing(TIntermBinary *node)
{
    ASSERT((node->getOp() == EOpIndexDirect) ||
           (node->getOp() == EOpIndexIndirect));

    bool valid = true;
    TIntermTyped *index = node->getRight();
    
    if (!index->isScalarInt()) {
        error(index->getLine(),
              "Index expression must have integral type",
              index->getCompleteString().c_str());
        valid = false;
    }
    
    
    TIntermTyped *operand = node->getLeft();
    bool skip = (mShaderType == GL_VERTEX_SHADER) &&
                (operand->getQualifier() == EvqUniform);
    if (!skip && !isConstIndexExpr(index))
    {
        error(index->getLine(), "Index expression must be constant", "[]");
        valid = false;
    }
    return valid;
}

