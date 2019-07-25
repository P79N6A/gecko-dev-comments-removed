







































#ifndef ParseNode_h__
#define ParseNode_h__

#include "jsscript.h"

#include "frontend/ParseMaps.h"
#include "frontend/TokenStream.h"

namespace js {




























































































































































































































































enum ParseNodeArity {
    PN_NULLARY,                         
    PN_UNARY,                           
    PN_BINARY,                          
    PN_TERNARY,                         
    PN_FUNC,                            
    PN_LIST,                            
    PN_NAME,                            
    PN_NAMESET                          
};

struct Definition;

struct ParseNode {
  private:
    uint32              pn_type   : 16, 
                        pn_op     : 8,  
                        pn_arity  : 5,  
                        pn_parens : 1,  
                        pn_used   : 1,  
                        pn_defn   : 1;  

  public:
    ParseNode(TokenKind type, JSOp op, ParseNodeArity arity)
      : pn_type(type), pn_op(op), pn_arity(arity), pn_parens(0), pn_used(0), pn_defn(0),
        pn_offset(0), pn_next(NULL), pn_link(NULL)
    {
        pn_pos.begin.index = 0;
        pn_pos.begin.lineno = 0;
        pn_pos.end.index = 0;
        pn_pos.end.lineno = 0;
        memset(&pn_u, 0, sizeof pn_u);
    }

    ParseNode(TokenKind type, JSOp op, ParseNodeArity arity, const TokenPos &pos)
      : pn_type(type), pn_op(op), pn_arity(arity), pn_parens(0), pn_used(0), pn_defn(0),
        pn_pos(pos), pn_offset(0), pn_next(NULL), pn_link(NULL)
    {
        memset(&pn_u, 0, sizeof pn_u);
    }

    JSOp getOp() const                     { return JSOp(pn_op); }
    void setOp(JSOp op)                    { pn_op = op; }
    bool isOp(JSOp op) const               { return getOp() == op; }
    TokenKind getKind() const              { return TokenKind(pn_type); }
    void setKind(TokenKind kind)           { pn_type = kind; }
    bool isKind(TokenKind kind) const      { return getKind() == kind; }
    ParseNodeArity getArity() const        { return ParseNodeArity(pn_arity); }
    bool isArity(ParseNodeArity a) const   { return getArity() == a; }
    void setArity(ParseNodeArity a)        { pn_arity = a; }

    bool isEquality() const                { return TokenKindIsEquality(getKind()); }
    bool isUnaryOp() const                 { return TokenKindIsUnaryOp(getKind()); }
    bool isXMLNameOp() const               { return TokenKindIsXML(getKind()); }

    
    bool isInParens() const                { return pn_parens; }
    void setInParens(bool enabled)         { pn_parens = enabled; }
    bool isUsed() const                    { return pn_used; }
    void setUsed(bool enabled)             { pn_used = enabled; }
    bool isDefn() const                    { return pn_defn; }
    void setDefn(bool enabled)             { pn_defn = enabled; }

    TokenPos            pn_pos;         
    int32               pn_offset;      
    ParseNode           *pn_next;       
    ParseNode           *pn_link;       



    union {
        struct {                        
            ParseNode   *head;          
            ParseNode   **tail;         
            uint32      count;          
            uint32      xflags:12,      
                        blockid:20;     
        } list;
        struct {                        
            ParseNode   *kid1;          
            ParseNode   *kid2;          
            ParseNode   *kid3;          
        } ternary;
        struct {                        
            ParseNode   *left;
            ParseNode   *right;
            Value       *pval;          
            uintN       iflags;         
        } binary;
        struct {                        
            ParseNode   *kid;
            jsint       num;            
            JSBool      hidden;         


        } unary;
        struct {                        
            union {
                JSAtom        *atom;    
                FunctionBox   *funbox;  
                ObjectBox     *objbox;  
            };
            union {
                ParseNode    *expr;     

                Definition   *lexdef;   
            };
            UpvarCookie cookie;         


            uint32      dflags:12,      
                        blockid:20;     

        } name;
        struct {                        
            AtomDefnMapPtr   defnMap;
            ParseNode        *tree;     
        } nameset;
        struct {                        
            PropertyName     *target;   
            JSAtom           *data;     
        } xmlpi;
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
#define pn_pval         pn_u.binary.pval
#define pn_iflags       pn_u.binary.iflags
#define pn_kid          pn_u.unary.kid
#define pn_num          pn_u.unary.num
#define pn_hidden       pn_u.unary.hidden
#define pn_prologue     pn_u.unary.hidden
#define pn_atom         pn_u.name.atom
#define pn_objbox       pn_u.name.objbox
#define pn_expr         pn_u.name.expr
#define pn_lexdef       pn_u.name.lexdef
#define pn_names        pn_u.nameset.defnMap
#define pn_tree         pn_u.nameset.tree
#define pn_dval         pn_u.dval
#define pn_pitarget     pn_u.xmlpi.target
#define pn_pidata       pn_u.xmlpi.data

  protected:
    void init(TokenKind type, JSOp op, ParseNodeArity arity) {
        pn_type = type;
        pn_op = op;
        pn_arity = arity;
        pn_parens = false;
        JS_ASSERT(!pn_used);
        JS_ASSERT(!pn_defn);
        pn_names.init();
        pn_next = pn_link = NULL;
    }

    static ParseNode *create(ParseNodeArity arity, TreeContext *tc);

  public:
    



    static ParseNode *
    append(TokenKind tt, JSOp op, ParseNode *left, ParseNode *right);

    




    static ParseNode *
    newBinaryOrAppend(TokenKind tt, JSOp op, ParseNode *left, ParseNode *right, TreeContext *tc);

    





    ParseNode *expr() const {
        JS_ASSERT(!pn_used);
        JS_ASSERT(pn_arity == PN_NAME || pn_arity == PN_FUNC);
        return pn_expr;
    }

    Definition *lexdef() const {
        JS_ASSERT(pn_used || isDeoptimized());
        JS_ASSERT(pn_arity == PN_NAME);
        return pn_lexdef;
    }

    ParseNode  *maybeExpr()   { return pn_used ? NULL : expr(); }
    Definition *maybeLexDef() { return pn_used ? lexdef() : NULL; }


#define PND_LET         0x01            /* let (block-scoped) binding */
#define PND_CONST       0x02            /* const binding (orthogonal to let) */
#define PND_INITIALIZED 0x04            /* initialized declaration */
#define PND_ASSIGNED    0x08            /* set if ever LHS of assignment */
#define PND_TOPLEVEL    0x10            /* see isTopLevel() below */
#define PND_BLOCKCHILD  0x20            /* use or def is direct block child */
#define PND_GVAR        0x40            /* gvar binding, can't close over
                                           because it could be deleted */
#define PND_PLACEHOLDER 0x80            /* placeholder definition for lexdep */
#define PND_FUNARG     0x100            /* downward or upward funarg usage */
#define PND_BOUND      0x200            /* bound to a stack or global slot */
#define PND_DEOPTIMIZED 0x400           /* former pn_used name node, pn_lexdef
                                           still valid, but this use no longer
                                           optimizable via an upvar opcode */
#define PND_CLOSED      0x800           /* variable is closed over */


#define PND_USE2DEF_FLAGS (PND_ASSIGNED | PND_FUNARG | PND_CLOSED)


#define PNX_STRCAT      0x01            /* TOK_PLUS list has string term */
#define PNX_CANTFOLD    0x02            /* TOK_PLUS list has unfoldable term */
#define PNX_POPVAR      0x04            /* TOK_VAR last result needs popping */
#define PNX_FORINVAR    0x08            /* TOK_VAR is left kid of TOK_IN node,
                                           which is left kid of TOK_FOR */
#define PNX_ENDCOMMA    0x10            /* array literal has comma at end */
#define PNX_XMLROOT     0x20            /* top-most node in XML literal tree */
#define PNX_GROUPINIT   0x40            /* var [a, b] = [c, d]; unit list */
#define PNX_NEEDBRACES  0x80            /* braces necessary due to closure */
#define PNX_FUNCDEFS   0x100            /* contains top-level function statements */
#define PNX_SETCALL    0x100            /* call expression in lvalue context */
#define PNX_DESTRUCT   0x200            /* destructuring special cases:
                                           1. shorthand syntax used, at present
                                              object destructuring ({x,y}) only;
                                           2. code evaluating destructuring
                                              arguments occurs before function
                                              body */
#define PNX_HOLEY      0x400            /* array initialiser has holes */
#define PNX_NONCONST   0x800            /* initialiser has non-constants */

    uintN frameLevel() const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return pn_cookie.level();
    }

    uintN frameSlot() const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return pn_cookie.slot();
    }

    inline bool test(uintN flag) const;

    bool isLet() const          { return test(PND_LET); }
    bool isConst() const        { return test(PND_CONST); }
    bool isInitialized() const  { return test(PND_INITIALIZED); }
    bool isBlockChild() const   { return test(PND_BLOCKCHILD); }
    bool isPlaceholder() const  { return test(PND_PLACEHOLDER); }
    bool isDeoptimized() const  { return test(PND_DEOPTIMIZED); }
    bool isAssigned() const     { return test(PND_ASSIGNED); }
    bool isFunArg() const       { return test(PND_FUNARG); }
    bool isClosed() const       { return test(PND_CLOSED); }

    











    bool isTopLevel() const     { return test(PND_TOPLEVEL); }

    
    void setFunArg();

    void become(ParseNode *pn2);
    void clear();

    
    bool isLiteral() const {
        return isKind(TOK_NUMBER) ||
               isKind(TOK_STRING) ||
               (isKind(TOK_PRIMARY) && !isOp(JSOP_THIS));
    }

    














    bool isStringExprStatement() const {
        if (getKind() == TOK_SEMI) {
            JS_ASSERT(pn_arity == PN_UNARY);
            ParseNode *kid = pn_kid;
            return kid && kid->getKind() == TOK_STRING && !kid->pn_parens;
        }
        return false;
    }

    




    bool isEscapeFreeStringLiteral() const {
        JS_ASSERT(pn_type == TOK_STRING && !pn_parens);
        JSString *str = pn_atom;

        




        return (pn_pos.begin.lineno == pn_pos.end.lineno &&
                pn_pos.begin.index + str->length() + 2 == pn_pos.end.index);
    }

    
    bool isDirectivePrologueMember() const { return pn_prologue; }

#ifdef JS_HAS_GENERATOR_EXPRS
    


    bool isGeneratorExpr() const {
        if (getKind() == TOK_LP) {
            ParseNode *callee = this->pn_head;
            if (callee->getKind() == TOK_FUNCTION) {
                ParseNode *body = (callee->pn_body->getKind() == TOK_UPVARS)
                                  ? callee->pn_body->pn_tree
                                  : callee->pn_body;
                if (body->getKind() == TOK_LEXICALSCOPE)
                    return true;
            }
        }
        return false;
    }

    ParseNode *generatorExpr() const {
        JS_ASSERT(isGeneratorExpr());
        ParseNode *callee = this->pn_head;
        ParseNode *body = callee->pn_body->getKind() == TOK_UPVARS
                          ? callee->pn_body->pn_tree
                          : callee->pn_body;
        JS_ASSERT(body->getKind() == TOK_LEXICALSCOPE);
        return body->pn_expr;
    }
#endif

    



    ParseNode *last() const {
        JS_ASSERT(pn_arity == PN_LIST);
        JS_ASSERT(pn_count != 0);
        return (ParseNode *)(uintptr_t(pn_tail) - offsetof(ParseNode, pn_next));
    }

    void makeEmpty() {
        JS_ASSERT(pn_arity == PN_LIST);
        pn_head = NULL;
        pn_tail = &pn_head;
        pn_count = 0;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void initList(ParseNode *pn) {
        JS_ASSERT(pn_arity == PN_LIST);
        pn_head = pn;
        pn_tail = &pn->pn_next;
        pn_count = 1;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void append(ParseNode *pn) {
        JS_ASSERT(pn_arity == PN_LIST);
        *pn_tail = pn;
        pn_tail = &pn->pn_next;
        pn_count++;
    }

    bool getConstantValue(JSContext *cx, bool strictChecks, Value *vp);
    inline bool isConstant();
};

struct NullaryNode : public ParseNode {
    static inline NullaryNode *create(TreeContext *tc) {
        return (NullaryNode *)ParseNode::create(PN_NULLARY, tc);
    }
};

struct UnaryNode : public ParseNode {
    UnaryNode(TokenKind type, JSOp op, const TokenPos &pos, ParseNode *kid)
      : ParseNode(type, op, PN_UNARY, pos)
    {
        pn_kid = kid;
    }

    static inline UnaryNode *create(TreeContext *tc) {
        return (UnaryNode *)ParseNode::create(PN_UNARY, tc);
    }
};

struct BinaryNode : public ParseNode {
    BinaryNode(TokenKind type, JSOp op, const TokenPos &pos, ParseNode *left, ParseNode *right)
      : ParseNode(type, op, PN_BINARY, pos)
    {
        pn_left = left;
        pn_right = right;
    }

    BinaryNode(TokenKind type, JSOp op, ParseNode *left, ParseNode *right)
      : ParseNode(type, op, PN_BINARY, TokenPos::box(left->pn_pos, right->pn_pos))
    {
        pn_left = left;
        pn_right = right;
    }

    static inline BinaryNode *create(TreeContext *tc) {
        return (BinaryNode *)ParseNode::create(PN_BINARY, tc);
    }
};

struct TernaryNode : public ParseNode {
    TernaryNode(TokenKind type, JSOp op, ParseNode *kid1, ParseNode *kid2, ParseNode *kid3)
      : ParseNode(type, op, PN_TERNARY,
                  TokenPos::make((kid1 ? kid1 : kid2 ? kid2 : kid3)->pn_pos.begin,
                                 (kid3 ? kid3 : kid2 ? kid2 : kid1)->pn_pos.end))
    {
        pn_kid1 = kid1;
        pn_kid2 = kid2;
        pn_kid3 = kid3;
    }

    static inline TernaryNode *create(TreeContext *tc) {
        return (TernaryNode *)ParseNode::create(PN_TERNARY, tc);
    }
};

struct ListNode : public ParseNode {
    static inline ListNode *create(TreeContext *tc) {
        return (ListNode *)ParseNode::create(PN_LIST, tc);
    }
};

struct FunctionNode : public ParseNode {
    static inline FunctionNode *create(TreeContext *tc) {
        return (FunctionNode *)ParseNode::create(PN_FUNC, tc);
    }
};

struct NameNode : public ParseNode {
    static NameNode *create(JSAtom *atom, TreeContext *tc);

    inline void initCommon(TreeContext *tc);
};

struct NameSetNode : public ParseNode {
    static inline NameSetNode *create(TreeContext *tc) {
        return (NameSetNode *)ParseNode::create(PN_NAMESET, tc);
    }
};

struct LexicalScopeNode : public ParseNode {
    static inline LexicalScopeNode *create(TreeContext *tc) {
        return (LexicalScopeNode *)ParseNode::create(PN_NAME, tc);
    }
};

ParseNode *
CloneLeftHandSide(ParseNode *opn, TreeContext *tc);























































































































#define dn_uses         pn_link

struct Definition : public ParseNode
{
    








    Definition *resolve() {
        ParseNode *pn = this;
        while (!pn->isDefn()) {
            if (pn->getKind() == TOK_ASSIGN) {
                pn = pn->pn_left;
                continue;
            }
            pn = pn->lexdef();
        }
        return (Definition *) pn;
    }

    bool isFreeVar() const {
        JS_ASSERT(isDefn());
        return pn_cookie.isFree() || test(PND_GVAR);
    }

    bool isGlobal() const {
        JS_ASSERT(isDefn());
        return test(PND_GVAR);
    }

    enum Kind { VAR, CONST, LET, FUNCTION, ARG, UNKNOWN };

    bool isBindingForm() { return int(kind()) <= int(LET); }

    static const char *kindString(Kind kind);

    Kind kind() {
        if (getKind() == TOK_FUNCTION)
            return FUNCTION;
        JS_ASSERT(getKind() == TOK_NAME);
        if (isOp(JSOP_NOP))
            return UNKNOWN;
        if (isOp(JSOP_GETARG))
            return ARG;
        if (isConst())
            return CONST;
        if (isLet())
            return LET;
        return VAR;
    }
};

class ParseNodeAllocator {
  public:
    explicit ParseNodeAllocator(JSContext *cx) : cx(cx), freelist(NULL) {}

    void *allocNode();
    void freeNode(ParseNode *pn);
    ParseNode *freeTree(ParseNode *pn);
    void prepareNodeForMutation(ParseNode *pn);

  private:
    JSContext *cx;
    ParseNode *freelist;
};

inline bool
ParseNode::test(uintN flag) const
{
    JS_ASSERT(pn_defn || pn_arity == PN_FUNC || pn_arity == PN_NAME);
#ifdef DEBUG
    if ((flag & (PND_ASSIGNED | PND_FUNARG)) && pn_defn && !(pn_dflags & flag)) {
        for (ParseNode *pn = ((Definition *) this)->dn_uses; pn; pn = pn->pn_link) {
            JS_ASSERT(!pn->pn_defn);
            JS_ASSERT(!(pn->pn_dflags & flag));
        }
    }
#endif
    return !!(pn_dflags & flag);
}

inline void
ParseNode::setFunArg()
{
    









    JS_ASSERT(!(pn_defn & pn_used));
    if (pn_used)
        pn_lexdef->pn_dflags |= PND_FUNARG;
    pn_dflags |= PND_FUNARG;
}

inline void
LinkUseToDef(ParseNode *pn, Definition *dn, TreeContext *tc)
{
    JS_ASSERT(!pn->isUsed());
    JS_ASSERT(!pn->isDefn());
    JS_ASSERT(pn != dn->dn_uses);
    pn->pn_link = dn->dn_uses;
    dn->dn_uses = pn;
    dn->pn_dflags |= pn->pn_dflags & PND_USE2DEF_FLAGS;
    pn->setUsed(true);
    pn->pn_lexdef = dn;
}

struct ObjectBox {
    ObjectBox           *traceLink;
    ObjectBox           *emitLink;
    JSObject            *object;
    ObjectBox           *parent;
    uintN               index;
    bool                isFunctionBox;
};

#define JSFB_LEVEL_BITS 14

struct FunctionBox : public ObjectBox
{
    ParseNode           *node;
    FunctionBox         *siblings;
    FunctionBox         *kids;
    FunctionBox         *parent;
    ParseNode           *methods;               




    Bindings            bindings;               
    uint32              queued:1,
                        inLoop:1,               
                        level:JSFB_LEVEL_BITS;
    uint32              tcflags;

    JSFunction *function() const { return (JSFunction *) object; }

    bool joinable() const;

    



    bool inAnyDynamicScope() const;

    



    bool scopeIsExtensible() const;

    









    bool shouldUnbrand(uintN methods, uintN slowMethods) const;
};

struct FunctionBoxQueue {
    FunctionBox         **vector;
    size_t              head, tail;
    size_t              lengthMask;

    size_t count()  { return head - tail; }
    size_t length() { return lengthMask + 1; }

    FunctionBoxQueue()
      : vector(NULL), head(0), tail(0), lengthMask(0) { }

    bool init(uint32 count) {
        lengthMask = JS_BITMASK(JS_CEILING_LOG2W(count));
        vector = (FunctionBox **) OffTheBooks::malloc_(sizeof(FunctionBox) * length());
        return !!vector;
    }

    ~FunctionBoxQueue() { UnwantedForeground::free_(vector); }

    void push(FunctionBox *funbox) {
        if (!funbox->queued) {
            JS_ASSERT(count() < length());
            vector[head++ & lengthMask] = funbox;
            funbox->queued = true;
        }
    }

    FunctionBox *pull() {
        if (tail == head)
            return NULL;
        JS_ASSERT(tail < head);
        FunctionBox *funbox = vector[tail++ & lengthMask];
        funbox->queued = false;
        return funbox;
    }
};

} 

#endif 
