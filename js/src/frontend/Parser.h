





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
    StmtInfoPC*     down;          
    StmtInfoPC*     downScope;     

    uint32_t        blockid;        
    uint32_t        innerBlockScopeDepth; 

    
    
    
    
    
    
    uint16_t        firstDominatingLexicalInCase;

    explicit StmtInfoPC(ExclusiveContext* cx)
      : StmtInfoBase(cx),
        innerBlockScopeDepth(0),
        firstDominatingLexicalInCase(0)
    {}
};

typedef HashSet<JSAtom*, DefaultHasher<JSAtom*>, LifoAllocPolicy<Fallible>> FuncStmtSet;
class SharedContext;

typedef Vector<Definition*, 16> DeclVector;

struct GenericParseContext
{
    
    GenericParseContext* parent;

    
    SharedContext* sc;

    
    

    
    bool funHasReturnExpr:1;

    
    bool funHasReturnVoid:1;

    
    

    
    
    bool parsingWith:1;

    GenericParseContext(GenericParseContext* parent, SharedContext* sc)
      : parent(parent),
        sc(sc),
        funHasReturnExpr(false),
        funHasReturnVoid(false),
        parsingWith(parent ? parent->parsingWith : false)
    {}
};

template <typename ParseHandler>
bool
GenerateBlockId(TokenStream& ts, ParseContext<ParseHandler>* pc, uint32_t& blockid);









template <typename ParseHandler>
struct ParseContext : public GenericParseContext
{
    typedef StmtInfoPC StmtInfo;
    typedef typename ParseHandler::Node Node;
    typedef typename ParseHandler::DefinitionNode DefinitionNode;

    uint32_t        bodyid;         
    uint32_t        blockidGen;     

    StmtInfoPC*     topStmt;       
    StmtInfoPC*     topScopeStmt;  
    Rooted<NestedScopeObject*> staticScope;  
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
    DeclVector      bodyLevelLexicals_; 

    bool checkLocalsOverflow(TokenStream& ts);

  public:
    const AtomDecls<ParseHandler>& decls() const {
        return decls_;
    }

    uint32_t numArgs() const {
        MOZ_ASSERT(sc->isFunctionBox());
        return args_.length();
    }

    

























    bool define(TokenStream& ts, HandlePropertyName name, Node pn, Definition::Kind);

    







    void popLetDecl(JSAtom* atom);

    
    void prepareToAddDuplicateArg(HandlePropertyName name, DefinitionNode prevDecl);

    
    void updateDecl(JSAtom* atom, Node newDecl);

    













    bool generateFunctionBindings(ExclusiveContext* cx, TokenStream& ts,
                                  LifoAlloc& alloc,
                                  InternalHandle<Bindings*> bindings) const;

  private:
    ParseContext**  parserPC;     



    
    
    
    ParseContext<ParseHandler>* oldpc;

  public:
    OwnedAtomDefnMapPtr lexdeps;    

    FuncStmtSet*    funcStmts;     



    
    AutoFunctionVector innerFunctions;

    
    
    
    
    
    Directives* newDirectives;

    
    
    
    
    
    
    
    
    
    bool            inDeclDestructuring:1;

    ParseContext(Parser<ParseHandler>* prs, GenericParseContext* parent,
                 Node maybeFunction, SharedContext* sc,
                 Directives* newDirectives,
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
        bodyLevelLexicals_(prs->context),
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

    bool init(TokenStream& ts);

    unsigned blockid() { return topStmt ? topStmt->blockid : bodyid; }

    
    
    
    
    
    
    
    bool atBodyLevel() { return !topStmt; }
    bool atGlobalLevel() { return atBodyLevel() && !sc->isFunctionBox() && (topStmt == topScopeStmt); }

    
    
    bool isFunctionConstructorBody() const {
        return sc->isFunctionBox() && staticLevel == 0;
    }

    inline bool useAsmOrInsideUseAsm() const {
        return sc->isFunctionBox() && sc->asFunctionBox()->useAsmOrInsideUseAsm();
    }
};

template <typename ParseHandler>
inline
Directives::Directives(ParseContext<ParseHandler>* parent)
  : strict_(parent->sc->strict()),
    asmJS_(parent->useAsmOrInsideUseAsm())
{}

template <typename ParseHandler>
struct BindData;

class CompExprTransplanter;

enum VarContext { HoistVars, DontHoistVars };
enum PropListType { ObjectLiteral, ClassBody, DerivedClassBody };

inline bool
IsClassBody(PropListType type)
{
    return type == ClassBody || type == DerivedClassBody;
}





enum YieldHandling { YieldIsName, YieldIsKeyword };
enum InHandling { InAllowed, InProhibited };
enum DefaultHandling { NameRequired, AllowDefaultName };

template <typename ParseHandler>
class Parser : private JS::AutoGCRooter, public StrictModeGetter
{
  public:
    ExclusiveContext* const context;
    LifoAlloc& alloc;

    TokenStream         tokenStream;
    LifoAlloc::Mark     tempPoolMark;

    
    ObjectBox* traceListHead;

    
    ParseContext<ParseHandler>* pc;

    
    SourceCompressionTask* sct;

    ScriptSource*       ss;

    
    AutoKeepAtoms       keepAtoms;

    
    const bool          foldConstants:1;

  private:
#if DEBUG
    
    bool checkOptionsCalled:1;
#endif

    




    bool abortedSyntaxParse:1;

    
    bool isUnexpectedEOF_:1;

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

    Parser(ExclusiveContext* cx, LifoAlloc* alloc, const ReadOnlyCompileOptions& options,
           const char16_t* chars, size_t length, bool foldConstants,
           Parser<SyntaxParseHandler>* syntaxParser,
           LazyScript* lazyOuterFunction);
    ~Parser();

    bool checkOptions();

    
    
    
    class Mark
    {
        friend class Parser;
        LifoAlloc::Mark mark;
        ObjectBox* traceListHead;
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

    friend void js::frontend::MarkParser(JSTracer* trc, JS::AutoGCRooter* parser);

    const char* getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    


    Node parse(JSObject* chain);

    



    ObjectBox* newObjectBox(JSObject* obj);
    FunctionBox* newFunctionBox(Node fn, JSFunction* fun, ParseContext<ParseHandler>* pc,
                                Directives directives, GeneratorKind generatorKind);

    



    JSFunction* newFunction(HandleAtom atom, FunctionSyntaxKind kind, GeneratorKind generatorKind,
                            HandleObject proto);

    void trace(JSTracer* trc);

    bool hadAbortedSyntaxParse() {
        return abortedSyntaxParse;
    }
    void clearAbortedSyntaxParse() {
        abortedSyntaxParse = false;
    }

    bool isUnexpectedEOF() const { return isUnexpectedEOF_; }

  private:
    Parser* thisForCtor() { return this; }

    JSAtom * stopStringCompression();

    Node stringLiteral();
    Node noSubstitutionTemplate();
    Node templateLiteral(YieldHandling yieldHandling);
    bool taggedTemplate(YieldHandling yieldHandling, Node nodeList, TokenKind tt);
    bool appendToCallSiteObj(Node callSiteObj);
    bool addExprAndGetNextTemplStrToken(YieldHandling yieldHandling, Node nodeList,
                                        TokenKind* ttp);

    inline Node newName(PropertyName* name);
    inline Node newYieldExpression(uint32_t begin, Node expr, bool isYieldStar = false);

    inline bool abortIfSyntaxParser();

  public:
    
    Node statement(YieldHandling yieldHandling, bool canHaveDirectives = false);

    bool maybeParseDirective(Node list, Node pn, bool* cont);

    
    
    Node standaloneFunctionBody(HandleFunction fun, const AutoNameVector& formals,
                                GeneratorKind generatorKind,
                                Directives inheritedDirectives, Directives* newDirectives);

    
    
    Node standaloneLazyFunction(HandleFunction fun, unsigned staticLevel, bool strict,
                                GeneratorKind generatorKind);

    



    enum FunctionBodyType { StatementListBody, ExpressionBody };
    Node functionBody(InHandling inHandling, YieldHandling yieldHandling, FunctionSyntaxKind kind,
                      FunctionBodyType type);

    bool functionArgsAndBodyGeneric(InHandling inHandling, YieldHandling yieldHandling, Node pn,
                                    HandleFunction fun, FunctionSyntaxKind kind);

    
    
    
    bool checkYieldNameValidity();
    bool yieldExpressionsSupported() {
        return versionNumber() >= JSVERSION_1_7 || pc->isGenerator();
    }

    virtual bool strictMode() { return pc->sc->strict(); }
    bool setLocalStrictMode(bool strict) {
        MOZ_ASSERT(tokenStream.debugHasNoLookahead());
        return pc->sc->setLocalStrictMode(strict);
    }

    const ReadOnlyCompileOptions& options() const {
        return tokenStream.options();
    }

  private:
    enum InvokedPrediction { PredictUninvoked = false, PredictInvoked = true };
    enum ForInitLocation { InForInit, NotInForInit };

  private:
    















    Node functionStmt(YieldHandling yieldHandling, DefaultHandling defaultHandling);
    Node functionExpr(InvokedPrediction invoked = PredictUninvoked);
    Node statements(YieldHandling yieldHandling);

    Node blockStatement(YieldHandling yieldHandling);
    Node ifStatement(YieldHandling yieldHandling);
    Node doWhileStatement(YieldHandling yieldHandling);
    Node whileStatement(YieldHandling yieldHandling);
    Node forStatement(YieldHandling yieldHandling);
    Node switchStatement(YieldHandling yieldHandling);
    Node continueStatement(YieldHandling yieldHandling);
    Node breakStatement(YieldHandling yieldHandling);
    Node returnStatement(YieldHandling yieldHandling);
    Node withStatement(YieldHandling yieldHandling);
    Node labeledStatement(YieldHandling yieldHandling);
    Node throwStatement(YieldHandling yieldHandling);
    Node tryStatement(YieldHandling yieldHandling);
    Node debuggerStatement();

    Node lexicalDeclaration(YieldHandling yieldHandling, bool isConst);
    Node letDeclarationOrBlock(YieldHandling yieldHandling);
    Node importDeclaration();
    Node exportDeclaration();
    Node expressionStatement(YieldHandling yieldHandling,
                             InvokedPrediction invoked = PredictUninvoked);
    Node variables(YieldHandling yieldHandling,
                   ParseNodeKind kind,
                   ForInitLocation location,
                   bool* psimple = nullptr, StaticBlockObject* blockObj = nullptr,
                   VarContext varContext = HoistVars);
    Node expr(InHandling inHandling, YieldHandling yieldHandling,
              InvokedPrediction invoked = PredictUninvoked);
    Node assignExpr(InHandling inHandling, YieldHandling yieldHandling,
                    InvokedPrediction invoked = PredictUninvoked);
    Node assignExprWithoutYield(YieldHandling yieldHandling, unsigned err);
    Node yieldExpression(InHandling inHandling);
    Node condExpr1(InHandling inHandling, YieldHandling yieldHandling,
                   InvokedPrediction invoked = PredictUninvoked);
    Node orExpr1(InHandling inHandling, YieldHandling yieldHandling,
                 InvokedPrediction invoked = PredictUninvoked);
    Node unaryExpr(YieldHandling yieldHandling, InvokedPrediction invoked = PredictUninvoked);
    Node memberExpr(YieldHandling yieldHandling, TokenKind tt, bool allowCallSyntax,
                    InvokedPrediction invoked = PredictUninvoked);
    Node primaryExpr(YieldHandling yieldHandling, TokenKind tt,
                     InvokedPrediction invoked = PredictUninvoked);
    Node parenExprOrGeneratorComprehension(YieldHandling yieldHandling);
    Node exprInParens(InHandling inHandling, YieldHandling yieldHandling);

    bool checkAllowedNestedSyntax(SharedContext::AllowedSyntax allowed,
                                  SharedContext** allowingContext = nullptr);
    bool tryNewTarget(Node& newTarget);
    bool checkAndMarkSuperScope();

    bool methodDefinition(YieldHandling yieldHandling, PropListType listType, Node propList,
                          Node propname, FunctionSyntaxKind kind, GeneratorKind generatorKind,
                          bool isStatic, JSOp Op);

    


    bool functionArguments(YieldHandling yieldHandling, FunctionSyntaxKind kind,
                           Node funcpn, bool* hasRest);

    Node functionDef(InHandling inHandling, YieldHandling uieldHandling, HandlePropertyName name,
                     FunctionSyntaxKind kind, GeneratorKind generatorKind,
                     InvokedPrediction invoked = PredictUninvoked);
    bool functionArgsAndBody(InHandling inHandling, Node pn, HandleFunction fun,
                             FunctionSyntaxKind kind, GeneratorKind generatorKind,
                             Directives inheritedDirectives, Directives* newDirectives);

    Node unaryOpExpr(YieldHandling yieldHandling, ParseNodeKind kind, JSOp op, uint32_t begin);

    Node condition(InHandling inHandling, YieldHandling yieldHandling);

    Node generatorComprehensionLambda(GeneratorKind comprehensionKind, unsigned begin,
                                      Node innerStmt);

    Node legacyComprehensionTail(Node kid, unsigned blockid, GeneratorKind comprehensionKind,
                                 ParseContext<ParseHandler>* outerpc,
                                 unsigned innerBlockScopeDepth);
    Node legacyArrayComprehension(Node array);
    Node legacyGeneratorExpr(Node kid);

    Node comprehensionTail(GeneratorKind comprehensionKind);
    Node comprehensionIf(GeneratorKind comprehensionKind);
    Node comprehensionFor(GeneratorKind comprehensionKind);
    Node comprehension(GeneratorKind comprehensionKind);
    Node arrayComprehension(uint32_t begin);
    Node generatorComprehension(uint32_t begin);

    bool argumentList(YieldHandling yieldHandling, Node listNode, bool* isSpread);
    Node deprecatedLetBlock(YieldHandling yieldHandling);
    Node destructuringExpr(YieldHandling yieldHandling, BindData<ParseHandler>* data,
                           TokenKind tt);
    Node destructuringExprWithoutYield(YieldHandling yieldHandling, BindData<ParseHandler>* data,
                                       TokenKind tt, unsigned msg);

    bool namedImportsOrNamespaceImport(TokenKind tt, Node importSpecSet);

    enum ClassContext { ClassStatement, ClassExpression };
    Node classDefinition(YieldHandling yieldHandling, ClassContext classContext, DefaultHandling defaultHandling);

    Node identifierName(YieldHandling yieldHandling);

    bool matchLabel(YieldHandling yieldHandling, MutableHandle<PropertyName*> label);

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
        IncrementAssignment,
        DecrementAssignment
    };

    bool checkAndMarkAsAssignmentLhs(Node pn, AssignmentFlavor flavor);
    bool matchInOrOf(bool* isForInp, bool* isForOfp);

    bool checkFunctionArguments();
    bool makeDefIntoUse(Definition* dn, Node pn, JSAtom* atom);
    bool checkFunctionDefinition(HandlePropertyName funName, Node* pn, FunctionSyntaxKind kind,
                                 bool* pbodyProcessed);
    bool finishFunctionDefinition(Node pn, FunctionBox* funbox, Node body);
    bool addFreeVariablesFromLazyFunction(JSFunction* fun, ParseContext<ParseHandler>* pc);

    bool isValidForStatementLHS(Node pn1, JSVersion version, bool forDecl, bool forEach,
                                ParseNodeKind headKind);
    bool checkForHeadConstInitializers(Node pn1);

    enum FunctionCallBehavior {
        PermitAssignmentToFunctionCalls,
        ForbidAssignmentToFunctionCalls
    };

    bool isValidSimpleAssignmentTarget(Node node,
                                       FunctionCallBehavior behavior = ForbidAssignmentToFunctionCalls);

    bool reportIfArgumentsEvalTarget(Node nameNode);
    bool reportIfNotValidSimpleAssignmentTarget(Node target, AssignmentFlavor flavor);

    bool checkAndMarkAsIncOperand(Node kid, AssignmentFlavor flavor);

    bool checkStrictAssignment(Node lhs);

    bool checkStrictBinding(PropertyName* name, Node pn);
    bool defineArg(Node funcpn, HandlePropertyName name,
                   bool disallowDuplicateArgs = false, Node* duplicatedArg = nullptr);
    Node pushLexicalScope(StmtInfoPC* stmt);
    Node pushLexicalScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC* stmt);
    Node pushLetScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC* stmt);
    bool noteNameUse(HandlePropertyName name, Node pn);
    Node computedPropertyName(YieldHandling yieldHandling, Node literal);
    Node arrayInitializer(YieldHandling yieldHandling);
    Node newRegExp();

    Node propertyList(YieldHandling yieldHandling, PropListType type);
    Node newPropertyListNode(PropListType type);

    bool checkAndPrepareLexical(bool isConst, const TokenPos& errorPos);
    Node makeInitializedLexicalBinding(HandlePropertyName name, bool isConst, const TokenPos& pos);

    Node newBindingNode(PropertyName* name, bool functionScope, VarContext varContext = HoistVars);

    
    bool checkDestructuringPattern(BindData<ParseHandler>* data, Node pattern);

    
    
    
    
    bool checkDestructuringArray(BindData<ParseHandler>* data, Node arrayPattern);
    bool checkDestructuringObject(BindData<ParseHandler>* data, Node objectPattern);
    bool checkDestructuringName(BindData<ParseHandler>* data, Node expr);

    bool bindInitialized(BindData<ParseHandler>* data, Node pn);
    bool makeSetCall(Node node, unsigned errnum);
    Node cloneDestructuringDefault(Node opn);
    Node cloneLeftHandSide(Node opn);
    Node cloneParseTree(Node opn);

    Node newNumber(const Token& tok) {
        return handler.newNumber(tok.number(), tok.decimalPoint(), tok.pos);
    }

    static bool
    bindDestructuringArg(BindData<ParseHandler>* data,
                         HandlePropertyName name, Parser<ParseHandler>* parser);

    static bool
    bindLexical(BindData<ParseHandler>* data,
                HandlePropertyName name, Parser<ParseHandler>* parser);

    static bool
    bindVarOrGlobalConst(BindData<ParseHandler>* data,
                         HandlePropertyName name, Parser<ParseHandler>* parser);

    static Node null() { return ParseHandler::null(); }

    bool reportRedeclaration(Node pn, Definition::Kind redeclKind, HandlePropertyName name);
    bool reportBadReturn(Node pn, ParseReportKind kind, unsigned errnum, unsigned anonerrnum);
    DefinitionNode getOrCreateLexicalDependency(ParseContext<ParseHandler>* pc, JSAtom* atom);

    bool leaveFunction(Node fn, ParseContext<ParseHandler>* outerpc,
                       FunctionSyntaxKind kind = Expression);

    TokenPos pos() const { return tokenStream.currentToken().pos; }

    bool asmJS(Node list);

    void addTelemetry(JSCompartment::DeprecatedLanguageExtension e);

    friend class LegacyCompExprTransplanter;
    friend struct BindData<ParseHandler>;
};

} 
} 




#define TS(p) (&(p)->tokenStream)

#endif 
