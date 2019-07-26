






#ifndef ParseNode_h__
#define ParseNode_h__

#include "mozilla/Attributes.h"

#include "jsscript.h"

#include "frontend/ParseMaps.h"
#include "frontend/TokenStream.h"

namespace js {
namespace frontend {

struct ParseContext;









class UpvarCookie
{
    uint16_t level_;
    uint16_t slot_;

    void checkInvariants() {
        JS_STATIC_ASSERT(sizeof(UpvarCookie) == sizeof(uint32_t));
    }

  public:
    
    static const uint16_t FREE_LEVEL = 0xffff;

    static const uint16_t CALLEE_SLOT = 0xffff;

    static bool isLevelReserved(uint16_t level) { return level == FREE_LEVEL; }

    bool isFree() const { return level_ == FREE_LEVEL; }
    uint16_t level() const { JS_ASSERT(!isFree()); return level_; }
    uint16_t slot()  const { JS_ASSERT(!isFree()); return slot_; }

    
    bool set(JSContext *cx, unsigned newLevel, uint16_t newSlot);

    void makeFree() {
        level_ = FREE_LEVEL;
        slot_ = 0;      
        JS_ASSERT(isFree());
    }
};

#define FOR_EACH_PARSE_NODE_KIND(F) \
    F(NOP) \
    F(SEMI) \
    F(COMMA) \
    F(CONDITIONAL) \
    F(COLON) \
    F(OR) \
    F(AND) \
    F(BITOR) \
    F(BITXOR) \
    F(BITAND) \
    F(POS) \
    F(NEG) \
    F(ADD) \
    F(SUB) \
    F(STAR) \
    F(DIV) \
    F(MOD) \
    F(PREINCREMENT) \
    F(POSTINCREMENT) \
    F(PREDECREMENT) \
    F(POSTDECREMENT) \
    F(DOT) \
    F(ELEM) \
    F(ARRAY) \
    F(STATEMENTLIST) \
    F(XMLCURLYEXPR) \
    F(OBJECT) \
    F(CALL) \
    F(NAME) \
    F(NUMBER) \
    F(STRING) \
    F(REGEXP) \
    F(TRUE) \
    F(FALSE) \
    F(NULL) \
    F(THIS) \
    F(FUNCTION) \
    F(IF) \
    F(ELSE) \
    F(SWITCH) \
    F(CASE) \
    F(DEFAULT) \
    F(WHILE) \
    F(DOWHILE) \
    F(FOR) \
    F(BREAK) \
    F(CONTINUE) \
    F(IN) \
    F(VAR) \
    F(CONST) \
    F(WITH) \
    F(RETURN) \
    F(NEW) \
    F(DELETE) \
    F(TRY) \
    F(CATCH) \
    F(CATCHLIST) \
    F(FINALLY) \
    F(THROW) \
    F(INSTANCEOF) \
    F(DEBUGGER) \
    F(DEFXMLNS) \
    F(XMLSTAGO) \
    F(XMLETAGO) \
    F(XMLPTAGC) \
    F(XMLTAGC) \
    F(XMLNAME) \
    F(XMLATTR) \
    F(XMLSPACE) \
    F(XMLTEXT) \
    F(XMLCOMMENT) \
    F(XMLCDATA) \
    F(XMLPI) \
    F(XMLUNARY) \
    F(AT) \
    F(FUNCTIONNS) \
    F(DBLCOLON) \
    F(ANYNAME) \
    F(DBLDOT) \
    F(FILTER) \
    F(XMLELEM) \
    F(XMLLIST) \
    F(YIELD) \
    F(ARRAYCOMP) \
    F(ARRAYPUSH) \
    F(LEXICALSCOPE) \
    F(LET) \
    F(SEQ) \
    F(FORIN) \
    F(FORHEAD) \
    F(ARGSBODY) \
    F(SPREAD) \
    \
    /* Equality operators. */ \
    F(STRICTEQ) \
    F(EQ) \
    F(STRICTNE) \
    F(NE) \
    \
    /* Unary operators. */ \
    F(TYPEOF) \
    F(VOID) \
    F(NOT) \
    F(BITNOT) \
    \
    /* Relational operators (< <= > >=). */ \
    F(LT) \
    F(LE) \
    F(GT) \
    F(GE) \
    \
    /* Shift operators (<< >> >>>). */ \
    F(LSH) \
    F(RSH) \
    F(URSH) \
    \
    /* Assignment operators (= += -= etc.). */ \
    /* ParseNode::isAssignment assumes all these are consecutive. */ \
    F(ASSIGN) \
    F(ADDASSIGN) \
    F(SUBASSIGN) \
    F(BITORASSIGN) \
    F(BITXORASSIGN) \
    F(BITANDASSIGN) \
    F(LSHASSIGN) \
    F(RSHASSIGN) \
    F(URSHASSIGN) \
    F(MULASSIGN) \
    F(DIVASSIGN) \
    F(MODASSIGN)











enum ParseNodeKind {
#define EMIT_ENUM(name) PNK_##name,
    FOR_EACH_PARSE_NODE_KIND(EMIT_ENUM)
#undef EMIT_ENUM
    PNK_LIMIT, 
    PNK_ASSIGNMENT_START = PNK_ASSIGN,
    PNK_ASSIGNMENT_LAST = PNK_MODASSIGN
};









































































































































































































































































enum ParseNodeArity {
    PN_NULLARY,                         
    PN_UNARY,                           
    PN_BINARY,                          
    PN_TERNARY,                         
    PN_FUNC,                            
    PN_LIST,                            
    PN_NAME                             
};

struct Definition;

class LoopControlStatement;
class BreakStatement;
class ContinueStatement;
class XMLProcessingInstruction;
class ConditionalExpression;
class PropertyAccess;

struct ParseNode {
  private:
    uint32_t            pn_type   : 16, 
                        pn_op     : 8,  
                        pn_arity  : 5,  
                        pn_parens : 1,  
                        pn_used   : 1,  
                        pn_defn   : 1;  

    ParseNode(const ParseNode &other) MOZ_DELETE;
    void operator=(const ParseNode &other) MOZ_DELETE;

  public:
    ParseNode(ParseNodeKind kind, JSOp op, ParseNodeArity arity)
      : pn_type(kind), pn_op(op), pn_arity(arity), pn_parens(0), pn_used(0), pn_defn(0),
        pn_offset(0), pn_next(NULL), pn_link(NULL)
    {
        JS_ASSERT(kind < PNK_LIMIT);
        pn_pos.begin.index = 0;
        pn_pos.begin.lineno = 0;
        pn_pos.end.index = 0;
        pn_pos.end.lineno = 0;
        memset(&pn_u, 0, sizeof pn_u);
    }

    ParseNode(ParseNodeKind kind, JSOp op, ParseNodeArity arity, const TokenPos &pos)
      : pn_type(kind), pn_op(op), pn_arity(arity), pn_parens(0), pn_used(0), pn_defn(0),
        pn_pos(pos), pn_offset(0), pn_next(NULL), pn_link(NULL)
    {
        JS_ASSERT(kind < PNK_LIMIT);
        memset(&pn_u, 0, sizeof pn_u);
    }

    JSOp getOp() const                     { return JSOp(pn_op); }
    void setOp(JSOp op)                    { pn_op = op; }
    bool isOp(JSOp op) const               { return getOp() == op; }

    ParseNodeKind getKind() const {
        JS_ASSERT(pn_type < PNK_LIMIT);
        return ParseNodeKind(pn_type);
    }
    void setKind(ParseNodeKind kind) {
        JS_ASSERT(kind < PNK_LIMIT);
        pn_type = kind;
    }
    bool isKind(ParseNodeKind kind) const  { return getKind() == kind; }

    ParseNodeArity getArity() const        { return ParseNodeArity(pn_arity); }
    bool isArity(ParseNodeArity a) const   { return getArity() == a; }
    void setArity(ParseNodeArity a)        { pn_arity = a; }

    bool isXMLNameOp() const {
        ParseNodeKind kind = getKind();
        return kind == PNK_ANYNAME || kind == PNK_AT || kind == PNK_DBLCOLON;
    }
    bool isAssignment() const {
        ParseNodeKind kind = getKind();
        return PNK_ASSIGNMENT_START <= kind && kind <= PNK_ASSIGNMENT_LAST;
    }

    bool isXMLPropertyIdentifier() const {
        ParseNodeKind kind = getKind();
        return kind == PNK_ANYNAME || kind == PNK_AT || kind == PNK_DBLCOLON;
    }

    bool isXMLItem() const {
        ParseNodeKind kind = getKind();
        return kind == PNK_XMLCOMMENT || kind == PNK_XMLCDATA || kind == PNK_XMLPI ||
               kind == PNK_XMLELEM || kind == PNK_XMLLIST;
    }

    
    bool isInParens() const                { return pn_parens; }
    void setInParens(bool enabled)         { pn_parens = enabled; }
    bool isUsed() const                    { return pn_used; }
    void setUsed(bool enabled)             { pn_used = enabled; }
    bool isDefn() const                    { return pn_defn; }
    void setDefn(bool enabled)             { pn_defn = enabled; }

    TokenPos            pn_pos;         
    int32_t             pn_offset;      
    ParseNode           *pn_next;       
    ParseNode           *pn_link;       

    union {
        struct {                        
            ParseNode   *head;          
            ParseNode   **tail;         
            uint32_t    count;          
            uint32_t    xflags:12,      
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
            unsigned       iflags;         
        } binary;
        struct {                        
            ParseNode   *kid;
            bool        hidden;         


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


            uint32_t    dflags:12,      
                        blockid:20;     

        } name;
        double        dval;             
        class {
            friend class LoopControlStatement;
            PropertyName     *label;    
        } loopControl;
        class {                         
            friend class XMLProcessingInstruction;
            PropertyName     *target;   
            JSAtom           *data;     
        } xmlpi;
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
#define pn_hidden       pn_u.unary.hidden
#define pn_prologue     pn_u.unary.hidden
#define pn_atom         pn_u.name.atom
#define pn_objbox       pn_u.name.objbox
#define pn_expr         pn_u.name.expr
#define pn_lexdef       pn_u.name.lexdef
#define pn_dval         pn_u.dval

  protected:
    void init(TokenKind type, JSOp op, ParseNodeArity arity) {
        pn_type = type;
        pn_op = op;
        pn_arity = arity;
        pn_parens = false;
        JS_ASSERT(!pn_used);
        JS_ASSERT(!pn_defn);
        pn_next = pn_link = NULL;
    }

    static ParseNode *create(ParseNodeKind kind, ParseNodeArity arity, Parser *parser);

  public:
    



    static ParseNode *
    append(ParseNodeKind tt, JSOp op, ParseNode *left, ParseNode *right);

    




    static ParseNode *
    newBinaryOrAppend(ParseNodeKind kind, JSOp op, ParseNode *left, ParseNode *right,
                      Parser *parser);

    inline PropertyName *name() const;

    





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

    Definition *resolve();


#define PND_LET                 0x01    /* let (block-scoped) binding */
#define PND_CONST               0x02    /* const binding (orthogonal to let) */
#define PND_ASSIGNED            0x04    /* set if ever LHS of assignment */
#define PND_BLOCKCHILD          0x08    /* use or def is direct block child */
#define PND_PLACEHOLDER         0x10    /* placeholder definition for lexdep */
#define PND_BOUND               0x20    /* bound to a stack or global slot */
#define PND_DEOPTIMIZED         0x40    /* former pn_used name node, pn_lexdef
                                           still valid, but this use no longer
                                           optimizable via an upvar opcode */
#define PND_CLOSED              0x80    /* variable is closed over */
#define PND_DEFAULT            0x100    /* definition is an arg with a default */
#define PND_IMPLICITARGUMENTS  0x200    /* the definition is a placeholder for
                                           'arguments' that has been converted
                                           into a definition after the function
                                           body has been parsed. */


#define PND_USE2DEF_FLAGS (PND_ASSIGNED | PND_CLOSED)


#define PNX_STRCAT      0x01            /* PNK_ADD list has string term */
#define PNX_CANTFOLD    0x02            /* PNK_ADD list has unfoldable term */
#define PNX_POPVAR      0x04            /* PNK_VAR or PNK_CONST last result
                                           needs popping */
#define PNX_FORINVAR    0x08            /* PNK_VAR is left kid of PNK_FORIN node
                                           which is left kid of PNK_FOR */
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

    unsigned frameLevel() const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return pn_cookie.level();
    }

    unsigned frameSlot() const {
        JS_ASSERT(pn_arity == PN_FUNC || pn_arity == PN_NAME);
        return pn_cookie.slot();
    }

    bool functionIsHoisted() const {
        JS_ASSERT(pn_arity == PN_FUNC);
        JS_ASSERT(isOp(JSOP_LAMBDA) ||    
                  isOp(JSOP_DEFFUN) ||    
                  isOp(JSOP_NOP) ||       
                  isOp(JSOP_GETLOCAL) ||  
                  isOp(JSOP_GETARG));     
        return !(isOp(JSOP_LAMBDA) || isOp(JSOP_DEFFUN));
    }

    














    bool isStringExprStatement() const {
        if (getKind() == PNK_SEMI) {
            JS_ASSERT(pn_arity == PN_UNARY);
            ParseNode *kid = pn_kid;
            return kid && kid->getKind() == PNK_STRING && !kid->pn_parens;
        }
        return false;
    }

    




    bool isEscapeFreeStringLiteral() const {
        JS_ASSERT(isKind(PNK_STRING) && !pn_parens);

        




        JSString *str = pn_atom;
        return (pn_pos.begin.lineno == pn_pos.end.lineno &&
                pn_pos.begin.index + str->length() + 2 == pn_pos.end.index);
    }

    inline bool test(unsigned flag) const;

    bool isLet() const          { return test(PND_LET); }
    bool isConst() const        { return test(PND_CONST); }
    bool isBlockChild() const   { return test(PND_BLOCKCHILD); }
    bool isPlaceholder() const  { return test(PND_PLACEHOLDER); }
    bool isDeoptimized() const  { return test(PND_DEOPTIMIZED); }
    bool isAssigned() const     { return test(PND_ASSIGNED); }
    bool isClosed() const       { return test(PND_CLOSED); }
    bool isBound() const        { return test(PND_BOUND); }
    bool isImplicitArguments() const { return test(PND_IMPLICITARGUMENTS); }

    void become(ParseNode *pn2);
    void clear();

    
    bool isLiteral() const {
        return isKind(PNK_NUMBER) ||
               isKind(PNK_STRING) ||
               isKind(PNK_TRUE) ||
               isKind(PNK_FALSE) ||
               isKind(PNK_NULL);
    }

    
    bool isDirectivePrologueMember() const { return pn_prologue; }

#ifdef JS_HAS_DESTRUCTURING
    
    bool isArrayHole() const { return isKind(PNK_COMMA) && isArity(PN_NULLARY); }
#endif

#ifdef JS_HAS_GENERATOR_EXPRS
    


    bool isGeneratorExpr() const {
        if (isKind(PNK_CALL)) {
            ParseNode *callee = this->pn_head;
            if (callee->getKind() == PNK_FUNCTION && callee->pn_body->getKind() == PNK_LEXICALSCOPE)
                return true;
        }
        return false;
    }

    ParseNode *generatorExpr() const {
        JS_ASSERT(isGeneratorExpr());
        ParseNode *callee = this->pn_head;
        ParseNode *body = callee->pn_body;
        JS_ASSERT(body->isKind(PNK_LEXICALSCOPE));
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
        if (pn->pn_pos.begin < pn_pos.begin)
            pn_pos.begin = pn->pn_pos.begin;
        pn_pos.end = pn->pn_pos.end;
        pn_head = pn;
        pn_tail = &pn->pn_next;
        pn_count = 1;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void append(ParseNode *pn) {
        JS_ASSERT(pn_arity == PN_LIST);
        JS_ASSERT(pn->pn_pos.begin >= pn_pos.begin);
        pn_pos.end = pn->pn_pos.end;
        *pn_tail = pn;
        pn_tail = &pn->pn_next;
        pn_count++;
    }

    void checkListConsistency()
#ifndef DEBUG
    {}
#endif
    ;

    bool getConstantValue(JSContext *cx, bool strictChecks, Value *vp);
    inline bool isConstant();

    
    template <class NodeType>
    inline NodeType &as() {
        JS_ASSERT(NodeType::test(*this));
        return *static_cast<NodeType *>(this);
    }

    template <class NodeType>
    inline const NodeType &as() const {
        JS_ASSERT(NodeType::test(*this));
        return *static_cast<const NodeType *>(this);
    }

#ifdef DEBUG
    void dump();
    void dump(int indent);
#endif
};

struct NullaryNode : public ParseNode {
    static inline NullaryNode *create(ParseNodeKind kind, Parser *parser) {
        return (NullaryNode *) ParseNode::create(kind, PN_NULLARY, parser);
    }

    static bool test(const ParseNode &node) {
        return node.isArity(PN_NULLARY);
    }

#ifdef DEBUG
    void dump();
#endif
};

struct UnaryNode : public ParseNode {
    UnaryNode(ParseNodeKind kind, JSOp op, const TokenPos &pos, ParseNode *kid)
      : ParseNode(kind, op, PN_UNARY, pos)
    {
        pn_kid = kid;
    }

    static inline UnaryNode *create(ParseNodeKind kind, Parser *parser) {
        return (UnaryNode *) ParseNode::create(kind, PN_UNARY, parser);
    }

    static bool test(const ParseNode &node) {
        return node.isArity(PN_UNARY);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct BinaryNode : public ParseNode {
    BinaryNode(ParseNodeKind kind, JSOp op, const TokenPos &pos, ParseNode *left, ParseNode *right)
      : ParseNode(kind, op, PN_BINARY, pos)
    {
        pn_left = left;
        pn_right = right;
    }

    BinaryNode(ParseNodeKind kind, JSOp op, ParseNode *left, ParseNode *right)
      : ParseNode(kind, op, PN_BINARY, TokenPos::box(left->pn_pos, right->pn_pos))
    {
        pn_left = left;
        pn_right = right;
    }

    static inline BinaryNode *create(ParseNodeKind kind, Parser *parser) {
        return (BinaryNode *) ParseNode::create(kind, PN_BINARY, parser);
    }

    static bool test(const ParseNode &node) {
        return node.isArity(PN_BINARY);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct TernaryNode : public ParseNode {
    TernaryNode(ParseNodeKind kind, JSOp op, ParseNode *kid1, ParseNode *kid2, ParseNode *kid3)
      : ParseNode(kind, op, PN_TERNARY,
                  TokenPos::make((kid1 ? kid1 : kid2 ? kid2 : kid3)->pn_pos.begin,
                                 (kid3 ? kid3 : kid2 ? kid2 : kid1)->pn_pos.end))
    {
        pn_kid1 = kid1;
        pn_kid2 = kid2;
        pn_kid3 = kid3;
    }

    static inline TernaryNode *create(ParseNodeKind kind, Parser *parser) {
        return (TernaryNode *) ParseNode::create(kind, PN_TERNARY, parser);
    }

    static bool test(const ParseNode &node) {
        return node.isArity(PN_TERNARY);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct ListNode : public ParseNode {
    static inline ListNode *create(ParseNodeKind kind, Parser *parser) {
        return (ListNode *) ParseNode::create(kind, PN_LIST, parser);
    }

    static bool test(const ParseNode &node) {
        return node.isArity(PN_LIST);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct FunctionNode : public ParseNode {
    static inline FunctionNode *create(ParseNodeKind kind, Parser *parser) {
        return (FunctionNode *) ParseNode::create(kind, PN_FUNC, parser);
    }

    static bool test(const ParseNode &node) {
        return node.isArity(PN_FUNC);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct NameNode : public ParseNode {
    static NameNode *create(ParseNodeKind kind, JSAtom *atom, Parser *parser, ParseContext *pc);

    inline void initCommon(ParseContext *pc);

    static bool test(const ParseNode &node) {
        return node.isArity(PN_NAME);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct LexicalScopeNode : public ParseNode {
    static inline LexicalScopeNode *create(ParseNodeKind kind, Parser *parser) {
        return (LexicalScopeNode *) ParseNode::create(kind, PN_NAME, parser);
    }
};

class LoopControlStatement : public ParseNode {
  protected:
    LoopControlStatement(ParseNodeKind kind, PropertyName *label,
                         const TokenPtr &begin, const TokenPtr &end)
      : ParseNode(kind, JSOP_NOP, PN_NULLARY, TokenPos::make(begin, end))
    {
        JS_ASSERT(kind == PNK_BREAK || kind == PNK_CONTINUE);
        pn_u.loopControl.label = label;
    }

  public:
    
    PropertyName *label() const {
        return pn_u.loopControl.label;
    }

    static bool test(const ParseNode &node) {
        bool match = node.isKind(PNK_BREAK) || node.isKind(PNK_CONTINUE);
        JS_ASSERT_IF(match, node.isArity(PN_NULLARY));
        JS_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class BreakStatement : public LoopControlStatement {
  public:
    BreakStatement(PropertyName *label, const TokenPtr &begin, const TokenPtr &end)
      : LoopControlStatement(PNK_BREAK, label, begin, end)
    { }

    static bool test(const ParseNode &node) {
        bool match = node.isKind(PNK_BREAK);
        JS_ASSERT_IF(match, node.isArity(PN_NULLARY));
        JS_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class ContinueStatement : public LoopControlStatement {
  public:
    ContinueStatement(PropertyName *label, TokenPtr &begin, TokenPtr &end)
      : LoopControlStatement(PNK_CONTINUE, label, begin, end)
    { }

    static bool test(const ParseNode &node) {
        bool match = node.isKind(PNK_CONTINUE);
        JS_ASSERT_IF(match, node.isArity(PN_NULLARY));
        JS_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class DebuggerStatement : public ParseNode {
  public:
    DebuggerStatement(const TokenPos &pos)
      : ParseNode(PNK_DEBUGGER, JSOP_NOP, PN_NULLARY, pos)
    { }
};

#if JS_HAS_XML_SUPPORT
class XMLProcessingInstruction : public ParseNode {
  public:
    XMLProcessingInstruction(PropertyName *target, JSAtom *data, const TokenPos &pos)
      : ParseNode(PNK_XMLPI, JSOP_NOP, PN_NULLARY, pos)
    {
        pn_u.xmlpi.target = target;
        pn_u.xmlpi.data = data;
    }

    static bool test(const ParseNode &node) {
        bool match = node.isKind(PNK_XMLPI);
        JS_ASSERT_IF(match, node.isArity(PN_NULLARY));
        JS_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }

    PropertyName *target() const {
        return pn_u.xmlpi.target;
    }

    JSAtom *data() const {
        return pn_u.xmlpi.data;
    }
};
#endif

class ConditionalExpression : public ParseNode {
  public:
    ConditionalExpression(ParseNode *condition, ParseNode *thenExpr, ParseNode *elseExpr)
      : ParseNode(PNK_CONDITIONAL, JSOP_NOP, PN_TERNARY,
                  TokenPos::make(condition->pn_pos.begin, elseExpr->pn_pos.end))
    {
        JS_ASSERT(condition);
        JS_ASSERT(thenExpr);
        JS_ASSERT(elseExpr);
        pn_u.ternary.kid1 = condition;
        pn_u.ternary.kid2 = thenExpr;
        pn_u.ternary.kid3 = elseExpr;
    }

    ParseNode &condition() const {
        return *pn_u.ternary.kid1;
    }

    ParseNode &thenExpression() const {
        return *pn_u.ternary.kid2;
    }

    ParseNode &elseExpression() const {
        return *pn_u.ternary.kid3;
    }

    static bool test(const ParseNode &node) {
        bool match = node.isKind(PNK_CONDITIONAL);
        JS_ASSERT_IF(match, node.isArity(PN_TERNARY));
        JS_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class ThisLiteral : public ParseNode {
  public:
    ThisLiteral(const TokenPos &pos) : ParseNode(PNK_THIS, JSOP_THIS, PN_NULLARY, pos) { }
};

class NullLiteral : public ParseNode {
  public:
    NullLiteral(const TokenPos &pos) : ParseNode(PNK_NULL, JSOP_NULL, PN_NULLARY, pos) { }
};

class BooleanLiteral : public ParseNode {
  public:
    BooleanLiteral(bool b, const TokenPos &pos)
      : ParseNode(b ? PNK_TRUE : PNK_FALSE, b ? JSOP_TRUE : JSOP_FALSE, PN_NULLARY, pos)
    { }
};

class XMLDoubleColonProperty : public ParseNode {
  public:
    XMLDoubleColonProperty(ParseNode *lhs, ParseNode *rhs,
                           const TokenPtr &begin, const TokenPtr &end)
      : ParseNode(PNK_ELEM, JSOP_GETELEM, PN_BINARY, TokenPos::make(begin, end))
    {
        JS_ASSERT(rhs->isKind(PNK_DBLCOLON));
        pn_u.binary.left = lhs;
        pn_u.binary.right = rhs;
    }

    ParseNode &left() const {
        return *pn_u.binary.left;
    }

    ParseNode &right() const {
        return *pn_u.binary.right;
    }
};

class XMLFilterExpression : public ParseNode {
  public:
    XMLFilterExpression(ParseNode *lhs, ParseNode *filterExpr,
                        const TokenPtr &begin, const TokenPtr &end)
      : ParseNode(PNK_FILTER, JSOP_FILTER, PN_BINARY, TokenPos::make(begin, end))
    {
        pn_u.binary.left = lhs;
        pn_u.binary.right = filterExpr;
    }

    ParseNode &left() const {
        return *pn_u.binary.left;
    }

    ParseNode &filter() const {
        return *pn_u.binary.right;
    }
};

class XMLProperty : public ParseNode {
  public:
    XMLProperty(ParseNode *lhs, ParseNode *propertyId,
                const TokenPtr &begin, const TokenPtr &end)
      : ParseNode(PNK_ELEM, JSOP_GETELEM, PN_BINARY, TokenPos::make(begin, end))
    {
        pn_u.binary.left = lhs;
        pn_u.binary.right = propertyId;
    }

    ParseNode &left() const {
        return *pn_u.binary.left;
    }

    ParseNode &right() const {
        return *pn_u.binary.right;
    }
};

class PropertyAccess : public ParseNode {
  public:
    PropertyAccess(ParseNode *lhs, PropertyName *name,
                   const TokenPtr &begin, const TokenPtr &end)
      : ParseNode(PNK_DOT, JSOP_GETPROP, PN_NAME, TokenPos::make(begin, end))
    {
        JS_ASSERT(lhs != NULL);
        JS_ASSERT(name != NULL);
        pn_u.name.expr = lhs;
        pn_u.name.atom = name;
    }

    static bool test(const ParseNode &node) {
        bool match = node.isKind(PNK_DOT);
        JS_ASSERT_IF(match, node.isArity(PN_NAME));
        return match;
    }

    ParseNode &expression() const {
        return *pn_u.name.expr;
    }

    PropertyName &name() const {
        return *pn_u.name.atom->asPropertyName();
    }
};

class PropertyByValue : public ParseNode {
  public:
    PropertyByValue(ParseNode *lhs, ParseNode *propExpr,
                    const TokenPtr &begin, const TokenPtr &end)
      : ParseNode(PNK_ELEM, JSOP_GETELEM, PN_BINARY, TokenPos::make(begin, end))
    {
        pn_u.binary.left = lhs;
        pn_u.binary.right = propExpr;
    }
};

ParseNode *
CloneLeftHandSide(ParseNode *opn, Parser *parser);

#ifdef DEBUG
void DumpParseTree(ParseNode *pn, int indent = 0);
#endif



































































































#define dn_uses         pn_link

struct Definition : public ParseNode
{
    bool isFreeVar() const {
        JS_ASSERT(isDefn());
        return pn_cookie.isFree();
    }

    enum Kind { VAR, CONST, LET, ARG, NAMED_LAMBDA, PLACEHOLDER };

    bool canHaveInitializer() { return int(kind()) <= int(ARG); }

    static const char *kindString(Kind kind);

    Kind kind() {
        if (getKind() == PNK_FUNCTION) {
            if (isOp(JSOP_GETARG))
                return ARG;
            return VAR;
        }
        JS_ASSERT(getKind() == PNK_NAME);
        if (isOp(JSOP_CALLEE))
            return NAMED_LAMBDA;
        if (isPlaceholder())
            return PLACEHOLDER;
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
ParseNode::test(unsigned flag) const
{
    JS_ASSERT(pn_defn || pn_arity == PN_FUNC || pn_arity == PN_NAME);
#ifdef DEBUG
    if ((flag & PND_ASSIGNED) && pn_defn && !(pn_dflags & flag)) {
        for (ParseNode *pn = ((Definition *) this)->dn_uses; pn; pn = pn->pn_link) {
            JS_ASSERT(!pn->pn_defn);
            JS_ASSERT(!(pn->pn_dflags & flag));
        }
    }
#endif
    return !!(pn_dflags & flag);
}

inline Definition *
ParseNode::resolve()
{
    if (isDefn())
        return (Definition *)this;
    JS_ASSERT(lexdef()->isDefn());
    return (Definition *)lexdef();
}

inline void
LinkUseToDef(ParseNode *pn, Definition *dn)
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

class ObjectBox {
  public:
    JSObject *object;

    ObjectBox(JSObject *object, ObjectBox *traceLink);
    bool isFunctionBox() { return object->isFunction(); }
    FunctionBox *asFunctionBox() { JS_ASSERT(isFunctionBox()); return (FunctionBox *)(this); }
    void trace(JSTracer *trc);

  protected:
    friend struct CGObjectList;

    ObjectBox *traceLink;
    ObjectBox *emitLink;

    ObjectBox(JSFunction *function, ObjectBox *traceLink);
};

} 
} 

#endif 
