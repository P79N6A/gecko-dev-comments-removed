





#ifndef frontend_Parser_h
#define frontend_Parser_h




#include "jsprvtd.h"
#include "jspubtd.h"

#include "frontend/BytecodeCompiler.h"
#include "frontend/FullParseHandler.h"
#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "frontend/SharedContext.h"
#include "frontend/SyntaxParseHandler.h"

namespace js {
namespace frontend {

struct StmtInfoPC : public StmtInfoBase {
    StmtInfoPC      *down;          
    StmtInfoPC      *downScope;     

    uint32_t        blockid;        

    StmtInfoPC(ExclusiveContext *cx) : StmtInfoBase(cx) {}
};

typedef HashSet<JSAtom *> FuncStmtSet;
class SharedContext;

typedef Vector<Definition *, 16> DeclVector;

struct GenericParseContext
{
    
    GenericParseContext *parent;

    
    SharedContext *sc;

    
    

    
    bool funHasReturnExpr:1;

    
    bool funHasReturnVoid:1;

    
    

    
    bool parsingForInit:1;

    
    
    bool parsingWith:1;

    GenericParseContext(GenericParseContext *parent, SharedContext *sc)
      : parent(parent),
        sc(sc),
        funHasReturnExpr(false),
        funHasReturnVoid(false),
        parsingForInit(false),
        parsingWith(parent ? parent->parsingWith : false)
    {}
};

template <typename ParseHandler>
bool
GenerateBlockId(ParseContext<ParseHandler> *pc, uint32_t &blockid);









template <typename ParseHandler>
struct ParseContext : public GenericParseContext
{
    typedef StmtInfoPC StmtInfo;
    typedef typename ParseHandler::Node Node;
    typedef typename ParseHandler::DefinitionNode DefinitionNode;

    uint32_t        bodyid;         
    uint32_t        blockidGen;     

    StmtInfoPC      *topStmt;       
    StmtInfoPC      *topScopeStmt;  
    Rooted<StaticBlockObject *> blockChain;
                                    

    const unsigned  staticLevel;    

    uint32_t        parenDepth;     

    uint32_t        yieldCount;     

    Node            blockNode;      

  private:
    AtomDecls<ParseHandler> decls_; 
    DeclVector      args_;          
    DeclVector      vars_;          

  public:
    const AtomDecls<ParseHandler> &decls() const {
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

    

























    bool define(TokenStream &ts, PropertyName *name, Node pn, Definition::Kind);

    







    void popLetDecl(JSAtom *atom);

    
    void prepareToAddDuplicateArg(HandlePropertyName name, DefinitionNode prevDecl);

    
    void updateDecl(JSAtom *atom, Node newDecl);

    













    bool generateFunctionBindings(ExclusiveContext *cx, LifoAlloc &alloc,
                                  InternalHandle<Bindings*> bindings) const;

  public:
    uint32_t         yieldOffset;   



  private:
    ParseContext    **parserPC;     



    
    
    ParseContext<ParseHandler> *oldpc;

  public:
    OwnedAtomDefnMapPtr lexdeps;    

    FuncStmtSet     *funcStmts;     



    
    AutoFunctionVector innerFunctions;

    
    
    
    
    
    
    
    
    
    bool            inDeclDestructuring:1;

    
    
    bool            funBecameStrict:1;

    ParseContext(Parser<ParseHandler> *prs, GenericParseContext *parent,
                 SharedContext *sc, unsigned staticLevel, uint32_t bodyid)
      : GenericParseContext(parent, sc),
        bodyid(0),           
        blockidGen(bodyid),  
        topStmt(NULL),
        topScopeStmt(NULL),
        blockChain(prs->context),
        staticLevel(staticLevel),
        parenDepth(0),
        yieldCount(0),
        blockNode(ParseHandler::null()),
        decls_(prs->context, prs->alloc),
        args_(prs->context),
        vars_(prs->context),
        yieldOffset(0),
        parserPC(&prs->pc),
        oldpc(prs->pc),
        lexdeps(prs->context),
        funcStmts(NULL),
        innerFunctions(prs->context),
        inDeclDestructuring(false),
        funBecameStrict(false)
    {
        prs->pc = this;
    }

    ~ParseContext() {
        
        
        JS_ASSERT(*parserPC == this);
        *parserPC = this->oldpc;
        js_delete(funcStmts);
    }

    inline bool init()
{
    if (!frontend::GenerateBlockId(this, this->bodyid))
        return false;

    return decls_.init() && lexdeps.ensureMap(sc->context);
}

    unsigned blockid() { return topStmt ? topStmt->blockid : bodyid; }

    
    
    
    
    
    
    
    bool atBodyLevel() { return !topStmt; }

    inline bool useAsmOrInsideUseAsm() const {
        return sc->isFunctionBox() && sc->asFunctionBox()->useAsmOrInsideUseAsm();
    }
};

template <typename ParseHandler>
struct BindData;

class CompExprTransplanter;

template <typename ParseHandler>
class GenexpGuard;

enum LetContext { LetExpresion, LetStatement };
enum VarContext { HoistVars, DontHoistVars };
enum FunctionType { Getter, Setter, Normal };

template <typename ParseHandler>
class Parser : private AutoGCRooter, public StrictModeGetter
{
  public:
    ExclusiveContext *const context;
    LifoAlloc &alloc;

    TokenStream         tokenStream;
    LifoAlloc::Mark     tempPoolMark;

    
    ObjectBox *traceListHead;

    
    ParseContext<ParseHandler> *pc;

    SourceCompressionToken *sct;        

    
    AutoKeepAtoms       keepAtoms;

    
    const bool          foldConstants:1;

  private:
    




    bool abortedSyntaxParse;

    typedef typename ParseHandler::Node Node;
    typedef typename ParseHandler::DefinitionNode DefinitionNode;

  public:
    
    ParseHandler handler;

  private:
    bool reportHelper(ParseReportKind kind, bool strict, uint32_t offset,
                      unsigned errorNumber, va_list args);
  public:
    bool report(ParseReportKind kind, bool strict, Node pn, unsigned errorNumber, ...);
    bool reportWithOffset(ParseReportKind kind, bool strict, uint32_t offset, unsigned errorNumber,
                          ...);

    Parser(ExclusiveContext *cx, LifoAlloc *alloc, const CompileOptions &options,
           const jschar *chars, size_t length, bool foldConstants,
           Parser<SyntaxParseHandler> *syntaxParser,
           LazyScript *lazyOuterFunction);
    ~Parser();

    friend void js::frontend::MarkParser(JSTracer *trc, AutoGCRooter *parser);

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    


    Node parse(JSObject *chain);

    



    ObjectBox *newObjectBox(JSObject *obj);
    ModuleBox *newModuleBox(Module *module, ParseContext<ParseHandler> *pc);
    FunctionBox *newFunctionBox(JSFunction *fun, ParseContext<ParseHandler> *pc, bool strict);

    



    JSFunction *newFunction(GenericParseContext *pc, HandleAtom atom, FunctionSyntaxKind kind);

    void trace(JSTracer *trc);

    bool hadAbortedSyntaxParse() {
        return abortedSyntaxParse;
    }
    void clearAbortedSyntaxParse() {
        abortedSyntaxParse = false;
    }

  private:
    Parser *thisForCtor() { return this; }

    Node stringLiteral();
    inline Node newName(PropertyName *name);

    inline bool abortIfSyntaxParser();

  public:

    
    Node statement(bool canHaveDirectives = false);
    bool maybeParseDirective(Node pn, bool *cont);

    
    Node standaloneFunctionBody(HandleFunction fun, const AutoNameVector &formals,
                                bool strict, bool *becameStrict);

    
    
    Node standaloneLazyFunction(HandleFunction fun, unsigned staticLevel, bool strict);

    



    enum FunctionBodyType { StatementListBody, ExpressionBody };
    Node functionBody(FunctionSyntaxKind kind, FunctionBodyType type);

    bool functionArgsAndBodyGeneric(Node pn, HandleFunction fun,
                                    HandlePropertyName funName, FunctionType type,
                                    FunctionSyntaxKind kind, bool *becameStrict);

    virtual bool strictMode() { return pc->sc->strict; }

    const CompileOptions &options() const {
        return tokenStream.options();
    }

  private:
    















    Node moduleDecl();
    Node functionStmt();
    Node functionExpr();
    Node statements();

    Node blockStatement();
    Node ifStatement();
    Node doWhileStatement();
    Node whileStatement();
    Node forStatement();
    Node switchStatement();
    Node continueStatement();
    Node breakStatement();
    Node returnStatementOrYieldExpression();
    Node withStatement();
    Node labeledStatement();
    Node throwStatement();
    Node tryStatement();
    Node debuggerStatement();

#if JS_HAS_BLOCK_SCOPE
    Node letStatement();
#endif
    Node expressionStatement();
    Node variables(ParseNodeKind kind, bool *psimple = NULL,
                   StaticBlockObject *blockObj = NULL,
                   VarContext varContext = HoistVars);
    Node expr();
    Node assignExpr();
    Node assignExprWithoutYield(unsigned err);
    Node condExpr1();
    Node orExpr1();
    Node unaryExpr();
    Node memberExpr(TokenKind tt, bool allowCallSyntax);
    Node primaryExpr(TokenKind tt);
    Node parenExpr(bool *genexp = NULL);

    


    bool functionArguments(FunctionSyntaxKind kind, Node *list, Node funcpn, bool &hasRest);

    Node functionDef(HandlePropertyName name, const TokenStream::Position &start,
                     size_t startOffset, FunctionType type, FunctionSyntaxKind kind);
    bool functionArgsAndBody(Node pn, HandleFunction fun, HandlePropertyName funName,
                             size_t startOffset, FunctionType type, FunctionSyntaxKind kind,
                             bool strict, bool *becameStrict = NULL);

    Node unaryOpExpr(ParseNodeKind kind, JSOp op, uint32_t begin);

    Node condition();
    Node comprehensionTail(Node kid, unsigned blockid, bool isGenexp,
                           ParseContext<ParseHandler> *outerpc,
                           ParseNodeKind kind = PNK_SEMI, JSOp op = JSOP_NOP);
    bool arrayInitializerComprehensionTail(Node pn);
    Node generatorExpr(Node kid);
    bool argumentList(Node listNode);
    Node letBlock(LetContext letContext);
    Node destructuringExpr(BindData<ParseHandler> *data, TokenKind tt);

    Node identifierName();

    bool allowsForEachIn() {
#if !JS_HAS_FOR_EACH_IN
        return false;
#else
        return versionNumber() >= JSVERSION_1_6;
#endif
    }

    enum AssignmentFlavor {
        PlainAssignment,
        CompoundAssignment,
        KeyedDestructuringAssignment,
        IncDecAssignment
    };

    bool checkAndMarkAsAssignmentLhs(Node pn, AssignmentFlavor flavor);
    bool matchInOrOf(bool *isForOfp);

    bool checkFunctionArguments();
    bool makeDefIntoUse(Definition *dn, Node pn, JSAtom *atom);
    bool checkFunctionDefinition(HandlePropertyName funName, Node *pn, FunctionSyntaxKind kind,
                                 bool *pbodyProcessed);
    bool finishFunctionDefinition(Node pn, FunctionBox *funbox, Node prelude, Node body);
    bool addFreeVariablesFromLazyFunction(JSFunction *fun, ParseContext<ParseHandler> *pc);

    bool isValidForStatementLHS(Node pn1, JSVersion version,
                                bool forDecl, bool forEach, bool forOf);
    bool checkAndMarkAsIncOperand(Node kid, TokenKind tt, bool preorder);
    bool checkStrictAssignment(Node lhs, AssignmentFlavor flavor);
    bool checkStrictBinding(HandlePropertyName name, Node pn);
    bool defineArg(Node funcpn, HandlePropertyName name,
                   bool disallowDuplicateArgs = false, Node *duplicatedArg = NULL);
    Node pushLexicalScope(StmtInfoPC *stmt);
    Node pushLexicalScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    Node pushLetScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    bool noteNameUse(HandlePropertyName name, Node pn);
    Node newRegExp();
    Node newBindingNode(PropertyName *name, bool functionScope, VarContext varContext = HoistVars);
    bool checkDestructuring(BindData<ParseHandler> *data, Node left, bool toplevel = true);
    bool bindDestructuringVar(BindData<ParseHandler> *data, Node pn);
    bool bindDestructuringLHS(Node pn);
    bool makeSetCall(Node pn, unsigned msg);
    Node cloneLeftHandSide(Node opn);
    Node cloneParseTree(Node opn);

    Node newNumber(const Token &tok) {
        return handler.newNumber(tok.number(), tok.decimalPoint(), tok.pos);
    }

    static bool
    bindDestructuringArg(BindData<ParseHandler> *data,
                         HandlePropertyName name, Parser<ParseHandler> *parser);

    static bool
    bindLet(BindData<ParseHandler> *data,
            HandlePropertyName name, Parser<ParseHandler> *parser);

    static bool
    bindVarOrConst(BindData<ParseHandler> *data,
                   HandlePropertyName name, Parser<ParseHandler> *parser);

    static Node null() { return ParseHandler::null(); }

    bool reportRedeclaration(Node pn, bool isConst, JSAtom *atom);
    bool reportBadReturn(Node pn, ParseReportKind kind, unsigned errnum, unsigned anonerrnum);
    bool checkFinalReturn(Node pn);
    DefinitionNode getOrCreateLexicalDependency(ParseContext<ParseHandler> *pc, JSAtom *atom);

    bool leaveFunction(Node fn, HandlePropertyName funName,
                       ParseContext<ParseHandler> *outerpc,
                       FunctionSyntaxKind kind = Expression);

    TokenPos pos() const { return tokenStream.currentToken().pos; }

    friend class CompExprTransplanter;
    friend class GenexpGuard<ParseHandler>;
    friend struct BindData<ParseHandler>;
};



template <>
bool
Parser<FullParseHandler>::checkAndMarkAsAssignmentLhs(ParseNode *pn, AssignmentFlavor flavor);

template <>
bool
Parser<SyntaxParseHandler>::checkAndMarkAsAssignmentLhs(Node pn, AssignmentFlavor flavor);

} 
} 




#define TS(p) (&(p)->tokenStream)

#endif 
