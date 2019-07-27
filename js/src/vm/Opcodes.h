






#ifndef vm_Opcodes_h
#define vm_Opcodes_h

#include "mozilla/Attributes.h"

#include <stddef.h>






































































#define FOR_EACH_OPCODE(macro) \
    /* legend:  op      val   name          image       len use def  format */ \
    /*
     * No operation is performed.
     *   Category: Other
     *   Operands:
     *   Stack: =>
     */ \
    macro(JSOP_NOP,       0,  "nop",        NULL,         1,  0,  0, JOF_BYTE) \
    \
     \
    





 \
    macro(JSOP_UNDEFINED, 1,  js_undefined_str, "",       1,  0,  1, JOF_BYTE) \
    macro(JSOP_UNUSED2,   2,  "unused2",    NULL,         1,  1,  0, JOF_BYTE) \
    










 \
    macro(JSOP_ENTERWITH, 3,  "enterwith",  NULL,         5,  1,  0, JOF_OBJECT) \
    





 \
    macro(JSOP_LEAVEWITH, 4,  "leavewith",  NULL,         1,  0,  0, JOF_BYTE) \
    






 \
    macro(JSOP_RETURN,    5,  "return",     NULL,         1,  1,  0, JOF_BYTE) \
    





 \
    macro(JSOP_GOTO,      6,  "goto",       NULL,         5,  0,  0, JOF_JUMP) \
    










 \
    macro(JSOP_IFEQ,      7,  "ifeq",       NULL,         5,  1,  0, JOF_JUMP|JOF_DETECTING) \
    






 \
    macro(JSOP_IFNE,      8,  "ifne",       NULL,         5,  1,  0, JOF_JUMP) \
    \
    











 \
    macro(JSOP_ARGUMENTS, 9,  "arguments",  NULL,         1,  0,  1, JOF_BYTE) \
    \
    






 \
    macro(JSOP_SWAP,      10, "swap",       NULL,         1,  2,  2, JOF_BYTE) \
    






 \
    macro(JSOP_POPN,      11, "popn",       NULL,         3, -1,  0, JOF_UINT16) \
    \
     \
    





 \
    macro(JSOP_DUP,       12, "dup",        NULL,         1,  1,  2, JOF_BYTE) \
    





 \
    macro(JSOP_DUP2,      13, "dup2",       NULL,         1,  2,  4, JOF_BYTE) \
    






 \
    macro(JSOP_SETCONST,  14, "setconst",   NULL,         5,  1,  1, JOF_ATOM|JOF_NAME|JOF_SET) \
    







 \
    macro(JSOP_BITOR,     15, "bitor",      "|",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_BITXOR,    16, "bitxor",     "^",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_BITAND,    17, "bitand",     "&",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    






 \
    macro(JSOP_EQ,        18, "eq",         "==",         1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH|JOF_DETECTING) \
    macro(JSOP_NE,        19, "ne",         "!=",         1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH|JOF_DETECTING) \
    macro(JSOP_LT,        20, "lt",         "<",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_LE,        21, "le",         "<=",         1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_GT,        22, "gt",         ">",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_GE,        23, "ge",         ">=",         1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    






 \
    macro(JSOP_LSH,       24, "lsh",        "<<",         1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_RSH,       25, "rsh",        ">>",         1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    






 \
    macro(JSOP_URSH,      26, "ursh",       ">>>",        1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    






 \
    macro(JSOP_ADD,       27, "add",        "+",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    






 \
    macro(JSOP_SUB,       28, "sub",        "-",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_MUL,       29, "mul",        "*",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_DIV,       30, "div",        "/",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_MOD,       31, "mod",        "%",          1,  2,  1, JOF_BYTE|JOF_LEFTASSOC|JOF_ARITH) \
    





 \
    macro(JSOP_NOT,       32, "not",        "!",          1,  1,  1, JOF_BYTE|JOF_ARITH|JOF_DETECTING) \
    





 \
    macro(JSOP_BITNOT,    33, "bitnot",     "~",          1,  1,  1, JOF_BYTE|JOF_ARITH) \
    





 \
    macro(JSOP_NEG,       34, "neg",        "- ",         1,  1,  1, JOF_BYTE|JOF_ARITH) \
    






 \
    macro(JSOP_POS,       35, "pos",        "+ ",         1,  1,  1, JOF_BYTE|JOF_ARITH) \
    









 \
    macro(JSOP_DELNAME,   36, "delname",    NULL,         5,  0,  1, JOF_ATOM|JOF_NAME|JOF_CHECKSLOPPY) \
    






 \
    macro(JSOP_DELPROP,   37, "delprop",    NULL,         5,  1,  1, JOF_ATOM|JOF_PROP|JOF_CHECKSLOPPY) \
    







 \
    macro(JSOP_DELELEM,   38, "delelem",    NULL,         1,  2,  1, JOF_BYTE |JOF_ELEM|JOF_CHECKSLOPPY) \
    





 \
    macro(JSOP_TYPEOF,    39, js_typeof_str,NULL,         1,  1,  1, JOF_BYTE|JOF_DETECTING) \
    





 \
    macro(JSOP_VOID,      40, js_void_str,  NULL,         1,  1,  1, JOF_BYTE) \
    \
    










 \
    macro(JSOP_SPREADCALL,41, "spreadcall", NULL,         1,  3,  1, JOF_BYTE|JOF_INVOKE|JOF_TYPESET) \
    








 \
    macro(JSOP_SPREADNEW, 42, "spreadnew",  NULL,         1,  3,  1, JOF_BYTE|JOF_INVOKE|JOF_TYPESET) \
    










 \
    macro(JSOP_SPREADEVAL,43, "spreadeval", NULL,         1,  3,  1, JOF_BYTE|JOF_INVOKE|JOF_TYPESET|JOF_CHECKSLOPPY) \
    \
    






 \
    macro(JSOP_DUPAT,     44, "dupat",      NULL,         4,  0,  1,  JOF_UINT24) \
    \
    





 \
    macro(JSOP_SYMBOL,    45, "symbol",     NULL,         2,  0,  1,  JOF_UINT8) \
    \
    







 \
    macro(JSOP_STRICTDELPROP,   46, "strict-delprop",    NULL,         5,  1,  1, JOF_ATOM|JOF_PROP|JOF_CHECKSTRICT) \
    








 \
    macro(JSOP_STRICTDELELEM,   47, "strict-delelem",    NULL,         1,  2,  1, JOF_BYTE|JOF_ELEM|JOF_CHECKSTRICT) \
    







 \
    macro(JSOP_STRICTSETPROP,   48, "strict-setprop",    NULL,         5,  2,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING|JOF_CHECKSTRICT) \
    







 \
    macro(JSOP_STRICTSETNAME,   49, "strict-setname",    NULL,         5,  2,  1,  JOF_ATOM|JOF_NAME|JOF_SET|JOF_DETECTING|JOF_CHECKSTRICT) \
    










 \
    macro(JSOP_STRICTSPREADEVAL,      50, "strict-spreadeval", NULL,         1,  3,  1, JOF_BYTE|JOF_INVOKE|JOF_TYPESET|JOF_CHECKSTRICT) \
    






 \
    macro(JSOP_CLASSHERITAGE,  51, "classheritage",   NULL,         1,  1,  2,  JOF_BYTE) \
    





 \
    macro(JSOP_FUNWITHPROTO,   52, "funwithproto",    NULL,         5,  1,  1,  JOF_OBJECT) \
    \
    





 \
    macro(JSOP_GETPROP,   53, "getprop",    NULL,         5,  1,  1, JOF_ATOM|JOF_PROP|JOF_TYPESET) \
    






 \
    macro(JSOP_SETPROP,   54, "setprop",    NULL,         5,  2,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING|JOF_CHECKSLOPPY) \
    






 \
    macro(JSOP_GETELEM,   55, "getelem",    NULL,         1,  2,  1, JOF_BYTE |JOF_ELEM|JOF_TYPESET|JOF_LEFTASSOC) \
    







 \
    macro(JSOP_SETELEM,   56, "setelem",    NULL,         1,  3,  1, JOF_BYTE |JOF_ELEM|JOF_SET|JOF_DETECTING|JOF_CHECKSLOPPY) \
    








 \
    macro(JSOP_STRICTSETELEM,   57, "strict-setelem",    NULL,         1,  3,  1, JOF_BYTE |JOF_ELEM|JOF_SET|JOF_DETECTING|JOF_CHECKSTRICT) \
    







 \
    macro(JSOP_CALL,      58, "call",       NULL,         3, -1,  1, JOF_UINT16|JOF_INVOKE|JOF_TYPESET) \
    





 \
    macro(JSOP_GETNAME,   59, "getname",    NULL,         5,  0,  1, JOF_ATOM|JOF_NAME|JOF_TYPESET) \
    





 \
    macro(JSOP_DOUBLE,    60, "double",     NULL,         5,  0,  1, JOF_DOUBLE) \
    





 \
    macro(JSOP_STRING,    61, "string",     NULL,         5,  0,  1, JOF_ATOM) \
    





 \
    macro(JSOP_ZERO,      62, "zero",       "0",          1,  0,  1, JOF_BYTE) \
    





 \
    macro(JSOP_ONE,       63, "one",        "1",          1,  0,  1, JOF_BYTE) \
    





 \
    macro(JSOP_NULL,      64, js_null_str,  js_null_str,  1,  0,  1, JOF_BYTE) \
    





 \
    macro(JSOP_THIS,      65, js_this_str,  js_this_str,  1,  0,  1, JOF_BYTE) \
    





 \
    macro(JSOP_FALSE,     66, js_false_str, js_false_str, 1,  0,  1, JOF_BYTE) \
    macro(JSOP_TRUE,      67, js_true_str,  js_true_str,  1,  0,  1, JOF_BYTE) \
    






 \
    macro(JSOP_OR,        68, "or",         NULL,         5,  1,  1, JOF_JUMP|JOF_DETECTING|JOF_LEFTASSOC) \
    






 \
    macro(JSOP_AND,       69, "and",        NULL,         5,  1,  1, JOF_JUMP|JOF_DETECTING|JOF_LEFTASSOC) \
    \
    











 \
    macro(JSOP_TABLESWITCH, 70, "tableswitch", NULL,     -1,  1,  0,  JOF_TABLESWITCH|JOF_DETECTING) \
    \
    






 \
    macro(JSOP_RUNONCE,   71, "runonce",    NULL,         1,  0,  0,  JOF_BYTE) \
    \
     \
    






 \
    macro(JSOP_STRICTEQ,  72, "stricteq",   "===",        1,  2,  1, JOF_BYTE|JOF_DETECTING|JOF_LEFTASSOC|JOF_ARITH) \
    macro(JSOP_STRICTNE,  73, "strictne",   "!==",        1,  2,  1, JOF_BYTE|JOF_DETECTING|JOF_LEFTASSOC|JOF_ARITH) \
    \
    








 \
    macro(JSOP_THROWMSG,   74, "throwmsg",    NULL,         3,  0,  0, JOF_UINT16) \
    \
    







 \
    macro(JSOP_ITER,      75, "iter",       NULL,         2,  1,  1,  JOF_UINT8) \
    







 \
    macro(JSOP_MOREITER,  76, "moreiter",   NULL,         1,  1,  2,  JOF_BYTE) \
    







 \
    macro(JSOP_ISNOITER,  77, "isnoiter",   NULL,         1,  1,  2,  JOF_BYTE) \
    






 \
    macro(JSOP_ENDITER,   78, "enditer",    NULL,         1,  1,  0,  JOF_BYTE) \
    \
    









 \
    macro(JSOP_FUNAPPLY,  79, "funapply",   NULL,         3, -1,  1,  JOF_UINT16|JOF_INVOKE|JOF_TYPESET) \
    \
    





 \
    macro(JSOP_OBJECT,    80, "object",     NULL,         5,  0,  1,  JOF_OBJECT) \
    \
    





 \
    macro(JSOP_POP,       81, "pop",        NULL,         1,  1,  0,  JOF_BYTE) \
    \
    







 \
    macro(JSOP_NEW,       82, js_new_str,   NULL,         3, -1,  1,  JOF_UINT16|JOF_INVOKE|JOF_TYPESET) \
    






 \
    macro(JSOP_OBJWITHPROTO,  83, "objwithproto",   NULL,         1,  1,  1,  JOF_BYTE) \
    \
    







 \
    macro(JSOP_GETARG,    84, "getarg",     NULL,         3,  0,  1,  JOF_QARG |JOF_NAME) \
    







 \
    macro(JSOP_SETARG,    85, "setarg",     NULL,         3,  1,  1,  JOF_QARG |JOF_NAME|JOF_SET) \
    





 \
    macro(JSOP_GETLOCAL,  86,"getlocal",    NULL,         4,  0,  1,  JOF_LOCAL|JOF_NAME) \
    





 \
    macro(JSOP_SETLOCAL,  87,"setlocal",    NULL,         4,  1,  1,  JOF_LOCAL|JOF_NAME|JOF_SET|JOF_DETECTING) \
    \
    





 \
    macro(JSOP_UINT16,    88, "uint16",     NULL,         3,  0,  1,  JOF_UINT16) \
    \
     \
    











 \
    macro(JSOP_NEWINIT,   89, "newinit",    NULL,         5,  0,  1, JOF_UINT8) \
    







 \
    macro(JSOP_NEWARRAY,  90, "newarray",   NULL,         4,  0,  1, JOF_UINT24) \
    








 \
    macro(JSOP_NEWOBJECT, 91, "newobject",  NULL,         5,  0,  1, JOF_OBJECT) \
    \
    




\
    macro(JSOP_INITHOMEOBJECT,  92, "inithomeobject",   NULL,         1,  2,  2, JOF_BYTE|JOF_SET|JOF_DETECTING) \
    \
    








 \
    macro(JSOP_INITPROP,  93, "initprop",   NULL,         5,  2,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING) \
    \
    








 \
    macro(JSOP_INITELEM,  94, "initelem",   NULL,         1,  3,  1, JOF_BYTE|JOF_ELEM|JOF_SET|JOF_DETECTING) \
    \
    










 \
    macro(JSOP_INITELEM_INC,95, "initelem_inc", NULL,     1,  3,  2, JOF_BYTE|JOF_ELEM|JOF_SET) \
    \
    








 \
    macro(JSOP_INITELEM_ARRAY,96, "initelem_array", NULL, 4,  2,  1,  JOF_UINT24|JOF_ELEM|JOF_SET|JOF_DETECTING) \
    \
    








 \
    macro(JSOP_INITPROP_GETTER,  97, "initprop_getter",   NULL, 5,  2,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING) \
    








 \
    macro(JSOP_INITPROP_SETTER,  98, "initprop_setter",   NULL, 5,  2,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING) \
    









 \
    macro(JSOP_INITELEM_GETTER,  99, "initelem_getter",   NULL, 1,  3,  1, JOF_BYTE|JOF_ELEM|JOF_SET|JOF_DETECTING) \
    









 \
    macro(JSOP_INITELEM_SETTER, 100, "initelem_setter",   NULL, 1,  3,  1, JOF_BYTE|JOF_ELEM|JOF_SET|JOF_DETECTING) \
    







 \
    macro(JSOP_CALLSITEOBJ,     101, "callsiteobj",     NULL,         5,  0,  1,  JOF_OBJECT) \
    \
    







 \
    macro(JSOP_NEWARRAY_COPYONWRITE, 102, "newarray_copyonwrite", NULL, 5, 0, 1, JOF_OBJECT) \
    \
    macro(JSOP_SUPERBASE,  103, "superbase",   NULL,         1,  0,  1,  JOF_BYTE) \
    






 \
    macro(JSOP_GETPROP_SUPER,   104, "getprop-super", NULL, 5,  2,  1, JOF_ATOM|JOF_PROP) \
    







 \
    macro(JSOP_STRICTSETPROP_SUPER,   105, "strictsetprop-super",    NULL,         5,  3,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING|JOF_CHECKSTRICT) \
    \
    








 \
    macro(JSOP_LABEL,     106,"label",     NULL,          5,  0,  0,  JOF_JUMP) \
    \
    






 \
    macro(JSOP_SETPROP_SUPER,   107, "setprop-super",    NULL,         5,  3,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING|JOF_CHECKSLOPPY) \
    \
    














 \
    macro(JSOP_FUNCALL,   108,"funcall",    NULL,         3, -1,  1, JOF_UINT16|JOF_INVOKE|JOF_TYPESET) \
    \
    







 \
    macro(JSOP_LOOPHEAD,  109,"loophead",   NULL,         1,  0,  0,  JOF_BYTE) \
    \
     \
    







 \
    macro(JSOP_BINDNAME,  110,"bindname",   NULL,         5,  0,  1,  JOF_ATOM|JOF_NAME|JOF_SET) \
    






 \
    macro(JSOP_SETNAME,   111,"setname",    NULL,         5,  2,  1,  JOF_ATOM|JOF_NAME|JOF_SET|JOF_DETECTING|JOF_CHECKSLOPPY) \
    \
     \
    






 \
    macro(JSOP_THROW,     112,js_throw_str, NULL,         1,  1,  0,  JOF_BYTE) \
    \
    








 \
    macro(JSOP_IN,        113,js_in_str,    js_in_str,    1,  2,  1, JOF_BYTE|JOF_LEFTASSOC) \
    







 \
    macro(JSOP_INSTANCEOF,114,js_instanceof_str,js_instanceof_str,1,2,1,JOF_BYTE|JOF_LEFTASSOC) \
    \
    





 \
    macro(JSOP_DEBUGGER,  115,"debugger",   NULL,         1,  0,  0, JOF_BYTE) \
    \
    








 \
    macro(JSOP_GOSUB,     116,"gosub",      NULL,         5,  0,  0,  JOF_JUMP) \
    









 \
    macro(JSOP_RETSUB,    117,"retsub",     NULL,         1,  2,  0,  JOF_BYTE) \
    \
     \
    








 \
    macro(JSOP_EXCEPTION, 118,"exception",  NULL,         1,  0,  1,  JOF_BYTE) \
    \
    




 \
    macro(JSOP_LINENO,    119,"lineno",     NULL,         3,  0,  0,  JOF_UINT16) \
    \
    






























 \
    macro(JSOP_CONDSWITCH,120,"condswitch", NULL,         1,  0,  0,  JOF_BYTE) \
    







 \
    macro(JSOP_CASE,      121,"case",       NULL,         5,  2,  1,  JOF_JUMP) \
    








 \
    macro(JSOP_DEFAULT,   122,"default",    NULL,         5,  1,  0,  JOF_JUMP) \
    \
     \
    









 \
    macro(JSOP_EVAL,      123,"eval",       NULL,         3, -1,  1, JOF_UINT16|JOF_INVOKE|JOF_TYPESET|JOF_CHECKSLOPPY) \
    \
     \
    









 \
    macro(JSOP_STRICTEVAL,       124, "strict-eval",       NULL,         3, -1,  1, JOF_UINT16|JOF_INVOKE|JOF_TYPESET|JOF_CHECKSTRICT) \
    






 \
    macro(JSOP_GETELEM_SUPER, 125, "getelem-super", NULL, 1,  3,  1, JOF_BYTE |JOF_ELEM|JOF_LEFTASSOC) \
    macro(JSOP_UNUSED126,  126, "unused126", NULL,      1,  0,  0,  JOF_BYTE) \
    \
    








 \
    macro(JSOP_DEFFUN,    127,"deffun",     NULL,         5,  0,  0,  JOF_OBJECT) \
    











 \
    macro(JSOP_DEFCONST,  128,"defconst",   NULL,         5,  0,  0,  JOF_ATOM) \
    









 \
    macro(JSOP_DEFVAR,    129,"defvar",     NULL,         5,  0,  0,  JOF_ATOM) \
    \
    






 \
    macro(JSOP_LAMBDA,    130, "lambda",    NULL,         5,  0,  1, JOF_OBJECT) \
    






 \
    macro(JSOP_LAMBDA_ARROW, 131, "lambda_arrow", NULL,   5,  1,  1, JOF_OBJECT) \
    \
    







 \
    macro(JSOP_CALLEE,    132, "callee",    NULL,         1,  0,  1, JOF_BYTE) \
    \
    






 \
    macro(JSOP_PICK,        133, "pick",      NULL,       2,  0,  0,  JOF_UINT8) \
    \
    








 \
    macro(JSOP_TRY,         134,"try",        NULL,       1,  0,  0,  JOF_BYTE) \
    






 \
    macro(JSOP_FINALLY,     135,"finally",    NULL,       1,  0,  2,  JOF_BYTE) \
    \
    

















 \
    macro(JSOP_GETALIASEDVAR, 136,"getaliasedvar",NULL,      5,  0,  1,  JOF_SCOPECOORD|JOF_NAME|JOF_TYPESET) \
    





 \
    macro(JSOP_SETALIASEDVAR, 137,"setaliasedvar",NULL,      5,  1,  1,  JOF_SCOPECOORD|JOF_NAME|JOF_SET|JOF_DETECTING) \
    \
    






 \
    macro(JSOP_CHECKLEXICAL,  138, "checklexical", NULL,     4,  0,  0, JOF_LOCAL|JOF_NAME) \
    






 \
    macro(JSOP_INITLEXICAL,   139, "initlexical",  NULL,      4,  1,  1, JOF_LOCAL|JOF_NAME|JOF_SET|JOF_DETECTING) \
    






 \
    macro(JSOP_CHECKALIASEDLEXICAL, 140, "checkaliasedlexical", NULL, 5,  0,  0, JOF_SCOPECOORD|JOF_NAME) \
    






 \
    macro(JSOP_INITALIASEDLEXICAL,  141, "initaliasedlexical",  NULL, 5,  1,  1, JOF_SCOPECOORD|JOF_NAME|JOF_SET|JOF_DETECTING) \
    








 \
    macro(JSOP_UNINITIALIZED, 142, "uninitialized", NULL, 1,  0,  1, JOF_BYTE) \
    










 \
    macro(JSOP_GETINTRINSIC,  143, "getintrinsic",  NULL, 5,  0,  1, JOF_ATOM|JOF_NAME|JOF_TYPESET) \
    








 \
    macro(JSOP_SETINTRINSIC,  144, "setintrinsic",  NULL, 5,  2,  1, JOF_ATOM|JOF_NAME|JOF_SET|JOF_DETECTING) \
    





 \
    macro(JSOP_BINDINTRINSIC, 145, "bindintrinsic", NULL, 5,  0,  1, JOF_ATOM|JOF_NAME|JOF_SET) \
    








 \
    macro(JSOP_INITLOCKEDPROP, 146, "initlockedprop", NULL, 5,  2,  1, JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING) \
    








 \
    macro(JSOP_INITHIDDENPROP, 147,"inithiddenprop", NULL, 5,  2,  1,  JOF_ATOM|JOF_PROP|JOF_SET|JOF_DETECTING) \
     \
    macro(JSOP_UNUSED148,     148,"unused148", NULL,      1,  0,  0,  JOF_BYTE) \
    \
    






 \
    macro(JSOP_BACKPATCH,     149,"backpatch", NULL,      5,  0,  0,  JOF_JUMP) \
    macro(JSOP_UNUSED150,     150,"unused150", NULL,      1,  0,  0,  JOF_BYTE) \
    \
    








 \
    macro(JSOP_THROWING,      151,"throwing", NULL,       1,  1,  0,  JOF_BYTE) \
    \
    






 \
    macro(JSOP_SETRVAL,       152,"setrval",    NULL,       1,  1,  0,  JOF_BYTE) \
    









 \
    macro(JSOP_RETRVAL,       153,"retrval",    NULL,       1,  0,  0,  JOF_BYTE) \
    \
    










 \
    macro(JSOP_GETGNAME,      154,"getgname",  NULL,       5,  0,  1, JOF_ATOM|JOF_NAME|JOF_TYPESET|JOF_GNAME) \
    









 \
    macro(JSOP_SETGNAME,      155,"setgname",  NULL,       5,  2,  1, JOF_ATOM|JOF_NAME|JOF_SET|JOF_DETECTING|JOF_GNAME|JOF_CHECKSLOPPY) \
    \
    










 \
    macro(JSOP_STRICTSETGNAME, 156, "strict-setgname",  NULL,       5,  2,  1, JOF_ATOM|JOF_NAME|JOF_SET|JOF_DETECTING|JOF_GNAME|JOF_CHECKSTRICT) \
    








                                                                 \
    macro(JSOP_GIMPLICITTHIS, 157, "gimplicitthis", "",      5,  0,  1,  JOF_ATOM) \
    






 \
    macro(JSOP_SETELEM_SUPER,   158, "setelem-super", NULL, 1,  4,  1, JOF_BYTE |JOF_ELEM|JOF_SET|JOF_DETECTING|JOF_CHECKSLOPPY) \
    






 \
    macro(JSOP_STRICTSETELEM_SUPER, 159, "strict-setelem-super", NULL, 1,  4, 1, JOF_BYTE |JOF_ELEM|JOF_SET|JOF_DETECTING|JOF_CHECKSTRICT) \
    \
    






 \
    macro(JSOP_REGEXP,        160,"regexp",   NULL,       5,  0,  1, JOF_REGEXP) \
    \
    macro(JSOP_UNUSED161,     161,"unused161",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED162,     162,"unused162",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED163,     163,"unused163",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED164,     164,"unused164",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED165,     165,"unused165",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED166,     166,"unused166",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED167,     167,"unused167",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED168,     168,"unused168",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED169,     169,"unused169",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED170,     170,"unused170",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED171,     171,"unused171",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED172,     172,"unused172",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED173,     173,"unused173",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED174,     174,"unused174",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED175,     175,"unused175",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED176,     176,"unused176",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED177,     177,"unused177",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED178,     178,"unused178",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED179,     179,"unused179",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED180,     180,"unused180",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED181,     181,"unused181",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED182,     182,"unused182",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED183,     183,"unused183",  NULL,     1,  0,  0,  JOF_BYTE) \
    \
    







 \
    macro(JSOP_CALLPROP,      184,"callprop",   NULL,     5,  1,  1, JOF_ATOM|JOF_PROP|JOF_TYPESET) \
    \
    macro(JSOP_UNUSED185,     185,"unused185",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED186,     186,"unused186",  NULL,     1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED187,     187,"unused187",  NULL,     1,  0,  0,  JOF_BYTE) \
    \
    





 \
    macro(JSOP_UINT24,        188,"uint24",     NULL,     4,  0,  1, JOF_UINT24) \
    \
    macro(JSOP_UNUSED189,     189,"unused189",   NULL,    1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED190,     190,"unused190",   NULL,    1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED191,     191,"unused191",   NULL,    1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED192,     192,"unused192",   NULL,    1,  0,  0,  JOF_BYTE) \
    \
    








 \
    macro(JSOP_CALLELEM,      193, "callelem",   NULL,    1,  2,  1, JOF_BYTE |JOF_ELEM|JOF_TYPESET|JOF_LEFTASSOC) \
    \
    









 \
    macro(JSOP_MUTATEPROTO,   194, "mutateproto",NULL,    1,  2,  1, JOF_BYTE) \
    \
    






 \
    macro(JSOP_GETXPROP,      195,"getxprop",    NULL,    5,  1,  1, JOF_ATOM|JOF_PROP|JOF_TYPESET) \
    \
    








 \
    macro(JSOP_TYPEOFEXPR,    196,"typeofexpr",  NULL,    1,  1,  1, JOF_BYTE|JOF_DETECTING) \
    \
     \
    









 \
    macro(JSOP_FRESHENBLOCKSCOPE,197,"freshenblockscope", NULL, 1, 0, 0, JOF_BYTE) \
    





 \
    macro(JSOP_PUSHBLOCKSCOPE,198,"pushblockscope", NULL, 5,  0,  0,  JOF_OBJECT) \
    





 \
    macro(JSOP_POPBLOCKSCOPE, 199,"popblockscope", NULL,  1,  0,  0,  JOF_BYTE) \
    





 \
    macro(JSOP_DEBUGLEAVEBLOCK, 200,"debugleaveblock", NULL, 1,  0,  0,  JOF_BYTE) \
    \
    






 \
    macro(JSOP_GENERATOR,     201,"generator",   NULL,    1,  0,  1,  JOF_BYTE) \
    






 \
    macro(JSOP_INITIALYIELD,  202,"initialyield", NULL,   4,  1,  1,  JOF_UINT24) \
    






 \
    macro(JSOP_YIELD,         203,"yield",       NULL,    4,  2,  1,  JOF_UINT24) \
    






 \
    macro(JSOP_FINALYIELDRVAL,204,"finalyieldrval",NULL,  1,  1,  0,  JOF_BYTE) \
    







 \
    macro(JSOP_RESUME,        205,"resume",      NULL,    3,  2,  1,  JOF_UINT8|JOF_INVOKE) \
    








 \
    macro(JSOP_ARRAYPUSH,     206,"arraypush",   NULL,    1,  2,  0,  JOF_BYTE) \
    \
    






 \
    macro(JSOP_FORCEINTERPRETER, 207, "forceinterpreter", NULL,  1,  0,  0,  JOF_BYTE) \
    \
    







 \
    macro(JSOP_DEBUGAFTERYIELD,  208, "debugafteryield",  NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED209,     209, "unused209",    NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED210,     210, "unused210",    NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED211,     211, "unused211",    NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED212,     212, "unused212",    NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED213,     213, "unused213",    NULL,  1,  0,  0,  JOF_BYTE) \
    








 \
    macro(JSOP_BINDGNAME,     214, "bindgname",    NULL,  5,  0,  1,  JOF_ATOM|JOF_NAME|JOF_SET|JOF_GNAME) \
    \
    





 \
    macro(JSOP_INT8,          215, "int8",         NULL,  2,  0,  1, JOF_INT8) \
    





 \
    macro(JSOP_INT32,         216, "int32",        NULL,  5,  0,  1, JOF_INT32) \
    \
    






 \
    macro(JSOP_LENGTH,        217, "length",       NULL,  5,  1,  1, JOF_ATOM|JOF_PROP|JOF_TYPESET) \
    \
    








 \
    macro(JSOP_HOLE,          218, "hole",         NULL,  1,  0,  1,  JOF_BYTE) \
    \
    macro(JSOP_UNUSED219,     219,"unused219",     NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED220,     220,"unused220",     NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED221,     221,"unused221",     NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED222,     222,"unused222",     NULL,  1,  0,  0,  JOF_BYTE) \
    macro(JSOP_UNUSED223,     223,"unused223",     NULL,  1,  0,  0,  JOF_BYTE) \
    \
    






 \
    macro(JSOP_REST,          224, "rest",         NULL,  1,  0,  1,  JOF_BYTE|JOF_TYPESET) \
    \
    






 \
    macro(JSOP_TOID,          225, "toid",         NULL,  1,  1,  1,  JOF_BYTE) \
    \
    






                                                                 \
    macro(JSOP_IMPLICITTHIS,  226, "implicitthis", "",    5,  0,  1,  JOF_ATOM) \
    \
    










 \
    macro(JSOP_LOOPENTRY,     227, "loopentry",    NULL,  2,  0,  0,  JOF_UINT8) \
    




 \
    macro(JSOP_TOSTRING,    228, "tostring",       NULL,  1,  1,  1,  JOF_BYTE)





#define FOR_EACH_TRAILING_UNUSED_OPCODE(macro) \
    macro(229) \
    macro(230) \
    macro(231) \
    macro(232) \
    macro(233) \
    macro(234) \
    macro(235) \
    macro(236) \
    macro(237) \
    macro(238) \
    macro(239) \
    macro(240) \
    macro(241) \
    macro(242) \
    macro(243) \
    macro(244) \
    macro(245) \
    macro(246) \
    macro(247) \
    macro(248) \
    macro(249) \
    macro(250) \
    macro(251) \
    macro(252) \
    macro(253) \
    macro(254) \
    macro(255)

namespace js {





#define VALUE_AND_VALUE_PLUS_ONE(op, val, ...) \
    val) && (val + 1 ==
#define TRAILING_VALUE_AND_VALUE_PLUS_ONE(val) \
    val) && (val + 1 ==
static_assert((0 ==
               FOR_EACH_OPCODE(VALUE_AND_VALUE_PLUS_ONE)
               FOR_EACH_TRAILING_UNUSED_OPCODE(TRAILING_VALUE_AND_VALUE_PLUS_ONE)
               256),
              "opcode values and trailing unused opcode values monotonically "
              "increase from zero to 255");
#undef TRAILING_VALUE_AND_VALUE_PLUS_ONE
#undef VALUE_AND_VALUE_PLUS_ONE


#define DEFINE_LENGTH_CONSTANT(op, val, name, image, len, ...) \
    MOZ_CONSTEXPR_VAR size_t op##_LENGTH = len;
FOR_EACH_OPCODE(DEFINE_LENGTH_CONSTANT)
#undef DEFINE_LENGTH_CONSTANT

} 

#endif 
