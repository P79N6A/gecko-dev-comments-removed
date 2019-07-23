






































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
#define JOF_TYPEMASK      0x000f  /* mask for above immediate types */
#define JOF_NAME          0x0010  /* name operation */
#define JOF_PROP          0x0020  /* obj.prop operation */
#define JOF_ELEM          0x0030  /* obj[index] operation */
#define JOF_XMLNAME       0x0040  /* XML name: *, a::b, @a, @a::b, etc. */
#define JOF_VARPROP       0x0050  /* x.prop for arg, var, or local x */
#define JOF_MODEMASK      0x0070  /* mask for above addressing modes */
#define JOF_SET           0x0080  /* set (i.e., assignment) operation */
#define JOF_DEL           0x0100  /* delete operation */
#define JOF_DEC           0x0200  /* decrement (--, not ++) opcode */
#define JOF_INC           0x0400  /* increment (++, not --) opcode */
#define JOF_INCDEC        0x0600  /* increment or decrement opcode */
#define JOF_POST          0x0800  /* postorder increment or decrement */
#define JOF_IMPORT        0x1000  /* import property op */
#define JOF_FOR           0x2000  /* for-in property op */
#define JOF_ASSIGNING     JOF_SET /* hint for JSClass.resolve, used for ops
                                     that do simplex assignment */
#define JOF_DETECTING     0x4000  /* object detection flag for JSNewResolveOp */
#define JOF_BACKPATCH     0x8000  /* backpatch placeholder during codegen */
#define JOF_LEFTASSOC    0x10000  /* left-associative operator */
#define JOF_DECLARING    0x20000  /* var, const, or function declaration op */
#define JOF_ATOMBASE     0x40000  /* atom segment base setting prefix op */
#define JOF_CALLOP       0x80000  /* call operation pushing function and this */
#define JOF_PARENHEAD   0x100000  /* opcode consumes value of expression in
                                     parenthesized statement head */
#define JOF_INVOKE      0x200000  /* JSOP_CALL, JSOP_NEW, JSOP_EVAL */

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







#define ATOM_INDEX_LEN          2
#define ATOM_INDEX_HI(i)        ((jsbytecode)((i) >> 8))
#define ATOM_INDEX_LO(i)        ((jsbytecode)(i))
#define GET_ATOM_INDEX(pc)      GET_UINT16(pc)
#define SET_ATOM_INDEX(pc,i)    ((pc)[1] = ATOM_INDEX_HI(i),                  \
                                 (pc)[2] = ATOM_INDEX_LO(i))

#define ASSERT_ATOM_INDEX_IN_MAP(script,atoms,index)                          \
    JS_ASSERT((size_t)((atoms) - (script)->atomMap.vector) <                  \
              (size_t)(script)->atomMap.length - (size_t)(index))

#define GET_ATOM(script,atoms,pc)                                             \
    (ASSERT_ATOM_INDEX_IN_MAP(script,atoms,GET_ATOM_INDEX(pc)),               \
     (atoms)[GET_ATOM_INDEX(pc)])

#define GET_ATOMBASE(pc)        (JS_ASSERT(*(pc) == JSOP_ATOMBASE),           \
                                 ((uintN)((pc)[1])) << 16)
#define ATOMBASE_LEN            1


#define UINT24_HI(i)            ((jsbytecode)((i) >> 16))
#define UINT24_MID(i)           ((jsbytecode)((i) >> 8))
#define UINT24_LO(i)            ((jsbytecode)(i))
#define GET_UINT24(pc)          ((jsatomid)(((pc)[1] << 16) |                 \
                                            ((pc)[2] << 8) |                  \
                                            (pc)[3]))
#define SET_UINT24(pc,i)        ((pc)[1] = UINT24_HI(i),                      \
                                 (pc)[2] = UINT24_MID(i),                     \
                                 (pc)[3] = UINT24_LO(i))


#define ATOM_INDEX_LIMIT_LOG2   23
#define ATOM_INDEX_LIMIT        ((uint32)1 << ATOM_INDEX_LIMIT_LOG2)

JS_STATIC_ASSERT(sizeof(jsatomid) * JS_BITS_PER_BYTE >=
                 ATOM_INDEX_LIMIT_LOG2 + 1);


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





extern JSAtom*
js_GetAtomFromBytecode(JSScript *script, jsbytecode *pc, ptrdiff_t pcoff);

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
