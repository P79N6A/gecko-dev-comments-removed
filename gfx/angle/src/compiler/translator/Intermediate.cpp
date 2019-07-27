









#include <float.h>
#include <limits.h>
#include <algorithm>

#include "compiler/translator/HashNames.h"
#include "compiler/translator/localintermediate.h"
#include "compiler/translator/QualifierAlive.h"
#include "compiler/translator/RemoveTree.h"
#include "compiler/translator/SymbolTable.h"

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray);

static TPrecision GetHigherPrecision(TPrecision left, TPrecision right)
{
    return left > right ? left : right;
}

const char* getOperatorString(TOperator op)
{
    switch (op) {
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














TIntermSymbol* TIntermediate::addSymbol(int id, const TString& name, const TType& type, const TSourceLoc& line)
{
    TIntermSymbol* node = new TIntermSymbol(id, name, type);
    node->setLine(line);

    return node;
}






TIntermTyped* TIntermediate::addBinaryMath(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc& line)
{
    switch (op) {
        case EOpEqual:
        case EOpNotEqual:
            if (left->isArray())
                return 0;
            break;
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
            if (left->isMatrix() || left->isArray() || left->isVector() || left->getBasicType() == EbtStruct) {
                return 0;
            }
            break;
        case EOpLogicalOr:
        case EOpLogicalXor:
        case EOpLogicalAnd:
            if (left->getBasicType() != EbtBool || left->isMatrix() || left->isArray() || left->isVector()) {
                return 0;
            }
            break;
        case EOpAdd:
        case EOpSub:
        case EOpDiv:
        case EOpMul:
            if (left->getBasicType() == EbtStruct || left->getBasicType() == EbtBool)
                return 0;
        default: break;
    }

    if (left->getBasicType() != right->getBasicType())
    {
        return 0;
    }

    
    
    
    
    TIntermBinary* node = new TIntermBinary(op);
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (!node->promote(infoSink))
        return 0;

    
    
    
    TIntermConstantUnion *leftTempConstant = left->getAsConstantUnion();
    TIntermConstantUnion *rightTempConstant = right->getAsConstantUnion();
    if (leftTempConstant && rightTempConstant) {
        TIntermTyped *typedReturnNode = leftTempConstant->fold(node->getOp(), rightTempConstant, infoSink);

        if (typedReturnNode)
            return typedReturnNode;
    }

    return node;
}






TIntermTyped* TIntermediate::addAssign(TOperator op, TIntermTyped* left, TIntermTyped* right, const TSourceLoc& line)
{
    if (left->getType().getStruct() || right->getType().getStruct())
    {
        if (left->getType() != right->getType())
        {
            return 0;
        }
    }

    TIntermBinary* node = new TIntermBinary(op);
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (! node->promote(infoSink))
        return 0;

    return node;
}








TIntermTyped* TIntermediate::addIndex(TOperator op, TIntermTyped* base, TIntermTyped* index, const TSourceLoc& line)
{
    TIntermBinary* node = new TIntermBinary(op);
    node->setLine(line);
    node->setLeft(base);
    node->setRight(index);

    

    return node;
}






TIntermTyped* TIntermediate::addUnaryMath(TOperator op, TIntermNode* childNode, const TSourceLoc& line)
{
    TIntermUnary* node;
    TIntermTyped* child = childNode->getAsTyped();

    if (child == 0) {
        infoSink.info.message(EPrefixInternalError, line, "Bad type in AddUnaryMath");
        return 0;
    }

    switch (op) {
        case EOpLogicalNot:
            if (child->getType().getBasicType() != EbtBool || child->getType().isMatrix() || child->getType().isArray() || child->getType().isVector()) {
                return 0;
            }
            break;

        case EOpPostIncrement:
        case EOpPreIncrement:
        case EOpPostDecrement:
        case EOpPreDecrement:
        case EOpNegative:
            if (child->getType().getBasicType() == EbtStruct || child->getType().isArray())
                return 0;
        default: break;
    }

    TIntermConstantUnion *childTempConstant = 0;
    if (child->getAsConstantUnion())
        childTempConstant = child->getAsConstantUnion();

    
    
    
    node = new TIntermUnary(op);
    node->setLine(line);
    node->setOperand(child);

    if (! node->promote(infoSink))
        return 0;

    if (childTempConstant)  {
        TIntermTyped* newChild = childTempConstant->fold(op, 0, infoSink);

        if (newChild)
            return newChild;
    }

    return node;
}











TIntermAggregate* TIntermediate::setAggregateOperator(TIntermNode* node, TOperator op, const TSourceLoc& line)
{
    TIntermAggregate* aggNode;

    
    
    
    if (node) {
        aggNode = node->getAsAggregate();
        if (aggNode == 0 || aggNode->getOp() != EOpNull) {
            
            
            
            aggNode = new TIntermAggregate();
            aggNode->getSequence().push_back(node);
        }
    } else
        aggNode = new TIntermAggregate();

    
    
    
    aggNode->setOp(op);
    aggNode->setLine(line);

    return aggNode;
}








TIntermAggregate* TIntermediate::growAggregate(TIntermNode* left, TIntermNode* right, const TSourceLoc& line)
{
    if (left == 0 && right == 0)
        return 0;

    TIntermAggregate* aggNode = 0;
    if (left)
        aggNode = left->getAsAggregate();
    if (!aggNode || aggNode->getOp() != EOpNull) {
        aggNode = new TIntermAggregate;
        if (left)
            aggNode->getSequence().push_back(left);
    }

    if (right)
        aggNode->getSequence().push_back(right);

    aggNode->setLine(line);

    return aggNode;
}






TIntermAggregate* TIntermediate::makeAggregate(TIntermNode* node, const TSourceLoc& line)
{
    if (node == 0)
        return 0;

    TIntermAggregate* aggNode = new TIntermAggregate;
    aggNode->getSequence().push_back(node);

    aggNode->setLine(line);

    return aggNode;
}








TIntermNode* TIntermediate::addSelection(TIntermTyped* cond, TIntermNodePair nodePair, const TSourceLoc& line)
{
    
    
    
    

    if (cond->getAsTyped() && cond->getAsTyped()->getAsConstantUnion()) {
        if (cond->getAsConstantUnion()->getBConst(0) == true)
            return nodePair.node1 ? setAggregateOperator(nodePair.node1, EOpSequence, nodePair.node1->getLine()) : NULL;
        else
            return nodePair.node2 ? setAggregateOperator(nodePair.node2, EOpSequence, nodePair.node2->getLine()) : NULL;
    }

    TIntermSelection* node = new TIntermSelection(cond, nodePair.node1, nodePair.node2);
    node->setLine(line);

    return node;
}


TIntermTyped* TIntermediate::addComma(TIntermTyped* left, TIntermTyped* right, const TSourceLoc& line)
{
    if (left->getType().getQualifier() == EvqConst && right->getType().getQualifier() == EvqConst) {
        return right;
    } else {
        TIntermTyped *commaAggregate = growAggregate(left, right, line);
        commaAggregate->getAsAggregate()->setOp(EOpComma);
        commaAggregate->setType(right->getType());
        commaAggregate->getTypePointer()->setQualifier(EvqTemporary);
        return commaAggregate;
    }
}








TIntermTyped* TIntermediate::addSelection(TIntermTyped* cond, TIntermTyped* trueBlock, TIntermTyped* falseBlock, const TSourceLoc& line)
{
    if (trueBlock->getType() != falseBlock->getType())
    {
        return 0;
    }

    
    
    

    if (cond->getAsConstantUnion() && trueBlock->getAsConstantUnion() && falseBlock->getAsConstantUnion()) {
        if (cond->getAsConstantUnion()->getBConst(0))
            return trueBlock;
        else
            return falseBlock;
    }

    
    
    
    TIntermSelection* node = new TIntermSelection(cond, trueBlock, falseBlock, trueBlock->getType());
    node->getTypePointer()->setQualifier(EvqTemporary);
    node->setLine(line);

    return node;
}







TIntermConstantUnion* TIntermediate::addConstantUnion(ConstantUnion* unionArrayPointer, const TType& t, const TSourceLoc& line)
{
    TIntermConstantUnion* node = new TIntermConstantUnion(unionArrayPointer, t);
    node->setLine(line);

    return node;
}

TIntermTyped* TIntermediate::addSwizzle(TVectorFields& fields, const TSourceLoc& line)
{

    TIntermAggregate* node = new TIntermAggregate(EOpSequence);

    node->setLine(line);
    TIntermConstantUnion* constIntNode;
    TIntermSequence &sequenceVector = node->getSequence();
    ConstantUnion* unionArray;

    for (int i = 0; i < fields.num; i++) {
        unionArray = new ConstantUnion[1];
        unionArray->setIConst(fields.offsets[i]);
        constIntNode = addConstantUnion(unionArray, TType(EbtInt, EbpUndefined, EvqConst), line);
        sequenceVector.push_back(constIntNode);
    }

    return node;
}




TIntermNode* TIntermediate::addLoop(TLoopType type, TIntermNode* init, TIntermTyped* cond, TIntermTyped* expr, TIntermNode* body, const TSourceLoc& line)
{
    TIntermNode* node = new TIntermLoop(type, init, cond, expr, body);
    node->setLine(line);

    return node;
}




TIntermBranch* TIntermediate::addBranch(TOperator branchOp, const TSourceLoc& line)
{
    return addBranch(branchOp, 0, line);
}

TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TIntermTyped* expression, const TSourceLoc& line)
{
    TIntermBranch* node = new TIntermBranch(branchOp, expression);
    node->setLine(line);

    return node;
}





bool TIntermediate::postProcess(TIntermNode* root)
{
    if (root == 0)
        return true;

    
    
    
    TIntermAggregate* aggRoot = root->getAsAggregate();
    if (aggRoot && aggRoot->getOp() == EOpNull)
        aggRoot->setOp(EOpSequence);

    return true;
}




void TIntermediate::remove(TIntermNode* root)
{
    if (root)
        RemoveAllTreeNodes(root);
}







#define REPLACE_IF_IS(node, type, original, replacement) \
    if (node == original) { \
        node = static_cast<type *>(replacement); \
        return true; \
    }

bool TIntermLoop::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(init, TIntermNode, original, replacement);
    REPLACE_IF_IS(cond, TIntermTyped, original, replacement);
    REPLACE_IF_IS(expr, TIntermTyped, original, replacement);
    REPLACE_IF_IS(body, TIntermNode, original, replacement);
    return false;
}

void TIntermLoop::enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const
{
    if (init)
    {
        nodeQueue->push(init);
    }
    if (cond)
    {
        nodeQueue->push(cond);
    }
    if (expr)
    {
        nodeQueue->push(expr);
    }
    if (body)
    {
        nodeQueue->push(body);
    }
}

bool TIntermBranch::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(expression, TIntermTyped, original, replacement);
    return false;
}

void TIntermBranch::enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const
{
    if (expression)
    {
        nodeQueue->push(expression);
    }
}

bool TIntermBinary::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(left, TIntermTyped, original, replacement);
    REPLACE_IF_IS(right, TIntermTyped, original, replacement);
    return false;
}

void TIntermBinary::enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const
{
    if (left)
    {
        nodeQueue->push(left);
    }
    if (right)
    {
        nodeQueue->push(right);
    }
}

bool TIntermUnary::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(operand, TIntermTyped, original, replacement);
    return false;
}

void TIntermUnary::enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const
{
    if (operand)
    {
        nodeQueue->push(operand);
    }
}

bool TIntermAggregate::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    for (size_t ii = 0; ii < sequence.size(); ++ii)
    {
        REPLACE_IF_IS(sequence[ii], TIntermNode, original, replacement);
    }
    return false;
}

void TIntermAggregate::enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const
{
    for (size_t childIndex = 0; childIndex < sequence.size(); childIndex++)
    {
        nodeQueue->push(sequence[childIndex]);
    }
}

bool TIntermSelection::replaceChildNode(
    TIntermNode *original, TIntermNode *replacement)
{
    REPLACE_IF_IS(condition, TIntermTyped, original, replacement);
    REPLACE_IF_IS(trueBlock, TIntermNode, original, replacement);
    REPLACE_IF_IS(falseBlock, TIntermNode, original, replacement);
    return false;
}

void TIntermSelection::enqueueChildren(std::queue<TIntermNode*> *nodeQueue) const
{
    if (condition)
    {
        nodeQueue->push(condition);
    }
    if (trueBlock)
    {
        nodeQueue->push(trueBlock);
    }
    if (falseBlock)
    {
        nodeQueue->push(falseBlock);
    }
}




bool TIntermOperator::isAssignment() const
{
    switch (op) {
        case EOpPostIncrement:
        case EOpPostDecrement:
        case EOpPreIncrement:
        case EOpPreDecrement:
        case EOpAssign:
        case EOpAddAssign:
        case EOpSubAssign:
        case EOpMulAssign:
        case EOpVectorTimesMatrixAssign:
        case EOpVectorTimesScalarAssign:
        case EOpMatrixTimesScalarAssign:
        case EOpMatrixTimesMatrixAssign:
        case EOpDivAssign:
            return true;
        default:
            return false;
    }
}




bool TIntermOperator::isConstructor() const
{
    switch (op) {
        case EOpConstructVec2:
        case EOpConstructVec3:
        case EOpConstructVec4:
        case EOpConstructMat2:
        case EOpConstructMat3:
        case EOpConstructMat4:
        case EOpConstructFloat:
        case EOpConstructIVec2:
        case EOpConstructIVec3:
        case EOpConstructIVec4:
        case EOpConstructInt:
        case EOpConstructUVec2:
        case EOpConstructUVec3:
        case EOpConstructUVec4:
        case EOpConstructUInt:
        case EOpConstructBVec2:
        case EOpConstructBVec3:
        case EOpConstructBVec4:
        case EOpConstructBool:
        case EOpConstructStruct:
            return true;
        default:
            return false;
    }
}







bool TIntermUnary::promote(TInfoSink&)
{
    switch (op) {
        case EOpLogicalNot:
            if (operand->getBasicType() != EbtBool)
                return false;
            break;
        case EOpNegative:
        case EOpPostIncrement:
        case EOpPostDecrement:
        case EOpPreIncrement:
        case EOpPreDecrement:
            if (operand->getBasicType() == EbtBool)
                return false;
            break;

            
        case EOpAny:
        case EOpAll:
        case EOpVectorLogicalNot:
            return true;

        default:
            if (operand->getBasicType() != EbtFloat)
                return false;
    }

    setType(operand->getType());
    type.setQualifier(EvqTemporary);

    return true;
}

bool validateMultiplication(TOperator op, const TType &left, const TType &right)
{
    switch (op)
    {
      case EOpMul:
      case EOpMulAssign:
        return left.getNominalSize() == right.getNominalSize() && left.getSecondarySize() == right.getSecondarySize();
      case EOpVectorTimesScalar:
      case EOpVectorTimesScalarAssign:
        return true;
      case EOpVectorTimesMatrix:
        return left.getNominalSize() == right.getRows();
      case EOpVectorTimesMatrixAssign:
        return left.getNominalSize() == right.getRows() && left.getNominalSize() == right.getCols();
      case EOpMatrixTimesVector:
        return left.getCols() == right.getNominalSize();
      case EOpMatrixTimesScalar:
      case EOpMatrixTimesScalarAssign:
        return true;
      case EOpMatrixTimesMatrix:
        return left.getCols() == right.getRows();
      case EOpMatrixTimesMatrixAssign:
        return left.getCols() == right.getCols() && left.getRows() == right.getRows();

      default:
        UNREACHABLE();
        return false;
    }
}







bool TIntermBinary::promote(TInfoSink& infoSink)
{
    
    if (left->isArray() || right->isArray())
    {
        infoSink.info.message(EPrefixInternalError, getLine(), "Invalid operation for arrays");
        return false;
    }

    
    
    if (left->getBasicType() != right->getBasicType())
    {
        return false;
    }

    
    
    
    
    setType(left->getType());

    
    TPrecision higherPrecision = GetHigherPrecision(left->getPrecision(), right->getPrecision());
    getTypePointer()->setPrecision(higherPrecision);

    
    
    if (left->getQualifier() != EvqConst || right->getQualifier() != EvqConst)
    {
        getTypePointer()->setQualifier(EvqTemporary);
    }

    const int nominalSize = std::max(left->getNominalSize(), right->getNominalSize());

    
    
    
    if (nominalSize == 1)
    {
        switch (op)
        {
            
            
            
            case EOpEqual:
            case EOpNotEqual:
            case EOpLessThan:
            case EOpGreaterThan:
            case EOpLessThanEqual:
            case EOpGreaterThanEqual:
                setType(TType(EbtBool, EbpUndefined));
                break;

            
            
            
            case EOpLogicalAnd:
            case EOpLogicalOr:
                
                if (left->getBasicType() != EbtBool || right->getBasicType() != EbtBool)
                {
                    return false;
                }
                setType(TType(EbtBool, EbpUndefined));
                break;

            default:
                break;
        }
        return true;
    }

    
    
    
    
    TBasicType basicType = left->getBasicType();
    switch (op)
    {
        case EOpMul:
            if (!left->isMatrix() && right->isMatrix())
            {
                if (left->isVector())
                {
                    op = EOpVectorTimesMatrix;
                    setType(TType(basicType, higherPrecision, EvqTemporary, right->getCols(), 1));
                }
                else
                {
                    op = EOpMatrixTimesScalar;
                    setType(TType(basicType, higherPrecision, EvqTemporary, right->getCols(), right->getRows()));
                }
            }
            else if (left->isMatrix() && !right->isMatrix())
            {
                if (right->isVector())
                {
                    op = EOpMatrixTimesVector;
                    setType(TType(basicType, higherPrecision, EvqTemporary, left->getRows(), 1));
                }
                else
                {
                    op = EOpMatrixTimesScalar;
                }
            }
            else if (left->isMatrix() && right->isMatrix())
            {
                op = EOpMatrixTimesMatrix;
                setType(TType(basicType, higherPrecision, EvqTemporary, right->getCols(), left->getRows()));
            }
            else if (!left->isMatrix() && !right->isMatrix())
            {
                if (left->isVector() && right->isVector())
                {
                    
                }
                else if (left->isVector() || right->isVector())
                {
                    op = EOpVectorTimesScalar;
                    setType(TType(basicType, higherPrecision, EvqTemporary, nominalSize, 1));
                }
            }
            else
            {
                infoSink.info.message(EPrefixInternalError, getLine(), "Missing elses");
                return false;
            }

            if (!validateMultiplication(op, left->getType(), right->getType()))
            {
                return false;
            }
            break;

        case EOpMulAssign:
            if (!left->isMatrix() && right->isMatrix())
            {
                if (left->isVector())
                {
                    op = EOpVectorTimesMatrixAssign;
                }
                else
                {
                    return false;
                }
            }
            else if (left->isMatrix() && !right->isMatrix())
            {
                if (right->isVector())
                {
                    return false;
                }
                else
                {
                    op = EOpMatrixTimesScalarAssign;
                }
            }
            else if (left->isMatrix() && right->isMatrix())
            {
                op = EOpMatrixTimesMatrixAssign;
                setType(TType(basicType, higherPrecision, EvqTemporary, right->getCols(), left->getRows()));
            }
            else if (!left->isMatrix() && !right->isMatrix())
            {
                if (left->isVector() && right->isVector())
                {
                    
                }
                else if (left->isVector() || right->isVector())
                {
                    if (! left->isVector())
                        return false;
                    op = EOpVectorTimesScalarAssign;
                    setType(TType(basicType, higherPrecision, EvqTemporary, left->getNominalSize(), 1));
                }
            }
            else
            {
                infoSink.info.message(EPrefixInternalError, getLine(), "Missing elses");
                return false;
            }

            if (!validateMultiplication(op, left->getType(), right->getType()))
            {
                return false;
            }
            break;

        case EOpAssign:
        case EOpInitialize:
        case EOpAdd:
        case EOpSub:
        case EOpDiv:
        case EOpAddAssign:
        case EOpSubAssign:
        case EOpDivAssign:
            {
                if ((left->isMatrix() && right->isVector()) ||
                    (left->isVector() && right->isMatrix()))
                    return false;

                
                if (left->getNominalSize() != right->getNominalSize() || left->getSecondarySize() != right->getSecondarySize())
                {
                    
                    
                    if (!left->isScalar() && !right->isScalar())
                        return false;

                    
                    if (op == EOpAssign || op == EOpInitialize)
                        return false;
                }

                const int secondarySize = std::max(left->getSecondarySize(), right->getSecondarySize());

                setType(TType(basicType, higherPrecision, EvqTemporary, nominalSize, secondarySize));
            }
            break;

        case EOpEqual:
        case EOpNotEqual:
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
            if ((left->getNominalSize() != right->getNominalSize()) ||
                (left->getSecondarySize() != right->getSecondarySize()))
                return false;
            setType(TType(EbtBool, EbpUndefined));
            break;

        default:
            return false;
    }
    
    return true;
}

bool CompareStruct(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray)
{
    const TFieldList& fields = leftNodeType.getStruct()->fields();

    size_t structSize = fields.size();
    size_t index = 0;

    for (size_t j = 0; j < structSize; j++) {
        size_t size = fields[j]->type()->getObjectSize();
        for (size_t i = 0; i < size; i++) {
            if (fields[j]->type()->getBasicType() == EbtStruct) {
                if (!CompareStructure(*fields[j]->type(), &rightUnionArray[index], &leftUnionArray[index]))
                    return false;
            } else {
                if (leftUnionArray[index] != rightUnionArray[index])
                    return false;
                index++;
            }

        }
    }
    return true;
}

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray)
{
    if (leftNodeType.isArray()) {
        TType typeWithoutArrayness = leftNodeType;
        typeWithoutArrayness.clearArrayness();

        size_t arraySize = leftNodeType.getArraySize();

        for (size_t i = 0; i < arraySize; ++i) {
            size_t offset = typeWithoutArrayness.getObjectSize() * i;
            if (!CompareStruct(typeWithoutArrayness, &rightUnionArray[offset], &leftUnionArray[offset]))
                return false;
        }
    } else
        return CompareStruct(leftNodeType, rightUnionArray, leftUnionArray);

    return true;
}








TIntermTyped* TIntermConstantUnion::fold(TOperator op, TIntermTyped* constantNode, TInfoSink& infoSink)
{
    ConstantUnion *unionArray = getUnionArrayPointer();

    if (!unionArray)
        return 0;

    size_t objectSize = getType().getObjectSize();

    if (constantNode)
    {
        
        TIntermConstantUnion *node = constantNode->getAsConstantUnion();
        ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
        TType returnType = getType();

        if (!rightUnionArray)
            return 0;

        
        if (constantNode->getType().getObjectSize() == 1 && objectSize > 1)
        {
            rightUnionArray = new ConstantUnion[objectSize];
            for (size_t i = 0; i < objectSize; ++i)
            {
                rightUnionArray[i] = *node->getUnionArrayPointer();
            }
            returnType = getType();
        }
        else if (constantNode->getType().getObjectSize() > 1 && objectSize == 1)
        {
            
            unionArray = new ConstantUnion[constantNode->getType().getObjectSize()];
            for (size_t i = 0; i < constantNode->getType().getObjectSize(); ++i)
            {
                unionArray[i] = *getUnionArrayPointer();
            }
            returnType = node->getType();
            objectSize = constantNode->getType().getObjectSize();
        }

        ConstantUnion* tempConstArray = 0;
        TIntermConstantUnion *tempNode;

        bool boolNodeFlag = false;
        switch(op) {
          case EOpAdd:
            tempConstArray = new ConstantUnion[objectSize];
            {
                for (size_t i = 0; i < objectSize; i++)
                    tempConstArray[i] = unionArray[i] + rightUnionArray[i];
            }
            break;
          case EOpSub:
            tempConstArray = new ConstantUnion[objectSize];
            {
                for (size_t i = 0; i < objectSize; i++)
                    tempConstArray[i] = unionArray[i] - rightUnionArray[i];
            }
            break;

          case EOpMul:
          case EOpVectorTimesScalar:
          case EOpMatrixTimesScalar:
            tempConstArray = new ConstantUnion[objectSize];
            {
                for (size_t i = 0; i < objectSize; i++)
                    tempConstArray[i] = unionArray[i] * rightUnionArray[i];
            }
            break;

          case EOpMatrixTimesMatrix:
            {
                if (getType().getBasicType() != EbtFloat || node->getBasicType() != EbtFloat)
                {
                    infoSink.info.message(EPrefixInternalError, getLine(), "Constant Folding cannot be done for matrix multiply");
                    return 0;
                }

                const int leftCols = getCols();
                const int leftRows = getRows();
                const int rightCols = constantNode->getType().getCols();
                const int rightRows = constantNode->getType().getRows();
                const int resultCols = rightCols;
                const int resultRows = leftRows;

                tempConstArray = new ConstantUnion[resultCols*resultRows];
                for (int row = 0; row < resultRows; row++)
                {
                    for (int column = 0; column < resultCols; column++)
                    {
                        tempConstArray[resultRows * column + row].setFConst(0.0f);
                        for (int i = 0; i < leftCols; i++)
                        {
                            tempConstArray[resultRows * column + row].setFConst(tempConstArray[resultRows * column + row].getFConst() + unionArray[i * leftRows + row].getFConst() * (rightUnionArray[column * rightRows + i].getFConst()));
                        }
                    }
                }

                
                returnType.setPrimarySize(resultCols);
                returnType.setSecondarySize(resultRows);
            }
            break;

          case EOpDiv:
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    switch (getType().getBasicType())
                    {
                      case EbtFloat:
                        if (rightUnionArray[i] == 0.0f)
                        {
                            infoSink.info.message(EPrefixWarning, getLine(), "Divide by zero error during constant folding");
                            tempConstArray[i].setFConst(unionArray[i].getFConst() < 0 ? -FLT_MAX : FLT_MAX);
                        }
                        else
                        {
                            tempConstArray[i].setFConst(unionArray[i].getFConst() / rightUnionArray[i].getFConst());
                        }
                        break;

                      case EbtInt:
                        if (rightUnionArray[i] == 0)
                        {
                            infoSink.info.message(EPrefixWarning, getLine(), "Divide by zero error during constant folding");
                            tempConstArray[i].setIConst(INT_MAX);
                        }
                        else
                        {
                            tempConstArray[i].setIConst(unionArray[i].getIConst() / rightUnionArray[i].getIConst());
                        }
                        break;

                      case EbtUInt:
                        if (rightUnionArray[i] == 0)
                        {
                            infoSink.info.message(EPrefixWarning, getLine(), "Divide by zero error during constant folding");
                            tempConstArray[i].setUConst(UINT_MAX);
                        }
                        else
                        {
                            tempConstArray[i].setUConst(unionArray[i].getUConst() / rightUnionArray[i].getUConst());
                        }
                        break;

                      default:
                        infoSink.info.message(EPrefixInternalError, getLine(), "Constant folding cannot be done for \"/\"");
                        return 0;
                    }
                }
            }
            break;

          case EOpMatrixTimesVector:
            {
                if (node->getBasicType() != EbtFloat)
                {
                    infoSink.info.message(EPrefixInternalError, getLine(), "Constant Folding cannot be done for matrix times vector");
                    return 0;
                }

                const int matrixCols = getCols();
                const int matrixRows = getRows();

                tempConstArray = new ConstantUnion[matrixRows];

                for (int matrixRow = 0; matrixRow < matrixRows; matrixRow++)
                {
                    tempConstArray[matrixRow].setFConst(0.0f);
                    for (int col = 0; col < matrixCols; col++)
                    {
                        tempConstArray[matrixRow].setFConst(tempConstArray[matrixRow].getFConst() + ((unionArray[col * matrixRows + matrixRow].getFConst()) * rightUnionArray[col].getFConst()));
                    }
                }

                returnType = node->getType();
                returnType.setPrimarySize(matrixRows);

                tempNode = new TIntermConstantUnion(tempConstArray, returnType);
                tempNode->setLine(getLine());

                return tempNode;
            }

          case EOpVectorTimesMatrix:
            {
                if (getType().getBasicType() != EbtFloat)
                {
                    infoSink.info.message(EPrefixInternalError, getLine(), "Constant Folding cannot be done for vector times matrix");
                    return 0;
                }

                const int matrixCols = constantNode->getType().getCols();
                const int matrixRows = constantNode->getType().getRows();

                tempConstArray = new ConstantUnion[matrixCols];

                for (int matrixCol = 0; matrixCol < matrixCols; matrixCol++)
                {
                    tempConstArray[matrixCol].setFConst(0.0f);
                    for (int matrixRow = 0; matrixRow < matrixRows; matrixRow++)
                    {
                        tempConstArray[matrixCol].setFConst(tempConstArray[matrixCol].getFConst() + ((unionArray[matrixRow].getFConst()) * rightUnionArray[matrixCol * matrixRows + matrixRow].getFConst()));
                    }
                }

                returnType.setPrimarySize(matrixCols);
            }
            break;

          case EOpLogicalAnd: 
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    tempConstArray[i] = unionArray[i] && rightUnionArray[i];
                }
            }
            break;

          case EOpLogicalOr: 
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    tempConstArray[i] = unionArray[i] || rightUnionArray[i];
                }
            }
            break;

          case EOpLogicalXor:
            {
                tempConstArray = new ConstantUnion[objectSize];
                for (size_t i = 0; i < objectSize; i++)
                {
                    switch (getType().getBasicType())
                    {
                      case EbtBool:
                        tempConstArray[i].setBConst((unionArray[i] == rightUnionArray[i]) ? false : true);
                        break;
                      default:
                        UNREACHABLE();
                        break;
                    }
                }
            }
            break;

          case EOpLessThan:
            assert(objectSize == 1);
            tempConstArray = new ConstantUnion[1];
            tempConstArray->setBConst(*unionArray < *rightUnionArray);
            returnType = TType(EbtBool, EbpUndefined, EvqConst);
            break;

          case EOpGreaterThan:
            assert(objectSize == 1);
            tempConstArray = new ConstantUnion[1];
            tempConstArray->setBConst(*unionArray > *rightUnionArray);
            returnType = TType(EbtBool, EbpUndefined, EvqConst);
            break;

          case EOpLessThanEqual:
            {
                assert(objectSize == 1);
                ConstantUnion constant;
                constant.setBConst(*unionArray > *rightUnionArray);
                tempConstArray = new ConstantUnion[1];
                tempConstArray->setBConst(!constant.getBConst());
                returnType = TType(EbtBool, EbpUndefined, EvqConst);
                break;
            }

          case EOpGreaterThanEqual:
            {
                assert(objectSize == 1);
                ConstantUnion constant;
                constant.setBConst(*unionArray < *rightUnionArray);
                tempConstArray = new ConstantUnion[1];
                tempConstArray->setBConst(!constant.getBConst());
                returnType = TType(EbtBool, EbpUndefined, EvqConst);
                break;
            }

          case EOpEqual:
            if (getType().getBasicType() == EbtStruct)
            {
                if (!CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray))
                    boolNodeFlag = true;
            }
            else
            {
                for (size_t i = 0; i < objectSize; i++)
                {
                    if (unionArray[i] != rightUnionArray[i])
                    {
                        boolNodeFlag = true;
                        break;  
                    }
                }
            }

            tempConstArray = new ConstantUnion[1];
            if (!boolNodeFlag)
            {
                tempConstArray->setBConst(true);
            }
            else
            {
                tempConstArray->setBConst(false);
            }

            tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
            tempNode->setLine(getLine());

            return tempNode;

          case EOpNotEqual:
            if (getType().getBasicType() == EbtStruct)
            {
                if (CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray))
                    boolNodeFlag = true;
            }
            else
            {
                for (size_t i = 0; i < objectSize; i++)
                {
                    if (unionArray[i] == rightUnionArray[i])
                    {
                        boolNodeFlag = true;
                        break;  
                    }
                }
            }

            tempConstArray = new ConstantUnion[1];
            if (!boolNodeFlag)
            {
                tempConstArray->setBConst(true);
            }
            else
            {
                tempConstArray->setBConst(false);
            }

            tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
            tempNode->setLine(getLine());

            return tempNode;

          default:
            infoSink.info.message(EPrefixInternalError, getLine(), "Invalid operator for constant folding");
            return 0;
        }
        tempNode = new TIntermConstantUnion(tempConstArray, returnType);
        tempNode->setLine(getLine());

        return tempNode;
    }
    else
    {
        
        
        
        TIntermConstantUnion *newNode = 0;
        ConstantUnion* tempConstArray = new ConstantUnion[objectSize];
        for (size_t i = 0; i < objectSize; i++)
        {
            switch(op)
            {
              case EOpNegative:
                switch (getType().getBasicType())
                {
                  case EbtFloat: tempConstArray[i].setFConst(-unionArray[i].getFConst()); break;
                  case EbtInt:   tempConstArray[i].setIConst(-unionArray[i].getIConst()); break;
                  case EbtUInt:  tempConstArray[i].setUConst(static_cast<unsigned int>(-static_cast<int>(unionArray[i].getUConst()))); break;
                  default:
                    infoSink.info.message(EPrefixInternalError, getLine(), "Unary operation not folded into constant");
                    return 0;
                }
                break;

              case EOpLogicalNot: 
                switch (getType().getBasicType())
                {
                  case EbtBool:  tempConstArray[i].setBConst(!unionArray[i].getBConst()); break;
                  default:
                    infoSink.info.message(EPrefixInternalError, getLine(), "Unary operation not folded into constant");
                    return 0;
                }
                break;

              default:
                return 0;
            }
        }
        newNode = new TIntermConstantUnion(tempConstArray, getType());
        newNode->setLine(getLine());
        return newNode;
    }
}


TString TIntermTraverser::hash(const TString& name, ShHashFunction64 hashFunction)
{
    if (hashFunction == NULL || name.empty())
        return name;
    khronos_uint64_t number = (*hashFunction)(name.c_str(), name.length());
    TStringStream stream;
    stream << HASHED_NAME_PREFIX << std::hex << number;
    TString hashedName = stream.str();
    return hashedName;
}
