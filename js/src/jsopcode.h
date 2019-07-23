






































#ifndef jsopcode_h___
#define jsopcode_h___



#include <stddef.h>
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C




typedef enum JSOp {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    op = val,
#include "jsopcode.tbl"
#undef OPDEF
    JSOP_LIMIT
} JSOp;

typedef enum JSOpLength {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    op##_LENGTH = length,
#include "jsopcode.tbl"
#undef OPDEF
    JSOP_LIMIT_LENGTH
} JSOpLength;




#define JOF_BYTE          0       /* single bytecode, no immediates */
#define JOF_JUMP          1       /* signed 16-bit jump offset immediate */
#define JOF_CONST         2       /* unsigned 16-bit constant pool index */
#define JOF_UINT16        3       /* unsigned 16-bit immediate operand */
#define JOF_TABLESWITCH   4       /* table switch */
#define JOF_LOOKUPSWITCH  5       /* lookup switch */
#define JOF_QARG          6       /* quickened get/set function argument ops */
#define JOF_QVAR          7       /* quickened get/set local variable ops */
#define JOF_INDEXCONST    8       /* uint16 slot index + constant pool index */
#define JOF_JUMPX         9       /* signed 32-bit jump offset immediate */
#define JOF_TABLESWITCHX  10      /* extended (32-bit offset) table switch */
#define JOF_LOOKUPSWITCHX 11      /* extended (32-bit offset) lookup switch */
#define JOF_UINT24        12      /* extended unsigned 24-bit literal (index) */
#define JOF_2BYTE         13      /* 2-byte opcode, e.g., upper 8 bits of 24-bit
                                     atom index */
#define JOF_LOCAL         14      /* block-local operand stack variable */
#define JOF_OBJECT        15      /* unsigned 16-bit object pool index */
#define JOF_INDEXOBJECT   16      /* uint16 slot index + object pool index */
#define JOF_REGEXP        17      /* unsigned 16-bit regexp pool index */
#define JOF_TYPEMASK      0x001f  /* mask for above immediate types */
#define JOF_NAME          (1U<<5) /* name operation */
#define JOF_PROP          (2U<<5) /* obj.prop operation */
#define JOF_ELEM          (3U<<5) /* obj[index] operation */
#define JOF_XMLNAME       (4U<<5) /* XML name: *, a::b, @a, @a::b, etc. */
#define JOF_VARPROP       (5U<<5) /* x.prop for arg, var, or local x */
#define JOF_MODEMASK      (7U<<5) /* mask for above addressing modes */
#define JOF_SET           (1U<<8) /* set (i.e., assignment) operation */
#define JOF_DEL           (1U<<9) /* delete operation */
#define JOF_DEC          (1U<<10) /* decrement (--, not ++) opcode */
#define JOF_INC          (2U<<10) /* increment (++, not --) opcode */
#define JOF_INCDEC       (3U<<10) /* increment or decrement opcode */
#define JOF_POST         (1U<<12) /* postorder increment or decrement */
#define JOF_IMPORT       (1U<<13) /* import property op */
#define JOF_FOR          (1U<<14) /* for-in property op */
#define JOF_ASSIGNING     JOF_SET /* hint for JSClass.resolve, used for ops
                                     that do simplex assignment */
#define JOF_DETECTING    (1U<<15) /* object detection for JSNewResolveOp */
#define JOF_BACKPATCH    (1U<<16) /* backpatch placeholder during codegen */
#define JOF_LEFTASSOC    (1U<<17) /* left-associative operator */
#define JOF_DECLARING    (1U<<18) /* var, const, or function declaration op */
#define JOF_INDEXBASE    (1U<<19) /* atom segment base setting prefix op */
#define JOF_CALLOP       (1U<<20) /* call operation that pushes function and
                                     this */
#define JOF_PARENHEAD    (1U<<21) 

#define JOF_INVOKE       (1U<<22) /* JSOP_CALL, JSOP_NEW, JSOP_EVAL */
#define JOF_TMPSLOT      (1U<<23) /* interpreter uses extra temporray slot
                                     to root intermediate objects */
#define JOF_TMPSLOT_SHIFT 23


#define JOF_MODE(fmt)   ((fmt) & JOF_MODEMASK)
#define JOF_OPMODE(op)  JOF_MODE(js_CodeSpec[op].format)

#define JOF_TYPE_IS_EXTENDED_JUMP(t) \
    ((unsigned)((t) - JOF_JUMPX) <= (unsigned)(JOF_LOOKUPSWITCHX - JOF_JUMPX))






#define UINT16_LEN              2
#define UINT16_HI(i)            ((jsbytecode)((i) >> 8))
#define UINT16_LO(i)            ((jsbytecode)(i))
#define GET_UINT16(pc)          ((uintN)(((pc)[1] << 8) | (pc)[2]))
#define SET_UINT16(pc,i)        ((pc)[1] = UINT16_HI(i), (pc)[2] = UINT16_LO(i))
#define UINT16_LIMIT            ((uintN)1 << 16)


#define JUMP_OFFSET_LEN         2
#define JUMP_OFFSET_HI(off)     ((jsbytecode)((off) >> 8))
#define JUMP_OFFSET_LO(off)     ((jsbytecode)(off))
#define GET_JUMP_OFFSET(pc)     ((int16)GET_UINT16(pc))
#define SET_JUMP_OFFSET(pc,off) ((pc)[1] = JUMP_OFFSET_HI(off),               \
                                 (pc)[2] = JUMP_OFFSET_LO(off))
#define JUMP_OFFSET_MIN         ((int16)0x8000)
#define JUMP_OFFSET_MAX         ((int16)0x7fff)












#define GET_SPANDEP_INDEX(pc)   ((uint16)GET_UINT16(pc))
#define SET_SPANDEP_INDEX(pc,i) ((pc)[1] = JUMP_OFFSET_HI(i),                 \
                                 (pc)[2] = JUMP_OFFSET_LO(i))
#define SPANDEP_INDEX_MAX       ((uint16)0xfffe)
#define SPANDEP_INDEX_HUGE      ((uint16)0xffff)


#define JUMPX_OFFSET_LEN        4
#define JUMPX_OFFSET_B3(off)    ((jsbytecode)((off) >> 24))
#define JUMPX_OFFSET_B2(off)    ((jsbytecode)((off) >> 16))
#define JUMPX_OFFSET_B1(off)    ((jsbytecode)((off) >> 8))
#define JUMPX_OFFSET_B0(off)    ((jsbytecode)(off))
#define GET_JUMPX_OFFSET(pc)    ((int32)(((pc)[1] << 24) | ((pc)[2] << 16)    \
                                         | ((pc)[3] << 8) | (pc)[4]))
#define SET_JUMPX_OFFSET(pc,off)((pc)[1] = JUMPX_OFFSET_B3(off),              \
                                 (pc)[2] = JUMPX_OFFSET_B2(off),              \
                                 (pc)[3] = JUMPX_OFFSET_B1(off),              \
                                 (pc)[4] = JUMPX_OFFSET_B0(off))
#define JUMPX_OFFSET_MIN        ((int32)0x80000000)
#define JUMPX_OFFSET_MAX        ((int32)0x7fffffff)








#define INDEX_LEN               2
#define INDEX_HI(i)             ((jsbytecode)((i) >> 8))
#define INDEX_LO(i)             ((jsbytecode)(i))
#define GET_INDEX(pc)           GET_UINT16(pc)
#define SET_INDEX(pc,i)         ((pc)[1] = INDEX_HI(i), (pc)[2] = INDEX_LO(i))

#define GET_INDEXBASE(pc)       (JS_ASSERT(*(pc) == JSOP_INDEXBASE),          \
                                 ((uintN)((pc)[1])) << 16)
#define INDEXBASE_LEN           1

#define UINT24_HI(i)            ((jsbytecode)((i) >> 16))
#define UINT24_MID(i)           ((jsbytecode)((i) >> 8))
#define UINT24_LO(i)            ((jsbytecode)(i))
#define GET_UINT24(pc)          ((jsatomid)(((pc)[1] << 16) |                 \
                                            ((pc)[2] << 8) |                  \
                                            (pc)[3]))
#define SET_UINT24(pc,i)        ((pc)[1] = UINT24_HI(i),                      \
                                 (pc)[2] = UINT24_MID(i),                     \
                                 (pc)[3] = UINT24_LO(i))


#define INDEX_LIMIT_LOG2        23
#define INDEX_LIMIT             ((uint32)1 << INDEX_LIMIT_LOG2)

JS_STATIC_ASSERT(sizeof(uint32) * JS_BITS_PER_BYTE >= INDEX_LIMIT_LOG2 + 1);


#define ARGC_HI(argc)           UINT16_HI(argc)
#define ARGC_LO(argc)           UINT16_LO(argc)
#define GET_ARGC(pc)            GET_UINT16(pc)
#define ARGC_LIMIT              UINT16_LIMIT


#define GET_ARGNO(pc)           GET_UINT16(pc)
#define SET_ARGNO(pc,argno)     SET_UINT16(pc,argno)
#define ARGNO_LEN               2
#define ARGNO_LIMIT             UINT16_LIMIT

#define GET_VARNO(pc)           GET_UINT16(pc)
#define SET_VARNO(pc,varno)     SET_UINT16(pc,varno)
#define VARNO_LEN               2
#define VARNO_LIMIT             UINT16_LIMIT

struct JSCodeSpec {
    int8                length;         
    int8                nuses;          
    int8                ndefs;          
    uint8               prec;           
    uint32              format;         
};

extern const JSCodeSpec js_CodeSpec[];
extern uintN            js_NumCodeSpecs;
extern const char       js_EscapeMap[];






extern JSString *
js_QuoteString(JSContext *cx, JSString *str, jschar quote);







#ifdef JS_ARENAMETER
# define JS_NEW_PRINTER(cx, name, indent, pretty)                              \
    js_NewPrinter(cx, name, indent, pretty)
#else
# define JS_NEW_PRINTER(cx, name, indent, pretty)                              \
    js_NewPrinter(cx, indent, pretty)
#endif

extern JSPrinter *
JS_NEW_PRINTER(JSContext *cx, const char *name, uintN indent, JSBool pretty);

extern void
js_DestroyPrinter(JSPrinter *jp);

extern JSString *
js_GetPrinterOutput(JSPrinter *jp);

extern int
js_printf(JSPrinter *jp, const char *format, ...);

extern JSBool
js_puts(JSPrinter *jp, const char *s);





uintN
js_GetIndexFromBytecode(JSScript *script, jsbytecode *pc, ptrdiff_t pcoff);





#define GET_ATOM_FROM_BYTECODE(script, pc, pcoff, atom)                       \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        JS_GET_SCRIPT_ATOM((script), index_, atom);                           \
    JS_END_MACRO

#define GET_OBJECT_FROM_BYTECODE(script, pc, pcoff, obj)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        JS_GET_SCRIPT_OBJECT((script), index_, obj);                          \
    JS_END_MACRO

#define GET_FUNCTION_FROM_BYTECODE(script, pc, pcoff, obj)                    \
    JS_BEGIN_MACRO                                                            \
        GET_OBJECT_FROM_BYTECODE(script, pc, pcoff, obj);                     \
        JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_FunctionClass);               \
    JS_END_MACRO

#define GET_REGEXP_FROM_BYTECODE(script, pc, pcoff, obj)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode((script), (pc), (pcoff));      \
        JS_GET_SCRIPT_REGEXP((script), index_, obj);                          \
    JS_END_MACRO

#ifdef DEBUG



#include <stdio.h>

extern JS_FRIEND_API(JSBool)
js_Disassemble(JSContext *cx, JSScript *script, JSBool lines, FILE *fp);

extern JS_FRIEND_API(uintN)
js_Disassemble1(JSContext *cx, JSScript *script, jsbytecode *pc, uintN loc,
                JSBool lines, FILE *fp);
#endif 




extern JSBool
js_DecompileCode(JSPrinter *jp, JSScript *script, jsbytecode *pc, uintN len,
                 uintN pcdepth);

extern JSBool
js_DecompileScript(JSPrinter *jp, JSScript *script);

extern JSBool
js_DecompileFunctionBody(JSPrinter *jp, JSFunction *fun);

extern JSBool
js_DecompileFunction(JSPrinter *jp, JSFunction *fun);













extern char *
js_DecompileValueGenerator(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback);

#define JSDVG_IGNORE_STACK      0
#define JSDVG_SEARCH_STACK      1

JS_END_EXTERN_C

#endif
