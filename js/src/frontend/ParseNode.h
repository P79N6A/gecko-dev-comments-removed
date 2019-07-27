





#ifndef frontend_ParseNode_h
#define frontend_ParseNode_h

#include "mozilla/Attributes.h"

#include "frontend/TokenStream.h"

namespace js {
namespace frontend {

template <typename ParseHandler>
struct ParseContext;

class FullParseHandler;
class FunctionBox;
class ObjectBox;









class UpvarCookie
{
    uint32_t level_ : SCOPECOORD_HOPS_BITS;
    uint32_t slot_ : SCOPECOORD_SLOT_BITS;

    void checkInvariants() {
        static_assert(sizeof(UpvarCookie) == sizeof(uint32_t),
                      "Not necessary for correctness, but good for ParseNode memory use");
    }

  public:
    
    static const uint32_t FREE_LEVEL = SCOPECOORD_HOPS_LIMIT - 1;
    bool isFree() const { return level_ == FREE_LEVEL; }

    uint32_t level() const { MOZ_ASSERT(!isFree()); return level_; }
    uint32_t slot()  const { MOZ_ASSERT(!isFree()); return slot_; }

    
    bool set(TokenStream& ts, unsigned newLevel, uint32_t newSlot) {
        if (newLevel >= FREE_LEVEL)
            return ts.reportError(JSMSG_TOO_DEEP, js_function_str);

        if (newSlot >= SCOPECOORD_SLOT_LIMIT)
            return ts.reportError(JSMSG_TOO_MANY_LOCALS);

        level_ = newLevel;
        slot_ = newSlot;
        return true;
    }

    void makeFree() {
        level_ = FREE_LEVEL;
        slot_ = 0;      
        MOZ_ASSERT(isFree());
    }
};

#define FOR_EACH_PARSE_NODE_KIND(F) \
    F(NOP) \
    F(SEMI) \
    F(COMMA) \
    F(CONDITIONAL) \
    F(COLON) \
    F(SHORTHAND) \
    F(POS) \
    F(NEG) \
    F(PREINCREMENT) \
    F(POSTINCREMENT) \
    F(PREDECREMENT) \
    F(POSTDECREMENT) \
    F(DOT) \
    F(ELEM) \
    F(ARRAY) \
    F(ELISION) \
    F(STATEMENTLIST) \
    F(LABEL) \
    F(OBJECT) \
    F(CALL) \
    F(NAME) \
    F(OBJECT_PROPERTY_NAME) \
    F(COMPUTED_NAME) \
    F(NUMBER) \
    F(STRING) \
    F(TEMPLATE_STRING_LIST) \
    F(TEMPLATE_STRING) \
    F(TAGGED_TEMPLATE) \
    F(CALLSITEOBJ) \
    F(REGEXP) \
    F(TRUE) \
    F(FALSE) \
    F(NULL) \
    F(THIS) \
    F(FUNCTION) \
    F(IF) \
    F(SWITCH) \
    F(CASE) \
    F(DEFAULT) \
    F(WHILE) \
    F(DOWHILE) \
    F(FOR) \
    F(BREAK) \
    F(CONTINUE) \
    F(VAR) \
    F(CONST) \
    F(GLOBALCONST) \
    F(WITH) \
    F(RETURN) \
    F(NEW) \
    F(DELETE) \
    F(TRY) \
    F(CATCH) \
    F(CATCHLIST) \
    F(THROW) \
    F(DEBUGGER) \
    F(GENERATOR) \
    F(YIELD) \
    F(YIELD_STAR) \
    F(GENEXP) \
    F(ARRAYCOMP) \
    F(ARRAYPUSH) \
    F(LEXICALSCOPE) \
    F(LET) \
    F(LETBLOCK) \
    F(LETEXPR) \
    F(IMPORT) \
    F(IMPORT_SPEC_LIST) \
    F(IMPORT_SPEC) \
    F(EXPORT) \
    F(EXPORT_FROM) \
    F(EXPORT_SPEC_LIST) \
    F(EXPORT_SPEC) \
    F(EXPORT_BATCH_SPEC) \
    F(SEQ) \
    F(FORIN) \
    F(FOROF) \
    F(FORHEAD) \
    F(FRESHENBLOCK) \
    F(ARGSBODY) \
    F(SPREAD) \
    F(MUTATEPROTO) \
    F(CLASS) \
    F(CLASSMETHOD) \
    F(CLASSMETHODLIST) \
    F(CLASSNAMES) \
    \
    /* Unary operators. */ \
    F(TYPEOF) \
    F(VOID) \
    F(NOT) \
    F(BITNOT) \
    \
    /* \
     * Binary operators. \
     * These must be in the same order as TOK_OR and friends in TokenStream.h. \
     */ \
    F(OR) \
    F(AND) \
    F(BITOR) \
    F(BITXOR) \
    F(BITAND) \
    F(STRICTEQ) \
    F(EQ) \
    F(STRICTNE) \
    F(NE) \
    F(LT) \
    F(LE) \
    F(GT) \
    F(GE) \
    F(INSTANCEOF) \
    F(IN) \
    F(LSH) \
    F(RSH) \
    F(URSH) \
    F(ADD) \
    F(SUB) \
    F(STAR) \
    F(DIV) \
    F(MOD) \
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











enum ParseNodeKind
{
#define EMIT_ENUM(name) PNK_##name,
    FOR_EACH_PARSE_NODE_KIND(EMIT_ENUM)
#undef EMIT_ENUM
    PNK_LIMIT, 
    PNK_BINOP_FIRST = PNK_OR,
    PNK_BINOP_LAST = PNK_MOD,
    PNK_ASSIGNMENT_START = PNK_ASSIGN,
    PNK_ASSIGNMENT_LAST = PNK_MODASSIGN
};


























































































































































































































enum ParseNodeArity
{
    PN_NULLARY,                         
    PN_UNARY,                           
    PN_BINARY,                          
    PN_BINARY_OBJ,                      
    PN_TERNARY,                         
    PN_CODE,                            
    PN_LIST,                            
    PN_NAME                             
};

struct Definition;

class LabeledStatement;
class LoopControlStatement;
class BreakStatement;
class ContinueStatement;
class ConditionalExpression;
class PropertyAccess;

class ParseNode
{
    uint32_t            pn_type   : 16, 
                        pn_op     : 8,  
                        pn_arity  : 5,  
                        pn_parens : 1,  
                        pn_used   : 1,  
                        pn_defn   : 1;  

    ParseNode(const ParseNode& other) = delete;
    void operator=(const ParseNode& other) = delete;

  public:
    ParseNode(ParseNodeKind kind, JSOp op, ParseNodeArity arity)
      : pn_type(kind), pn_op(op), pn_arity(arity), pn_parens(0), pn_used(0), pn_defn(0),
        pn_pos(0, 0), pn_offset(0), pn_next(nullptr), pn_link(nullptr)
    {
        MOZ_ASSERT(kind < PNK_LIMIT);
        memset(&pn_u, 0, sizeof pn_u);
    }

    ParseNode(ParseNodeKind kind, JSOp op, ParseNodeArity arity, const TokenPos& pos)
      : pn_type(kind), pn_op(op), pn_arity(arity), pn_parens(0), pn_used(0), pn_defn(0),
        pn_pos(pos), pn_offset(0), pn_next(nullptr), pn_link(nullptr)
    {
        MOZ_ASSERT(kind < PNK_LIMIT);
        memset(&pn_u, 0, sizeof pn_u);
    }

    JSOp getOp() const                     { return JSOp(pn_op); }
    void setOp(JSOp op)                    { pn_op = op; }
    bool isOp(JSOp op) const               { return getOp() == op; }

    ParseNodeKind getKind() const {
        MOZ_ASSERT(pn_type < PNK_LIMIT);
        return ParseNodeKind(pn_type);
    }
    void setKind(ParseNodeKind kind) {
        MOZ_ASSERT(kind < PNK_LIMIT);
        pn_type = kind;
    }
    bool isKind(ParseNodeKind kind) const  { return getKind() == kind; }

    ParseNodeArity getArity() const        { return ParseNodeArity(pn_arity); }
    bool isArity(ParseNodeArity a) const   { return getArity() == a; }
    void setArity(ParseNodeArity a)        { pn_arity = a; }

    bool isAssignment() const {
        ParseNodeKind kind = getKind();
        return PNK_ASSIGNMENT_START <= kind && kind <= PNK_ASSIGNMENT_LAST;
    }

    bool isBinaryOperation() const {
        ParseNodeKind kind = getKind();
        return PNK_BINOP_FIRST <= kind && kind <= PNK_BINOP_LAST;
    }

    
    bool isInParens() const                { return pn_parens; }
    bool isLikelyIIFE() const              { return isInParens(); }
    void setInParens(bool enabled)         { pn_parens = enabled; }
    bool isUsed() const                    { return pn_used; }
    void setUsed(bool enabled)             { pn_used = enabled; }
    bool isDefn() const                    { return pn_defn; }
    void setDefn(bool enabled)             { pn_defn = enabled; }

    static const unsigned NumDefinitionFlagBits = 10;
    static const unsigned NumListFlagBits = 10;
    static const unsigned NumBlockIdBits = 22;
    static_assert(NumDefinitionFlagBits == NumListFlagBits,
                  "Assumed below to achieve consistent blockid offset");
    static_assert(NumDefinitionFlagBits + NumBlockIdBits <= 32,
                  "This is supposed to fit in a single uint32_t");

    TokenPos            pn_pos;         
    int32_t             pn_offset;      
    ParseNode*          pn_next;       

    









    union {
        ParseNode*      dn_uses;
        ParseNode*      pn_link;
    };

    union {
        struct {                        
            ParseNode*  head;          
            ParseNode** tail;         
            uint32_t    count;          
            uint32_t    xflags:NumListFlagBits, 
                        blockid:NumBlockIdBits; 
        } list;
        struct {                        
            ParseNode*  kid1;          
            ParseNode*  kid2;          
            ParseNode*  kid3;          
        } ternary;
        struct {                        
            ParseNode*  left;
            ParseNode*  right;
            union {
                unsigned iflags;        
                ObjectBox* objbox;      
                bool isStatic;          
            };
        } binary;
        struct {                        
            ParseNode*  kid;
            bool        prologue;       

        } unary;
        struct {                        
            union {
                JSAtom*     atom;      
                ObjectBox*  objbox;    
                FunctionBox* funbox;    
            };
            union {
                ParseNode*  expr;      


                Definition* lexdef;    
            };
            UpvarCookie cookie;         


            uint32_t    dflags:NumDefinitionFlagBits, 
                        blockid:NumBlockIdBits;  

        } name;
        struct {
            double      value;          
            DecimalPoint decimalPoint;  
        } number;
        class {
            friend class LoopControlStatement;
            PropertyName*    label;    
        } loopControl;
    } pn_u;

#define pn_modulebox    pn_u.name.modulebox
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
#define pn_binary_obj   pn_u.binary.objbox
#define pn_kid          pn_u.unary.kid
#define pn_prologue     pn_u.unary.prologue
#define pn_atom         pn_u.name.atom
#define pn_objbox       pn_u.name.objbox
#define pn_expr         pn_u.name.expr
#define pn_lexdef       pn_u.name.lexdef
#define pn_dval         pn_u.number.value

  protected:
    void init(TokenKind type, JSOp op, ParseNodeArity arity) {
        pn_type = type;
        pn_op = op;
        pn_arity = arity;
        pn_parens = false;
        MOZ_ASSERT(!pn_used);
        MOZ_ASSERT(!pn_defn);
        pn_next = pn_link = nullptr;
    }

  public:
    



    static ParseNode*
    appendOrCreateList(ParseNodeKind kind, JSOp op, ParseNode* left, ParseNode* right,
                       FullParseHandler* handler, ParseContext<FullParseHandler>* pc);

    inline PropertyName* name() const;
    inline JSAtom* atom() const;

    





    ParseNode* expr() const {
        MOZ_ASSERT(!pn_used);
        MOZ_ASSERT(pn_arity == PN_NAME || pn_arity == PN_CODE);
        return pn_expr;
    }

    Definition* lexdef() const {
        MOZ_ASSERT(pn_used || isDeoptimized());
        MOZ_ASSERT(pn_arity == PN_NAME);
        return pn_lexdef;
    }

    ParseNode* maybeExpr()   { return pn_used ? nullptr : expr(); }
    Definition* maybeLexDef() { return pn_used ? lexdef() : nullptr; }

    Definition* resolve();


#define PND_LEXICAL             0x01    /* lexical (block-scoped) binding or use of a hoisted
                                           let or const */
#define PND_CONST               0x02    /* const binding (orthogonal to let) */
#define PND_ASSIGNED            0x04    /* set if ever LHS of assignment */
#define PND_PLACEHOLDER         0x08    /* placeholder definition for lexdep */
#define PND_BOUND               0x10    /* bound to a stack or global slot */
#define PND_DEOPTIMIZED         0x20    /* former pn_used name node, pn_lexdef
                                           still valid, but this use no longer
                                           optimizable via an upvar opcode */
#define PND_CLOSED              0x40    /* variable is closed over */
#define PND_DEFAULT             0x80    /* definition is an arg with a default */
#define PND_IMPLICITARGUMENTS  0x100    /* the definition is a placeholder for
                                           'arguments' that has been converted
                                           into a definition after the function
                                           body has been parsed. */
#define PND_EMITTEDFUNCTION    0x200    /* hoisted function that was emitted */

    static_assert(PND_EMITTEDFUNCTION < (1 << NumDefinitionFlagBits), "Not enough bits");


#define PND_USE2DEF_FLAGS (PND_ASSIGNED | PND_CLOSED)


#define PNX_POPVAR      0x01            /* PNK_VAR or PNK_CONST last result
                                           needs popping */
#define PNX_GROUPINIT   0x02            /* var [a, b] = [c, d]; unit list */
#define PNX_FUNCDEFS    0x04            /* contains top-level function statements */
#define PNX_SETCALL     0x08            /* call expression in lvalue context */
#define PNX_DESTRUCT    0x10            /* code evaluating destructuring
                                           arguments occurs before function body */
#define PNX_SPECIALARRAYINIT 0x20       /* one or more of
                                           1. array initialiser has holes
                                           2. array initializer has spread node */
#define PNX_NONCONST    0x40            /* initialiser has non-constants */

    static_assert(PNX_NONCONST < (1 << NumListFlagBits), "Not enough bits");

    unsigned frameLevel() const {
        MOZ_ASSERT(pn_arity == PN_CODE || pn_arity == PN_NAME);
        return pn_cookie.level();
    }

    uint32_t frameSlot() const {
        MOZ_ASSERT(pn_arity == PN_CODE || pn_arity == PN_NAME);
        return pn_cookie.slot();
    }

    bool functionIsHoisted() const {
        MOZ_ASSERT(pn_arity == PN_CODE && getKind() == PNK_FUNCTION);
        MOZ_ASSERT(isOp(JSOP_LAMBDA) ||        
                   isOp(JSOP_LAMBDA_ARROW) ||  
                   isOp(JSOP_DEFFUN) ||        
                   isOp(JSOP_NOP) ||           
                   isOp(JSOP_GETLOCAL) ||      
                   isOp(JSOP_GETARG));         
        return !isOp(JSOP_LAMBDA) && !isOp(JSOP_LAMBDA_ARROW) && !isOp(JSOP_DEFFUN);
    }

    












    JSAtom* isStringExprStatement() const {
        if (getKind() == PNK_SEMI) {
            MOZ_ASSERT(pn_arity == PN_UNARY);
            ParseNode* kid = pn_kid;
            if (kid && kid->getKind() == PNK_STRING && !kid->pn_parens)
                return kid->pn_atom;
        }
        return nullptr;
    }

    inline bool test(unsigned flag) const;

    bool isLexical() const      { return test(PND_LEXICAL) && !isUsed(); }
    bool isConst() const        { return test(PND_CONST); }
    bool isPlaceholder() const  { return test(PND_PLACEHOLDER); }
    bool isDeoptimized() const  { return test(PND_DEOPTIMIZED); }
    bool isAssigned() const     { return test(PND_ASSIGNED); }
    bool isClosed() const       { return test(PND_CLOSED); }
    bool isBound() const        { return test(PND_BOUND); }
    bool isImplicitArguments() const { return test(PND_IMPLICITARGUMENTS); }
    bool isHoistedLexicalUse() const { return test(PND_LEXICAL) && isUsed(); }

    
    bool isLiteral() const {
        return isKind(PNK_NUMBER) ||
               isKind(PNK_STRING) ||
               isKind(PNK_TRUE) ||
               isKind(PNK_FALSE) ||
               isKind(PNK_NULL);
    }

    
    bool isDirectivePrologueMember() const { return pn_prologue; }

#ifdef JS_HAS_GENERATOR_EXPRS
    ParseNode* generatorExpr() const {
        MOZ_ASSERT(isKind(PNK_GENEXP));
        ParseNode* callee = this->pn_head;
        ParseNode* body = callee->pn_body;
        MOZ_ASSERT(body->isKind(PNK_STATEMENTLIST));
        MOZ_ASSERT(body->last()->isKind(PNK_LEXICALSCOPE) || body->last()->isKind(PNK_FOR));
        return body->last();
    }
#endif

    inline void markAsAssigned();

    



    ParseNode* last() const {
        MOZ_ASSERT(pn_arity == PN_LIST);
        MOZ_ASSERT(pn_count != 0);
        return (ParseNode*)(uintptr_t(pn_tail) - offsetof(ParseNode, pn_next));
    }

    void initNumber(double value, DecimalPoint decimalPoint) {
        MOZ_ASSERT(pn_arity == PN_NULLARY);
        MOZ_ASSERT(getKind() == PNK_NUMBER);
        pn_u.number.value = value;
        pn_u.number.decimalPoint = decimalPoint;
    }

    void makeEmpty() {
        MOZ_ASSERT(pn_arity == PN_LIST);
        pn_head = nullptr;
        pn_tail = &pn_head;
        pn_count = 0;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void initList(ParseNode* pn) {
        MOZ_ASSERT(pn_arity == PN_LIST);
        if (pn->pn_pos.begin < pn_pos.begin)
            pn_pos.begin = pn->pn_pos.begin;
        pn_pos.end = pn->pn_pos.end;
        pn_head = pn;
        pn_tail = &pn->pn_next;
        pn_count = 1;
        pn_xflags = 0;
        pn_blockid = 0;
    }

    void append(ParseNode* pn) {
        MOZ_ASSERT(pn_arity == PN_LIST);
        MOZ_ASSERT(pn->pn_pos.begin >= pn_pos.begin);
        pn_pos.end = pn->pn_pos.end;
        *pn_tail = pn;
        pn_tail = &pn->pn_next;
        pn_count++;
    }

    void prepend(ParseNode* pn) {
        MOZ_ASSERT(pn_arity == PN_LIST);
        pn->pn_next = pn_head;
        pn_head = pn;
        if (pn_tail == &pn_head)
            pn_tail = &pn->pn_next;
        pn_count++;
    }

    void checkListConsistency()
#ifndef DEBUG
    {}
#endif
    ;

    enum AllowConstantObjects {
        DontAllowObjects = 0,
        DontAllowNestedObjects,
        AllowObjects
    };

    bool getConstantValue(ExclusiveContext* cx, AllowConstantObjects allowObjects, MutableHandleValue vp,
                          NewObjectKind newKind = TenuredObject);
    inline bool isConstant();

    template <class NodeType>
    inline bool is() const {
        return NodeType::test(*this);
    }

    
    template <class NodeType>
    inline NodeType& as() {
        MOZ_ASSERT(NodeType::test(*this));
        return *static_cast<NodeType*>(this);
    }

    template <class NodeType>
    inline const NodeType& as() const {
        MOZ_ASSERT(NodeType::test(*this));
        return *static_cast<const NodeType*>(this);
    }

#ifdef DEBUG
    void dump();
    void dump(int indent);
#endif
};

struct NullaryNode : public ParseNode
{
    NullaryNode(ParseNodeKind kind, const TokenPos& pos)
      : ParseNode(kind, JSOP_NOP, PN_NULLARY, pos) {}
    NullaryNode(ParseNodeKind kind, JSOp op, const TokenPos& pos)
      : ParseNode(kind, op, PN_NULLARY, pos) {}

    
    
    
    NullaryNode(ParseNodeKind kind, JSOp op, const TokenPos& pos, JSAtom* atom)
      : ParseNode(kind, op, PN_NULLARY, pos)
    {
        pn_atom = atom;
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_NULLARY);
    }

#ifdef DEBUG
    void dump();
#endif
};

struct UnaryNode : public ParseNode
{
    UnaryNode(ParseNodeKind kind, JSOp op, const TokenPos& pos, ParseNode* kid)
      : ParseNode(kind, op, PN_UNARY, pos)
    {
        pn_kid = kid;
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_UNARY);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct BinaryNode : public ParseNode
{
    BinaryNode(ParseNodeKind kind, JSOp op, const TokenPos& pos, ParseNode* left, ParseNode* right)
      : ParseNode(kind, op, PN_BINARY, pos)
    {
        pn_left = left;
        pn_right = right;
    }

    BinaryNode(ParseNodeKind kind, JSOp op, ParseNode* left, ParseNode* right)
      : ParseNode(kind, op, PN_BINARY, TokenPos::box(left->pn_pos, right->pn_pos))
    {
        pn_left = left;
        pn_right = right;
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_BINARY);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct BinaryObjNode : public ParseNode
{
    BinaryObjNode(ParseNodeKind kind, JSOp op, const TokenPos& pos, ParseNode* left, ParseNode* right,
                  ObjectBox* objbox)
      : ParseNode(kind, op, PN_BINARY_OBJ, pos)
    {
        pn_left = left;
        pn_right = right;
        pn_binary_obj = objbox;
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_BINARY_OBJ);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct TernaryNode : public ParseNode
{
    TernaryNode(ParseNodeKind kind, JSOp op, ParseNode* kid1, ParseNode* kid2, ParseNode* kid3)
      : ParseNode(kind, op, PN_TERNARY,
                  TokenPos((kid1 ? kid1 : kid2 ? kid2 : kid3)->pn_pos.begin,
                           (kid3 ? kid3 : kid2 ? kid2 : kid1)->pn_pos.end))
    {
        pn_kid1 = kid1;
        pn_kid2 = kid2;
        pn_kid3 = kid3;
    }

    TernaryNode(ParseNodeKind kind, JSOp op, ParseNode* kid1, ParseNode* kid2, ParseNode* kid3,
                const TokenPos& pos)
      : ParseNode(kind, op, PN_TERNARY, pos)
    {
        pn_kid1 = kid1;
        pn_kid2 = kid2;
        pn_kid3 = kid3;
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_TERNARY);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct ListNode : public ParseNode
{
    ListNode(ParseNodeKind kind, const TokenPos& pos)
      : ParseNode(kind, JSOP_NOP, PN_LIST, pos)
    {
        makeEmpty();
    }

    ListNode(ParseNodeKind kind, JSOp op, const TokenPos& pos)
      : ParseNode(kind, op, PN_LIST, pos)
    {
        makeEmpty();
    }

    ListNode(ParseNodeKind kind, JSOp op, ParseNode* kid)
      : ParseNode(kind, op, PN_LIST, kid->pn_pos)
    {
        initList(kid);
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_LIST);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct CodeNode : public ParseNode
{
    explicit CodeNode(const TokenPos& pos)
      : ParseNode(PNK_FUNCTION, JSOP_NOP, PN_CODE, pos)
    {
        MOZ_ASSERT(!pn_body);
        MOZ_ASSERT(!pn_funbox);
        MOZ_ASSERT(pn_dflags == 0);
        pn_cookie.makeFree();
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_CODE);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct NameNode : public ParseNode
{
    NameNode(ParseNodeKind kind, JSOp op, JSAtom* atom, uint32_t blockid,
             const TokenPos& pos)
      : ParseNode(kind, op, PN_NAME, pos)
    {
        pn_atom = atom;
        pn_expr = nullptr;
        pn_cookie.makeFree();
        pn_dflags = 0;
        pn_blockid = blockid;
        MOZ_ASSERT(pn_blockid == blockid);  
    }

    static bool test(const ParseNode& node) {
        return node.isArity(PN_NAME);
    }

#ifdef DEBUG
    void dump(int indent);
#endif
};

struct LexicalScopeNode : public ParseNode
{
    LexicalScopeNode(ObjectBox* blockBox, const TokenPos& pos)
      : ParseNode(PNK_LEXICALSCOPE, JSOP_NOP, PN_NAME, pos)
    {
        MOZ_ASSERT(!pn_expr);
        MOZ_ASSERT(pn_dflags == 0);
        MOZ_ASSERT(pn_blockid == 0);
        pn_objbox = blockBox;
        pn_cookie.makeFree();
    }

    LexicalScopeNode(ObjectBox* blockBox, ParseNode* blockNode)
      : ParseNode(PNK_LEXICALSCOPE, JSOP_NOP, PN_NAME, blockNode->pn_pos)
    {
        pn_objbox = blockBox;
        pn_expr = blockNode;
        pn_blockid = blockNode->pn_blockid;
    }

    static bool test(const ParseNode& node) {
        return node.isKind(PNK_LEXICALSCOPE);
    }
};

class LabeledStatement : public ParseNode
{
  public:
    LabeledStatement(PropertyName* label, ParseNode* stmt, uint32_t begin)
      : ParseNode(PNK_LABEL, JSOP_NOP, PN_NAME, TokenPos(begin, stmt->pn_pos.end))
    {
        pn_atom = label;
        pn_expr = stmt;
    }

    PropertyName* label() const {
        return pn_atom->asPropertyName();
    }

    ParseNode* statement() const {
        return pn_expr;
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_LABEL);
        MOZ_ASSERT_IF(match, node.isArity(PN_NAME));
        MOZ_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class LoopControlStatement : public ParseNode
{
  protected:
    LoopControlStatement(ParseNodeKind kind, PropertyName* label, const TokenPos& pos)
      : ParseNode(kind, JSOP_NOP, PN_NULLARY, pos)
    {
        MOZ_ASSERT(kind == PNK_BREAK || kind == PNK_CONTINUE);
        pn_u.loopControl.label = label;
    }

  public:
    
    PropertyName* label() const {
        return pn_u.loopControl.label;
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_BREAK) || node.isKind(PNK_CONTINUE);
        MOZ_ASSERT_IF(match, node.isArity(PN_NULLARY));
        MOZ_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class BreakStatement : public LoopControlStatement
{
  public:
    BreakStatement(PropertyName* label, const TokenPos& pos)
      : LoopControlStatement(PNK_BREAK, label, pos)
    { }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_BREAK);
        MOZ_ASSERT_IF(match, node.isArity(PN_NULLARY));
        MOZ_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class ContinueStatement : public LoopControlStatement
{
  public:
    ContinueStatement(PropertyName* label, const TokenPos& pos)
      : LoopControlStatement(PNK_CONTINUE, label, pos)
    { }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_CONTINUE);
        MOZ_ASSERT_IF(match, node.isArity(PN_NULLARY));
        MOZ_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class DebuggerStatement : public ParseNode
{
  public:
    explicit DebuggerStatement(const TokenPos& pos)
      : ParseNode(PNK_DEBUGGER, JSOP_NOP, PN_NULLARY, pos)
    { }
};

class ConditionalExpression : public ParseNode
{
  public:
    ConditionalExpression(ParseNode* condition, ParseNode* thenExpr, ParseNode* elseExpr)
      : ParseNode(PNK_CONDITIONAL, JSOP_NOP, PN_TERNARY,
                  TokenPos(condition->pn_pos.begin, elseExpr->pn_pos.end))
    {
        MOZ_ASSERT(condition);
        MOZ_ASSERT(thenExpr);
        MOZ_ASSERT(elseExpr);
        pn_u.ternary.kid1 = condition;
        pn_u.ternary.kid2 = thenExpr;
        pn_u.ternary.kid3 = elseExpr;
    }

    ParseNode& condition() const {
        return *pn_u.ternary.kid1;
    }

    ParseNode& thenExpression() const {
        return *pn_u.ternary.kid2;
    }

    ParseNode& elseExpression() const {
        return *pn_u.ternary.kid3;
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_CONDITIONAL);
        MOZ_ASSERT_IF(match, node.isArity(PN_TERNARY));
        MOZ_ASSERT_IF(match, node.isOp(JSOP_NOP));
        return match;
    }
};

class ThisLiteral : public ParseNode
{
  public:
    explicit ThisLiteral(const TokenPos& pos) : ParseNode(PNK_THIS, JSOP_THIS, PN_NULLARY, pos) { }
};

class NullLiteral : public ParseNode
{
  public:
    explicit NullLiteral(const TokenPos& pos) : ParseNode(PNK_NULL, JSOP_NULL, PN_NULLARY, pos) { }
};

class BooleanLiteral : public ParseNode
{
  public:
    BooleanLiteral(bool b, const TokenPos& pos)
      : ParseNode(b ? PNK_TRUE : PNK_FALSE, b ? JSOP_TRUE : JSOP_FALSE, PN_NULLARY, pos)
    { }
};

class RegExpLiteral : public NullaryNode
{
  public:
    RegExpLiteral(ObjectBox* reobj, const TokenPos& pos)
      : NullaryNode(PNK_REGEXP, JSOP_REGEXP, pos)
    {
        pn_objbox = reobj;
    }

    ObjectBox* objbox() const { return pn_objbox; }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_REGEXP);
        MOZ_ASSERT_IF(match, node.isArity(PN_NULLARY));
        MOZ_ASSERT_IF(match, node.isOp(JSOP_REGEXP));
        return match;
    }
};

class PropertyAccess : public ParseNode
{
  public:
    PropertyAccess(ParseNode* lhs, PropertyName* name, uint32_t begin, uint32_t end)
      : ParseNode(PNK_DOT, JSOP_NOP, PN_NAME, TokenPos(begin, end))
    {
        MOZ_ASSERT(lhs != nullptr);
        MOZ_ASSERT(name != nullptr);
        pn_u.name.expr = lhs;
        pn_u.name.atom = name;
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_DOT);
        MOZ_ASSERT_IF(match, node.isArity(PN_NAME));
        return match;
    }

    ParseNode& expression() const {
        return *pn_u.name.expr;
    }

    PropertyName& name() const {
        return *pn_u.name.atom->asPropertyName();
    }
};

class PropertyByValue : public ParseNode
{
  public:
    PropertyByValue(ParseNode* lhs, ParseNode* propExpr, uint32_t begin, uint32_t end)
      : ParseNode(PNK_ELEM, JSOP_NOP, PN_BINARY, TokenPos(begin, end))
    {
        pn_u.binary.left = lhs;
        pn_u.binary.right = propExpr;
    }
};




struct CallSiteNode : public ListNode {
    explicit CallSiteNode(uint32_t begin): ListNode(PNK_CALLSITEOBJ, TokenPos(begin, begin + 1)) {}

    static bool test(const ParseNode& node) {
        return node.isKind(PNK_CALLSITEOBJ);
    }

    bool getRawArrayValue(ExclusiveContext* cx, MutableHandleValue vp) {
        return pn_head->getConstantValue(cx, AllowObjects, vp);
    }
};

struct ClassMethod : public BinaryNode {
    



    ClassMethod(ParseNode* name, ParseNode* body, JSOp op, bool isStatic)
      : BinaryNode(PNK_CLASSMETHOD, op, TokenPos(name->pn_pos.begin, body->pn_pos.end), name, body)
    {
        pn_u.binary.isStatic = isStatic;
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_CLASSMETHOD);
        MOZ_ASSERT_IF(match, node.isArity(PN_BINARY));
        return match;
    }

    ParseNode& name() const {
        return *pn_u.binary.left;
    }
    ParseNode& method() const {
        return *pn_u.binary.right;
    }
    bool isStatic() const {
        return pn_u.binary.isStatic;
    }
};

struct ClassNames : public BinaryNode {
    ClassNames(ParseNode* outerBinding, ParseNode* innerBinding, const TokenPos& pos)
      : BinaryNode(PNK_CLASSNAMES, JSOP_NOP, pos, outerBinding, innerBinding)
    {
        MOZ_ASSERT_IF(outerBinding, outerBinding->isKind(PNK_NAME));
        MOZ_ASSERT(innerBinding->isKind(PNK_NAME));
        MOZ_ASSERT_IF(outerBinding, innerBinding->pn_atom == outerBinding->pn_atom);
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_CLASSNAMES);
        MOZ_ASSERT_IF(match, node.isArity(PN_BINARY));
        return match;
    }

    







    ParseNode* outerBinding() const {
        return pn_u.binary.left;
    }
    ParseNode* innerBinding() const {
        return pn_u.binary.right;
    }
};

struct ClassNode : public TernaryNode {
    ClassNode(ParseNode* names, ParseNode* heritage, ParseNode* methodsOrBlock)
      : TernaryNode(PNK_CLASS, JSOP_NOP, names, heritage, methodsOrBlock)
    {
        MOZ_ASSERT_IF(names, names->is<ClassNames>());
        MOZ_ASSERT(methodsOrBlock->is<LexicalScopeNode>() ||
                   methodsOrBlock->isKind(PNK_CLASSMETHODLIST));
    }

    static bool test(const ParseNode& node) {
        bool match = node.isKind(PNK_CLASS);
        MOZ_ASSERT_IF(match, node.isArity(PN_TERNARY));
        return match;
    }

    ClassNames* names() const {
        return pn_kid1 ? &pn_kid1->as<ClassNames>() : nullptr;
    }
    ParseNode* heritage() const {
        return pn_kid2;
    }
    ParseNode* methodList() const {
        if (pn_kid3->isKind(PNK_CLASSMETHODLIST))
            return pn_kid3;

        MOZ_ASSERT(pn_kid3->is<LexicalScopeNode>());
        ParseNode* list = pn_kid3->pn_expr;
        MOZ_ASSERT(list->isKind(PNK_CLASSMETHODLIST));
        return list;
    }
    ObjectBox* scopeObject() const {
        MOZ_ASSERT(pn_kid3->is<LexicalScopeNode>());
        return pn_kid3->pn_objbox;
    }
};


#ifdef DEBUG
void DumpParseTree(ParseNode* pn, int indent = 0);
#endif



































































































struct Definition : public ParseNode
{
    bool isFreeVar() const {
        MOZ_ASSERT(isDefn());
        return pn_cookie.isFree();
    }

    enum Kind { MISSING = 0, VAR, GLOBALCONST, CONST, LET, ARG, NAMED_LAMBDA, PLACEHOLDER };

    bool canHaveInitializer() { return int(kind()) <= int(ARG); }

    static const char* kindString(Kind kind);

    Kind kind() {
        if (getKind() == PNK_FUNCTION) {
            if (isOp(JSOP_GETARG))
                return ARG;
            return VAR;
        }
        MOZ_ASSERT(getKind() == PNK_NAME);
        if (isOp(JSOP_CALLEE))
            return NAMED_LAMBDA;
        if (isPlaceholder())
            return PLACEHOLDER;
        if (isOp(JSOP_GETARG))
            return ARG;
        if (isLexical())
            return isConst() ? CONST : LET;
        if (isConst())
            return GLOBALCONST;
        return VAR;
    }
};

class ParseNodeAllocator
{
  public:
    explicit ParseNodeAllocator(ExclusiveContext* cx, LifoAlloc& alloc)
      : cx(cx), alloc(alloc), freelist(nullptr)
    {}

    void* allocNode();
    void freeNode(ParseNode* pn);
    ParseNode* freeTree(ParseNode* pn);
    void prepareNodeForMutation(ParseNode* pn);

  private:
    ExclusiveContext* cx;
    LifoAlloc& alloc;
    ParseNode* freelist;
};

inline bool
ParseNode::test(unsigned flag) const
{
    MOZ_ASSERT(pn_defn || pn_arity == PN_CODE || pn_arity == PN_NAME);
#ifdef DEBUG
    if ((flag & PND_ASSIGNED) && pn_defn && !(pn_dflags & flag)) {
        for (ParseNode* pn = ((Definition*) this)->dn_uses; pn; pn = pn->pn_link) {
            MOZ_ASSERT(!pn->pn_defn);
            MOZ_ASSERT(!(pn->pn_dflags & flag));
        }
    }
#endif
    return !!(pn_dflags & flag);
}

inline void
ParseNode::markAsAssigned()
{
    MOZ_ASSERT(js_CodeSpec[pn_op].format & JOF_NAME);
    if (isUsed())
        pn_lexdef->pn_dflags |= PND_ASSIGNED;
    pn_dflags |= PND_ASSIGNED;
}

inline Definition*
ParseNode::resolve()
{
    if (isDefn())
        return (Definition*)this;
    MOZ_ASSERT(lexdef()->isDefn());
    return (Definition*)lexdef();
}

inline bool
ParseNode::isConstant()
{
    switch (pn_type) {
      case PNK_NUMBER:
      case PNK_STRING:
      case PNK_TEMPLATE_STRING:
      case PNK_NULL:
      case PNK_FALSE:
      case PNK_TRUE:
        return true;
      case PNK_ARRAY:
      case PNK_OBJECT:
        MOZ_ASSERT(isOp(JSOP_NEWINIT));
        return !(pn_xflags & PNX_NONCONST);
      default:
        return false;
    }
}

class ObjectBox
{
  public:
    JSObject* object;

    ObjectBox(JSObject* object, ObjectBox* traceLink);
    bool isFunctionBox() { return object->is<JSFunction>(); }
    FunctionBox* asFunctionBox();
    void trace(JSTracer* trc);

  protected:
    friend struct CGObjectList;

    ObjectBox* traceLink;
    ObjectBox* emitLink;

    ObjectBox(JSFunction* function, ObjectBox* traceLink);
};

enum ParseReportKind
{
    ParseError,
    ParseWarning,
    ParseExtraWarning,
    ParseStrictError
};

enum FunctionSyntaxKind { Expression, Statement, Arrow, Method };

static inline ParseNode*
FunctionArgsList(ParseNode* fn, unsigned* numFormals)
{
    MOZ_ASSERT(fn->isKind(PNK_FUNCTION));
    ParseNode* argsBody = fn->pn_body;
    MOZ_ASSERT(argsBody->isKind(PNK_ARGSBODY));
    *numFormals = argsBody->pn_count;
    if (*numFormals > 0 && argsBody->last()->isKind(PNK_STATEMENTLIST))
        (*numFormals)--;
    MOZ_ASSERT(argsBody->isArity(PN_LIST));
    return argsBody->pn_head;
}

} 
} 

#endif 
