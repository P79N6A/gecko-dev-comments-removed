






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

struct StmtInfoTC : public StmtInfoBase {
    StmtInfoTC      *down;          
    StmtInfoTC      *downScope;     

    uint32_t        blockid;        

    
    bool            isFunctionBodyBlock;

    StmtInfoTC(JSContext *cx) : StmtInfoBase(cx), isFunctionBodyBlock(false) {}
};

typedef HashSet<JSAtom *> FuncStmtSet;
struct Parser;
struct SharedContext;

typedef Vector<Definition *, 16> DeclVector;









struct TreeContext                  
{
    typedef StmtInfoTC StmtInfo;

    SharedContext   *sc;            

    uint32_t        bodyid;         
    uint32_t        blockidGen;     

    StmtInfoTC      *topStmt;       
    StmtInfoTC      *topScopeStmt;  
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
        JS_ASSERT(sc->inFunction());
        return args_.length();
    }

    uint32_t numVars() const {
        JS_ASSERT(sc->inFunction());
        return vars_.length();
    }

    

























    bool define(JSContext *cx, PropertyName *name, ParseNode *pn, Definition::Kind);

    







    void popLetDecl(JSAtom *atom);

    
    void prepareToAddDuplicateArg(Definition *prevDecl);

    
    void updateDecl(JSAtom *atom, ParseNode *newDecl);

    













    bool generateFunctionBindings(JSContext *cx, Bindings *bindings) const;

  public:
    ParseNode       *yieldNode;     


    FunctionBox     *functionList;

    
    
    
    CompileError    *queuedStrictModeError;

  private:
    TreeContext     **parserTC;     



  public:
    OwnedAtomDefnMapPtr lexdeps;    

    TreeContext     *parent;        

    ParseNode       *innermostWith; 

    FuncStmtSet     *funcStmts;     



    



    bool            hasReturnExpr:1; 
    bool            hasReturnVoid:1; 

    bool            inForInit:1;    

    
    
    
    
    
    
    
    
    
    bool            inDeclDestructuring:1;

    inline TreeContext(Parser *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid);
    inline ~TreeContext();

    inline bool init();

    inline void setQueuedStrictModeError(CompileError *e);

    unsigned blockid();

    
    
    
    
    
    
    
    bool atBodyLevel();
};

bool
GenerateBlockId(TreeContext *tc, uint32_t &blockid);

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

    TreeContext         *tc;            

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

#if JS_HAS_XML_SUPPORT
    ParseNode *parseXMLText(JSObject *chain, bool allowList);
#endif

    



    ObjectBox *newObjectBox(JSObject *obj);

    FunctionBox *newFunctionBox(JSObject *obj, ParseNode *fn, TreeContext *tc,
                                StrictMode::StrictModeState sms);

    



    JSFunction *newFunction(TreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind);

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

    
    JS_DECLARE_NEW_METHODS(allocParseNode, inline)

    ParseNode *cloneNode(const ParseNode &other) {
        ParseNode *node = allocParseNode(sizeof(ParseNode));
        if (!node)
            return NULL;
        PodAssign(node, &other);
        return node;
    }

    
    ParseNode *statement();
    bool processDirectives(ParseNode *stringsAtStart);

    



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
    ParseNode *primaryExpr(TokenKind tt, bool afterDoubleDot);
    ParseNode *parenExpr(bool *genexp = NULL);

    


    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(ParseNode **list, bool &hasRest);

    ParseNode *functionDef(HandlePropertyName name, FunctionType type, FunctionSyntaxKind kind);

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

    bool checkForFunctionNode(PropertyName *name, ParseNode *node);

    ParseNode *identifierName(bool afterDoubleDot);
    ParseNode *intrinsicName();

#if JS_HAS_XML_SUPPORT
    
    
    
    
    
    
    
    
    
    bool allowsXML() const {
        return tc->sc->strictModeState == StrictMode::NOTSTRICT && tokenStream.allowsXML();
    }

    ParseNode *endBracketedExpr();

    ParseNode *propertySelector();
    ParseNode *qualifiedSuffix(ParseNode *pn);
    ParseNode *qualifiedIdentifier();
    ParseNode *attributeIdentifier();
    ParseNode *xmlExpr(bool inTag);
    ParseNode *xmlNameExpr();
    ParseNode *xmlTagContent(ParseNodeKind tagkind, JSAtom **namep);
    bool xmlElementContent(ParseNode *pn);
    ParseNode *xmlElementOrList(bool allowList);
    ParseNode *xmlElementOrListRoot(bool allowList);

    ParseNode *starOrAtPropertyIdentifier(TokenKind tt);
    ParseNode *propertyQualifiedIdentifier();
#endif 

    bool setStrictMode(bool strictMode);
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
    bool result = tokenStream.reportCompileErrorNumberVA(pn, JSREPORT_STRICT | JSREPORT_WARNING,
                                                         errorNumber, args);
    va_end(args);
    return result;
}

inline bool
Parser::reportStrictModeError(ParseNode *pn, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportStrictModeErrorNumberVA(pn, errorNumber, args);
    va_end(args);
    return result;
}

bool
DefineArg(Parser *parser, ParseNode *funcpn, HandlePropertyName name, bool destructuringArg = false,
          Definition **duplicatedArg = NULL);

} 
} 




#define TS(p) (&(p)->tokenStream)

#endif 
