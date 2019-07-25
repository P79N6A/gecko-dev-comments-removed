









#include <float.h>
#include <limits.h>
#include <algorithm>

#include "compiler/localintermediate.h"
#include "compiler/QualifierAlive.h"
#include "compiler/RemoveTree.h"

bool CompareStructure(const TType& leftNodeType, ConstantUnion* rightUnionArray, ConstantUnion* leftUnionArray);

static TPrecision GetHigherPrecision( TPrecision left, TPrecision right ){
    return left > right ? left : right;
}

const char* getOperatorString(TOperator op) {
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

      case EOpIndexDirectStruct: return ".";
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

      
      case EOpConvIntToBool:
      case EOpConvFloatToBool: return "bool";
 
      
      case EOpConvBoolToFloat:
      case EOpConvIntToFloat: return "float";
 
      
      case EOpConvFloatToInt:
      case EOpConvBoolToInt: return "int";

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














TIntermSymbol* TIntermediate::addSymbol(int id, const TString& name, const TType& type, TSourceLoc line)
{
    TIntermSymbol* node = new TIntermSymbol(id, name, type);
    node->setLine(line);

    return node;
}






TIntermTyped* TIntermediate::addBinaryMath(TOperator op, TIntermTyped* left, TIntermTyped* right, TSourceLoc line, TSymbolTable& symbolTable)
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

    
    
    
    if (left->getType().getStruct() && right->getType().getStruct()) {
        if (left->getType() != right->getType())
            return 0;
    } else {
        TIntermTyped* child = addConversion(op, left->getType(), right);
        if (child)
            right = child;
        else {
            child = addConversion(op, right->getType(), left);
            if (child)
                left = child;
            else
                return 0;
        }
    }

    
    
    
    
    TIntermBinary* node = new TIntermBinary(op);
    if (line == 0)
        line = right->getLine();
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (!node->promote(infoSink))
        return 0;

    
    
    
    TIntermTyped* typedReturnNode = 0;
    TIntermConstantUnion *leftTempConstant = left->getAsConstantUnion();
    TIntermConstantUnion *rightTempConstant = right->getAsConstantUnion();
    if (leftTempConstant && rightTempConstant) {
        typedReturnNode = leftTempConstant->fold(node->getOp(), rightTempConstant, infoSink);

        if (typedReturnNode)
            return typedReturnNode;
    }

    return node;
}






TIntermTyped* TIntermediate::addAssign(TOperator op, TIntermTyped* left, TIntermTyped* right, TSourceLoc line)
{
    
    
    
    
    TIntermBinary* node = new TIntermBinary(op);
    if (line == 0)
        line = left->getLine();
    node->setLine(line);

    TIntermTyped* child = addConversion(op, left->getType(), right);
    if (child == 0)
        return 0;

    node->setLeft(left);
    node->setRight(child);
    if (! node->promote(infoSink))
        return 0;

    return node;
}








TIntermTyped* TIntermediate::addIndex(TOperator op, TIntermTyped* base, TIntermTyped* index, TSourceLoc line)
{
    TIntermBinary* node = new TIntermBinary(op);
    if (line == 0)
        line = index->getLine();
    node->setLine(line);
    node->setLeft(base);
    node->setRight(index);

    

    return node;
}






TIntermTyped* TIntermediate::addUnaryMath(TOperator op, TIntermNode* childNode, TSourceLoc line, TSymbolTable& symbolTable)
{
    TIntermUnary* node;
    TIntermTyped* child = childNode->getAsTyped();

    if (child == 0) {
        infoSink.info.message(EPrefixInternalError, "Bad type in AddUnaryMath", line);
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

    
    
    
    
    
    TBasicType newType = EbtVoid;
    switch (op) {
        case EOpConstructInt:   newType = EbtInt;   break;
        case EOpConstructBool:  newType = EbtBool;  break;
        case EOpConstructFloat: newType = EbtFloat; break;
        default: break;
    }

    if (newType != EbtVoid) {
        child = addConversion(op, TType(newType, child->getPrecision(), EvqTemporary,
            child->getNominalSize(),
            child->isMatrix(),
            child->isArray()),
            child);
        if (child == 0)
            return 0;
    }

    
    
    
    switch (op) {
        case EOpConstructInt:
        case EOpConstructBool:
        case EOpConstructFloat:
            return child;
        default: break;
    }

    TIntermConstantUnion *childTempConstant = 0;
    if (child->getAsConstantUnion())
        childTempConstant = child->getAsConstantUnion();

    
    
    
    node = new TIntermUnary(op);
    if (line == 0)
        line = child->getLine();
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











TIntermAggregate* TIntermediate::setAggregateOperator(TIntermNode* node, TOperator op, TSourceLoc line)
{
    TIntermAggregate* aggNode;

    
    
    
    if (node) {
        aggNode = node->getAsAggregate();
        if (aggNode == 0 || aggNode->getOp() != EOpNull) {
            
            
            
            aggNode = new TIntermAggregate();
            aggNode->getSequence().push_back(node);
            if (line == 0)
                line = node->getLine();
        }
    } else
        aggNode = new TIntermAggregate();

    
    
    
    aggNode->setOp(op);
    if (line != 0)
        aggNode->setLine(line);

    return aggNode;
}









TIntermTyped* TIntermediate::addConversion(TOperator op, const TType& type, TIntermTyped* node)
{
    
    
    
    switch (node->getBasicType()) {
        case EbtVoid:
        case EbtSampler2D:
        case EbtSamplerCube:
            return 0;
        default: break;
    }

    
    
    
    if (type == node->getType())
        return node;

    
    
    
    if (type.getStruct() || node->getType().getStruct())
        return 0;

    
    
    
    if (type.isArray() || node->getType().isArray())
        return 0;

    TBasicType promoteTo;

    switch (op) {
        
        
        
        case EOpConstructBool:
            promoteTo = EbtBool;
            break;
        case EOpConstructFloat:
            promoteTo = EbtFloat;
            break;
        case EOpConstructInt:
            promoteTo = EbtInt;
            break;
        default:
            
            
            
            if (type.getBasicType() != node->getType().getBasicType())
                return 0;
            
            
            
            
            return node;
    }

    if (node->getAsConstantUnion()) {

        return (promoteConstantUnion(promoteTo, node->getAsConstantUnion()));
    } else {

        
        
        
        TIntermUnary* newNode = 0;

        TOperator newOp = EOpNull;
        switch (promoteTo) {
            case EbtFloat:
                switch (node->getBasicType()) {
                    case EbtInt:   newOp = EOpConvIntToFloat;  break;
                    case EbtBool:  newOp = EOpConvBoolToFloat; break;
                    default:
                        infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                        return 0;
                }
                break;
            case EbtBool:
                switch (node->getBasicType()) {
                    case EbtInt:   newOp = EOpConvIntToBool;   break;
                    case EbtFloat: newOp = EOpConvFloatToBool; break;
                    default:
                        infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                        return 0;
                }
                break;
            case EbtInt:
                switch (node->getBasicType()) {
                    case EbtBool:   newOp = EOpConvBoolToInt;  break;
                    case EbtFloat:  newOp = EOpConvFloatToInt; break;
                    default:
                        infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                        return 0;
                }
                break;
            default:
                infoSink.info.message(EPrefixInternalError, "Bad promotion type", node->getLine());
                return 0;
        }

        TType type(promoteTo, node->getPrecision(), EvqTemporary, node->getNominalSize(), node->isMatrix(), node->isArray());
        newNode = new TIntermUnary(newOp, type);
        newNode->setLine(node->getLine());
        newNode->setOperand(node);

        return newNode;
    }
}








TIntermAggregate* TIntermediate::growAggregate(TIntermNode* left, TIntermNode* right, TSourceLoc line)
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

    if (line != 0)
        aggNode->setLine(line);

    return aggNode;
}






TIntermAggregate* TIntermediate::makeAggregate(TIntermNode* node, TSourceLoc line)
{
    if (node == 0)
        return 0;

    TIntermAggregate* aggNode = new TIntermAggregate;
    aggNode->getSequence().push_back(node);

    if (line != 0)
        aggNode->setLine(line);
    else
        aggNode->setLine(node->getLine());

    return aggNode;
}








TIntermNode* TIntermediate::addSelection(TIntermTyped* cond, TIntermNodePair nodePair, TSourceLoc line)
{
    
    
    
    

    if (cond->getAsTyped() && cond->getAsTyped()->getAsConstantUnion()) {
        if (cond->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->getBConst())
            return nodePair.node1;
        else
            return nodePair.node2;
    }

    TIntermSelection* node = new TIntermSelection(cond, nodePair.node1, nodePair.node2);
    node->setLine(line);

    return node;
}


TIntermTyped* TIntermediate::addComma(TIntermTyped* left, TIntermTyped* right, TSourceLoc line)
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








TIntermTyped* TIntermediate::addSelection(TIntermTyped* cond, TIntermTyped* trueBlock, TIntermTyped* falseBlock, TSourceLoc line)
{
    
    
    
    TIntermTyped* child = addConversion(EOpSequence, trueBlock->getType(), falseBlock);
    if (child)
        falseBlock = child;
    else {
        child = addConversion(EOpSequence, falseBlock->getType(), trueBlock);
        if (child)
            trueBlock = child;
        else
            return 0;
    }

    
    
    

    if (cond->getAsConstantUnion() && trueBlock->getAsConstantUnion() && falseBlock->getAsConstantUnion()) {
        if (cond->getAsConstantUnion()->getUnionArrayPointer()->getBConst())
            return trueBlock;
        else
            return falseBlock;
    }

    
    
    
    TIntermSelection* node = new TIntermSelection(cond, trueBlock, falseBlock, trueBlock->getType());
    node->setLine(line);

    return node;
}







TIntermConstantUnion* TIntermediate::addConstantUnion(ConstantUnion* unionArrayPointer, const TType& t, TSourceLoc line)
{
    TIntermConstantUnion* node = new TIntermConstantUnion(unionArrayPointer, t);
    node->setLine(line);

    return node;
}

TIntermTyped* TIntermediate::addSwizzle(TVectorFields& fields, TSourceLoc line)
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




TIntermNode* TIntermediate::addLoop(TLoopType type, TIntermNode* init, TIntermTyped* cond, TIntermTyped* expr, TIntermNode* body, TSourceLoc line)
{
    TIntermNode* node = new TIntermLoop(type, init, cond, expr, body);
    node->setLine(line);

    return node;
}




TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TSourceLoc line)
{
    return addBranch(branchOp, 0, line);
}

TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TIntermTyped* expression, TSourceLoc line)
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












bool TIntermOperator::modifiesState() const
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

    return true;
}







bool TIntermBinary::promote(TInfoSink& infoSink)
{
    
    if (left->isArray() || right->isArray()) {
        infoSink.info.message(EPrefixInternalError, "Invalid operation for arrays", getLine());
        return false;
    }

    
    
    if (left->getBasicType() != right->getBasicType())
        return false;

    
    
    
    
    setType(left->getType());

    
    TPrecision higherPrecision = GetHigherPrecision(left->getPrecision(), right->getPrecision());
    getTypePointer()->setPrecision(higherPrecision);

    
    
    if (left->getQualifier() != EvqConst || right->getQualifier() != EvqConst) {
        getTypePointer()->setQualifier(EvqTemporary);
    }

    int size = std::max(left->getNominalSize(), right->getNominalSize());

    
    
    
    if (size == 1) {
        switch (op) {
            
            
            
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
                    return false;
                setType(TType(EbtBool, EbpUndefined));
                break;

            default:
                break;
        }
        return true;
    }

    
    
    
    
    if (left->getNominalSize() != right->getNominalSize()) {
        
        
        if (left->getNominalSize() != 1 && right->getNominalSize() != 1)
            return false;
        
        if (op == EOpAssign || op == EOpInitialize)
            return false;
    }

    
    
    
    TBasicType basicType = left->getBasicType();
    switch (op) {
        case EOpMul:
            if (!left->isMatrix() && right->isMatrix()) {
                if (left->isVector())
                    op = EOpVectorTimesMatrix;
                else {
                    op = EOpMatrixTimesScalar;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, true));
                }
            } else if (left->isMatrix() && !right->isMatrix()) {
                if (right->isVector()) {
                    op = EOpMatrixTimesVector;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, false));
                } else {
                    op = EOpMatrixTimesScalar;
                }
            } else if (left->isMatrix() && right->isMatrix()) {
                op = EOpMatrixTimesMatrix;
            } else if (!left->isMatrix() && !right->isMatrix()) {
                if (left->isVector() && right->isVector()) {
                    
                } else if (left->isVector() || right->isVector()) {
                    op = EOpVectorTimesScalar;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, false));
                }
            } else {
                infoSink.info.message(EPrefixInternalError, "Missing elses", getLine());
                return false;
            }
            break;
        case EOpMulAssign:
            if (!left->isMatrix() && right->isMatrix()) {
                if (left->isVector())
                    op = EOpVectorTimesMatrixAssign;
                else {
                    return false;
                }
            } else if (left->isMatrix() && !right->isMatrix()) {
                if (right->isVector()) {
                    return false;
                } else {
                    op = EOpMatrixTimesScalarAssign;
                }
            } else if (left->isMatrix() && right->isMatrix()) {
                op = EOpMatrixTimesMatrixAssign;
            } else if (!left->isMatrix() && !right->isMatrix()) {
                if (left->isVector() && right->isVector()) {
                    
                } else if (left->isVector() || right->isVector()) {
                    if (! left->isVector())
                        return false;
                    op = EOpVectorTimesScalarAssign;
                    setType(TType(basicType, higherPrecision, EvqTemporary, size, false));
                }
            } else {
                infoSink.info.message(EPrefixInternalError, "Missing elses", getLine());
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
            if ((left->isMatrix() && right->isVector()) ||
                (left->isVector() && right->isMatrix()))
                return false;
            setType(TType(basicType, higherPrecision, EvqTemporary, size, left->isMatrix() || right->isMatrix()));
            break;

        case EOpEqual:
        case EOpNotEqual:
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
            if ((left->isMatrix() && right->isVector()) ||
                (left->isVector() && right->isMatrix()))
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
    const TTypeList* fields = leftNodeType.getStruct();

    size_t structSize = fields->size();
    int index = 0;

    for (size_t j = 0; j < structSize; j++) {
        int size = (*fields)[j].type->getObjectSize();
        for (int i = 0; i < size; i++) {
            if ((*fields)[j].type->getBasicType() == EbtStruct) {
                if (!CompareStructure(*(*fields)[j].type, &rightUnionArray[index], &leftUnionArray[index]))
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

        int arraySize = leftNodeType.getArraySize();

        for (int i = 0; i < arraySize; ++i) {
            int offset = typeWithoutArrayness.getObjectSize() * i;
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
    int objectSize = getType().getObjectSize();

    if (constantNode) {  
        TIntermConstantUnion *node = constantNode->getAsConstantUnion();
        ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
        TType returnType = getType();

        
        if (constantNode->getType().getObjectSize() == 1 && objectSize > 1) {
            rightUnionArray = new ConstantUnion[objectSize];
            for (int i = 0; i < objectSize; ++i)
                rightUnionArray[i] = *node->getUnionArrayPointer();
            returnType = getType();
        } else if (constantNode->getType().getObjectSize() > 1 && objectSize == 1) {
            
            unionArray = new ConstantUnion[constantNode->getType().getObjectSize()];
            for (int i = 0; i < constantNode->getType().getObjectSize(); ++i)
                unionArray[i] = *getUnionArrayPointer();
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
                    for (int i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] + rightUnionArray[i];
                }
                break;
            case EOpSub:
                tempConstArray = new ConstantUnion[objectSize];
                {
                    for (int i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] - rightUnionArray[i];
                }
                break;

            case EOpMul:
            case EOpVectorTimesScalar:
            case EOpMatrixTimesScalar:
                tempConstArray = new ConstantUnion[objectSize];
                {
                    for (int i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] * rightUnionArray[i];
                }
                break;
            case EOpMatrixTimesMatrix:
                if (getType().getBasicType() != EbtFloat || node->getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix multiply", getLine());
                    return 0;
                }
                {
                    int size = getNominalSize();
                    tempConstArray = new ConstantUnion[size*size];
                    for (int row = 0; row < size; row++) {
                        for (int column = 0; column < size; column++) {
                            tempConstArray[size * column + row].setFConst(0.0f);
                            for (int i = 0; i < size; i++) {
                                tempConstArray[size * column + row].setFConst(tempConstArray[size * column + row].getFConst() + unionArray[i * size + row].getFConst() * (rightUnionArray[column * size + i].getFConst()));
                            }
                        }
                    }
                }
                break;
            case EOpDiv:
                tempConstArray = new ConstantUnion[objectSize];
                {
                    for (int i = 0; i < objectSize; i++) {
                        switch (getType().getBasicType()) {
            case EbtFloat:
                if (rightUnionArray[i] == 0.0f) {
                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", getLine());
                    tempConstArray[i].setFConst(FLT_MAX);
                } else
                    tempConstArray[i].setFConst(unionArray[i].getFConst() / rightUnionArray[i].getFConst());
                break;

            case EbtInt:
                if (rightUnionArray[i] == 0) {
                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", getLine());
                    tempConstArray[i].setIConst(INT_MAX);
                } else
                    tempConstArray[i].setIConst(unionArray[i].getIConst() / rightUnionArray[i].getIConst());
                break;
            default:
                infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"/\"", getLine());
                return 0;
                        }
                    }
                }
                break;

            case EOpMatrixTimesVector:
                if (node->getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix times vector", getLine());
                    return 0;
                }
                tempConstArray = new ConstantUnion[getNominalSize()];

                {
                    for (int size = getNominalSize(), i = 0; i < size; i++) {
                        tempConstArray[i].setFConst(0.0f);
                        for (int j = 0; j < size; j++) {
                            tempConstArray[i].setFConst(tempConstArray[i].getFConst() + ((unionArray[j*size + i].getFConst()) * rightUnionArray[j].getFConst()));
                        }
                    }
                }

                tempNode = new TIntermConstantUnion(tempConstArray, node->getType());
                tempNode->setLine(getLine());

                return tempNode;

            case EOpVectorTimesMatrix:
                if (getType().getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for vector times matrix", getLine());
                    return 0;
                }

                tempConstArray = new ConstantUnion[getNominalSize()];
                {
                    for (int size = getNominalSize(), i = 0; i < size; i++) {
                        tempConstArray[i].setFConst(0.0f);
                        for (int j = 0; j < size; j++) {
                            tempConstArray[i].setFConst(tempConstArray[i].getFConst() + ((unionArray[j].getFConst()) * rightUnionArray[i*size + j].getFConst()));
                        }
                    }
                }
                break;

            case EOpLogicalAnd: 
                tempConstArray = new ConstantUnion[objectSize];
                {
                    for (int i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] && rightUnionArray[i];
                }
                break;

            case EOpLogicalOr: 
                tempConstArray = new ConstantUnion[objectSize];
                {
                    for (int i = 0; i < objectSize; i++)
                        tempConstArray[i] = unionArray[i] || rightUnionArray[i];
                }
                break;

            case EOpLogicalXor:
                tempConstArray = new ConstantUnion[objectSize];
                {
                    for (int i = 0; i < objectSize; i++)
                        switch (getType().getBasicType()) {
            case EbtBool: tempConstArray[i].setBConst((unionArray[i] == rightUnionArray[i]) ? false : true); break;
            default: assert(false && "Default missing");
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
                if (getType().getBasicType() == EbtStruct) {
                    if (!CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray))
                        boolNodeFlag = true;
                } else {
                    for (int i = 0; i < objectSize; i++) {
                        if (unionArray[i] != rightUnionArray[i]) {
                            boolNodeFlag = true;
                            break;  
                        }
                    }
                }

                tempConstArray = new ConstantUnion[1];
                if (!boolNodeFlag) {
                    tempConstArray->setBConst(true);
                }
                else {
                    tempConstArray->setBConst(false);
                }

                tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
                tempNode->setLine(getLine());

                return tempNode;

            case EOpNotEqual:
                if (getType().getBasicType() == EbtStruct) {
                    if (CompareStructure(node->getType(), node->getUnionArrayPointer(), unionArray))
                        boolNodeFlag = true;
                } else {
                    for (int i = 0; i < objectSize; i++) {
                        if (unionArray[i] == rightUnionArray[i]) {
                            boolNodeFlag = true;
                            break;  
                        }
                    }
                }

                tempConstArray = new ConstantUnion[1];
                if (!boolNodeFlag) {
                    tempConstArray->setBConst(true);
                }
                else {
                    tempConstArray->setBConst(false);
                }

                tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EbpUndefined, EvqConst));
                tempNode->setLine(getLine());

                return tempNode;

            default:
                infoSink.info.message(EPrefixInternalError, "Invalid operator for constant folding", getLine());
                return 0;
        }
        tempNode = new TIntermConstantUnion(tempConstArray, returnType);
        tempNode->setLine(getLine());

        return tempNode;
    } else {
        
        
        
        TIntermConstantUnion *newNode = 0;
        ConstantUnion* tempConstArray = new ConstantUnion[objectSize];
        for (int i = 0; i < objectSize; i++) {
            switch(op) {
                case EOpNegative:
                    switch (getType().getBasicType()) {
                        case EbtFloat: tempConstArray[i].setFConst(-unionArray[i].getFConst()); break;
                        case EbtInt:   tempConstArray[i].setIConst(-unionArray[i].getIConst()); break;
                        default:
                            infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
                            return 0;
                    }
                    break;
                case EOpLogicalNot: 
                    switch (getType().getBasicType()) {
                        case EbtBool:  tempConstArray[i].setBConst(!unionArray[i].getBConst()); break;
                        default:
                            infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", getLine());
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

    return this;
}

TIntermTyped* TIntermediate::promoteConstantUnion(TBasicType promoteTo, TIntermConstantUnion* node)
{
    ConstantUnion *rightUnionArray = node->getUnionArrayPointer();
    int size = node->getType().getObjectSize();

    ConstantUnion *leftUnionArray = new ConstantUnion[size];

    for (int i=0; i < size; i++) {

        switch (promoteTo) {
            case EbtFloat:
                switch (node->getType().getBasicType()) {
                    case EbtInt:
                        leftUnionArray[i].setFConst(static_cast<float>(rightUnionArray[i].getIConst()));
                        break;
                    case EbtBool:
                        leftUnionArray[i].setFConst(static_cast<float>(rightUnionArray[i].getBConst()));
                        break;
                    case EbtFloat:
                        leftUnionArray[i] = rightUnionArray[i];
                        break;
                    default:
                        infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
                        return 0;
                }
                break;
            case EbtInt:
                switch (node->getType().getBasicType()) {
                    case EbtInt:
                        leftUnionArray[i] = rightUnionArray[i];
                        break;
                    case EbtBool:
                        leftUnionArray[i].setIConst(static_cast<int>(rightUnionArray[i].getBConst()));
                        break;
                    case EbtFloat:
                        leftUnionArray[i].setIConst(static_cast<int>(rightUnionArray[i].getFConst()));
                        break;
                    default:
                        infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
                        return 0;
                }
                break;
            case EbtBool:
                switch (node->getType().getBasicType()) {
                    case EbtInt:
                        leftUnionArray[i].setBConst(rightUnionArray[i].getIConst() != 0);
                        break;
                    case EbtBool:
                        leftUnionArray[i] = rightUnionArray[i];
                        break;
                    case EbtFloat:
                        leftUnionArray[i].setBConst(rightUnionArray[i].getFConst() != 0.0f);
                        break;
                    default:
                        infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
                        return 0;
                }

                break;
            default:
                infoSink.info.message(EPrefixInternalError, "Incorrect data type found", node->getLine());
                return 0;
        }

    }

    const TType& t = node->getType();

    return addConstantUnion(leftUnionArray, TType(promoteTo, t.getPrecision(), t.getQualifier(), t.getNominalSize(), t.isMatrix(), t.isArray()), node->getLine());
}

void TIntermAggregate::addToPragmaTable(const TPragmaTable& pTable)
{
    assert(!pragmaTable);
    pragmaTable = new TPragmaTable();
    *pragmaTable = pTable;
}
