





#ifndef frontend_Parser_h
#define frontend_Parser_h





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
    uint32_t        innerBlockScopeDepth; 

    explicit StmtInfoPC(ExclusiveContext *cx) : StmtInfoBase(cx), innerBlockScopeDepth(0) {}
};

typedef HashSet<JSAtom *, DefaultHasher<JSAtom *>, LifoAllocPolicy<Fallible>> FuncStmtSet;
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
GenerateBlockId(TokenStream &ts, ParseContext<ParseHandler> *pc, uint32_t &blockid);









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
    Rooted<NestedScopeObject *> staticScope;  
    Node            maybeFunction;  

    const unsigned  staticLevel;    

    
    
    static const uint32_t NoYieldOffset = UINT32_MAX;
    uint32_t         lastYieldOffset;

    
    
    
    GeneratorKind generatorKind() const {
        return sc->isFunctionBox() ? sc->asFunctionBox()->generatorKind() : NotGenerator;
    }
    bool isGenerator() const { return generatorKind() != NotGenerator; }
    bool isLegacyGenerator() const { return generatorKind() == LegacyGenerator; }
    bool isStarGenerator() const { return generatorKind() == StarGenerator; }

    bool isArrowFunction() const {
        return sc->isFunctionBox() && sc->asFunctionBox()->function()->isArrow();
    }

    uint32_t        blockScopeDepth; 
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

    

























    bool define(TokenStream &ts, HandlePropertyName name, Node pn, Definition::Kind);

    







    void popLetDecl(JSAtom *atom);

    
    void prepareToAddDuplicateArg(HandlePropertyName name, DefinitionNode prevDecl);

    
    void updateDecl(JSAtom *atom, Node newDecl);

    













    bool generateFunctionBindings(ExclusiveContext *cx, TokenStream &ts,
                                  LifoAlloc &alloc,
                                  InternalHandle<Bindings*> bindings) const;

  private:
    ParseContext    **parserPC;     



    
    
    
    ParseContext<ParseHandler> *oldpc;

  public:
    OwnedAtomDefnMapPtr lexdeps;    

    FuncStmtSet     *funcStmts;     



    
    AutoFunctionVector innerFunctions;

    
    
    
    
    
    Directives *newDirectives;

    
    
    
    
    
    
    
    
    
    bool            inDeclDestructuring:1;

    ParseContext(Parser<ParseHandler> *prs, GenericParseContext *parent,
                 Node maybeFunction, SharedContext *sc,
                 Directives *newDirectives,
                 unsigned staticLevel, uint32_t bodyid, uint32_t blockScopeDepth)
      : GenericParseContext(parent, sc),
        bodyid(0),           
        blockidGen(bodyid),  
        topStmt(nullptr),
        topScopeStmt(nullptr),
        staticScope(prs->context),
        maybeFunction(maybeFunction),
        staticLevel(staticLevel),
        lastYieldOffset(NoYieldOffset),
        blockScopeDepth(blockScopeDepth),
        blockNode(ParseHandler::null()),
        decls_(prs->context, prs->alloc),
        args_(prs->context),
        vars_(prs->context),
        parserPC(&prs->pc),
        oldpc(prs->pc),
        lexdeps(prs->context),
        funcStmts(nullptr),
        innerFunctions(prs->context),
        newDirectives(newDirectives),
        inDeclDestructuring(false)
    {
        prs->pc = this;
    }

    ~ParseContext();

    bool init(TokenStream &ts);

    unsigned blockid() { return topStmt ? topStmt->blockid : bodyid; }

    
    
    
    
    
    
    
    bool atBodyLevel() { return !topStmt; }

    
    
    bool isFunctionConstructorBody() const {
        return sc->isFunctionBox() && staticLevel == 0;
    }

    inline bool useAsmOrInsideUseAsm() const {
        return sc->isFunctionBox() && sc->asFunctionBox()->useAsmOrInsideUseAsm();
    }
};

template <typename ParseHandler>
inline
Directives::Directives(ParseContext<ParseHandler> *parent)
  : strict_(parent->sc->strict),
    asmJS_(parent->useAsmOrInsideUseAsm())
{}

template <typename ParseHandler>
struct BindData;

class CompExprTransplanter;

enum LetContext { LetExpresion, LetStatement };
enum VarContext { HoistVars, DontHoistVars };
enum FunctionType { Getter, Setter, Normal };

template <typename ParseHandler>
class Parser : private JS::AutoGCRooter, public StrictModeGetter
{
  public:
    ExclusiveContext *const context;
    LifoAlloc &alloc;

    TokenStream         tokenStream;
    LifoAlloc::Mark     tempPoolMark;

    
    ObjectBox *traceListHead;

    
    ParseContext<ParseHandler> *pc;

    
    SourceCompressionTask *sct;

    ScriptSource        *ss;

    
    AutoKeepAtoms       keepAtoms;

    
    const bool          foldConstants:1;

  private:
    




    bool abortedSyntaxParse:1;

    
    bool isUnexpectedEOF_:1;

    
    bool sawDeprecatedForEach:1;
    bool sawDeprecatedDestructuringForIn:1;
    bool sawDeprecatedLegacyGenerator:1;
    bool sawDeprecatedExpressionClosure:1;

    typedef typename ParseHandler::Node Node;
    typedef typename ParseHandler::DefinitionNode DefinitionNode;

  public:
    
    ParseHandler handler;

  private:
    bool reportHelper(ParseReportKind kind, bool strict, uint32_t offset,
                      unsigned errorNumber, va_list args);
  public:
    bool report(ParseReportKind kind, bool strict, Node pn, unsigned errorNumber, ...);
    bool reportNoOffset(ParseReportKind kind, bool strict, unsigned errorNumber, ...);
    bool reportWithOffset(ParseReportKind kind, bool strict, uint32_t offset, unsigned errorNumber,
                          ...);

    Parser(ExclusiveContext *cx, LifoAlloc *alloc, const ReadOnlyCompileOptions &options,
           const jschar *chars, size_t length, bool foldConstants,
           Parser<SyntaxParseHandler> *syntaxParser,
           LazyScript *lazyOuterFunction);
    ~Parser();

    
    
    
    class Mark
    {
        friend class Parser;
        LifoAlloc::Mark mark;
        ObjectBox *traceListHead;
    };
    Mark mark() const {
        Mark m;
        m.mark = alloc.mark();
        m.traceListHead = traceListHead;
        return m;
    }
    void release(Mark m) {
        alloc.release(m.mark);
        traceListHead = m.traceListHead;
    }

    friend void js::frontend::MarkParser(JSTracer *trc, JS::AutoGCRooter *parser);

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    


    Node parse(JSObject *chain);

    



    ObjectBox *newObjectBox(JSObject *obj);
    FunctionBox *newFunctionBox(Node fn, JSFunction *fun, ParseContext<ParseHandler> *pc,
                                Directives directives, GeneratorKind generatorKind);

    



    JSFunction *newFunction(GenericParseContext *pc, HandleAtom atom, FunctionSyntaxKind kind,
                            JSObject *proto = nullptr);

    void trace(JSTracer *trc);

    bool hadAbortedSyntaxParse() {
        return abortedSyntaxParse;
    }
    void clearAbortedSyntaxParse() {
        abortedSyntaxParse = false;
    }

    bool isUnexpectedEOF() const { return isUnexpectedEOF_; }

  private:
    Parser *thisForCtor() { return this; }

    JSAtom * stopStringCompression();

    Node stringLiteral();
    Node noSubstitutionTemplate();
    Node templateLiteral();
    bool taggedTemplate(Node nodeList, TokenKind tt);
    bool appendToCallSiteObj(Node callSiteObj);
    bool addExprAndGetNextTemplStrToken(Node nodeList, TokenKind &tt);

    inline Node newName(PropertyName *name);

    inline bool abortIfSyntaxParser();

  public:

    
    Node statement(bool canHaveDirectives = false);
    bool maybeParseDirective(Node list, Node pn, bool *cont);

    
    
    Node standaloneFunctionBody(HandleFunction fun, const AutoNameVector &formals,
                                GeneratorKind generatorKind,
                                Directives inheritedDirectives, Directives *newDirectives);

    
    
    Node standaloneLazyFunction(HandleFunction fun, unsigned staticLevel, bool strict,
                                GeneratorKind generatorKind);

    



    enum FunctionBodyType { StatementListBody, ExpressionBody };
    Node functionBody(FunctionSyntaxKind kind, FunctionBodyType type);

    bool functionArgsAndBodyGeneric(Node pn, HandleFunction fun, FunctionType type,
                                    FunctionSyntaxKind kind);

    
    
    
    bool checkYieldNameValidity();

    virtual bool strictMode() { return pc->sc->strict; }

    const ReadOnlyCompileOptions &options() const {
        return tokenStream.options();
    }

  private:
    















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
    Node returnStatement();
    Node withStatement();
    Node labeledStatement();
    Node throwStatement();
    Node tryStatement();
    Node debuggerStatement();

    Node letDeclaration();
    Node letStatement();
    Node importDeclaration();
    Node exportDeclaration();
    Node expressionStatement();
    Node variables(ParseNodeKind kind, bool *psimple = nullptr,
                   StaticBlockObject *blockObj = nullptr,
                   VarContext varContext = HoistVars);
    Node expr();
    Node assignExpr();
    Node assignExprWithoutYield(unsigned err);
    Node yieldExpression();
    Node condExpr1();
    Node orExpr1();
    Node unaryExpr();
    Node memberExpr(TokenKind tt, bool allowCallSyntax);
    Node primaryExpr(TokenKind tt);
    Node parenExprOrGeneratorComprehension();
    Node exprInParens();

    bool methodDefinition(Node literal, Node propname, FunctionType type, FunctionSyntaxKind kind,
                          GeneratorKind generatorKind, JSOp Op);

    


    bool functionArguments(FunctionSyntaxKind kind, Node *list, Node funcpn, bool *hasRest);

    Node functionDef(HandlePropertyName name, const TokenStream::Position &start,
                     FunctionType type, FunctionSyntaxKind kind, GeneratorKind generatorKind);
    bool functionArgsAndBody(Node pn, HandleFunction fun,
                             FunctionType type, FunctionSyntaxKind kind,
                             GeneratorKind generatorKind,
                             Directives inheritedDirectives, Directives *newDirectives);

    Node unaryOpExpr(ParseNodeKind kind, JSOp op, uint32_t begin);

    Node condition();

    Node generatorComprehensionLambda(GeneratorKind comprehensionKind, unsigned begin,
                                      Node innerStmt);

    Node legacyComprehensionTail(Node kid, unsigned blockid, GeneratorKind comprehensionKind,
                                 ParseContext<ParseHandler> *outerpc,
                                 unsigned innerBlockScopeDepth);
    Node legacyArrayComprehension(Node array);
    Node legacyGeneratorExpr(Node kid);

    Node comprehensionTail(GeneratorKind comprehensionKind);
    Node comprehensionIf(GeneratorKind comprehensionKind);
    Node comprehensionFor(GeneratorKind comprehensionKind);
    Node comprehension(GeneratorKind comprehensionKind);
    Node arrayComprehension(uint32_t begin);
    Node generatorComprehension(uint32_t begin);

    bool argumentList(Node listNode, bool *isSpread);
    Node letBlock(LetContext letContext);
    Node destructuringExpr(BindData<ParseHandler> *data, TokenKind tt);

    Node identifierName();

    bool matchLabel(MutableHandle<PropertyName*> label);

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

    bool isValidForStatementLHS(Node pn1, JSVersion version, bool forDecl, bool forEach,
                                ParseNodeKind headKind);
    bool checkAndMarkAsIncOperand(Node kid, TokenKind tt, bool preorder);
    bool checkStrictAssignment(Node lhs);
    bool checkStrictBinding(PropertyName *name, Node pn);
    bool defineArg(Node funcpn, HandlePropertyName name,
                   bool disallowDuplicateArgs = false, Node *duplicatedArg = nullptr);
    Node pushLexicalScope(StmtInfoPC *stmt);
    Node pushLexicalScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    Node pushLetScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    bool noteNameUse(HandlePropertyName name, Node pn);
    Node objectLiteral();
    Node computedPropertyName(Node literal);
    Node arrayInitializer();
    Node newRegExp();

    Node newBindingNode(PropertyName *name, bool functionScope, VarContext varContext = HoistVars);
    bool checkDestructuring(BindData<ParseHandler> *data, Node left);
    bool checkDestructuringObject(BindData<ParseHandler> *data, Node objectPattern);
    bool checkDestructuringArray(BindData<ParseHandler> *data, Node arrayPattern);
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

    bool reportRedeclaration(Node pn, bool isConst, HandlePropertyName name);
    bool reportBadReturn(Node pn, ParseReportKind kind, unsigned errnum, unsigned anonerrnum);
    DefinitionNode getOrCreateLexicalDependency(ParseContext<ParseHandler> *pc, JSAtom *atom);

    bool leaveFunction(Node fn, ParseContext<ParseHandler> *outerpc,
                       FunctionSyntaxKind kind = Expression);

    TokenPos pos() const { return tokenStream.currentToken().pos; }

    bool asmJS(Node list);

    void accumulateTelemetry();

    friend class LegacyCompExprTransplanter;
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
