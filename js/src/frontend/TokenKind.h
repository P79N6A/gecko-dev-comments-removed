





#ifndef frontend_TokenKind_h
#define frontend_TokenKind_h












































#define FOR_EACH_TOKEN_KIND_WITH_RANGE(macro, range) \
    macro(EOF,         "end of script") \
    \
    /* only returned by peekTokenSameLine() */ \
    macro(EOL,          "line terminator") \
    \
    macro(SEMI,         "';'") \
    macro(COMMA,        "','") \
    macro(HOOK,         "'?'")    /* conditional */ \
    macro(COLON,        "':'")    /* conditional */ \
    macro(INC,          "'++'")   /* increment */ \
    macro(DEC,          "'--'")   /* decrement */ \
    macro(DOT,          "'.'")    /* member operator */ \
    macro(TRIPLEDOT,    "'...'")  /* rest arguments and spread operator */ \
    macro(LB,           "'['") \
    macro(RB,           "']'") \
    macro(LC,           "'{'") \
    macro(RC,           "'}'") \
    macro(LP,           "'('") \
    macro(RP,           "')'") \
    macro(NAME,         "identifier") \
    macro(NUMBER,       "numeric literal") \
    macro(STRING,       "string literal") \
    \
    /* start of template literal with substitutions */ \
    macro(TEMPLATE_HEAD,    "'${'") \
    /* template literal without substitutions */ \
    macro(NO_SUBS_TEMPLATE, "template literal") \
    \
    macro(REGEXP,       "regular expression literal") \
    macro(TRUE,         "boolean literal 'true'") \
    macro(FALSE,        "boolean literal 'false'") \
    macro(NULL,         "null literal") \
    macro(THIS,         "keyword 'this'") \
    macro(FUNCTION,     "keyword 'function'") \
    macro(IF,           "keyword 'if'") \
    macro(ELSE,         "keyword 'else'") \
    macro(SWITCH,       "keyword 'switch'") \
    macro(CASE,         "keyword 'case'") \
    macro(DEFAULT,      "keyword 'default'") \
    macro(WHILE,        "keyword 'while'") \
    macro(DO,           "keyword 'do'") \
    macro(FOR,          "keyword 'for'") \
    macro(BREAK,        "keyword 'break'") \
    macro(CONTINUE,     "keyword 'continue'") \
    macro(VAR,          "keyword 'var'") \
    macro(CONST,        "keyword 'const'") \
    macro(WITH,         "keyword 'with'") \
    macro(RETURN,       "keyword 'return'") \
    macro(NEW,          "keyword 'new'") \
    macro(DELETE,       "keyword 'delete'") \
    macro(TRY,          "keyword 'try'") \
    macro(CATCH,        "keyword 'catch'") \
    macro(FINALLY,      "keyword 'finally'") \
    macro(THROW,        "keyword 'throw'") \
    macro(DEBUGGER,     "keyword 'debugger'") \
    macro(YIELD,        "keyword 'yield'") \
    macro(LET,          "keyword 'let'") \
    macro(EXPORT,       "keyword 'export'") \
    macro(IMPORT,       "keyword 'import'") \
    macro(CLASS,        "keyword 'class'") \
    macro(EXTENDS,      "keyword 'extends'") \
    macro(SUPER,        "keyword 'super'") \
    macro(RESERVED,     "reserved keyword") \
    /* reserved keywords in strict mode */ \
    macro(STRICT_RESERVED, "reserved keyword") \
    \
    /* \
     * The following token types occupy contiguous ranges to enable easy \
     * range-testing. \
     */ \
    /* \
     * Binary operators tokens, TOK_OR thru TOK_MOD. These must be in the same \
     * order as F(OR) and friends in FOR_EACH_PARSE_NODE_KIND in ParseNode.h. \
     */ \
    macro(OR,           "'||'")   /* logical or */ \
    range(BINOP_FIRST, OR) \
    macro(AND,          "'&&'")   /* logical and */ \
    macro(BITOR,        "'|'")    /* bitwise-or */ \
    macro(BITXOR,       "'^'")    /* bitwise-xor */ \
    macro(BITAND,       "'&'")    /* bitwise-and */ \
    \
    /* Equality operation tokens, per TokenKindIsEquality. */ \
    macro(STRICTEQ,     "'==='") \
    range(EQUALITY_START, STRICTEQ) \
    macro(EQ,           "'=='") \
    macro(STRICTNE,     "'!=='") \
    macro(NE,           "'!='") \
    range(EQUALITY_LAST, NE) \
    \
    /* Relational ops, per TokenKindIsRelational. */ \
    macro(LT,           "'<'") \
    range(RELOP_START, LT) \
    macro(LE,           "'<='") \
    macro(GT,           "'>'") \
    macro(GE,           "'>='") \
    range(RELOP_LAST, GE) \
    \
    macro(INSTANCEOF,   "keyword 'instanceof'") \
    macro(IN,           "keyword 'in'") \
    \
    /* Shift ops, per TokenKindIsShift. */ \
    macro(LSH,          "'<<'") \
    range(SHIFTOP_START, LSH) \
    macro(RSH,          "'>>'") \
    macro(URSH,         "'>>>'") \
    range(SHIFTOP_LAST, URSH) \
    \
    macro(ADD,          "'+'") \
    macro(SUB,          "'-'") \
    macro(MUL,          "'*'") \
    macro(DIV,          "'/'") \
    macro(MOD,          "'%'") \
    range(BINOP_LAST, MOD) \
    \
    /* Unary operation tokens. */ \
    macro(TYPEOF,       "keyword 'typeof'") \
    macro(VOID,         "keyword 'void'") \
    macro(NOT,          "'!'") \
    macro(BITNOT,       "'~'") \
    \
    macro(ARROW,        "'=>'")   /* function arrow */ \
    \
    /* Assignment ops, per TokenKindIsAssignment */ \
    macro(ASSIGN,       "'='") \
    range(ASSIGNMENT_START, ASSIGN) \
    macro(ADDASSIGN,    "'+='") \
    macro(SUBASSIGN,    "'-='") \
    macro(BITORASSIGN,  "'|='") \
    macro(BITXORASSIGN, "'^='") \
    macro(BITANDASSIGN, "'&='") \
    macro(LSHASSIGN,    "'<<='") \
    macro(RSHASSIGN,    "'>>='") \
    macro(URSHASSIGN,   "'>>>='") \
    macro(MULASSIGN,    "'*='") \
    macro(DIVASSIGN,    "'/='") \
    macro(MODASSIGN,    "'%='") \
    range(ASSIGNMENT_LAST, MODASSIGN)

#define TOKEN_KIND_RANGE_EMIT_NONE(name, value)
#define FOR_EACH_TOKEN_KIND(macro) \
    FOR_EACH_TOKEN_KIND_WITH_RANGE(macro, TOKEN_KIND_RANGE_EMIT_NONE)

namespace js {
namespace frontend {



enum TokenKind {
#define EMIT_ENUM(name, desc) TOK_##name,
#define EMIT_ENUM_RANGE(name, value) TOK_##name = TOK_##value,
    FOR_EACH_TOKEN_KIND_WITH_RANGE(EMIT_ENUM, EMIT_ENUM_RANGE)
#undef EMIT_ENUM
#undef EMIT_ENUM_RANGE
    TOK_LIMIT                      
};

inline bool
TokenKindIsBinaryOp(TokenKind tt)
{
    return TOK_BINOP_FIRST <= tt && tt <= TOK_BINOP_LAST;
}

inline bool
TokenKindIsEquality(TokenKind tt)
{
    return TOK_EQUALITY_START <= tt && tt <= TOK_EQUALITY_LAST;
}

inline bool
TokenKindIsRelational(TokenKind tt)
{
    return TOK_RELOP_START <= tt && tt <= TOK_RELOP_LAST;
}

inline bool
TokenKindIsShift(TokenKind tt)
{
    return TOK_SHIFTOP_START <= tt && tt <= TOK_SHIFTOP_LAST;
}

inline bool
TokenKindIsAssignment(TokenKind tt)
{
    return TOK_ASSIGNMENT_START <= tt && tt <= TOK_ASSIGNMENT_LAST;
}

inline bool
TokenKindIsDecl(TokenKind tt)
{
    return tt == TOK_VAR || tt == TOK_LET;
}

} 
} 

#endif 
