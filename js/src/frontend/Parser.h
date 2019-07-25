







































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

JS_BEGIN_EXTERN_C

namespace js {

struct GlobalScope {
    GlobalScope(JSContext *cx, JSObject *globalObj, JSCodeGenerator *cg)
      : globalObj(globalObj), cg(cg), defs(cx), names(cx)
    { }

    struct GlobalDef {
        JSAtom        *atom;        
        JSFunctionBox *funbox;      
                                    
        uint32        knownSlot;    

        GlobalDef() { }
        GlobalDef(uint32 knownSlot)
          : atom(NULL), knownSlot(knownSlot)
        { }
        GlobalDef(JSAtom *atom, JSFunctionBox *box) :
          atom(atom), funbox(box)
        { }
    };

    JSObject        *globalObj;
    JSCodeGenerator *cg;

    







    Vector<GlobalDef, 16> defs;
    AtomIndexMap      names;
};

} 

#define NUM_TEMP_FREELISTS      6U      /* 32 to 2048 byte size classes (32 bit) */

typedef struct BindData BindData;

namespace js {

enum FunctionSyntaxKind { Expression, Statement };

struct Parser : private js::AutoGCRooter
{
    JSContext           *const context; 
    void                *tempFreeList[NUM_TEMP_FREELISTS];
    TokenStream         tokenStream;
    void                *tempPoolMark;  
    JSPrincipals        *principals;    
    StackFrame          *const callerFrame;  
    JSObject            *const callerVarObj; 
    JSParseNode         *nodeList;      
    uint32              functionCount;  
    JSObjectBox         *traceListHead; 
    JSTreeContext       *tc;            

    
    js::AutoKeepAtoms   keepAtoms;

    
    bool                foldConstants;

    Parser(JSContext *cx, JSPrincipals *prin = NULL, StackFrame *cfp = NULL, bool fold = true);
    ~Parser();

    friend void js::AutoGCRooter::trace(JSTracer *trc);
    friend struct ::JSTreeContext;
    friend struct Compiler;

    





    bool init(const jschar *base, size_t length, const char *filename, uintN lineno,
              JSVersion version);

    void setPrincipals(JSPrincipals *prin);

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionWithFlags() const { return tokenStream.versionWithFlags(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }
    bool hasXML() const { return tokenStream.hasXML(); }

    


    JSParseNode *parse(JSObject *chain);

#if JS_HAS_XML_SUPPORT
    JSParseNode *parseXMLText(JSObject *chain, bool allowList);
#endif

    


    JSObjectBox *newObjectBox(JSObject *obj);

    JSFunctionBox *newFunctionBox(JSObject *obj, JSParseNode *fn, JSTreeContext *tc);

    



    JSFunction *newFunction(JSTreeContext *tc, JSAtom *atom, FunctionSyntaxKind kind);

    




    bool analyzeFunctions(JSTreeContext *tc);
    void cleanFunctionList(JSFunctionBox **funbox);
    bool markFunArgs(JSFunctionBox *funbox);
    bool markExtensibleScopeDescendants(JSFunctionBox *funbox, bool hasExtensibleParent);
    void setFunctionKinds(JSFunctionBox *funbox, uint32 *tcflags);

    void trace(JSTracer *trc);

    


    inline bool reportErrorNumber(JSParseNode *pn, uintN flags, uintN errorNumber, ...);

private:
    















    JSParseNode *functionStmt();
    JSParseNode *functionExpr();
    JSParseNode *statements();
    JSParseNode *statement();
    JSParseNode *switchStatement();
    JSParseNode *forStatement();
    JSParseNode *tryStatement();
    JSParseNode *withStatement();
#if JS_HAS_BLOCK_SCOPE
    JSParseNode *letStatement();
#endif
    JSParseNode *expressionStatement();
    JSParseNode *variables(bool inLetHead);
    JSParseNode *expr();
    JSParseNode *assignExpr();
    JSParseNode *condExpr1();
    JSParseNode *orExpr1();
    JSParseNode *andExpr1i();
    JSParseNode *andExpr1n();
    JSParseNode *bitOrExpr1i();
    JSParseNode *bitOrExpr1n();
    JSParseNode *bitXorExpr1i();
    JSParseNode *bitXorExpr1n();
    JSParseNode *bitAndExpr1i();
    JSParseNode *bitAndExpr1n();
    JSParseNode *eqExpr1i();
    JSParseNode *eqExpr1n();
    JSParseNode *relExpr1i();
    JSParseNode *relExpr1n();
    JSParseNode *shiftExpr1i();
    JSParseNode *shiftExpr1n();
    JSParseNode *addExpr1i();
    JSParseNode *addExpr1n();
    JSParseNode *mulExpr1i();
    JSParseNode *mulExpr1n();
    JSParseNode *unaryExpr();
    JSParseNode *memberExpr(JSBool allowCallSyntax);
    JSParseNode *primaryExpr(js::TokenKind tt, JSBool afterDot);
    JSParseNode *parenExpr(JSBool *genexp = NULL);

    


    bool recognizeDirectivePrologue(JSParseNode *pn, bool *isDirectivePrologueMember);

    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(JSTreeContext &funtc, JSFunctionBox *funbox, JSParseNode **list);
    JSParseNode *functionBody();
    JSParseNode *functionDef(PropertyName *name, FunctionType type, FunctionSyntaxKind kind);

    JSParseNode *condition();
    JSParseNode *comprehensionTail(JSParseNode *kid, uintN blockid, bool isGenexp,
                                   js::TokenKind type = js::TOK_SEMI, JSOp op = JSOP_NOP);
    JSParseNode *generatorExpr(JSParseNode *kid);
    JSBool argumentList(JSParseNode *listNode);
    JSParseNode *bracketedExpr();
    JSParseNode *letBlock(JSBool statement);
    JSParseNode *returnOrYield(bool useAssignExpr);
    JSParseNode *destructuringExpr(BindData *data, js::TokenKind tt);

#if JS_HAS_XML_SUPPORT
    JSParseNode *endBracketedExpr();

    JSParseNode *propertySelector();
    JSParseNode *qualifiedSuffix(JSParseNode *pn);
    JSParseNode *qualifiedIdentifier();
    JSParseNode *attributeIdentifier();
    JSParseNode *xmlExpr(JSBool inTag);
    JSParseNode *xmlAtomNode();
    JSParseNode *xmlNameExpr();
    JSParseNode *xmlTagContent(js::TokenKind tagtype, JSAtom **namep);
    JSBool xmlElementContent(JSParseNode *pn);
    JSParseNode *xmlElementOrList(JSBool allowList);
    JSParseNode *xmlElementOrListRoot(JSBool allowList);
#endif 

    bool setAssignmentLhsOps(JSParseNode *pn, JSOp op);
};

inline bool
Parser::reportErrorNumber(JSParseNode *pn, uintN flags, uintN errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = tokenStream.reportCompileErrorNumberVA(pn, flags, errorNumber, args);
    va_end(args);
    return result;
}

bool
CheckStrictParameters(JSContext *cx, JSTreeContext *tc);

bool
DefineArg(JSParseNode *pn, JSAtom *atom, uintN i, JSTreeContext *tc);

} 




#define TS(p) (&(p)->tokenStream)

JS_END_EXTERN_C

#endif 
