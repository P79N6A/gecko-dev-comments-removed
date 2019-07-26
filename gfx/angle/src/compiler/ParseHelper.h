




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
            lexAfterType(false),
            loopNestingLevel(0),
            structNestingLevel(0),
            inTypeParen(false),
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
    bool lexAfterType;           
    int loopNestingLevel;        
    int structNestingLevel;      
    bool inTypeParen;            
    const TType* currentFunctionType;  
    bool functionReturnsValue;   
    bool checksPrecisionErrors;  
    TString HashErrMsg;
    bool AfterEOF;
    TDiagnostics diagnostics;
    TDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor;
    void* scanner;

    int numErrors() const { return diagnostics.numErrors(); }
    TInfoSink& infoSink() { return diagnostics.infoSink(); }
    void error(TSourceLoc loc, const char *reason, const char* token,
               const char* extraInfo="");
    void warning(TSourceLoc loc, const char* reason, const char* token,
                 const char* extraInfo="");
    void trace(const char* str);
    void recover();

    bool parseVectorFields(const TString&, int vecSize, TVectorFields&, int line);
    bool parseMatrixFields(const TString&, int matSize, TMatrixFields&, int line);

    bool reservedErrorCheck(int line, const TString& identifier);
    void assignError(int line, const char* op, TString left, TString right);
    void unaryOpError(int line, const char* op, TString operand);
    void binaryOpError(int line, const char* op, TString left, TString right);
    bool precisionErrorCheck(int line, TPrecision precision, TBasicType type);
    bool lValueErrorCheck(int line, const char* op, TIntermTyped*);
    bool constErrorCheck(TIntermTyped* node);
    bool integerErrorCheck(TIntermTyped* node, const char* token);
    bool globalErrorCheck(int line, bool global, const char* token);
    bool constructorErrorCheck(int line, TIntermNode*, TFunction&, TOperator, TType*);
    bool arraySizeErrorCheck(int line, TIntermTyped* expr, int& size);
    bool arrayQualifierErrorCheck(int line, TPublicType type);
    bool arrayTypeErrorCheck(int line, TPublicType type);
    bool arrayErrorCheck(int line, TString& identifier, TPublicType type, TVariable*& variable);
    bool voidErrorCheck(int, const TString&, const TPublicType&);
    bool boolErrorCheck(int, const TIntermTyped*);
    bool boolErrorCheck(int, const TPublicType&);
    bool samplerErrorCheck(int line, const TPublicType& pType, const char* reason);
    bool structQualifierErrorCheck(int line, const TPublicType& pType);
    bool parameterSamplerErrorCheck(int line, TQualifier qualifier, const TType& type);
    bool nonInitConstErrorCheck(int line, TString& identifier, TPublicType& type, bool array);
    bool nonInitErrorCheck(int line, TString& identifier, TPublicType& type, TVariable*& variable);
    bool paramErrorCheck(int line, TQualifier qualifier, TQualifier paramQualifier, TType* type);
    bool extensionErrorCheck(int line, const TString&);

    const TExtensionBehavior& extensionBehavior() const { return directiveHandler.extensionBehavior(); }
    bool supportsExtension(const char* extension);
    void handleExtensionDirective(int line, const char* extName, const char* behavior);

    const TPragma& pragma() const { return directiveHandler.pragma(); }
    void handlePragmaDirective(int line, const char* name, const char* value);

    bool containsSampler(TType& type);
    bool areAllChildConst(TIntermAggregate* aggrNode);
    const TFunction* findFunction(int line, TFunction* pfnCall, bool *builtIn = 0);
    bool executeInitializer(TSourceLoc line, TString& identifier, TPublicType& pType,
                            TIntermTyped* initializer, TIntermNode*& intermNode, TVariable* variable = 0);
    bool arraySetMaxSize(TIntermSymbol*, TType*, int, bool, TSourceLoc);

    TIntermTyped* addConstructor(TIntermNode*, const TType*, TOperator, TFunction*, TSourceLoc);
    TIntermTyped* foldConstConstructor(TIntermAggregate* aggrNode, const TType& type);
    TIntermTyped* constructStruct(TIntermNode*, TType*, int, TSourceLoc, bool subset);
    TIntermTyped* constructBuiltIn(const TType*, TOperator, TIntermNode*, TSourceLoc, bool subset);
    TIntermTyped* addConstVectorNode(TVectorFields&, TIntermTyped*, TSourceLoc);
    TIntermTyped* addConstMatrixNode(int , TIntermTyped*, TSourceLoc);
    TIntermTyped* addConstArrayNode(int index, TIntermTyped* node, TSourceLoc line);
    TIntermTyped* addConstStruct(TString& , TIntermTyped*, TSourceLoc);

    
    
    
    bool enterStructDeclaration(TSourceLoc line, const TString& identifier);
    void exitStructDeclaration();

    bool structNestingErrorCheck(TSourceLoc line, const TType& fieldType);
};

int PaParseStrings(int count, const char* const string[], const int length[],
                   TParseContext* context);

#endif 
