







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"

JS_BEGIN_EXTERN_C

typedef struct JSFrameRegs {
    jsbytecode      *pc;            
    jsval           *sp;            
} JSFrameRegs;











struct JSStackFrame {
    JSFrameRegs     *regs;
    jsbytecode      *imacpc;        
    jsval           *slots;         
    JSObject        *callobj;       
    jsval           argsobj;        

    JSObject        *varobj;        
    JSScript        *script;        
    JSFunction      *fun;           
    JSObject        *thisp;         
    uintN           argc;           
    jsval           *argv;          
    jsval           rval;           
    JSStackFrame    *down;          
    void            *annotation;    

    



































    JSObject        *scopeChain;
    JSObject        *blockChain;

    uint32          flags;          
    JSStackFrame    *dormantNext;   
    JSStackFrame    *displaySave;   


    inline void assertValidStackDepth(uintN depth);

    void putActivationObjects(JSContext *cx) {
        



        if (callobj) {
            js_PutCallObject(cx, this);
            JS_ASSERT(!argsobj);
        } else if (argsobj) {
            js_PutArgsObject(cx, this);
        }
    }

    JSObject *callee() {
        return argv ? JSVAL_TO_OBJECT(argv[-2]) : NULL;
    }
};

#ifdef __cplusplus
static JS_INLINE uintN
FramePCOffset(JSStackFrame* fp)
{
    return uintN((fp->imacpc ? fp->imacpc : fp->regs->pc) - fp->script->code);
}
#endif

static JS_INLINE jsval *
StackBase(JSStackFrame *fp)
{
    return fp->slots + fp->script->nfixed;
}

void
JSStackFrame::assertValidStackDepth(uintN depth)
{
    JS_ASSERT(0 <= regs->sp - StackBase(this));
    JS_ASSERT(depth <= uintptr_t(regs->sp - StackBase(this)));
}

static JS_INLINE uintN
GlobalVarCount(JSStackFrame *fp)
{
    uintN n;

    JS_ASSERT(!fp->fun);
    n = fp->script->nfixed;
    if (fp->script->regexpsOffset != 0)
        n -= fp->script->regexps()->length;
    return n;
}

typedef struct JSInlineFrame {
    JSStackFrame    frame;          
    JSFrameRegs     callerRegs;     
    void            *mark;          
    void            *hookData;      
    JSVersion       callerVersion;  
} JSInlineFrame;


#define JSFRAME_CONSTRUCTING   0x01 /* frame is for a constructor invocation */
#define JSFRAME_COMPUTED_THIS  0x02 /* frame.thisp was computed already */
#define JSFRAME_ASSIGNING      0x04 /* a complex (not simplex JOF_ASSIGNING) op
                                       is currently assigning to a property */
#define JSFRAME_DEBUGGER       0x08 /* frame for JS_EvaluateInStackFrame */
#define JSFRAME_EVAL           0x10 /* frame for obj_eval */
#define JSFRAME_ROOTED_ARGV    0x20 /* frame.argv is rooted by the caller */
#define JSFRAME_YIELDING       0x40 /* js_Interpret dispatched JSOP_YIELD */
#define JSFRAME_ITERATOR       0x80 /* trying to get an iterator for for-in */
#define JSFRAME_GENERATOR     0x200 /* frame belongs to generator-iterator */
#define JSFRAME_OVERRIDE_ARGS 0x400 /* overridden arguments local variable */

#define JSFRAME_SPECIAL       (JSFRAME_DEBUGGER | JSFRAME_EVAL)






#define PROPERTY_CACHE_LOG2     12
#define PROPERTY_CACHE_SIZE     JS_BIT(PROPERTY_CACHE_LOG2)
#define PROPERTY_CACHE_MASK     JS_BITMASK(PROPERTY_CACHE_LOG2)






#define PROPERTY_CACHE_HASH(pc,kshape)                                        \
    (((((jsuword)(pc) >> PROPERTY_CACHE_LOG2) ^ (jsuword)(pc)) + (kshape)) &  \
     PROPERTY_CACHE_MASK)

#define PROPERTY_CACHE_HASH_PC(pc,kshape)                                     \
    PROPERTY_CACHE_HASH(pc, kshape)

#define PROPERTY_CACHE_HASH_ATOM(atom,obj)                                    \
    PROPERTY_CACHE_HASH((jsuword)(atom) >> 2, OBJ_SHAPE(obj))




#define PCVCAP_PROTOBITS        4
#define PCVCAP_PROTOSIZE        JS_BIT(PCVCAP_PROTOBITS)
#define PCVCAP_PROTOMASK        JS_BITMASK(PCVCAP_PROTOBITS)

#define PCVCAP_SCOPEBITS        4
#define PCVCAP_SCOPESIZE        JS_BIT(PCVCAP_SCOPEBITS)
#define PCVCAP_SCOPEMASK        JS_BITMASK(PCVCAP_SCOPEBITS)

#define PCVCAP_TAGBITS          (PCVCAP_PROTOBITS + PCVCAP_SCOPEBITS)
#define PCVCAP_TAGMASK          JS_BITMASK(PCVCAP_TAGBITS)
#define PCVCAP_TAG(t)           ((t) & PCVCAP_TAGMASK)

#define PCVCAP_MAKE(t,s,p)      ((uint32(t) << PCVCAP_TAGBITS) |              \
                                 ((s) << PCVCAP_PROTOBITS) |                  \
                                 (p))
#define PCVCAP_SHAPE(t)         ((t) >> PCVCAP_TAGBITS)

#define SHAPE_OVERFLOW_BIT      JS_BIT(32 - PCVCAP_TAGBITS)

struct JSPropCacheEntry {
    jsbytecode          *kpc;           
    jsuword             kshape;         
    jsuword             vcap;           
    jsuword             vword;          

    bool adding() const {
        return PCVCAP_TAG(vcap) == 0 && kshape != PCVCAP_SHAPE(vcap);
    }

    bool directHit() const {
        return PCVCAP_TAG(vcap) == 0 && kshape == PCVCAP_SHAPE(vcap);
    }
};





#define JS_NO_PROP_CACHE_FILL ((JSPropCacheEntry *) NULL + 1)

#if defined DEBUG_brendan || defined DEBUG_brendaneich
#define JS_PROPERTY_CACHE_METERING 1
#endif

typedef struct JSPropertyCache {
    JSPropCacheEntry    table[PROPERTY_CACHE_SIZE];
    JSBool              empty;
#ifdef JS_PROPERTY_CACHE_METERING
    JSPropCacheEntry    *pctestentry;   
    uint32              fills;          
    uint32              nofills;        
    uint32              rofills;        
    uint32              disfills;       
    uint32              oddfills;       
    uint32              modfills;       
    uint32              brandfills;     

    uint32              noprotos;       
    uint32              longchains;     
    uint32              recycles;       
    uint32              pcrecycles;     

    uint32              tests;          
    uint32              pchits;         
    uint32              protopchits;    
    uint32              initests;       
    uint32              inipchits;      
    uint32              inipcmisses;    
    uint32              settests;       
    uint32              addpchits;      
    uint32              setpchits;      
    uint32              setpcmisses;    
    uint32              slotchanges;    

    uint32              setmisses;      
    uint32              idmisses;       
    uint32              komisses;       
    uint32              vcmisses;       
    uint32              misses;         
    uint32              flushes;        
    uint32              pcpurges;       
# define PCMETER(x)     x
#else
# define PCMETER(x)     ((void)0)
#endif
} JSPropertyCache;




#define PCVAL_OBJECT            0
#define PCVAL_SLOT              1
#define PCVAL_SPROP             2

#define PCVAL_TAGBITS           2
#define PCVAL_TAGMASK           JS_BITMASK(PCVAL_TAGBITS)
#define PCVAL_TAG(v)            ((v) & PCVAL_TAGMASK)
#define PCVAL_CLRTAG(v)         ((v) & ~(jsuword)PCVAL_TAGMASK)
#define PCVAL_SETTAG(v,t)       ((jsuword)(v) | (t))

#define PCVAL_NULL              0
#define PCVAL_IS_NULL(v)        ((v) == PCVAL_NULL)

#define PCVAL_IS_OBJECT(v)      (PCVAL_TAG(v) == PCVAL_OBJECT)
#define PCVAL_TO_OBJECT(v)      ((JSObject *) (v))
#define OBJECT_TO_PCVAL(obj)    ((jsuword) (obj))

#define PCVAL_OBJECT_TO_JSVAL(v) OBJECT_TO_JSVAL(PCVAL_TO_OBJECT(v))
#define JSVAL_OBJECT_TO_PCVAL(v) OBJECT_TO_PCVAL(JSVAL_TO_OBJECT(v))

#define PCVAL_IS_SLOT(v)        ((v) & PCVAL_SLOT)
#define PCVAL_TO_SLOT(v)        ((jsuint)(v) >> 1)
#define SLOT_TO_PCVAL(i)        (((jsuword)(i) << 1) | PCVAL_SLOT)

#define PCVAL_IS_SPROP(v)       (PCVAL_TAG(v) == PCVAL_SPROP)
#define PCVAL_TO_SPROP(v)       ((JSScopeProperty *) PCVAL_CLRTAG(v))
#define SPROP_TO_PCVAL(sprop)   PCVAL_SETTAG(sprop, PCVAL_SPROP)









extern JS_REQUIRES_STACK JSPropCacheEntry *
js_FillPropertyCache(JSContext *cx, JSObject *obj,
                     uintN scopeIndex, uintN protoIndex, JSObject *pobj,
                     JSScopeProperty *sprop, JSBool adding);

















#define PROPERTY_CACHE_TEST(cx, pc, obj, pobj, entry, atom)                   \
    do {                                                                      \
        JSPropertyCache *cache_ = &JS_PROPERTY_CACHE(cx);                     \
        uint32 kshape_ = (JS_ASSERT(OBJ_IS_NATIVE(obj)), OBJ_SHAPE(obj));     \
        entry = &cache_->table[PROPERTY_CACHE_HASH_PC(pc, kshape_)];          \
        PCMETER(cache_->pctestentry = entry);                                 \
        PCMETER(cache_->tests++);                                             \
        JS_ASSERT(&obj != &pobj);                                             \
        if (entry->kpc == pc && entry->kshape == kshape_) {                   \
            JSObject *tmp_;                                                   \
            pobj = obj;                                                       \
            JS_ASSERT(PCVCAP_TAG(entry->vcap) <= 1);                          \
            if (PCVCAP_TAG(entry->vcap) == 1 &&                               \
                (tmp_ = OBJ_GET_PROTO(cx, pobj)) != NULL &&                   \
                OBJ_IS_NATIVE(tmp_)) {                                        \
                pobj = tmp_;                                                  \
            }                                                                 \
                                                                              \
            if (JS_LOCK_OBJ_IF_SHAPE(cx, pobj, PCVCAP_SHAPE(entry->vcap))) {  \
                PCMETER(cache_->pchits++);                                    \
                PCMETER(!PCVCAP_TAG(entry->vcap) || cache_->protopchits++);   \
                atom = NULL;                                                  \
                break;                                                        \
            }                                                                 \
        }                                                                     \
        atom = js_FullTestPropertyCache(cx, pc, &obj, &pobj, &entry);         \
        if (atom)                                                             \
            PCMETER(cache_->misses++);                                        \
    } while (0)

extern JS_REQUIRES_STACK JSAtom *
js_FullTestPropertyCache(JSContext *cx, jsbytecode *pc,
                         JSObject **objp, JSObject **pobjp,
                         JSPropCacheEntry **entryp);


#define js_FinishPropertyCache(cache) ((void) 0)

extern void
js_PurgePropertyCache(JSContext *cx, JSPropertyCache *cache);

extern void
js_PurgePropertyCacheForScript(JSContext *cx, JSScript *script);




extern JS_REQUIRES_STACK JS_FRIEND_API(jsval *)
js_AllocStack(JSContext *cx, uintN nslots, void **markp);

extern JS_REQUIRES_STACK JS_FRIEND_API(void)
js_FreeStack(JSContext *cx, void *mark);








extern JSObject *
js_GetScopeChain(JSContext *cx, JSStackFrame *fp);










extern JSBool
js_GetPrimitiveThis(JSContext *cx, jsval *vp, JSClass *clasp, jsval *thisvp);








extern JSObject *
js_ComputeThis(JSContext *cx, JSBool lazy, jsval *argv);

extern const uint16 js_PrimitiveTestFlags[];

#define PRIMITIVE_THIS_TEST(fun,thisv)                                        \
    (JS_ASSERT(!JSVAL_IS_VOID(thisv)),                                        \
     JSFUN_THISP_TEST(JSFUN_THISP_FLAGS((fun)->flags),                        \
                      js_PrimitiveTestFlags[JSVAL_TAG(thisv) - 1]))

#ifdef __cplusplus 
static JS_INLINE JSObject *
js_ComputeThisForFrame(JSContext *cx, JSStackFrame *fp)
{
    if (fp->flags & JSFRAME_COMPUTED_THIS)
        return fp->thisp;
    JSObject* obj = js_ComputeThis(cx, JS_TRUE, fp->argv);
    if (!obj)
        return NULL;
    fp->thisp = obj;
    fp->flags |= JSFRAME_COMPUTED_THIS;
    return obj;
}
#endif










extern JS_REQUIRES_STACK JS_FRIEND_API(JSBool)
js_Invoke(JSContext *cx, uintN argc, jsval *vp, uintN flags);














#define JSINVOKE_CONSTRUCT      JSFRAME_CONSTRUCTING
#define JSINVOKE_ITERATOR       JSFRAME_ITERATOR




#define JSINVOKE_FUNFLAGS       (JSINVOKE_CONSTRUCT | JSINVOKE_ITERATOR)





#define js_InternalCall(cx,obj,fval,argc,argv,rval)                           \
    js_InternalInvoke(cx, obj, fval, 0, argc, argv, rval)

#define js_InternalConstruct(cx,obj,fval,argc,argv,rval)                      \
    js_InternalInvoke(cx, obj, fval, JSINVOKE_CONSTRUCT, argc, argv, rval)

extern JSBool
js_InternalInvoke(JSContext *cx, JSObject *obj, jsval fval, uintN flags,
                  uintN argc, jsval *argv, jsval *rval);

extern JSBool
js_InternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, jsval fval,
                    JSAccessMode mode, uintN argc, jsval *argv, jsval *rval);

extern JS_FORCES_STACK JSBool
js_Execute(JSContext *cx, JSObject *chain, JSScript *script,
           JSStackFrame *down, uintN flags, jsval *result);

extern JS_REQUIRES_STACK JSBool
js_InvokeConstructor(JSContext *cx, uintN argc, JSBool clampReturn, jsval *vp);

extern JS_REQUIRES_STACK JSBool
js_Interpret(JSContext *cx);

#define JSPROP_INITIALIZER 0x100   /* NB: Not a valid property attribute. */

extern JSBool
js_CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs,
                      JSObject **objp, JSProperty **propp);

extern JSBool
js_StrictlyEqual(JSContext *cx, jsval lval, jsval rval);

extern JSBool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, jsval idval, jsid *idp);





extern jsval&
js_GetUpvar(JSContext *cx, uintN level, uintN cookie);












#ifndef JS_LONE_INTERPRET
# ifdef _MSC_VER
#  define JS_LONE_INTERPRET 0
# else
#  define JS_LONE_INTERPRET 1
# endif
#endif

#if !JS_LONE_INTERPRET
# define JS_STATIC_INTERPRET    static
#else
# define JS_STATIC_INTERPRET

extern JS_REQUIRES_STACK jsval *
js_AllocRawStack(JSContext *cx, uintN nslots, void **markp);

extern JS_REQUIRES_STACK void
js_FreeRawStack(JSContext *cx, void *mark);
















extern JSObject *
js_ComputeGlobalThis(JSContext *cx, JSBool lazy, jsval *argv);

extern JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex);

extern JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx);

extern JS_REQUIRES_STACK JSClass *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth);





extern JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, JSStackFrame *fp, jsint stackDepth,
               JSBool normalUnwind);

extern JSBool
js_OnUnknownMethod(JSContext *cx, jsval *vp);







extern JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, jsval *vp, jsval *vp2);





extern JS_REQUIRES_STACK void
js_TraceOpcode(JSContext *cx);




extern void
js_MeterOpcodePair(JSOp op1, JSOp op2);

extern void
js_MeterSlotOpcode(JSOp op, uint32 slot);

#endif 

JS_END_EXTERN_C

#endif 
