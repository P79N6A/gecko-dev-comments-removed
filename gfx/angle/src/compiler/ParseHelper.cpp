





#include "compiler/ParseHelper.h"

#include <stdarg.h>
#include <stdio.h>

#include "compiler/glslang.h"
#include "compiler/preprocessor/SourceLocation.h"











bool TParseContext::parseVectorFields(const TString& compString, int vecSize, TVectorFields& fields, int line)
{
    fields.num = (int) compString.size();
    if (fields.num > 4) {
        error(line, "illegal vector field selection", compString.c_str());
        return false;
    }

    enum {
        exyzw,
        ergba,
        estpq,
    } fieldSet[4];

    for (int i = 0; i < fields.num; ++i) {
        switch (compString[i])  {
        case 'x': 
            fields.offsets[i] = 0;
            fieldSet[i] = exyzw;
            break;
        case 'r': 
            fields.offsets[i] = 0;
            fieldSet[i] = ergba;
            break;
        case 's':
            fields.offsets[i] = 0;
            fieldSet[i] = estpq;
            break;
        case 'y': 
            fields.offsets[i] = 1;
            fieldSet[i] = exyzw;
            break;
        case 'g': 
            fields.offsets[i] = 1;
            fieldSet[i] = ergba;
            break;
        case 't':
            fields.offsets[i] = 1;
            fieldSet[i] = estpq;
            break;
        case 'z': 
            fields.offsets[i] = 2;
            fieldSet[i] = exyzw;
            break;
        case 'b': 
            fields.offsets[i] = 2;
            fieldSet[i] = ergba;
            break;
        case 'p':
            fields.offsets[i] = 2;
            fieldSet[i] = estpq;
            break;
        
        case 'w': 
            fields.offsets[i] = 3;
            fieldSet[i] = exyzw;
            break;
        case 'a': 
            fields.offsets[i] = 3;
            fieldSet[i] = ergba;
            break;
        case 'q':
            fields.offsets[i] = 3;
            fieldSet[i] = estpq;
            break;
        default:
            error(line, "illegal vector field selection", compString.c_str());
            return false;
        }
    }

    for (int i = 0; i < fields.num; ++i) {
        if (fields.offsets[i] >= vecSize) {
            error(line, "vector field selection out of range",  compString.c_str());
            return false;
        }

        if (i > 0) {
            if (fieldSet[i] != fieldSet[i-1]) {
                error(line, "illegal - vector component fields not from the same set", compString.c_str());
                return false;
            }
        }
    }

    return true;
}






bool TParseContext::parseMatrixFields(const TString& compString, int matSize, TMatrixFields& fields, int line)
{
    fields.wholeRow = false;
    fields.wholeCol = false;
    fields.row = -1;
    fields.col = -1;

    if (compString.size() != 2) {
        error(line, "illegal length of matrix field selection", compString.c_str());
        return false;
    }

    if (compString[0] == '_') {
        if (compString[1] < '0' || compString[1] > '3') {
            error(line, "illegal matrix field selection", compString.c_str());
            return false;
        }
        fields.wholeCol = true;
        fields.col = compString[1] - '0';
    } else if (compString[1] == '_') {
        if (compString[0] < '0' || compString[0] > '3') {
            error(line, "illegal matrix field selection", compString.c_str());
            return false;
        }
        fields.wholeRow = true;
        fields.row = compString[0] - '0';
    } else {
        if (compString[0] < '0' || compString[0] > '3' ||
            compString[1] < '0' || compString[1] > '3') {
            error(line, "illegal matrix field selection", compString.c_str());
            return false;
        }
        fields.row = compString[0] - '0';
        fields.col = compString[1] - '0';
    }

    if (fields.row >= matSize || fields.col >= matSize) {
        error(line, "matrix field selection out of range", compString.c_str());
        return false;
    }

    return true;
}










void TParseContext::recover()
{
}




void TParseContext::error(TSourceLoc loc,
                          const char* reason, const char* token, 
                          const char* extraInfo)
{
    pp::SourceLocation srcLoc;
    DecodeSourceLoc(loc, &srcLoc.file, &srcLoc.line);
    diagnostics.writeInfo(pp::Diagnostics::ERROR,
                          srcLoc, reason, token, extraInfo);

}

void TParseContext::warning(TSourceLoc loc,
                            const char* reason, const char* token,
                            const char* extraInfo) {
    pp::SourceLocation srcLoc;
    DecodeSourceLoc(loc, &srcLoc.file, &srcLoc.line);
    diagnostics.writeInfo(pp::Diagnostics::WARNING,
                          srcLoc, reason, token, extraInfo);
}

void TParseContext::trace(const char* str)
{
    diagnostics.writeDebug(str);
}




void TParseContext::assignError(int line, const char* op, TString left, TString right)
{
    std::stringstream extraInfoStream;
    extraInfoStream << "cannot convert from '" << right << "' to '" << left << "'";
    std::string extraInfo = extraInfoStream.str();
    error(line, "", op, extraInfo.c_str());
}




void TParseContext::unaryOpError(int line, const char* op, TString operand)
{
    std::stringstream extraInfoStream;
    extraInfoStream << "no operation '" << op << "' exists that takes an operand of type " << operand 
                    << " (or there is no acceptable conversion)";
    std::string extraInfo = extraInfoStream.str();
    error(line, " wrong operand type", op, extraInfo.c_str());
}




void TParseContext::binaryOpError(int line, const char* op, TString left, TString right)
{
    std::stringstream extraInfoStream;
    extraInfoStream << "no operation '" << op << "' exists that takes a left-hand operand of type '" << left 
                    << "' and a right operand of type '" << right << "' (or there is no acceptable conversion)";
    std::string extraInfo = extraInfoStream.str();
    error(line, " wrong operand types ", op, extraInfo.c_str()); 
}

bool TParseContext::precisionErrorCheck(int line, TPrecision precision, TBasicType type){
    if (!checksPrecisionErrors)
        return false;
    switch( type ){
    case EbtFloat:
        if( precision == EbpUndefined ){
            error( line, "No precision specified for (float)", "" );
            return true;
        }
        break;
    case EbtInt:
        if( precision == EbpUndefined ){
            error( line, "No precision specified (int)", "" );
            return true;
        }
        break;
    default:
        return false;
    }
    return false;
}







bool TParseContext::lValueErrorCheck(int line, const char* op, TIntermTyped* node)
{
    TIntermSymbol* symNode = node->getAsSymbolNode();
    TIntermBinary* binaryNode = node->getAsBinaryNode();

    if (binaryNode) {
        bool errorReturn;

        switch(binaryNode->getOp()) {
        case EOpIndexDirect:
        case EOpIndexIndirect:
        case EOpIndexDirectStruct:
            return lValueErrorCheck(line, op, binaryNode->getLeft());
        case EOpVectorSwizzle:
            errorReturn = lValueErrorCheck(line, op, binaryNode->getLeft());
            if (!errorReturn) {
                int offset[4] = {0,0,0,0};

                TIntermTyped* rightNode = binaryNode->getRight();
                TIntermAggregate *aggrNode = rightNode->getAsAggregate();
                
                for (TIntermSequence::iterator p = aggrNode->getSequence().begin(); 
                                               p != aggrNode->getSequence().end(); p++) {
                    int value = (*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->getIConst();
                    offset[value]++;     
                    if (offset[value] > 1) {
                        error(line, " l-value of swizzle cannot have duplicate components", op);

                        return true;
                    }
                }
            } 

            return errorReturn;
        default: 
            break;
        }
        error(line, " l-value required", op);

        return true;
    }


    const char* symbol = 0;
    if (symNode != 0)
        symbol = symNode->getSymbol().c_str();

    const char* message = 0;
    switch (node->getQualifier()) {
    case EvqConst:          message = "can't modify a const";        break;
    case EvqConstReadOnly:  message = "can't modify a const";        break;
    case EvqAttribute:      message = "can't modify an attribute";   break;
    case EvqUniform:        message = "can't modify a uniform";      break;
    case EvqVaryingIn:      message = "can't modify a varying";      break;
    case EvqInput:          message = "can't modify an input";       break;
    case EvqFragCoord:      message = "can't modify gl_FragCoord";   break;
    case EvqFrontFacing:    message = "can't modify gl_FrontFacing"; break;
    case EvqPointCoord:     message = "can't modify gl_PointCoord";  break;
    default:

        
        
        
        switch (node->getBasicType()) {
        case EbtSampler2D:
        case EbtSamplerCube:
            message = "can't modify a sampler";
            break;
        case EbtVoid:
            message = "can't modify void";
            break;
        default: 
            break;
        }
    }

    if (message == 0 && binaryNode == 0 && symNode == 0) {
        error(line, " l-value required", op);

        return true;
    }


    
    
    
    if (message == 0)
        return false;

    
    
    
    if (symNode) {
        std::stringstream extraInfoStream;
        extraInfoStream << "\"" << symbol << "\" (" << message << ")";
        std::string extraInfo = extraInfoStream.str();
        error(line, " l-value required", op, extraInfo.c_str());
    }
    else {
        std::stringstream extraInfoStream;
        extraInfoStream << "(" << message << ")";
        std::string extraInfo = extraInfoStream.str();
        error(line, " l-value required", op, extraInfo.c_str());
    }

    return true;
}







bool TParseContext::constErrorCheck(TIntermTyped* node)
{
    if (node->getQualifier() == EvqConst)
        return false;

    error(node->getLine(), "constant expression required", "");

    return true;
}







bool TParseContext::integerErrorCheck(TIntermTyped* node, const char* token)
{
    if (node->getBasicType() == EbtInt && node->getNominalSize() == 1)
        return false;

    error(node->getLine(), "integer expression required", token);

    return true;
}







bool TParseContext::globalErrorCheck(int line, bool global, const char* token)
{
    if (global)
        return false;

    error(line, "only allowed at global scope", token);

    return true;
}










bool TParseContext::reservedErrorCheck(int line, const TString& identifier)
{
    static const char* reservedErrMsg = "reserved built-in name";
    if (!symbolTable.atBuiltInLevel()) {
        if (identifier.compare(0, 3, "gl_") == 0) {
            error(line, reservedErrMsg, "gl_");
            return true;
        }
        if (isWebGLBasedSpec(shaderSpec)) {
            if (identifier.compare(0, 6, "webgl_") == 0) {
                error(line, reservedErrMsg, "webgl_");
                return true;
            }
            if (identifier.compare(0, 7, "_webgl_") == 0) {
                error(line, reservedErrMsg, "_webgl_");
                return true;
            }
            if (shaderSpec == SH_CSS_SHADERS_SPEC && identifier.compare(0, 4, "css_") == 0) {
                error(line, reservedErrMsg, "css_");
                return true;
            }
        }
        if (identifier.find("__") != TString::npos) {
            error(line, "identifiers containing two consecutive underscores (__) are reserved as possible future keywords", identifier.c_str());
            return true;
        }
    }

    return false;
}








bool TParseContext::constructorErrorCheck(int line, TIntermNode* node, TFunction& function, TOperator op, TType* type)
{
    *type = function.getReturnType();

    bool constructingMatrix = false;
    switch(op) {
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
        constructingMatrix = true;
        break;
    default: 
        break;
    }

    
    
    
    
    

    int size = 0;
    bool constType = true;
    bool full = false;
    bool overFull = false;
    bool matrixInMatrix = false;
    bool arrayArg = false;
    for (int i = 0; i < function.getParamCount(); ++i) {
        const TParameter& param = function.getParam(i);
        size += param.type->getObjectSize();
        
        if (constructingMatrix && param.type->isMatrix())
            matrixInMatrix = true;
        if (full)
            overFull = true;
        if (op != EOpConstructStruct && !type->isArray() && size >= type->getObjectSize())
            full = true;
        if (param.type->getQualifier() != EvqConst)
            constType = false;
        if (param.type->isArray())
            arrayArg = true;
    }
    
    if (constType)
        type->setQualifier(EvqConst);

    if (type->isArray() && type->getArraySize() != function.getParamCount()) {
        error(line, "array constructor needs one argument per array element", "constructor");
        return true;
    }

    if (arrayArg && op != EOpConstructStruct) {
        error(line, "constructing from a non-dereferenced array", "constructor");
        return true;
    }

    if (matrixInMatrix && !type->isArray()) {
        if (function.getParamCount() != 1) {
          error(line, "constructing matrix from matrix can only take one argument", "constructor");
          return true;
        }
    }

    if (overFull) {
        error(line, "too many arguments", "constructor");
        return true;
    }
    
    if (op == EOpConstructStruct && !type->isArray() && int(type->getStruct()->size()) != function.getParamCount()) {
        error(line, "Number of constructor parameters does not match the number of structure fields", "constructor");
        return true;
    }

    if (!type->isMatrix() || !matrixInMatrix) {
        if ((op != EOpConstructStruct && size != 1 && size < type->getObjectSize()) ||
            (op == EOpConstructStruct && size < type->getObjectSize())) {
            error(line, "not enough data provided for construction", "constructor");
            return true;
        }
    }

    TIntermTyped *typed = node ? node->getAsTyped() : 0;
    if (typed == 0) {
        error(line, "constructor argument does not have a type", "constructor");
        return true;
    }
    if (op != EOpConstructStruct && IsSampler(typed->getBasicType())) {
        error(line, "cannot convert a sampler", "constructor");
        return true;
    }
    if (typed->getBasicType() == EbtVoid) {
        error(line, "cannot convert a void", "constructor");
        return true;
    }

    return false;
}





bool TParseContext::voidErrorCheck(int line, const TString& identifier, const TPublicType& pubType)
{
    if (pubType.type == EbtVoid) {
        error(line, "illegal use of type 'void'", identifier.c_str());
        return true;
    } 

    return false;
}





bool TParseContext::boolErrorCheck(int line, const TIntermTyped* type)
{
    if (type->getBasicType() != EbtBool || type->isArray() || type->isMatrix() || type->isVector()) {
        error(line, "boolean expression expected", "");
        return true;
    } 

    return false;
}





bool TParseContext::boolErrorCheck(int line, const TPublicType& pType)
{
    if (pType.type != EbtBool || pType.array || pType.matrix || (pType.size > 1)) {
        error(line, "boolean expression expected", "");
        return true;
    } 

    return false;
}

bool TParseContext::samplerErrorCheck(int line, const TPublicType& pType, const char* reason)
{
    if (pType.type == EbtStruct) {
        if (containsSampler(*pType.userDef)) {
            error(line, reason, getBasicString(pType.type), "(structure contains a sampler)");
        
            return true;
        }
        
        return false;
    } else if (IsSampler(pType.type)) {
        error(line, reason, getBasicString(pType.type));

        return true;
    }

    return false;
}

bool TParseContext::structQualifierErrorCheck(int line, const TPublicType& pType)
{
    if ((pType.qualifier == EvqVaryingIn || pType.qualifier == EvqVaryingOut || pType.qualifier == EvqAttribute) &&
        pType.type == EbtStruct) {
        error(line, "cannot be used with a structure", getQualifierString(pType.qualifier));
        
        return true;
    }

    if (pType.qualifier != EvqUniform && samplerErrorCheck(line, pType, "samplers must be uniform"))
        return true;

    return false;
}

bool TParseContext::parameterSamplerErrorCheck(int line, TQualifier qualifier, const TType& type)
{
    if ((qualifier == EvqOut || qualifier == EvqInOut) && 
             type.getBasicType() != EbtStruct && IsSampler(type.getBasicType())) {
        error(line, "samplers cannot be output parameters", type.getBasicString());
        return true;
    }

    return false;
}

bool TParseContext::containsSampler(TType& type)
{
    if (IsSampler(type.getBasicType()))
        return true;

    if (type.getBasicType() == EbtStruct) {
        TTypeList& structure = *type.getStruct();
        for (unsigned int i = 0; i < structure.size(); ++i) {
            if (containsSampler(*structure[i].type))
                return true;
        }
    }

    return false;
}






bool TParseContext::arraySizeErrorCheck(int line, TIntermTyped* expr, int& size)
{
    TIntermConstantUnion* constant = expr->getAsConstantUnion();
    if (constant == 0 || constant->getBasicType() != EbtInt) {
        error(line, "array size must be a constant integer expression", "");
        return true;
    }

    size = constant->getUnionArrayPointer()->getIConst();

    if (size <= 0) {
        error(line, "array size must be a positive integer", "");
        size = 1;
        return true;
    }

    return false;
}






bool TParseContext::arrayQualifierErrorCheck(int line, TPublicType type)
{
    if ((type.qualifier == EvqAttribute) || (type.qualifier == EvqConst)) {
        error(line, "cannot declare arrays of this qualifier", TType(type).getCompleteString().c_str());
        return true;
    }

    return false;
}






bool TParseContext::arrayTypeErrorCheck(int line, TPublicType type)
{
    
    
    
    if (type.array) {
        error(line, "cannot declare arrays of arrays", TType(type).getCompleteString().c_str());
        return true;
    }

    return false;
}









bool TParseContext::arrayErrorCheck(int line, TString& identifier, TPublicType type, TVariable*& variable)
{
    
    
    
    

    bool builtIn = false; 
    bool sameScope = false;
    TSymbol* symbol = symbolTable.find(identifier, &builtIn, &sameScope);
    if (symbol == 0 || !sameScope) {
        if (reservedErrorCheck(line, identifier))
            return true;
        
        variable = new TVariable(&identifier, TType(type));

        if (type.arraySize)
            variable->getType().setArraySize(type.arraySize);

        if (! symbolTable.insert(*variable)) {
            delete variable;
            error(line, "INTERNAL ERROR inserting new symbol", identifier.c_str());
            return true;
        }
    } else {
        if (! symbol->isVariable()) {
            error(line, "variable expected", identifier.c_str());
            return true;
        }

        variable = static_cast<TVariable*>(symbol);
        if (! variable->getType().isArray()) {
            error(line, "redeclaring non-array as array", identifier.c_str());
            return true;
        }
        if (variable->getType().getArraySize() > 0) {
            error(line, "redeclaration of array with size", identifier.c_str());
            return true;
        }
        
        if (! variable->getType().sameElementType(TType(type))) {
            error(line, "redeclaration of array with a different type", identifier.c_str());
            return true;
        }

        TType* t = variable->getArrayInformationType();
        while (t != 0) {
            if (t->getMaxArraySize() > type.arraySize) {
                error(line, "higher index value already used for the array", identifier.c_str());
                return true;
            }
            t->setArraySize(type.arraySize);
            t = t->getArrayInformationType();
        }

        if (type.arraySize)
            variable->getType().setArraySize(type.arraySize);
    } 

    if (voidErrorCheck(line, identifier, type))
        return true;

    return false;
}

bool TParseContext::arraySetMaxSize(TIntermSymbol *node, TType* type, int size, bool updateFlag, TSourceLoc line)
{
    bool builtIn = false;
    TSymbol* symbol = symbolTable.find(node->getSymbol(), &builtIn);
    if (symbol == 0) {
        error(line, " undeclared identifier", node->getSymbol().c_str());
        return true;
    }
    TVariable* variable = static_cast<TVariable*>(symbol);

    type->setArrayInformationType(variable->getArrayInformationType());
    variable->updateArrayInformationType(type);

    
    
    if (node->getSymbol() == "gl_FragData") {
        TSymbol* fragData = symbolTable.find("gl_MaxDrawBuffers", &builtIn);
        ASSERT(fragData);

        int fragDataValue = static_cast<TVariable*>(fragData)->getConstPointer()[0].getIConst();
        if (fragDataValue <= size) {
            error(line, "", "[", "gl_FragData can only have a max array size of up to gl_MaxDrawBuffers");
            return true;
        }
    }

    
    
    if (!updateFlag)
        return false;

    size++;
    variable->getType().setMaxArraySize(size);
    type->setMaxArraySize(size);
    TType* tt = type;

    while(tt->getArrayInformationType() != 0) {
        tt = tt->getArrayInformationType();
        tt->setMaxArraySize(size);
    }

    return false;
}






bool TParseContext::nonInitConstErrorCheck(int line, TString& identifier, TPublicType& type, bool array)
{
    if (type.qualifier == EvqConst)
    {
        
        type.qualifier = EvqTemporary;
        
        if (array)
        {
            error(line, "arrays may not be declared constant since they cannot be initialized", identifier.c_str());
        }
        else if (type.isStructureContainingArrays())
        {
            error(line, "structures containing arrays may not be declared constant since they cannot be initialized", identifier.c_str());
        }
        else
        {
            error(line, "variables with qualifier 'const' must be initialized", identifier.c_str());
        }

        return true;
    }

    return false;
}







bool TParseContext::nonInitErrorCheck(int line, TString& identifier, TPublicType& type, TVariable*& variable)
{
    if (reservedErrorCheck(line, identifier))
        recover();

    variable = new TVariable(&identifier, TType(type));

    if (! symbolTable.insert(*variable)) {
        error(line, "redefinition", variable->getName().c_str());
        delete variable;
        variable = 0;
        return true;
    }

    if (voidErrorCheck(line, identifier, type))
        return true;

    return false;
}

bool TParseContext::paramErrorCheck(int line, TQualifier qualifier, TQualifier paramQualifier, TType* type)
{    
    if (qualifier != EvqConst && qualifier != EvqTemporary) {
        error(line, "qualifier not allowed on function parameter", getQualifierString(qualifier));
        return true;
    }
    if (qualifier == EvqConst && paramQualifier != EvqIn) {
        error(line, "qualifier not allowed with ", getQualifierString(qualifier), getQualifierString(paramQualifier));
        return true;
    }

    if (qualifier == EvqConst)
        type->setQualifier(EvqConstReadOnly);
    else
        type->setQualifier(paramQualifier);

    return false;
}

bool TParseContext::extensionErrorCheck(int line, const TString& extension)
{
    const TExtensionBehavior& extBehavior = extensionBehavior();
    TExtensionBehavior::const_iterator iter = extBehavior.find(extension.c_str());
    if (iter == extBehavior.end()) {
        error(line, "extension", extension.c_str(), "is not supported");
        return true;
    }
    
    if (iter->second == EBhDisable || iter->second == EBhUndefined) {
        error(line, "extension", extension.c_str(), "is disabled");
        return true;
    }
    if (iter->second == EBhWarn) {
        warning(line, "extension", extension.c_str(), "is being used");
        return false;
    }

    return false;
}

bool TParseContext::supportsExtension(const char* extension)
{
    const TExtensionBehavior& extbehavior = extensionBehavior();
    TExtensionBehavior::const_iterator iter = extbehavior.find(extension);
    return (iter != extbehavior.end());
}

void TParseContext::handleExtensionDirective(int line, const char* extName, const char* behavior)
{
    pp::SourceLocation loc;
    DecodeSourceLoc(line, &loc.file, &loc.line);
    directiveHandler.handleExtension(loc, extName, behavior);
}

void TParseContext::handlePragmaDirective(int line, const char* name, const char* value)
{
    pp::SourceLocation loc;
    DecodeSourceLoc(line, &loc.file, &loc.line);
    directiveHandler.handlePragma(loc, name, value);
}












const TFunction* TParseContext::findFunction(int line, TFunction* call, bool *builtIn)
{
    
    
    const TSymbol* symbol = symbolTable.find(call->getName(), builtIn);
    if (symbol == 0) {
        symbol = symbolTable.find(call->getMangledName(), builtIn);
    }

    if (symbol == 0) {
        error(line, "no matching overloaded function found", call->getName().c_str());
        return 0;
    }

    if (!symbol->isFunction()) {
        error(line, "function name expected", call->getName().c_str());
        return 0;
    }

    return static_cast<const TFunction*>(symbol);
}





bool TParseContext::executeInitializer(TSourceLoc line, TString& identifier, TPublicType& pType, 
                                       TIntermTyped* initializer, TIntermNode*& intermNode, TVariable* variable)
{
    TType type = TType(pType);

    if (variable == 0) {
        if (reservedErrorCheck(line, identifier))
            return true;

        if (voidErrorCheck(line, identifier, pType))
            return true;

        
        
        
        variable = new TVariable(&identifier, type);
        if (! symbolTable.insert(*variable)) {
            error(line, "redefinition", variable->getName().c_str());
            return true;
            
            
        }
    }

    
    
    
    TQualifier qualifier = variable->getType().getQualifier();
    if ((qualifier != EvqTemporary) && (qualifier != EvqGlobal) && (qualifier != EvqConst)) {
        error(line, " cannot initialize this type of qualifier ", variable->getType().getQualifierString());
        return true;
    }
    
    
    

    if (qualifier == EvqConst) {
        if (qualifier != initializer->getType().getQualifier()) {
            std::stringstream extraInfoStream;
            extraInfoStream << "'" << variable->getType().getCompleteString() << "'";
            std::string extraInfo = extraInfoStream.str();
            error(line, " assigning non-constant to", "=", extraInfo.c_str());
            variable->getType().setQualifier(EvqTemporary);
            return true;
        }
        if (type != initializer->getType()) {
            error(line, " non-matching types for const initializer ", 
                variable->getType().getQualifierString());
            variable->getType().setQualifier(EvqTemporary);
            return true;
        }
        if (initializer->getAsConstantUnion()) { 
            ConstantUnion* unionArray = variable->getConstPointer();

            if (type.getObjectSize() == 1 && type.getBasicType() != EbtStruct) {
                *unionArray = (initializer->getAsConstantUnion()->getUnionArrayPointer())[0];
            } else {
                variable->shareConstPointer(initializer->getAsConstantUnion()->getUnionArrayPointer());
            }
        } else if (initializer->getAsSymbolNode()) {
            const TSymbol* symbol = symbolTable.find(initializer->getAsSymbolNode()->getSymbol());
            const TVariable* tVar = static_cast<const TVariable*>(symbol);

            ConstantUnion* constArray = tVar->getConstPointer();
            variable->shareConstPointer(constArray);
        } else {
            std::stringstream extraInfoStream;
            extraInfoStream << "'" << variable->getType().getCompleteString() << "'";
            std::string extraInfo = extraInfoStream.str();
            error(line, " cannot assign to", "=", extraInfo.c_str());
            variable->getType().setQualifier(EvqTemporary);
            return true;
        }
    }
 
    if (qualifier != EvqConst) {
        TIntermSymbol* intermSymbol = intermediate.addSymbol(variable->getUniqueId(), variable->getName(), variable->getType(), line);
        intermNode = intermediate.addAssign(EOpInitialize, intermSymbol, initializer, line);
        if (intermNode == 0) {
            assignError(line, "=", intermSymbol->getCompleteString(), initializer->getCompleteString());
            return true;
        }
    } else 
        intermNode = 0;

    return false;
}

bool TParseContext::areAllChildConst(TIntermAggregate* aggrNode)
{
    ASSERT(aggrNode != NULL);
    if (!aggrNode->isConstructor())
        return false;

    bool allConstant = true;

    
    
    TIntermSequence &sequence = aggrNode->getSequence() ;
    for (TIntermSequence::iterator p = sequence.begin(); p != sequence.end(); ++p) {
        if (!(*p)->getAsTyped()->getAsConstantUnion())
            return false;
    }

    return allConstant;
}






TIntermTyped* TParseContext::addConstructor(TIntermNode* node, const TType* type, TOperator op, TFunction* fnCall, TSourceLoc line)
{
    if (node == 0)
        return 0;

    TIntermAggregate* aggrNode = node->getAsAggregate();
    
    TTypeList::const_iterator memberTypes;
    if (op == EOpConstructStruct)
        memberTypes = type->getStruct()->begin();
    
    TType elementType = *type;
    if (type->isArray())
        elementType.clearArrayness();

    bool singleArg;
    if (aggrNode) {
        if (aggrNode->getOp() != EOpNull || aggrNode->getSequence().size() == 1)
            singleArg = true;
        else
            singleArg = false;
    } else
        singleArg = true;

    TIntermTyped *newNode;
    if (singleArg) {
        
        
        if (type->isArray())
            newNode = constructStruct(node, &elementType, 1, node->getLine(), false);
        else if (op == EOpConstructStruct)
            newNode = constructStruct(node, (*memberTypes).type, 1, node->getLine(), false);
        else
            newNode = constructBuiltIn(type, op, node, node->getLine(), false);

        if (newNode && newNode->getAsAggregate()) {
            TIntermTyped* constConstructor = foldConstConstructor(newNode->getAsAggregate(), *type);
            if (constConstructor)
                return constConstructor;
        }

        return newNode;
    }
    
    
    
    
    TIntermSequence &sequenceVector = aggrNode->getSequence() ;    
    
    
    
    int paramCount = 0;  
    
    
    
    
    
    for (TIntermSequence::iterator p = sequenceVector.begin(); 
                                   p != sequenceVector.end(); p++, paramCount++) {
        if (type->isArray())
            newNode = constructStruct(*p, &elementType, paramCount+1, node->getLine(), true);
        else if (op == EOpConstructStruct)
            newNode = constructStruct(*p, (memberTypes[paramCount]).type, paramCount+1, node->getLine(), true);
        else
            newNode = constructBuiltIn(type, op, *p, node->getLine(), true);
        
        if (newNode) {
            *p = newNode;
        }
    }

    TIntermTyped* constructor = intermediate.setAggregateOperator(aggrNode, op, line);
    TIntermTyped* constConstructor = foldConstConstructor(constructor->getAsAggregate(), *type);
    if (constConstructor)
        return constConstructor;

    return constructor;
}

TIntermTyped* TParseContext::foldConstConstructor(TIntermAggregate* aggrNode, const TType& type)
{
    bool canBeFolded = areAllChildConst(aggrNode);
    aggrNode->setType(type);
    if (canBeFolded) {
        bool returnVal = false;
        ConstantUnion* unionArray = new ConstantUnion[type.getObjectSize()];
        if (aggrNode->getSequence().size() == 1)  {
            returnVal = intermediate.parseConstTree(aggrNode->getLine(), aggrNode, unionArray, aggrNode->getOp(), symbolTable,  type, true);
        }
        else {
            returnVal = intermediate.parseConstTree(aggrNode->getLine(), aggrNode, unionArray, aggrNode->getOp(), symbolTable,  type);
        }
        if (returnVal)
            return 0;

        return intermediate.addConstantUnion(unionArray, type, aggrNode->getLine());
    }

    return 0;
}








TIntermTyped* TParseContext::constructBuiltIn(const TType* type, TOperator op, TIntermNode* node, TSourceLoc line, bool subset)
{
    TIntermTyped* newNode;
    TOperator basicOp;

    
    
    
    switch (op) {
    case EOpConstructVec2:
    case EOpConstructVec3:
    case EOpConstructVec4:
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
    case EOpConstructFloat:
        basicOp = EOpConstructFloat;
        break;

    case EOpConstructIVec2:
    case EOpConstructIVec3:
    case EOpConstructIVec4:
    case EOpConstructInt:
        basicOp = EOpConstructInt;
        break;

    case EOpConstructBVec2:
    case EOpConstructBVec3:
    case EOpConstructBVec4:
    case EOpConstructBool:
        basicOp = EOpConstructBool;
        break;

    default:
        error(line, "unsupported construction", "");
        recover();

        return 0;
    }
    newNode = intermediate.addUnaryMath(basicOp, node, node->getLine(), symbolTable);
    if (newNode == 0) {
        error(line, "can't convert", "constructor");
        return 0;
    }

    
    
    
    
    
    if (subset || (newNode != node && newNode->getType() == *type))
        return newNode;

    
    return intermediate.setAggregateOperator(newNode, op, line);
}






TIntermTyped* TParseContext::constructStruct(TIntermNode* node, TType* type, int paramCount, TSourceLoc line, bool subset)
{
    if (*type == node->getAsTyped()->getType()) {
        if (subset)
            return node->getAsTyped();
        else
            return intermediate.setAggregateOperator(node->getAsTyped(), EOpConstructStruct, line);
    } else {
        std::stringstream extraInfoStream;
        extraInfoStream << "cannot convert parameter " << paramCount 
                        << " from '" << node->getAsTyped()->getType().getBasicString()
                        << "' to '" << type->getBasicString() << "'";
        std::string extraInfo = extraInfoStream.str();
        error(line, "", "constructor", extraInfo.c_str());
        recover();
    }

    return 0;
}








TIntermTyped* TParseContext::addConstVectorNode(TVectorFields& fields, TIntermTyped* node, TSourceLoc line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();

    ConstantUnion *unionArray;
    if (tempConstantNode) {
        unionArray = tempConstantNode->getUnionArrayPointer();
        ASSERT(unionArray);

        if (!unionArray) {
            return node;
        }
    } else { 
        error(line, "Cannot offset into the vector", "Error");
        recover();

        return 0;
    }

    ConstantUnion* constArray = new ConstantUnion[fields.num];

    for (int i = 0; i < fields.num; i++) {
        if (fields.offsets[i] >= node->getType().getObjectSize()) {
            std::stringstream extraInfoStream;
            extraInfoStream << "vector field selection out of range '" << fields.offsets[i] << "'";
            std::string extraInfo = extraInfoStream.str();
            error(line, "", "[", extraInfo.c_str());
            recover();
            fields.offsets[i] = 0;
        }
        
        constArray[i] = unionArray[fields.offsets[i]];

    } 
    typedNode = intermediate.addConstantUnion(constArray, node->getType(), line);
    return typedNode;
}







TIntermTyped* TParseContext::addConstMatrixNode(int index, TIntermTyped* node, TSourceLoc line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();

    if (index >= node->getType().getNominalSize()) {
        std::stringstream extraInfoStream;
        extraInfoStream << "matrix field selection out of range '" << index << "'";
        std::string extraInfo = extraInfoStream.str();
        error(line, "", "[", extraInfo.c_str());
        recover();
        index = 0;
    }

    if (tempConstantNode) {
         ConstantUnion* unionArray = tempConstantNode->getUnionArrayPointer();
         int size = tempConstantNode->getType().getNominalSize();
         typedNode = intermediate.addConstantUnion(&unionArray[size*index], tempConstantNode->getType(), line);
    } else {
        error(line, "Cannot offset into the matrix", "Error");
        recover();

        return 0;
    }

    return typedNode;
}








TIntermTyped* TParseContext::addConstArrayNode(int index, TIntermTyped* node, TSourceLoc line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();
    TType arrayElementType = node->getType();
    arrayElementType.clearArrayness();

    if (index >= node->getType().getArraySize()) {
        std::stringstream extraInfoStream;
        extraInfoStream << "array field selection out of range '" << index << "'";
        std::string extraInfo = extraInfoStream.str();
        error(line, "", "[", extraInfo.c_str());
        recover();
        index = 0;
    }

    int arrayElementSize = arrayElementType.getObjectSize();

    if (tempConstantNode) {
         ConstantUnion* unionArray = tempConstantNode->getUnionArrayPointer();
         typedNode = intermediate.addConstantUnion(&unionArray[arrayElementSize * index], tempConstantNode->getType(), line);
    } else {
        error(line, "Cannot offset into the array", "Error");
        recover();

        return 0;
    }

    return typedNode;
}







TIntermTyped* TParseContext::addConstStruct(TString& identifier, TIntermTyped* node, TSourceLoc line)
{
    const TTypeList* fields = node->getType().getStruct();
    TIntermTyped *typedNode;
    int instanceSize = 0;
    unsigned int index = 0;
    TIntermConstantUnion *tempConstantNode = node->getAsConstantUnion();

    for ( index = 0; index < fields->size(); ++index) {
        if ((*fields)[index].type->getFieldName() == identifier) {
            break;
        } else {
            instanceSize += (*fields)[index].type->getObjectSize();
        }
    }

    if (tempConstantNode) {
         ConstantUnion* constArray = tempConstantNode->getUnionArrayPointer();

         typedNode = intermediate.addConstantUnion(constArray+instanceSize, tempConstantNode->getType(), line); 
    } else {
        error(line, "Cannot offset into the structure", "Error");
        recover();

        return 0;
    }

    return typedNode;
}

bool TParseContext::enterStructDeclaration(int line, const TString& identifier)
{
    ++structNestingLevel;

    
    
    
    if (structNestingLevel > 1) {
        error(line, "", "Embedded struct definitions are not allowed");
        return true;
    }

    return false;
}

void TParseContext::exitStructDeclaration()
{
    --structNestingLevel;
}

namespace {

const int kWebGLMaxStructNesting = 4;

}  

bool TParseContext::structNestingErrorCheck(TSourceLoc line, const TType& fieldType)
{
    if (!isWebGLBasedSpec(shaderSpec)) {
        return false;
    }

    if (fieldType.getBasicType() != EbtStruct) {
        return false;
    }

    
    
    if (1 + fieldType.getDeepestStructNesting() > kWebGLMaxStructNesting) {
        std::stringstream extraInfoStream;
        extraInfoStream << "Reference of struct type " << fieldType.getTypeName() 
                        << " exceeds maximum struct nesting of " << kWebGLMaxStructNesting;
        std::string extraInfo = extraInfoStream.str();
        error(line, "", "", extraInfo.c_str());
        return true;
    }

    return false;
}






int PaParseStrings(int count, const char* const string[], const int length[],
                   TParseContext* context) {
    if ((count == 0) || (string == NULL))
        return 1;

    if (glslang_initialize(context))
        return 1;

    int error = glslang_scan(count, string, length, context);
    if (!error)
        error = glslang_parse(context);

    glslang_finalize(context);

    return (error == 0) && (context->numErrors() == 0) ? 0 : 1;
}



