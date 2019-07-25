









































#ifndef jsreflect_h___
#define jsreflect_h___

#include <stdlib.h>
#include "jspubtd.h"

namespace js {

typedef enum ASTType {
    AST_ERROR = -1,
#define ASTDEF(ast, val, str) ast = val,
#include "jsast.tbl"
#undef ASTDEF
    AST_LIMIT
} ASTType;

typedef enum {
    AOP_ERR = -1,

    
    AOP_ASSIGN = 0,
    
    AOP_PLUS, AOP_MINUS, AOP_STAR, AOP_DIV, AOP_MOD,
    
    AOP_LSH, AOP_RSH, AOP_URSH,
    
    AOP_BITOR, AOP_BITXOR, AOP_BITAND,

    AOP_LIMIT
} AssignmentOperator;

typedef enum {
    BINOP_ERR = -1,

    
    BINOP_EQ = 0, BINOP_NE, BINOP_STRICTEQ, BINOP_STRICTNE,
    
    BINOP_LT, BINOP_LE, BINOP_GT, BINOP_GE,
    
    BINOP_LSH, BINOP_RSH, BINOP_URSH,
    
    BINOP_PLUS, BINOP_MINUS, BINOP_STAR, BINOP_DIV, BINOP_MOD,
    
    BINOP_BITOR, BINOP_BITXOR, BINOP_BITAND,
    
    BINOP_IN, BINOP_INSTANCEOF,
    
    BINOP_DBLDOT,

    BINOP_LIMIT
} BinaryOperator;

typedef enum {
    UNOP_ERR = -1,

    UNOP_DELETE = 0,
    UNOP_NEG,
    UNOP_POS,
    UNOP_NOT,
    UNOP_BITNOT,
    UNOP_TYPEOF,
    UNOP_VOID,

    UNOP_LIMIT
} UnaryOperator;

typedef enum {
    VARDECL_ERR = -1,
    VARDECL_VAR = 0,
    VARDECL_CONST,
    VARDECL_LET,
    VARDECL_LIMIT
} VarDeclKind;

typedef enum {
    PROP_ERR = -1,
    PROP_INIT = 0,
    PROP_GETTER,
    PROP_SETTER,
    PROP_LIMIT
} PropKind;

extern char const *aopNames[];
extern char const *binopNames[];
extern char const *unopNames[];
extern char const *nodeTypeNames[];

} 

JS_BEGIN_EXTERN_C

extern JSClass js_ReflectClass;
extern JSClass js_ASTNodeClass;

extern JSObject *
js_InitReflectClasses(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif 
