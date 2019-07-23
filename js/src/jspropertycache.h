




































#ifndef jspropertycache_h___
#define jspropertycache_h___

#include "jsapi.h"
#include "jsprvtd.h"
#include "jstypes.h"

JS_BEGIN_EXTERN_C







enum {
    PROPERTY_CACHE_LOG2 = 12,
    PROPERTY_CACHE_SIZE = JS_BIT(PROPERTY_CACHE_LOG2),
    PROPERTY_CACHE_MASK = JS_BITMASK(PROPERTY_CACHE_LOG2)
};






inline jsuword
PROPERTY_CACHE_HASH(jsbytecode *pc, jsuword kshape)
{
    return ((((jsuword(pc) >> PROPERTY_CACHE_LOG2) ^ jsuword(pc)) + kshape) & PROPERTY_CACHE_MASK);
}


enum {
    PCVCAP_PROTOBITS = 4,
    PCVCAP_PROTOSIZE = JS_BIT(PCVCAP_PROTOBITS),
    PCVCAP_PROTOMASK = JS_BITMASK(PCVCAP_PROTOBITS),

    PCVCAP_SCOPEBITS = 4,
    PCVCAP_SCOPESIZE = JS_BIT(PCVCAP_SCOPEBITS),
    PCVCAP_SCOPEMASK = JS_BITMASK(PCVCAP_SCOPEBITS),

    PCVCAP_TAGBITS = PCVCAP_PROTOBITS + PCVCAP_SCOPEBITS,
    PCVCAP_TAGMASK = JS_BITMASK(PCVCAP_TAGBITS)
};

const uint32 SHAPE_OVERFLOW_BIT = JS_BIT(32 - PCVCAP_TAGBITS);

inline jsuword
PCVCAP_TAG(jsuword t)
{
    return t & PCVCAP_TAGMASK;
}

inline jsuword
PCVCAP_MAKE(uint32 t, unsigned int s, unsigned int p)
{
    JS_ASSERT(t < SHAPE_OVERFLOW_BIT);
    JS_ASSERT(s <= PCVCAP_SCOPEMASK);
    JS_ASSERT(p <= PCVCAP_PROTOMASK);
    return (t << PCVCAP_TAGBITS) | (s << PCVCAP_PROTOBITS) | p;
}

inline uint32
PCVCAP_SHAPE(jsuword t)
{
    return t >> PCVCAP_TAGBITS;
}

struct JSPropCacheEntry
{
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




enum {
    PCVAL_OBJECT = 0,
    PCVAL_SLOT = 1,
    PCVAL_SPROP = 2,

    PCVAL_TAGBITS = 2,
    PCVAL_TAGMASK = JS_BITMASK(PCVAL_TAGBITS),

    PCVAL_NULL = 0
};

inline jsuword PCVAL_TAG(jsuword v) { return v & PCVAL_TAGMASK; }
inline jsuword PCVAL_CLRTAG(jsuword v) { return v & ~jsuword(PCVAL_TAGMASK); }
inline jsuword PCVAL_SETTAG(jsuword v, jsuword t) { return v | t; }

inline bool PCVAL_IS_NULL(jsuword v) { return v == PCVAL_NULL; }

inline bool PCVAL_IS_OBJECT(jsuword v) { return PCVAL_TAG(v) == PCVAL_OBJECT; }
inline JSObject *PCVAL_TO_OBJECT(jsuword v) { JS_ASSERT(PCVAL_IS_OBJECT(v)); return (JSObject *) v; }
inline jsuword OBJECT_TO_PCVAL(JSObject *obj) { return jsuword(obj); }

inline jsval PCVAL_OBJECT_TO_JSVAL(jsuword v) { return OBJECT_TO_JSVAL(PCVAL_TO_OBJECT(v)); }
inline jsuword JSVAL_OBJECT_TO_PCVAL(jsval v) { return OBJECT_TO_PCVAL(JSVAL_TO_OBJECT(v)); }

inline bool PCVAL_IS_SLOT(jsuword v) { return v & PCVAL_SLOT; }
inline jsuint PCVAL_TO_SLOT(jsuword v) { JS_ASSERT(PCVAL_IS_SLOT(v)); return jsuint(v) >> 1; }
inline jsuword SLOT_TO_PCVAL(jsuint i) { return (jsuword(i) << 1) | PCVAL_SLOT; }

inline bool PCVAL_IS_SPROP(jsuword v) { return PCVAL_TAG(v) == PCVAL_SPROP; }
inline JSScopeProperty *PCVAL_TO_SPROP(jsuword v) { JS_ASSERT(PCVAL_IS_SPROP(v)); return (JSScopeProperty *) PCVAL_CLRTAG(v); }
inline jsuword SPROP_TO_PCVAL(JSScopeProperty *sprop) { return PCVAL_SETTAG(jsuword(sprop), PCVAL_SPROP); }









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
