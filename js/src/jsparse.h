







































#ifndef jsparse_h___
#define jsparse_h___



#include "jsversion.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsscan.h"

JS_BEGIN_EXTERN_C
































































































































































































































typedef enum JSParseNodeArity {
    PN_NULLARY,                         
    PN_UNARY,                           
    PN_BINARY,                          
    PN_TERNARY,                         
    PN_FUNC,                            
    PN_LIST,                            
    PN_NAME,                            
    PN_NAMESET                          
} JSParseNodeArity;

struct JSDefinition;

struct JSParseNode {
    uint32              pn_type:16,     
                        pn_op:8,        
                        pn_arity:5,     
                        pn_parens:1,    
                        pn_used:1,      
                        pn_defn:1;      

#define PN_OP(pn)    ((JSOp)(pn)->pn_op)
#define PN_TYPE(pn)  ((js::TokenKind)(pn)->pn_type)

    js::TokenPos        pn_pos;         
    int32               pn_offset;      
    JSParseNode         *pn_next;       
    JSParseNode         *pn_link;       


    union {
        struct {                        
            JSParseNode *head;          
            JSParseNode **tail;         
            uint32      count;          
            uint32      xflags:12,      
                        blockid:20;     
        } list;
        struct {                        
            JSParseNode *kid1;          
            JSParseNode *kid2;          
            JSParseNode *kid3;          
        } ternary;
        struct {                        
            JSParseNode *left;
            JSParseNode *right;
            jsval       val;            
            uintN       iflags;         
        } binary;
        struct {                        
            JSParseNode *kid;
            jsint       num;            
            JSBool      hidden;         
        } unary;
        struct {                        
            union {
                JSAtom        *atom;    
                JSFunctionBox *funbox;  
                JSObjectBox   *objbox;  
            };
            union {
                JSParseNode  *expr;     

                JSDefinition *lexdef;   
            };
            uint32      cookie;         


            uint32      dflags:12,      
                        blockid:20;     

        } name;
        struct {                        
            JSAtomSet   names;          
            JSParseNode *tree;          
        } nameset;
        struct {                        
            JSAtom      *atom;          
            JSAtom      *atom2;         
        } apair;
        jsdouble        dval;           
    } pn_u;

#define pn_funbox       pn_u.name.funbox
#define pn_body         pn_u.name.expr
#define pn_cookie       pn_u.name.cookie
#define pn_dflags       pn_u.name.dflags
#define pn_blockid      pn_u.name.blockid
#define pn_index        pn_u.name.blockid /* reuse as object table index */
#define pn_head         pn_u.list.head
#define pn_tail         pn_u.list.tail
#define pn_count        pn_u.list.count
#define pn_xflags       pn_u.list.xflags
#define pn_kid1         pn_u.ternary.kid1
#define pn_kid2         pn_u.ternary.kid2
#define pn_kid3         pn_u.ternary.kid3
#define pn_left         pn_u.binary.left
#define pn_right        pn_u.binary.right
#define pn_val          pn_u.binary.val
#define pn_iflags       pn_u.binary.iflags
#define pn_kid          pn_u.unary.kid
#define pn_num          pn_u.unary.num
#define pn_hidden       pn_u.unary.hidden
#define pn_atom         pn_u.name.atom
#define pn_objbox       pn_u.name.objbox
#define pn_expr         pn_u.name.expr
#define pn_lexdef       pn_u.name.lexdef
#define pn_names        pn_u.nameset.names
#define pn_tree         pn_u.nameset.tree
#define pn_dval         pn_u.dval
#define pn_atom2        pn_u.apair.atom2

protected:
    void inline init(js::TokenKind type, JSOp op, JSParseNodeArity arity) {
        pn_type = type;
        pn_op = op;
        pn_arity = arity;
        pn_parens = false;
        JS_ASSERT(!pn_used);
        JS_ASSERT(!pn_defn);
        pn_next = pn_link = NULL;
    }

    static JSParseNode *create(JSParseNodeArity arity, JSTreeContext *tc);

public:
    static JSParseNode *newBinaryOrAppend(js::TokenKind tt, JSOp op, JSParseNode *left,
                                          JSParseNode *right, JSTreeContext *tc);

    





    JSParseNode  *expr() const {
        JS_ASSERT(!pn_used);
        JS_ASSERT(pn_arity == PN_NAME || pn_arity == PN_FUNC);
        return pn_expr;
    }

    JSDefinition *lexdef() const {
        JS_ASSERT(pn_used || isDeoptimized());
        JS_ASSERT(pn_arity == PN_NAME);
        return pn_lexdef;
    }

    JSParseNode  *maybeExpr()   { return pn_used ? NULL : expr(); }
    JSDefinition *maybeLexDef() { return pn_used ? lexdef() : NULL; }


#define PND_LET         0x01            /* let (block-scoped) binding */
#define PND_CONST       0x02            /* const binding (orthogonal to let) */
#define PND_INITIALIZED 0x04            /* initialized declaration */
#define PND_ASSIGNED    0x08            /* set if ever LHS of assignment */
#define PND_TOPLEVEL    0x10            /* function at top of body or prog */
#define PND_BLOCKCHILD  0x20            /* use or def is direct block child */
#define PND_GVAR        0x40            /* gvar binding, can't close over
                                           because it could be deleted */
#define PND_PLACEHOLDER 0x80            /* placeholder definition for lexdep */
#define PND_FUNARG     0x100            /* downward or upward funarg usage */
#define PND_BOUND      0x200            /* bound to a stack or global slot */
#define PND_DEOPTIMIZED 0x400           /* former pn_used name node, pn_lexdef
                                           still valid, but this use no longer
                                           optimizable via an upvar opcode */


#define PND_USE2DEF_FLAGS (PND_ASSIGNED | PND_FUNARG)


#define PNX_STRCAT      0x01            /* TOK_PLUS list has string term */
#define PNX_CANTFOLD    0x02            /* TOK_PLUS list has unfoldable term */
#define PNX_POPVAR      0x04            /* TOK_VAR last result needs popping */
#define PNX_FORINVAR    0x08            /* TOK_VAR is left kid of TOK_IN node,
                                           which is left kid of TOK_FOR */
#define PNX_ENDCOMMA    0x10            /* array literal has comma at end */
#define PNX_XMLROOT     0x20            /* top-most node in XML literal tree */
#define PNX_GROUPINIT   0x40            /* var [a, b] = [c, d]; unit list */
#define PNX_NEEDBRACES  0x80            /* braces necessary due to closure */
#define PNX_FUNCDEFS   0x100            /* contains top-level function
                                           statements */
#define PNX_DESTRUCT   0x200            /* destructuring special cases:
                                           1. shorthand syntax used, at present
                                              object destructuring ({x,y}) only;
                                           2. the first child of function body
                                              is code evaluating destructuring
                                              arguments */
#define PNX_HOLEY      0x400            /* array initialiser has holes */

    uintN frameLevel() const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return UPVAR_FRAME_SKIP(pn_cookie);
    }

    uintN frameSlot() const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return UPVAR_FRAME_SLOT(pn_cookie);
    }

    inline bool test(uintN flag) const;

    bool isLet() const          { return test(PND_LET); }
    bool isConst() const        { return test(PND_CONST); }
    bool isInitialized() const  { return test(PND_INITIALIZED); }
    bool isTopLevel() const     { return test(PND_TOPLEVEL); }
    bool isBlockChild() const   { return test(PND_BLOCKCHILD); }
    bool isPlaceholder() const  { return test(PND_PLACEHOLDER); }
    bool isDeoptimized() const  { return test(PND_DEOPTIMIZED); }
    bool isAssigned() const     { return test(PND_ASSIGNED); }
    bool isFunArg() const       { return test(PND_FUNARG); }

    
    void setFunArg();

    void become(JSParseNode *pn2);
    void clear();

    
    bool isLiteral() const {
        return PN_TYPE(this) == js::TOK_NUMBER ||
               PN_TYPE(this) == js::TOK_STRING ||
               (PN_TYPE(this) == js::TOK_PRIMARY && PN_OP(this) != JSOP_THIS);
    }

    






    bool isDirectivePrologueMember() const {
        if (PN_TYPE(this) == js::TOK_SEMI) {
            JS_ASSERT(pn_arity == PN_UNARY);
            JSParseNode *kid = pn_kid;
            return kid && PN_TYPE(kid) == js::TOK_STRING && !kid->pn_parens;
        }
        return false;
    }

    



    bool isDirective() const {
        JS_ASSERT(isDirectivePrologueMember());
        JSParseNode *kid = pn_kid;
        JSString *str = ATOM_TO_STRING(kid->pn_atom);

        




        return (pn_pos.begin.lineno == pn_pos.end.lineno &&
                pn_pos.begin.index + str->length() + 2 == pn_pos.end.index);
    }

    



    JSParseNode *last() const {
        JS_ASSERT(pn_arity == PN_LIST);
        JS_ASSERT(pn_count != 0);
        return (JSParseNode *)((char *)pn_tail - offsetof(JSParseNode, pn_next));
    }

    void makeEmpty() {
        JS_ASSERT(pn_arity == PN_LIST);
        pn_head = NULL;
        pn_tail = &pn_head;
        pn_count = 0;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void initList(JSParseNode *pn) {
        JS_ASSERT(pn_arity == PN_LIST);
        pn_head = pn;
        pn_tail = &pn->pn_next;
        pn_count = 1;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void append(JSParseNode *pn) {
        JS_ASSERT(pn_arity == PN_LIST);
        *pn_tail = pn;
        pn_tail = &pn->pn_next;
        pn_count++;
    }
};

namespace js {

struct NullaryNode : public JSParseNode {
    static inline NullaryNode *create(JSTreeContext *tc) {
        return (NullaryNode *)JSParseNode::create(PN_NULLARY, tc);
    }
};

struct UnaryNode : public JSParseNode {
    static inline UnaryNode *create(JSTreeContext *tc) {
        return (UnaryNode *)JSParseNode::create(PN_UNARY, tc);
    }
};

struct BinaryNode : public JSParseNode {
    static inline BinaryNode *create(JSTreeContext *tc) {
        return (BinaryNode *)JSParseNode::create(PN_BINARY, tc);
    }
};

struct TernaryNode : public JSParseNode {
    static inline TernaryNode *create(JSTreeContext *tc) {
        return (TernaryNode *)JSParseNode::create(PN_TERNARY, tc);
    }
};

struct ListNode : public JSParseNode {
    static inline ListNode *create(JSTreeContext *tc) {
        return (ListNode *)JSParseNode::create(PN_LIST, tc);
    }
};

struct FunctionNode : public JSParseNode {
    static inline FunctionNode *create(JSTreeContext *tc) {
        return (FunctionNode *)JSParseNode::create(PN_FUNC, tc);
    }
};

struct NameNode : public JSParseNode {
    static NameNode *create(JSAtom *atom, JSTreeContext *tc);

    void inline initCommon(JSTreeContext *tc);
};

struct NameSetNode : public JSParseNode {
    static inline NameSetNode *create(JSTreeContext *tc) {
        return (NameSetNode *)JSParseNode::create(PN_NAMESET, tc);
    }
};

struct LexicalScopeNode : public JSParseNode {
    static inline LexicalScopeNode *create(JSTreeContext *tc) {
        return (LexicalScopeNode *)JSParseNode::create(PN_NAME, tc);
    }
};

} 




















































































































#define dn_uses         pn_link

struct JSDefinition : public JSParseNode
{
    







    JSDefinition *resolve() {
        JSParseNode *pn = this;
        while (!pn->pn_defn) {
            if (pn->pn_type == js::TOK_ASSIGN) {
                pn = pn->pn_left;
                continue;
            }
            pn = pn->lexdef();
        }
        return (JSDefinition *) pn;
    }

    bool isFreeVar() const {
        JS_ASSERT(pn_defn);
        return pn_cookie == FREE_UPVAR_COOKIE || test(PND_GVAR);
    }

    
#ifdef CONST
# undef CONST
#endif
    enum Kind { VAR, CONST, LET, FUNCTION, ARG, UNKNOWN };

    bool isBindingForm() { return int(kind()) <= int(LET); }

    static const char *kindString(Kind kind);

    Kind kind() {
        if (PN_TYPE(this) == js::TOK_FUNCTION)
            return FUNCTION;
        JS_ASSERT(PN_TYPE(this) == js::TOK_NAME);
        if (PN_OP(this) == JSOP_NOP)
            return UNKNOWN;
        if (PN_OP(this) == JSOP_GETARG)
            return ARG;
        if (isConst())
            return CONST;
        if (isLet())
            return LET;
        return VAR;
    }
};

inline bool
JSParseNode::test(uintN flag) const
{
    JS_ASSERT(pn_defn || pn_arity == PN_FUNC || pn_arity == PN_NAME);
#ifdef DEBUG
    if ((flag & (PND_ASSIGNED | PND_FUNARG)) && pn_defn && !(pn_dflags & flag)) {
        for (JSParseNode *pn = ((JSDefinition *) this)->dn_uses; pn; pn = pn->pn_link) {
            JS_ASSERT(!pn->pn_defn);
            JS_ASSERT(!(pn->pn_dflags & flag));
        }
    }
#endif
    return !!(pn_dflags & flag);
}

inline void
JSParseNode::setFunArg()
{
    









    JS_ASSERT(!(pn_defn & pn_used));
    if (pn_used)
        pn_lexdef->pn_dflags |= PND_FUNARG;
    pn_dflags |= PND_FUNARG;
}

struct JSObjectBox {
    JSObjectBox         *traceLink;
    JSObjectBox         *emitLink;
    JSObject            *object;
};

#define JSFB_LEVEL_BITS 14

struct JSFunctionBox : public JSObjectBox
{
    JSParseNode         *node;
    JSFunctionBox       *siblings;
    JSFunctionBox       *kids;
    JSFunctionBox       *parent;
    JSParseNode         *methods;               




    uint32              queued:1,
                        inLoop:1,               
                        level:JSFB_LEVEL_BITS;
    uint32              tcflags;

    bool joinable() const;

    









    bool shouldUnbrand(uintN methods, uintN slowMethods) const;
};

struct JSFunctionBoxQueue {
    JSFunctionBox       **vector;
    size_t              head, tail;
    size_t              lengthMask;

    size_t count()  { return head - tail; }
    size_t length() { return lengthMask + 1; }

    JSFunctionBoxQueue()
      : vector(NULL), head(0), tail(0), lengthMask(0) { }

    bool init(uint32 count) {
        lengthMask = JS_BITMASK(JS_CeilingLog2(count));
        vector = new JSFunctionBox*[length()];
        return !!vector;
    }

    ~JSFunctionBoxQueue() { delete[] vector; }

    void push(JSFunctionBox *funbox) {
        if (!funbox->queued) {
            JS_ASSERT(count() < length());
            vector[head++ & lengthMask] = funbox;
            funbox->queued = true;
        }
    }

    JSFunctionBox *pull() {
        if (tail == head)
            return NULL;
        JS_ASSERT(tail < head);
        JSFunctionBox *funbox = vector[tail++ & lengthMask];
        funbox->queued = false;
        return funbox;
    }
};

#define NUM_TEMP_FREELISTS      6U      /* 32 to 2048 byte size classes (32 bit) */

typedef struct BindData BindData;

namespace js {

struct Parser : private js::AutoGCRooter
{
    JSContext           * const context; 
    JSAtomListElement   *aleFreeList;
    void                *tempFreeList[NUM_TEMP_FREELISTS];
    js::TokenStream     tokenStream;
    void                *tempPoolMark;  
    JSPrincipals        *principals;    
    JSStackFrame *const callerFrame;    
    JSObject     *const callerVarObj;   
    JSParseNode         *nodeList;      
    uint32              functionCount;  
    JSObjectBox         *traceListHead; 
    JSTreeContext       *tc;            

    
    js::AutoKeepAtoms   keepAtoms;

    Parser(JSContext *cx, JSPrincipals *prin = NULL, JSStackFrame *cfp = NULL)
      : js::AutoGCRooter(cx, PARSER), context(cx),
        aleFreeList(NULL), tokenStream(cx), principals(NULL), callerFrame(cfp),
        callerVarObj(cfp ? cfp->varobj(cx->containingCallStack(cfp)) : NULL),
        nodeList(NULL), functionCount(0), traceListHead(NULL), tc(NULL),
        keepAtoms(cx->runtime)
    {
        js::PodArrayZero(tempFreeList);
        setPrincipals(prin);
        JS_ASSERT_IF(cfp, cfp->script);
    }

    ~Parser();

    friend void js::AutoGCRooter::trace(JSTracer *trc);
    friend struct JSTreeContext;
    friend struct Compiler;

    





    bool init(const jschar *base, size_t length,
              FILE *fp, const char *filename, uintN lineno);

    void setPrincipals(JSPrincipals *prin);

    


    JSParseNode *parse(JSObject *chain);

#if JS_HAS_XML_SUPPORT
    JSParseNode *parseXMLText(JSObject *chain, bool allowList);
#endif

    


    JSObjectBox *newObjectBox(JSObject *obj);

    JSFunctionBox *newFunctionBox(JSObject *obj, JSParseNode *fn, JSTreeContext *tc);

    



    JSFunction *newFunction(JSTreeContext *tc, JSAtom *atom, uintN lambda);

    




    bool analyzeFunctions(JSFunctionBox *funbox, uint32& tcflags);
    bool markFunArgs(JSFunctionBox *funbox, uintN tcflags);
    void setFunctionKinds(JSFunctionBox *funbox, uint32& tcflags);

    void trace(JSTracer *trc);

private:
    







    JSParseNode *functionStmt();
    JSParseNode *functionExpr();
    JSParseNode *statements();
    JSParseNode *statement();
    JSParseNode *variables(bool inLetHead);
    JSParseNode *expr();
    JSParseNode *assignExpr();
    JSParseNode *condExpr();
    JSParseNode *orExpr();
    JSParseNode *andExpr();
    JSParseNode *bitOrExpr();
    JSParseNode *bitXorExpr();
    JSParseNode *bitAndExpr();
    JSParseNode *eqExpr();
    JSParseNode *relExpr();
    JSParseNode *shiftExpr();
    JSParseNode *addExpr();
    JSParseNode *mulExpr();
    JSParseNode *unaryExpr();
    JSParseNode *memberExpr(JSBool allowCallSyntax);
    JSParseNode *primaryExpr(js::TokenKind tt, JSBool afterDot);
    JSParseNode *parenExpr(JSParseNode *pn1, JSBool *genexp);

    


    bool recognizeDirectivePrologue(JSParseNode *pn);
    JSParseNode *functionBody();
    JSParseNode *functionDef(uintN lambda, bool namePermitted);
    JSParseNode *condition();
    JSParseNode *comprehensionTail(JSParseNode *kid, uintN blockid,
                                   js::TokenKind type = js::TOK_SEMI, JSOp op = JSOP_NOP);
    JSParseNode *generatorExpr(JSParseNode *pn, JSParseNode *kid);
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
};

struct Compiler
{
    Parser parser;

    Compiler(JSContext *cx, JSPrincipals *prin = NULL, JSStackFrame *cfp = NULL)
      : parser(cx, prin, cfp)
    {
    }

    


    inline bool
    init(const jschar *base, size_t length,
         FILE *fp, const char *filename, uintN lineno)
    {
        return parser.init(base, length, fp, filename, lineno);
    }

    static bool
    compileFunctionBody(JSContext *cx, JSFunction *fun, JSPrincipals *principals,
                        const jschar *chars, size_t length,
                        const char *filename, uintN lineno);

    static JSScript *
    compileScript(JSContext *cx, JSObject *scopeChain, JSStackFrame *callerFrame,
                  JSPrincipals *principals, uint32 tcflags,
                  const jschar *chars, size_t length,
                  FILE *file, const char *filename, uintN lineno,
                  JSString *source = NULL,
                  unsigned staticLevel = 0);
};

} 




#define TS(p) (&(p)->tokenStream)

extern JSBool
js_FoldConstants(JSContext *cx, JSParseNode *pn, JSTreeContext *tc,
                 bool inCond = false);

JS_END_EXTERN_C

#endif 
