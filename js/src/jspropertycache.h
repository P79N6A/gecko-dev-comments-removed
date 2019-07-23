




































#ifndef jspropertycache_h___
#define jspropertycache_h___

#include "jsapi.h"
#include "jsprvtd.h"
#include "jstypes.h"

JS_BEGIN_EXTERN_C






#define PROPERTY_CACHE_LOG2     12
#define PROPERTY_CACHE_SIZE     JS_BIT(PROPERTY_CACHE_LOG2)
#define PROPERTY_CACHE_MASK     JS_BITMASK(PROPERTY_CACHE_LOG2)






#define PROPERTY_CACHE_HASH(pc,kshape)                                        \
    (((((jsuword)(pc) >> PROPERTY_CACHE_LOG2) ^ (jsuword)(pc)) + (kshape)) &  \
     PROPERTY_CACHE_MASK)




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

    static inline bool matchShape(JSContext *cx, JSObject *obj, uint32 shape);
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
    uint32              setmisses;      
    uint32              kpcmisses;      
    uint32              kshapemisses;   
    uint32              vcapmisses;     
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
                     JSScopeProperty *sprop, JSBool adding = false);

extern JS_REQUIRES_STACK JSAtom *
js_FullTestPropertyCache(JSContext *cx, jsbytecode *pc,
                         JSObject **objp, JSObject **pobjp,
                         JSPropCacheEntry *entry);


#define js_FinishPropertyCache(cache) ((void) 0)

extern void
js_PurgePropertyCache(JSContext *cx, JSPropertyCache *cache);

extern void
js_PurgePropertyCacheForScript(JSContext *cx, JSScript *script);

JS_END_EXTERN_C

#endif 
