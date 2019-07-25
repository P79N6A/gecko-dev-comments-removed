






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
#include "frontend/TreeContext.h"

typedef struct BindData BindData;

namespace js {

class StaticBlockObject;

enum FunctionSyntaxKind { Expression, Statement };
enum LetContext { LetExpresion, LetStatement };
enum VarContext { HoistVars, DontHoistVars };

struct Parser : private AutoGCRooter
{
    JSContext           *const context; 
    StrictModeGetter    strictModeGetter; 
    TokenStream         tokenStream;
    void                *tempPoolMark;  
    JSPrincipals        *principals;    
    JSPrincipals        *originPrincipals;   
    StackFrame          *const callerFrame;  
    ParseNodeAllocator  allocator;
    ObjectBox           *traceListHead; 

    TreeContext         *tc;            

    
    AutoKeepAtoms       keepAtoms;

    
    const bool          foldConstants:1;

    
    const bool          compileAndGo:1;

    Parser(JSContext *cx, JSPrincipals *prin, JSPrincipals *originPrin,
           const jschar *chars, size_t length, const char *fn, unsigned ln, JSVersion version,
           StackFrame *cfp, bool foldConstants, bool compileAndGo);
    ~Parser();

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend struct TreeContext;

    





    bool init();

    void setPrincipals(JSPrincipals *prin, JSPrincipals *originPrin);

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionWithFlags() const { return tokenStream.versionWithFlags(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    


    ParseNode *parse(JSObject *chain);

#if JS_HAS_XML_SUPPORT
    ParseNode *parseXMLText(JSObject *chain, bool allowList);
#endif

    



    ObjectBox *newObjectBox(JSObject *obj);

    FunctionBox *newFunctionBox(JSObject *obj, ParseNode *fn, TreeContext *tc);

    



    JSFunction *newFunction(TreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind);

    void trace(JSTracer *trc);

    


    inline bool reportErrorNumber(ParseNode *pn, unsigned flags, unsigned errorNumber, ...);

  private:
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
    bool recognizeDirectivePrologue(ParseNode *pn, bool *isDirectivePrologueMember);

    



    enum FunctionBodyType { StatementListBody, ExpressionBody };
    ParseNode *functionBody(FunctionBodyType type);

    bool checkForArgumentsAndRest();

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
    ParseNode *memberExpr(JSBool allowCallSyntax);
    ParseNode *primaryExpr(TokenKind tt, bool afterDoubleDot);
    ParseNode *parenExpr(JSBool *genexp = NULL);

    


    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(ParseNode **list, bool &hasDefaults, bool &hasRest);

    ParseNode *functionDef(HandlePropertyName name, FunctionType type, FunctionSyntaxKind kind);

    ParseNode *unaryOpExpr(ParseNodeKind kind, JSOp op);

    ParseNode *condition();
    ParseNode *comprehensionTail(ParseNode *kid, unsigned blockid, bool isGenexp,
                                 ParseNodeKind kind = PNK_SEMI, JSOp op = JSOP_NOP);
    ParseNode *generatorExpr(ParseNode *kid);
    JSBool argumentList(ParseNode *listNode);
    ParseNode *bracketedExpr();
    ParseNode *letBlock(LetContext letContext);
    ParseNode *returnOrYield(bool useAssignExpr);
    ParseNode *destructuringExpr(BindData *data, TokenKind tt);

    bool checkForFunctionNode(PropertyName *name, ParseNode *node);

    ParseNode *identifierName(bool afterDoubleDot);

#if JS_HAS_XML_SUPPORT
    
    bool allowsXML() const { return !tc->sc->inStrictMode() && tokenStream.allowsXML(); }

    ParseNode *endBracketedExpr();

    ParseNode *propertySelector();
    ParseNode *qualifiedSuffix(ParseNode *pn);
    ParseNode *qualifiedIdentifier();
    ParseNode *attributeIdentifier();
    ParseNode *xmlExpr(JSBool inTag);
    ParseNode *xmlNameExpr();
    ParseNode *xmlTagContent(ParseNodeKind tagkind, JSAtom **namep);
    JSBool xmlElementContent(ParseNode *pn);
    ParseNode *xmlElementOrList(JSBool allowList);
    ParseNode *xmlElementOrListRoot(JSBool allowList);

    ParseNode *starOrAtPropertyIdentifier(TokenKind tt);
    ParseNode *propertyQualifiedIdentifier();
#endif 

    bool setAssignmentLhsOps(ParseNode *pn, JSOp op);
    bool matchInOrOf(bool *isForOfp);
};

inline bool
Parser::reportErrorNumber(ParseNode *pn, unsigned flags, unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportCompileErrorNumberVA(pn, flags, errorNumber, args);
    va_end(args);
    return result;
}

bool
DefineArg(ParseNode *pn, JSAtom *atom, unsigned i, Parser *parser);

} 




#define TS(p) (&(p)->tokenStream)

#endif 
