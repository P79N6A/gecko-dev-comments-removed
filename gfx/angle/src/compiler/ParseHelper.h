




#ifndef _PARSER_HELPER_INCLUDED_
#define _PARSER_HELPER_INCLUDED_

#include "compiler/Diagnostics.h"
#include "compiler/DirectiveHandler.h"
#include "compiler/localintermediate.h"
#include "compiler/preprocessor/Preprocessor.h"
#include "compiler/ShHandle.h"
#include "compiler/SymbolTable.h"

struct TMatrixFields {
    bool wholeRow;
    bool wholeCol;
    int row;
    int col;
};





struct TParseContext {
    TParseContext(TSymbolTable& symt, TExtensionBehavior& ext, TIntermediate& interm, ShShaderType type, ShShaderSpec spec, int options, bool checksPrecErrors, const char* sourcePath, TInfoSink& is) :
            intermediate(interm),
            symbolTable(symt),
            shaderType(type),
            shaderSpec(spec),
            compileOptions(options),
            sourcePath(sourcePath),
            treeRoot(0),
            loopNestingLevel(0),
            structNestingLevel(0),
            currentFunctionType(NULL),
            functionReturnsValue(false),
            checksPrecisionErrors(checksPrecErrors),
            diagnostics(is),
            directiveHandler(ext, diagnostics),
            preprocessor(&diagnostics, &directiveHandler),
            scanner(NULL) {  }
    TIntermediate& intermediate; 
    TSymbolTable& symbolTable;   
    ShShaderType shaderType;              
    ShShaderSpec shaderSpec;              
    int compileOptions;
    const char* sourcePath;      
    TIntermNode* treeRoot;       
    int loopNestingLevel;        
    int structNestingLevel;      
    const TType* currentFunctionType;  
    bool functionReturnsValue;   
    bool checksPrecisionErrors;  
    bool fragmentPrecisionHigh;  
    TString HashErrMsg;
    TDiagnostics diagnostics;
    TDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor;
    void* scanner;

    int numErrors() const { return diagnostics.numErrors(); }
    TInfoSink& infoSink() { return diagnostics.infoSink(); }
    void error(const TSourceLoc& loc, const char *reason, const char* token,
               const char* extraInfo="");
    void warning(const TSourceLoc& loc, const char* reason, const char* token,
                 const char* extraInfo="");
    void trace(const char* str);
    void recover();

    bool parseVectorFields(const TString&, int vecSize, TVectorFields&, const TSourceLoc& line);
    bool parseMatrixFields(const TString&, int matSize, TMatrixFields&, const TSourceLoc& line);

    bool reservedErrorCheck(const TSourceLoc& line, const TString& identifier);
    void assignError(const TSourceLoc& line, const char* op, TString left, TString right);
    void unaryOpError(const TSourceLoc& line, const char* op, TString operand);
    void binaryOpError(const TSourceLoc& line, const char* op, TString left, TString right);
    bool precisionErrorCheck(const TSourceLoc& line, TPrecision precision, TBasicType type);
    bool lValueErrorCheck(const TSourceLoc& line, const char* op, TIntermTyped*);
    bool constErrorCheck(TIntermTyped* node);
    bool integerErrorCheck(TIntermTyped* node, const char* token);
    bool globalErrorCheck(const TSourceLoc& line, bool global, const char* token);
    bool constructorErrorCheck(const TSourceLoc& line, TIntermNode*, TFunction&, TOperator, TType*);
    bool arraySizeErrorCheck(const TSourceLoc& line, TIntermTyped* expr, int& size);
    bool arrayQualifierErrorCheck(const TSourceLoc& line, TPublicType type);
    bool arrayTypeErrorCheck(const TSourceLoc& line, TPublicType type);
    bool arrayErrorCheck(const TSourceLoc& line, TString& identifier, TPublicType type, TVariable*& variable);
    bool voidErrorCheck(const TSourceLoc&, const TString&, const TPublicType&);
    bool boolErrorCheck(const TSourceLoc&, const TIntermTyped*);
    bool boolErrorCheck(const TSourceLoc&, const TPublicType&);
    bool samplerErrorCheck(const TSourceLoc& line, const TPublicType& pType, const char* reason);
    bool structQualifierErrorCheck(const TSourceLoc& line, const TPublicType& pType);
    bool parameterSamplerErrorCheck(const TSourceLoc& line, TQualifier qualifier, const TType& type);
    bool nonInitConstErrorCheck(const TSourceLoc& line, TString& identifier, TPublicType& type, bool array);
    bool nonInitErrorCheck(const TSourceLoc& line, TString& identifier, TPublicType& type, TVariable*& variable);
    bool paramErrorCheck(const TSourceLoc& line, TQualifier qualifier, TQualifier paramQualifier, TType* type);
    bool extensionErrorCheck(const TSourceLoc& line, const TString&);

    const TPragma& pragma() const { return directiveHandler.pragma(); }
    const TExtensionBehavior& extensionBehavior() const { return directiveHandler.extensionBehavior(); }
    bool supportsExtension(const char* extension);
    bool isExtensionEnabled(const char* extension) const;

    bool containsSampler(TType& type);
    bool areAllChildConst(TIntermAggregate* aggrNode);
    const TFunction* findFunction(const TSourceLoc& line, TFunction* pfnCall, bool *builtIn = 0);
    bool executeInitializer(const TSourceLoc& line, TString& identifier, TPublicType& pType,
                            TIntermTyped* initializer, TIntermNode*& intermNode, TVariable* variable = 0);

    TIntermTyped* addConstructor(TIntermNode*, const TType*, TOperator, TFunction*, const TSourceLoc&);
    TIntermTyped* foldConstConstructor(TIntermAggregate* aggrNode, const TType& type);
    TIntermTyped* constructStruct(TIntermNode*, TType*, int, const TSourceLoc&, bool subset);
    TIntermTyped* constructBuiltIn(const TType*, TOperator, TIntermNode*, const TSourceLoc&, bool subset);
    TIntermTyped* addConstVectorNode(TVectorFields&, TIntermTyped*, const TSourceLoc&);
    TIntermTyped* addConstMatrixNode(int , TIntermTyped*, const TSourceLoc&);
    TIntermTyped* addConstArrayNode(int index, TIntermTyped* node, const TSourceLoc& line);
    TIntermTyped* addConstStruct(TString& , TIntermTyped*, const TSourceLoc&);
    TIntermTyped* addIndexExpression(TIntermTyped *baseExpression, const TSourceLoc& location, TIntermTyped *indexExpression);

    
    
    
    bool enterStructDeclaration(const TSourceLoc& line, const TString& identifier);
    void exitStructDeclaration();

    bool structNestingErrorCheck(const TSourceLoc& line, const TField& field);
};

int PaParseStrings(size_t count, const char* const string[], const int length[],
                   TParseContext* context);

#endif 
