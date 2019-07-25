







































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

#define NUM_TEMP_FREELISTS      6U      /* 32 to 2048 byte size classes (32 bit) */

typedef struct BindData BindData;

namespace js {

enum FunctionSyntaxKind { Expression, Statement };

struct Parser : private AutoGCRooter
{
    JSContext           *const context; 
    void                *tempFreeList[NUM_TEMP_FREELISTS];
    TokenStream         tokenStream;
    void                *tempPoolMark;  
    JSPrincipals        *principals;    
    StackFrame          *const callerFrame;  
    JSObject            *const callerVarObj; 
    ParseNodeAllocator  allocator;
    uint32              functionCount;  
    ObjectBox           *traceListHead; 
    TreeContext         *tc;            

    
    AutoKeepAtoms       keepAtoms;

    
    bool                foldConstants;

    Parser(JSContext *cx, JSPrincipals *prin = NULL, StackFrame *cfp = NULL, bool fold = true);
    ~Parser();

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend struct TreeContext;

    






    bool init(const jschar *base, size_t length, const char *filename, uintN lineno,
              JSVersion version);

    void setPrincipals(JSPrincipals *prin);

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionWithFlags() const { return tokenStream.versionWithFlags(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }
    bool hasXML() const { return tokenStream.hasXML(); }

    


    ParseNode *parse(JSObject *chain);

#if JS_HAS_XML_SUPPORT
    ParseNode *parseXMLText(JSObject *chain, bool allowList);
#endif

    



    ObjectBox *newObjectBox(JSObject *obj);

    FunctionBox *newFunctionBox(JSObject *obj, ParseNode *fn, TreeContext *tc);

    



    JSFunction *newFunction(TreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind);

    void trace(JSTracer *trc);

    


    inline bool reportErrorNumber(ParseNode *pn, uintN flags, uintN errorNumber, ...);

  private:
    void *allocParseNode(size_t size) {
        JS_ASSERT(size == sizeof(ParseNode));
        return allocator.allocNode();
    }

    



    ParseNode *atomNode(ParseNodeKind kind, JSOp op);

  public:
    ParseNode *freeTree(ParseNode *pn) { return allocator.freeTree(pn); }
    void prepareNodeForMutation(ParseNode *pn) { return allocator.prepareNodeForMutation(pn); }

    
    JS_DECLARE_NEW_METHODS(allocParseNode, inline)

    
    ParseNode *statement();
    bool recognizeDirectivePrologue(ParseNode *pn, bool *isDirectivePrologueMember);
    ParseNode *functionBody();

  private:
    















    ParseNode *functionStmt();
    ParseNode *functionExpr();
    ParseNode *statements();

    ParseNode *switchStatement();
    ParseNode *forStatement();
    ParseNode *tryStatement();
    ParseNode *withStatement();
#if JS_HAS_BLOCK_SCOPE
    ParseNode *letStatement();
#endif
    ParseNode *expressionStatement();
    ParseNode *variables(ParseNodeKind kind, bool inLetHead);
    ParseNode *expr();
    ParseNode *assignExpr();
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
    ParseNode *primaryExpr(TokenKind tt, JSBool afterDot);
    ParseNode *parenExpr(JSBool *genexp = NULL);

    


    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(TreeContext &funtc, FunctionBox *funbox, ParseNode **list);

    ParseNode *functionDef(PropertyName *name, FunctionType type, FunctionSyntaxKind kind);

    ParseNode *unaryOpExpr(ParseNodeKind kind, JSOp op);

    ParseNode *condition();
    ParseNode *comprehensionTail(ParseNode *kid, uintN blockid, bool isGenexp,
                                 ParseNodeKind kind = PNK_SEMI, JSOp op = JSOP_NOP);
    ParseNode *generatorExpr(ParseNode *kid);
    JSBool argumentList(ParseNode *listNode);
    ParseNode *bracketedExpr();
    ParseNode *letBlock(JSBool statement);
    ParseNode *returnOrYield(bool useAssignExpr);
    ParseNode *destructuringExpr(BindData *data, TokenKind tt);

#if JS_HAS_XML_SUPPORT
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
#endif 

    bool setAssignmentLhsOps(ParseNode *pn, JSOp op);
};

inline bool
Parser::reportErrorNumber(ParseNode *pn, uintN flags, uintN errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportCompileErrorNumberVA(pn, flags, errorNumber, args);
    va_end(args);
    return result;
}

bool
CheckStrictParameters(JSContext *cx, TreeContext *tc);

bool
DefineArg(ParseNode *pn, JSAtom *atom, uintN i, TreeContext *tc);

} 




#define TS(p) (&(p)->tokenStream)

#endif 
