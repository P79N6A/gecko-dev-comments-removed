






#ifndef Parser_h__
#define Parser_h__




#include "jsversion.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsscript.h"
#include "jswin.h"

#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "frontend/SharedContext.h"

namespace js {
namespace frontend {

struct StmtInfoPC : public StmtInfoBase {
    StmtInfoPC      *down;          
    StmtInfoPC      *downScope;     

    uint32_t        blockid;        

    StmtInfoPC(JSContext *cx) : StmtInfoBase(cx) {}
};

typedef HashSet<JSAtom *> FuncStmtSet;
struct Parser;
class SharedContext;

typedef Vector<Definition *, 16> DeclVector;









struct ParseContext                 
{
    typedef StmtInfoPC StmtInfo;

    SharedContext   *sc;            

    uint32_t        bodyid;         
    uint32_t        blockidGen;     

    StmtInfoPC      *topStmt;       
    StmtInfoPC      *topScopeStmt;  
    Rooted<StaticBlockObject *> blockChain;
                                    

    const unsigned  staticLevel;    

    uint32_t        parenDepth;     

    uint32_t        yieldCount;     

    ParseNode       *blockNode;     

  private:
    AtomDecls       decls_;         
    DeclVector      args_;          
    DeclVector      vars_;          

  public:
    const AtomDecls &decls() const {
        return decls_;
    }

    uint32_t numArgs() const {
        JS_ASSERT(sc->isFunctionBox());
        return args_.length();
    }

    uint32_t numVars() const {
        JS_ASSERT(sc->isFunctionBox());
        return vars_.length();
    }

    

























    bool define(JSContext *cx, HandlePropertyName name, ParseNode *pn, Definition::Kind);

    







    void popLetDecl(JSAtom *atom);

    
    void prepareToAddDuplicateArg(Definition *prevDecl);

    
    void updateDecl(JSAtom *atom, ParseNode *newDecl);

    













    bool generateFunctionBindings(JSContext *cx, InternalHandle<Bindings*> bindings) const;

  public:
    ParseNode       *yieldNode;     



  private:
    ParseContext    **parserPC;     



  public:
    OwnedAtomDefnMapPtr lexdeps;    

    ParseContext     *parent;       

    FuncStmtSet     *funcStmts;     



    
    
    bool            funHasReturnExpr:1; 
    bool            funHasReturnVoid:1; 

    
    
    bool            parsingForInit:1;   

    bool            parsingWith:1;  




    
    
    
    
    
    
    
    
    
    bool            inDeclDestructuring:1;

    
    
    bool            funBecameStrict:1;

    inline ParseContext(Parser *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid);
    inline ~ParseContext();

    inline bool init();

    unsigned blockid();

    
    
    
    
    
    
    
    bool atBodyLevel();
};

bool
GenerateBlockId(ParseContext *pc, uint32_t &blockid);

struct BindData;

enum FunctionSyntaxKind { Expression, Statement };
enum LetContext { LetExpresion, LetStatement };
enum VarContext { HoistVars, DontHoistVars };

struct Parser : private AutoGCRooter
{
    JSContext           *const context; 
    StrictModeGetter    strictModeGetter; 
    TokenStream         tokenStream;
    void                *tempPoolMark;  
    ParseNodeAllocator  allocator;
    ObjectBox           *traceListHead; 

    ParseContext        *pc;            

    SourceCompressionToken *sct;        

    
    AutoKeepAtoms       keepAtoms;

    
    const bool          foldConstants:1;

  private:
    
    const bool          compileAndGo:1;

    













    const bool          selfHostingMode:1;

  public:
    Parser(JSContext *cx, const CompileOptions &options,
           const jschar *chars, size_t length, bool foldConstants);
    ~Parser();

    friend void AutoGCRooter::trace(JSTracer *trc);

    





    bool init();

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    


    ParseNode *parse(JSObject *chain);

    



    ObjectBox *newObjectBox(JSObject *obj);

    FunctionBox *newFunctionBox(JSFunction *fun, ParseContext *pc, bool strict);

    



    JSFunction *newFunction(ParseContext *pc, HandleAtom atom, FunctionSyntaxKind kind);

    void trace(JSTracer *trc);

    


    inline bool reportError(ParseNode *pn, unsigned errorNumber, ...);
    inline bool reportUcError(ParseNode *pn, unsigned errorNumber, ...);
    inline bool reportWarning(ParseNode *pn, unsigned errorNumber, ...);
    inline bool reportStrictWarning(ParseNode *pn, unsigned errorNumber, ...);
    inline bool reportStrictModeError(ParseNode *pn, unsigned errorNumber, ...);
    typedef bool (Parser::*Reporter)(ParseNode *pn, unsigned errorNumber, ...);

  private:
    Parser *thisForCtor() { return this; }

    ParseNode *allocParseNode(size_t size) {
        JS_ASSERT(size == sizeof(ParseNode));
        return static_cast<ParseNode *>(allocator.allocNode());
    }

    



    ParseNode *atomNode(ParseNodeKind kind, JSOp op);

  public:
    ParseNode *freeTree(ParseNode *pn) { return allocator.freeTree(pn); }
    void prepareNodeForMutation(ParseNode *pn) { return allocator.prepareNodeForMutation(pn); }

    
    JS_DECLARE_NEW_METHODS(new_, allocParseNode, inline)

    ParseNode *cloneNode(const ParseNode &other) {
        ParseNode *node = allocParseNode(sizeof(ParseNode));
        if (!node)
            return NULL;
        PodAssign(node, &other);
        return node;
    }

    
    ParseNode *statement();
    bool maybeParseDirective(ParseNode *pn, bool *cont);

    
    ParseNode *standaloneFunctionBody(HandleFunction fun, const AutoNameVector &formals, HandleScript script,
                                      ParseNode *fn, FunctionBox **funbox, bool strict,
                                      bool *becameStrict = NULL);

    



    enum FunctionBodyType { StatementListBody, ExpressionBody };
    ParseNode *functionBody(FunctionBodyType type);

  private:
    















    ParseNode *functionStmt();
    ParseNode *functionExpr();
    ParseNode *statements(bool *hasFunctionStmt = NULL);

    ParseNode *switchStatement();
    ParseNode *forStatement();
    ParseNode *tryStatement();
    ParseNode *withStatement();
#if JS_HAS_BLOCK_SCOPE
    ParseNode *letStatement();
#endif
    ParseNode *expressionStatement();
    ParseNode *variables(ParseNodeKind kind, StaticBlockObject *blockObj = NULL,
                         VarContext varContext = HoistVars);
    ParseNode *expr();
    ParseNode *assignExpr();
    ParseNode *assignExprWithoutYield(unsigned err);
    ParseNode *condExpr1();
    ParseNode *orExpr1();
    ParseNode *andExpr1i();
    ParseNode *andExpr1n();
    ParseNode *bitOrExpr1i();
    ParseNode *bitOrExpr1n();
    ParseNode *bitXorExpr1i();
    ParseNode *bitXorExpr1n();
    ParseNode *bitAndExpr1i();
    ParseNode *bitAndExpr1n();
    ParseNode *eqExpr1i();
    ParseNode *eqExpr1n();
    ParseNode *relExpr1i();
    ParseNode *relExpr1n();
    ParseNode *shiftExpr1i();
    ParseNode *shiftExpr1n();
    ParseNode *addExpr1i();
    ParseNode *addExpr1n();
    ParseNode *mulExpr1i();
    ParseNode *mulExpr1n();
    ParseNode *unaryExpr();
    ParseNode *memberExpr(bool allowCallSyntax);
    ParseNode *primaryExpr(TokenKind tt);
    ParseNode *parenExpr(bool *genexp = NULL);

    


    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(ParseNode **list, ParseNode *funcpn, bool &hasRest);

    ParseNode *functionDef(HandlePropertyName name, const TokenStream::Position &start,
                           FunctionType type, FunctionSyntaxKind kind);
    bool functionArgsAndBody(ParseNode *pn, HandleFunction fun, HandlePropertyName funName,
                             FunctionType type, FunctionSyntaxKind kind, bool strict,
                             bool *becameStrict = NULL);

    ParseNode *unaryOpExpr(ParseNodeKind kind, JSOp op);

    ParseNode *condition();
    ParseNode *comprehensionTail(ParseNode *kid, unsigned blockid, bool isGenexp,
                                 ParseNodeKind kind = PNK_SEMI, JSOp op = JSOP_NOP);
    ParseNode *generatorExpr(ParseNode *kid);
    bool argumentList(ParseNode *listNode);
    ParseNode *bracketedExpr();
    ParseNode *letBlock(LetContext letContext);
    ParseNode *returnOrYield(bool useAssignExpr);
    ParseNode *destructuringExpr(BindData *data, TokenKind tt);

    ParseNode *identifierName();

    bool allowsForEachIn() {
#if !JS_HAS_FOR_EACH_IN
        return false;
#else
        return versionNumber() >= JSVERSION_1_6;
#endif
    }

    bool setAssignmentLhsOps(ParseNode *pn, JSOp op);
    bool matchInOrOf(bool *isForOfp);
};

inline bool
Parser::reportError(ParseNode *pn, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportCompileErrorNumberVA(pn, JSREPORT_ERROR, errorNumber, args);
    va_end(args);
    return result;
}

inline bool
Parser::reportUcError(ParseNode *pn, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportCompileErrorNumberVA(pn, JSREPORT_UC | JSREPORT_ERROR,
                                                         errorNumber, args);
    va_end(args);
    return result;
}

inline bool
Parser::reportWarning(ParseNode *pn, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportCompileErrorNumberVA(pn, JSREPORT_WARNING, errorNumber, args);
    va_end(args);
    return result;
}

inline bool
Parser::reportStrictWarning(ParseNode *pn, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportStrictWarningErrorNumberVA(pn, errorNumber, args);
    va_end(args);
    return result;
}

inline bool
Parser::reportStrictModeError(ParseNode *pn, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result =
        tokenStream.reportStrictModeErrorNumberVA(pn, pc->sc->strict, errorNumber, args);
    va_end(args);
    return result;
}

bool
DefineArg(Parser *parser, ParseNode *funcpn, HandlePropertyName name,
          bool disallowDuplicateArgs = false, Definition **duplicatedArg = NULL);

} 
} 




#define TS(p) (&(p)->tokenStream)

#endif 
