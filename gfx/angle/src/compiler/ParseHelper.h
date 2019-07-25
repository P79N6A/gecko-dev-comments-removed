




#ifndef _PARSER_HELPER_INCLUDED_
#define _PARSER_HELPER_INCLUDED_

#include "compiler/ExtensionBehavior.h"
#include "compiler/localintermediate.h"
#include "compiler/ShHandle.h"
#include "compiler/SymbolTable.h"

struct TMatrixFields {
    bool wholeRow;
    bool wholeCol;
    int row;
    int col;
};

struct TPragma {
    TPragma(bool o, bool d) : optimize(o), debug(d) { }
    bool optimize;
    bool debug;
    TPragmaTable pragmaTable;
};





struct TParseContext {
    TParseContext(TSymbolTable& symt, const TExtensionBehavior& ext, TIntermediate& interm, ShShaderType type, ShShaderSpec spec, TInfoSink& is) :
            intermediate(interm), symbolTable(symt), extensionBehavior(ext), infoSink(is), shaderType(type), shaderSpec(spec), treeRoot(0),
            recoveredFromError(false), numErrors(0), lexAfterType(false), loopNestingLevel(0),
            inTypeParen(false), contextPragma(true, false) {  }
    TIntermediate& intermediate; 
    TSymbolTable& symbolTable;   
    TExtensionBehavior extensionBehavior;  
    TInfoSink& infoSink;
    ShShaderType shaderType;              
    ShShaderSpec shaderSpec;              
    TIntermNode* treeRoot;       
    bool recoveredFromError;     
    int numErrors;
    bool lexAfterType;           
    int loopNestingLevel;        
    bool inTypeParen;            
    const TType* currentFunctionType;  
    bool functionReturnsValue;   

    void error(TSourceLoc, const char *szReason, const char *szToken,
               const char *szExtraInfoFormat, ...);
    bool reservedErrorCheck(int line, const TString& identifier);
    void recover();

    bool parseVectorFields(const TString&, int vecSize, TVectorFields&, int line);
    bool parseMatrixFields(const TString&, int matSize, TMatrixFields&, int line);
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
    bool containsSampler(TType& type);
    bool nonInitConstErrorCheck(int line, TString& identifier, TPublicType& type);
    bool nonInitErrorCheck(int line, TString& identifier, TPublicType& type);
    bool paramErrorCheck(int line, TQualifier qualifier, TQualifier paramQualifier, TType* type);
    bool extensionErrorCheck(int line, const TString&);
    const TFunction* findFunction(int line, TFunction* pfnCall, bool *builtIn = 0);
    bool executeInitializer(TSourceLoc line, TString& identifier, TPublicType& pType,
                            TIntermTyped* initializer, TIntermNode*& intermNode, TVariable* variable = 0);
    bool areAllChildConst(TIntermAggregate* aggrNode);
    TIntermTyped* addConstructor(TIntermNode*, const TType*, TOperator, TFunction*, TSourceLoc);
    TIntermTyped* foldConstConstructor(TIntermAggregate* aggrNode, const TType& type);
    TIntermTyped* constructStruct(TIntermNode*, TType*, int, TSourceLoc, bool subset);
    TIntermTyped* constructBuiltIn(const TType*, TOperator, TIntermNode*, TSourceLoc, bool subset);
    TIntermTyped* addConstVectorNode(TVectorFields&, TIntermTyped*, TSourceLoc);
    TIntermTyped* addConstMatrixNode(int , TIntermTyped*, TSourceLoc);
    TIntermTyped* addConstArrayNode(int index, TIntermTyped* node, TSourceLoc line);
    TIntermTyped* addConstStruct(TString& , TIntermTyped*, TSourceLoc);
    bool arraySetMaxSize(TIntermSymbol*, TType*, int, bool, TSourceLoc);
    struct TPragma contextPragma;
    TString HashErrMsg;
    bool AfterEOF;
};

int PaParseStrings(const char* const argv[], const int strLen[], int argc, TParseContext&);
void PaReservedWord();
int PaIdentOrType(TString& id, TParseContext&, TSymbol*&);
int PaParseComment(int &lineno, TParseContext&);
void setInitialState();

typedef TParseContext* TParseContextPointer;
extern TParseContextPointer& GetGlobalParseContext();
#define GlobalParseContext GetGlobalParseContext()

typedef struct TThreadParseContextRec
{
    TParseContext *lpGlobalParseContext;
} TThreadParseContext;

#endif 
