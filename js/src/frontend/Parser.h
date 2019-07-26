






#ifndef Parser_h__
#define Parser_h__




#include "jsversion.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsscript.h"
#include "jswin.h"

#include "frontend/FoldConstants.h"
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

    StmtInfoPC(JSContext *cx) : StmtInfoBase(cx) {}
};

typedef HashSet<JSAtom *> FuncStmtSet;
class SharedContext;

typedef Vector<Definition *, 16> DeclVector;









template <typename ParseHandler>
struct ParseContext                 
{
    typedef StmtInfoPC StmtInfo;
    typedef typename ParseHandler::Node Node;

    SharedContext   *sc;            

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

    

























    bool define(JSContext *cx, HandlePropertyName name, Node pn, Definition::Kind);

    







    void popLetDecl(JSAtom *atom);

    
    void prepareToAddDuplicateArg(Definition *prevDecl);

    
    void updateDecl(JSAtom *atom, Node newDecl);

    













    bool generateFunctionBindings(JSContext *cx, InternalHandle<Bindings*> bindings) const;

  public:
    uint32_t         yieldOffset;   



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

    inline ParseContext(Parser<ParseHandler> *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid);
    inline ~ParseContext();

    inline bool init();

    unsigned blockid();

    
    
    
    
    
    
    
    bool atBodyLevel();

    inline bool useAsmOrInsideUseAsm() const {
        return sc->isFunctionBox() && sc->asFunctionBox()->useAsmOrInsideUseAsm();
    }
};

template <typename ParseHandler>
bool
GenerateBlockId(ParseContext<ParseHandler> *pc, uint32_t &blockid);

template <typename ParseHandler>
struct BindData;

class CompExprTransplanter;

template <typename ParseHandler>
class GenexpGuard;


bool EmitElemOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce);

enum LetContext { LetExpresion, LetStatement };
enum VarContext { HoistVars, DontHoistVars };

template <typename ParseHandler>
struct Parser : private AutoGCRooter, public StrictModeGetter
{
    JSContext           *const context; 
    TokenStream         tokenStream;
    void                *tempPoolMark;  

    
    ObjectBox *traceListHead;

    
    ParseContext<ParseHandler> *pc;

    SourceCompressionToken *sct;        

    
    AutoKeepAtoms       keepAtoms;

    
    const bool          foldConstants:1;

  private:
    
    const bool          compileAndGo:1;

    













    const bool          selfHostingMode:1;

    




    bool unknownResult;

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

    Parser(JSContext *cx, const CompileOptions &options,
           const jschar *chars, size_t length, bool foldConstants);
    ~Parser();

    friend void AutoGCRooter::trace(JSTracer *trc);

    





    bool init();

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    


    Node parse(JSObject *chain);

    



    ObjectBox *newObjectBox(JSObject *obj);
    ModuleBox *newModuleBox(Module *module, ParseContext<ParseHandler> *pc);
    FunctionBox *newFunctionBox(JSFunction *fun, ParseContext<ParseHandler> *pc, bool strict);

    



    JSFunction *newFunction(ParseContext<ParseHandler> *pc, HandleAtom atom, FunctionSyntaxKind kind);

    void trace(JSTracer *trc);

    bool hadUnknownResult() {
        return unknownResult;
    }

  private:
    Parser *thisForCtor() { return this; }

    



    Node atomNode(ParseNodeKind kind, JSOp op);

    void setUnknownResult() {
        unknownResult = true;
    }

  public:

    
    Node statement();
    bool maybeParseDirective(Node pn, bool *cont);

    
    Node standaloneFunctionBody(HandleFunction fun, const AutoNameVector &formals, HandleScript script,
                                Node fn, FunctionBox **funbox, bool strict,
                                bool *becameStrict = NULL);

    



    enum FunctionBodyType { StatementListBody, ExpressionBody };
    Node functionBody(FunctionSyntaxKind kind, FunctionBodyType type);

    virtual bool strictMode() { return pc->sc->strict; }

  private:
    















    Node moduleDecl();
    Node functionStmt();
    Node functionExpr();
    Node statements(bool *hasFunctionStmt = NULL);

    Node switchStatement();
    Node forStatement();
    Node tryStatement();
    Node withStatement();
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

    


    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(FunctionSyntaxKind kind, Node *list, Node funcpn, bool &hasRest);

    Node functionDef(HandlePropertyName name, const TokenStream::Position &start,
                     size_t startOffset, FunctionType type, FunctionSyntaxKind kind);
    bool functionArgsAndBody(Node pn, HandleFunction fun, HandlePropertyName funName,
                             size_t startOffset, FunctionType type, FunctionSyntaxKind kind,
                             bool strict, bool *becameStrict = NULL);

    Node unaryOpExpr(ParseNodeKind kind, JSOp op);

    Node condition();
    Node comprehensionTail(Node kid, unsigned blockid, bool isGenexp,
                               ParseNodeKind kind = PNK_SEMI, JSOp op = JSOP_NOP);
    bool arrayInitializerComprehensionTail(Node pn);
    Node generatorExpr(Node kid);
    bool argumentList(Node listNode);
    Node bracketedExpr();
    Node letBlock(LetContext letContext);
    Node returnOrYield(bool useAssignExpr);
    Node destructuringExpr(BindData<ParseHandler> *data, TokenKind tt);

    Node identifierName();

    bool allowsForEachIn() {
#if !JS_HAS_FOR_EACH_IN
        return false;
#else
        return versionNumber() >= JSVERSION_1_6;
#endif
    }

    bool setAssignmentLhsOps(Node pn, JSOp op);
    bool matchInOrOf(bool *isForOfp);

    void addStatementToList(Node pn, Node kid, bool *hasFunctionStmt);
    bool checkFunctionArguments();
    bool makeDefIntoUse(Definition *dn, Node pn, JSAtom *atom);
    bool checkFunctionDefinition(HandlePropertyName funName, Node *pn, FunctionSyntaxKind kind);
    bool finishFunctionDefinition(Node pn, FunctionBox *funbox,
                                  Node prelude, Node body,
                                  ParseContext<ParseHandler> *outerpc);

    bool isValidForStatementLHS(Node pn1, JSVersion version,
                                bool forDecl, bool forEach, bool forOf);
    bool setLvalKid(Node pn, Node kid, const char *name);
    bool setIncOpKid(Node pn, Node kid, TokenKind tt, bool preorder);
    bool checkStrictAssignment(Node lhs);
    bool checkStrictBinding(HandlePropertyName name, Node pn);
    bool checkDeleteExpression(Node *pn);
    bool defineArg(Node funcpn, HandlePropertyName name,
                   bool disallowDuplicateArgs = false, DefinitionNode *duplicatedArg = NULL);
    Node pushLexicalScope(StmtInfoPC *stmt);
    Node pushLexicalScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    Node pushLetScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    bool noteNameUse(Node pn);
    Node newRegExp(const jschar *chars, size_t length, RegExpFlag flags);
    Node newBindingNode(PropertyName *name, VarContext varContext = HoistVars);
    bool checkDestructuring(BindData<ParseHandler> *data, Node left, bool toplevel = true);
    bool bindDestructuringVar(BindData<ParseHandler> *data, Node pn);
    bool bindDestructuringLHS(Node pn);
    bool makeSetCall(Node pn, unsigned msg);
    PropertyName *foldPropertyByValue(Node pn);
    Node cloneLeftHandSide(Node opn);
    Node cloneParseTree(Node opn);

    static bool
    bindDestructuringArg(JSContext *cx, BindData<ParseHandler> *data,
                         HandlePropertyName name, Parser<ParseHandler> *parser);

    static bool
    bindLet(JSContext *cx, BindData<ParseHandler> *data,
            HandlePropertyName name, Parser<ParseHandler> *parser);

    static bool
    bindVarOrConst(JSContext *cx, BindData<ParseHandler> *data,
                   HandlePropertyName name, Parser<ParseHandler> *parser);

    static DefinitionNode null() { return ParseHandler::null(); }

    bool reportRedeclaration(Node pn, bool isConst, JSAtom *atom);
    bool reportBadReturn(Node pn, ParseReportKind kind, unsigned errnum, unsigned anonerrnum);
    bool checkFinalReturn(Node pn);

    bool leaveFunction(Node fn, HandlePropertyName funName,
                       FunctionSyntaxKind kind = Expression);

    friend class CompExprTransplanter;
    friend class GenexpGuard<ParseHandler>;
    friend struct BindData<ParseHandler>;
};



template <>
ParseNode *
Parser<FullParseHandler>::expr();

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::expr();

template <>
bool
Parser<FullParseHandler>::setAssignmentLhsOps(ParseNode *pn, JSOp op);

template <>
bool
Parser<SyntaxParseHandler>::setAssignmentLhsOps(Node pn, JSOp op);

} 
} 




#define TS(p) (&(p)->tokenStream)

#endif 
