






































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




#define JOF_BYTE          0       /* single bytecode, no immediates */
#define JOF_JUMP          1       /* signed 16-bit jump offset immediate */
#define JOF_ATOM          2       /* unsigned 16-bit constant index */
#define JOF_UINT16        3       /* unsigned 16-bit immediate operand */
#define JOF_TABLESWITCH   4       /* table switch */
#define JOF_LOOKUPSWITCH  5       /* lookup switch */
#define JOF_QARG          6       /* quickened get/set function argument ops */
#define JOF_LOCAL         7       /* var or block-local variable */
#define JOF_SLOTATOM      8       /* uint16 slot + constant index */
#define JOF_JUMPX         9       /* signed 32-bit jump offset immediate */
#define JOF_TABLESWITCHX  10      /* extended (32-bit offset) table switch */
#define JOF_LOOKUPSWITCHX 11      /* extended (32-bit offset) lookup switch */
#define JOF_UINT24        12      /* extended unsigned 24-bit literal (index) */
#define JOF_UINT8         13      /* uint8 immediate, e.g. top 8 bits of 24-bit
                                     atom index */
#define JOF_INT32         14      /* int32 immediate operand */
#define JOF_OBJECT        15      /* unsigned 16-bit object index */
#define JOF_SLOTOBJECT    16      /* uint16 slot index + object index */
#define JOF_REGEXP        17      /* unsigned 16-bit regexp index */
#define JOF_INT8          18      /* int8 immediate operand */
#define JOF_ATOMOBJECT    19      /* uint16 constant index + object index */
#define JOF_UINT16PAIR    20      /* pair of uint16 immediates */
#define JOF_TYPEMASK      0x001f  /* mask for above immediate types */

#define JOF_NAME          (1U<<5) /* name operation */
#define JOF_PROP          (2U<<5) /* obj.prop operation */
#define JOF_ELEM          (3U<<5) /* obj[index] operation */
#define JOF_XMLNAME       (4U<<5) /* XML name: *, a::b, @a, @a::b, etc. */
#define JOF_VARPROP       (5U<<5) /* x.prop for this, arg, var, or local x */
#define JOF_MODEMASK      (7U<<5) /* mask for above addressing modes */
#define JOF_SET           (1U<<8) /* set (i.e., assignment) operation */
#define JOF_DEL           (1U<<9) /* delete operation */
#define JOF_DEC          (1U<<10) /* decrement (--, not ++) opcode */
#define JOF_INC          (2U<<10) /* increment (++, not --) opcode */
#define JOF_INCDEC       (3U<<10) /* increment or decrement opcode */
#define JOF_POST         (1U<<12) /* postorder increment or decrement */
#define JOF_FOR          (1U<<13) /* for-in property op (akin to JOF_SET) */
#define JOF_ASSIGNING     JOF_SET /* hint for JSClass.resolve, used for ops
                                     that do simplex assignment */
#define JOF_DETECTING    (1U<<14) /* object detection for JSNewResolveOp */
#define JOF_BACKPATCH    (1U<<15) /* backpatch placeholder during codegen */
#define JOF_LEFTASSOC    (1U<<16) /* left-associative operator */
#define JOF_DECLARING    (1U<<17) /* var, const, or function declaration op */
#define JOF_INDEXBASE    (1U<<18) /* atom segment base setting prefix op */
#define JOF_CALLOP       (1U<<19) /* call operation that pushes function and
                                     this */
#define JOF_PARENHEAD    (1U<<20) 

#define JOF_INVOKE       (1U<<21) /* JSOP_CALL, JSOP_NEW, JSOP_EVAL */
#define JOF_TMPSLOT      (1U<<22) /* interpreter uses extra temporary slot
                                     to root intermediate objects besides
                                     the slots opcode uses */
#define JOF_TMPSLOT2     (2U<<22) 

#define JOF_TMPSLOT_SHIFT 22
#define JOF_TMPSLOT_MASK  (JS_BITMASK(2) << JOF_TMPSLOT_SHIFT)

#define JOF_SHARPSLOT    (1U<<24) /* first immediate is uint16 stack slot no.
                                     that needs fixup when in global code (see
                                     JSCompiler::compileScript) */


#define JOF_TYPE(fmt)   ((fmt) & JOF_TYPEMASK)
#define JOF_OPTYPE(op)  JOF_TYPE(js_CodeSpec[op].format)


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

#define GET_INT8(pc)            ((jsint)(int8)(pc)[1])

#define GET_INT32(pc)           ((jsint)(((uint32)((pc)[1]) << 24) |          \
                                         ((uint32)((pc)[2]) << 16) |          \
                                         ((uint32)((pc)[3]) << 8)  |          \
                                         (uint32)(pc)[4]))
#define SET_INT32(pc,i)         ((pc)[1] = (jsbytecode)((uint32)(i) >> 24),   \
                                 (pc)[2] = (jsbytecode)((uint32)(i) >> 16),   \
                                 (pc)[3] = (jsbytecode)((uint32)(i) >> 8),    \
                                 (pc)[4] = (jsbytecode)(uint32)(i))


#define INDEX_LIMIT_LOG2        23
#define INDEX_LIMIT             ((uint32)1 << INDEX_LIMIT_LOG2)


#define ARGC_HI(argc)           UINT16_HI(argc)
#define ARGC_LO(argc)           UINT16_LO(argc)
#define GET_ARGC(pc)            GET_UINT16(pc)
#define ARGC_LIMIT              UINT16_LIMIT


#define GET_ARGNO(pc)           GET_UINT16(pc)
#define SET_ARGNO(pc,argno)     SET_UINT16(pc,argno)
#define ARGNO_LEN               2
#define ARGNO_LIMIT             UINT16_LIMIT

#define GET_SLOTNO(pc)          GET_UINT16(pc)
#define SET_SLOTNO(pc,varno)    SET_UINT16(pc,varno)
#define SLOTNO_LEN              2
#define SLOTNO_LIMIT            UINT16_LIMIT

struct JSCodeSpec {
    int8                length;         
    int8                nuses;          
    int8                ndefs;          
    uint8               prec;           
    uint32              format;         
};

extern const JSCodeSpec js_CodeSpec[];
extern uintN            js_NumCodeSpecs;
extern const char       *js_CodeName[];
extern const char       js_EscapeMap[];






extern JSString *
js_QuoteString(JSContext *cx, JSString *str, jschar quote);












extern JSPrinter *
js_NewPrinter(JSContext *cx, const char *name, JSFunction *fun,
              uintN indent, JSBool pretty, JSBool grouped, JSBool strict);

extern void
js_DestroyPrinter(JSPrinter *jp);

extern JSString *
js_GetPrinterOutput(JSPrinter *jp);

extern int
js_printf(JSPrinter *jp, const char *format, ...);

extern JSBool
js_puts(JSPrinter *jp, const char *s);








uintN
js_GetIndexFromBytecode(JSContext *cx, JSScript *script, jsbytecode *pc,
                        ptrdiff_t pcoff);





#define GET_ATOM_FROM_BYTECODE(script, pc, pcoff, atom)                       \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(*(pc) != JSOP_DOUBLE);                                      \
        uintN index_ = js_GetIndexFromBytecode(cx, (script), (pc), (pcoff));  \
        JS_GET_SCRIPT_ATOM(script, pc, index_, atom);                         \
    JS_END_MACRO











#define GET_DOUBLE_FROM_BYTECODE(script, pc, pcoff, atom)                     \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode(cx, (script), (pc), (pcoff));  \
        JS_ASSERT(index_ < (script)->atomMap.length);                         \
        (atom) = (script)->atomMap.vector[index_];                            \
        JS_ASSERT(ATOM_IS_DOUBLE(atom));                                      \
    JS_END_MACRO

#define GET_OBJECT_FROM_BYTECODE(script, pc, pcoff, obj)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode(cx, (script), (pc), (pcoff));  \
        obj = (script)->getObject(index_);                                    \
    JS_END_MACRO

#define GET_FUNCTION_FROM_BYTECODE(script, pc, pcoff, fun)                    \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode(cx, (script), (pc), (pcoff));  \
        fun = (script)->getFunction(index_);                                  \
    JS_END_MACRO

#define GET_REGEXP_FROM_BYTECODE(script, pc, pcoff, obj)                      \
    JS_BEGIN_MACRO                                                            \
        uintN index_ = js_GetIndexFromBytecode(cx, (script), (pc), (pcoff));  \
        obj = (script)->getRegExp(index_);                                    \
    JS_END_MACRO




extern uintN
js_GetVariableBytecodeLength(jsbytecode *pc);





extern uintN
js_GetVariableStackUses(JSOp op, jsbytecode *pc);





extern uintN
js_GetEnterBlockStackDefs(JSContext *cx, JSScript *script, jsbytecode *pc);

#ifdef __cplusplus 
static JS_INLINE uintN
js_GetStackUses(const JSCodeSpec *cs, JSOp op, jsbytecode *pc)
{
    JS_ASSERT(cs == &js_CodeSpec[op]);
    if (cs->nuses >= 0)
        return cs->nuses;
    return js_GetVariableStackUses(op, pc);
}

static JS_INLINE uintN
js_GetStackDefs(JSContext *cx, const JSCodeSpec *cs, JSOp op, JSScript *script,
                jsbytecode *pc)
{
    JS_ASSERT(cs == &js_CodeSpec[op]);
    if (cs->ndefs >= 0)
        return cs->ndefs;

    
    JS_ASSERT(op == JSOP_ENTERBLOCK);
    return js_GetEnterBlockStackDefs(cx, script, pc);
}
#endif

#ifdef DEBUG



#include <stdio.h>

extern JS_FRIEND_API(JSBool)
js_Disassemble(JSContext *cx, JSScript *script, JSBool lines, FILE *fp);

extern JS_FRIEND_API(uintN)
js_Disassemble1(JSContext *cx, JSScript *script, jsbytecode *pc, uintN loc,
                JSBool lines, FILE *fp);
#endif 




extern JSBool
js_DecompileScript(JSPrinter *jp, JSScript *script);

extern JSBool
js_DecompileFunctionBody(JSPrinter *jp);

extern JSBool
js_DecompileFunction(JSPrinter *jp);

extern JSString *
js_DecompileToString(JSContext *cx, const char *name, JSFunction *fun,
                     uintN indent, JSBool pretty, JSBool grouped, JSBool strict,
                     JSBool (*decompiler)(JSPrinter *jp));













extern char *
js_DecompileValueGenerator(JSContext *cx, intN spindex, jsval v,
                           JSString *fallback);

#define JSDVG_IGNORE_STACK      0
#define JSDVG_SEARCH_STACK      1





extern uintN
js_ReconstructStackDepth(JSContext *cx, JSScript *script, jsbytecode *pc);

JS_END_EXTERN_C

#endif
