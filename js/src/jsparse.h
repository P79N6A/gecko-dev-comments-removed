







































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
#define PN_TYPE(pn)  ((JSTokenType)(pn)->pn_type)

    JSTokenPos          pn_pos;         
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

    





    JSParseNode  *expr() const {
        JS_ASSERT(!pn_used);
        JS_ASSERT(pn_arity == PN_NAME || pn_arity == PN_FUNC);
        return pn_expr;
    }

    JSDefinition *lexdef() const {
        JS_ASSERT(pn_used);
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

    bool test(uintN flag) const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return !!(pn_dflags & flag);
    }

    bool isLet() const          { return test(PND_LET); }
    bool isConst() const        { return test(PND_CONST); }
    bool isInitialized() const  { return test(PND_INITIALIZED); }
    bool isTopLevel() const     { return test(PND_TOPLEVEL); }
    bool isBlockChild() const   { return test(PND_BLOCKCHILD); }
    bool isPlaceholder() const  { return test(PND_PLACEHOLDER); }

    
    bool isAssigned() const;
    bool isFunArg() const;
    void setFunArg();

    void become(JSParseNode *pn2);
    void clear();

    
    bool isLiteral() const {
        return PN_TYPE(this) == TOK_NUMBER ||
               PN_TYPE(this) == TOK_STRING ||
               (PN_TYPE(this) == TOK_PRIMARY && PN_OP(this) != JSOP_THIS);
    }

    






    bool isDirectivePrologueMember() const {
        if (PN_TYPE(this) == TOK_SEMI &&
            pn_arity == PN_UNARY) {
            JSParseNode *kid = pn_kid;
            return kid && PN_TYPE(kid) == TOK_STRING && !kid->pn_parens;
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




















































































































#define dn_uses         pn_link

struct JSDefinition : public JSParseNode
{
    







    JSDefinition *resolve() {
        JSParseNode *pn = this;
        while (!pn->pn_defn) {
            if (pn->pn_type == TOK_ASSIGN) {
                pn = pn->pn_left;
                continue;
            }
            pn = pn->lexdef();
        }
        return (JSDefinition *) pn;
    }

    bool test(uintN flag) const {
        JS_ASSERT(pn_defn);
        if (pn_dflags & flag)
            return true;
#ifdef DEBUG
        for (JSParseNode *pn = dn_uses; pn; pn = pn->pn_link) {
            JS_ASSERT(!pn->pn_defn);
            JS_ASSERT(!(pn->pn_dflags & flag));
        }
#endif
        return false;
    }

    bool isAssigned() const {
        return test(PND_ASSIGNED);
    }

    bool isFunArg() const {
        return test(PND_FUNARG);
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
        if (PN_TYPE(this) == TOK_FUNCTION)
            return FUNCTION;
        JS_ASSERT(PN_TYPE(this) == TOK_NAME);
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
JSParseNode::isAssigned() const
{
#ifdef DEBUG
    if (pn_defn)
        return ((JSDefinition *)this)->isAssigned();
#endif
    return test(PND_ASSIGNED);
}

inline bool
JSParseNode::isFunArg() const
{
#ifdef DEBUG
    if (pn_defn)
        return ((JSDefinition *)this)->isFunArg();
#endif
    return test(PND_FUNARG);
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
    uint32              queued:1,
                        inLoop:1,               
                        level:JSFB_LEVEL_BITS;
    uint32              tcflags;
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

struct JSCompiler {
    JSContext           *context;
    JSAtomListElement   *aleFreeList;
    void                *tempFreeList[NUM_TEMP_FREELISTS];
    JSTokenStream       tokenStream;
    void                *tempPoolMark;  
    JSPrincipals        *principals;    
    JSStackFrame        *callerFrame;   
    JSParseNode         *nodeList;      
    uint32              functionCount;  
    JSObjectBox         *traceListHead; 
    JSTempValueRooter   tempRoot;       

    JSCompiler(JSContext *cx, JSPrincipals *prin = NULL, JSStackFrame *cfp = NULL)
      : context(cx), aleFreeList(NULL), tokenStream(cx), principals(NULL),
        callerFrame(cfp), nodeList(NULL), functionCount(0), traceListHead(NULL)
    {
        memset(tempFreeList, 0, sizeof tempFreeList);
        setPrincipals(prin);
        JS_ASSERT_IF(cfp, cfp->script);
    }

    ~JSCompiler();

    





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




#define TS(jsc) (&(jsc)->tokenStream)

extern JSBool
js_FoldConstants(JSContext *cx, JSParseNode *pn, JSTreeContext *tc,
                 bool inCond = false);

JS_END_EXTERN_C

#endif 
