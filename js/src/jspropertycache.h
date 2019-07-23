







































#ifndef jspropertycache_h___
#define jspropertycache_h___

#include "jsapi.h"
#include "jsprvtd.h"
#include "jstypes.h"

namespace js {








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

struct PropertyCacheEntry
{
    jsbytecode          *kpc;           
    jsuword             kshape;         
    jsuword             vcap;           
    jsuword             vword;          

    bool adding() const { return vcapTag() == 0 && kshape != vshape(); }
    bool directHit() const { return vcapTag() == 0 && kshape == vshape(); }

    jsuword vcapTag() const { return vcap & PCVCAP_TAGMASK; }
    uint32 vshape() const { return uint32(vcap >> PCVCAP_TAGBITS); }
    jsuword scopeIndex() const { return (vcap >> PCVCAP_PROTOBITS) & PCVCAP_SCOPEMASK; }
    jsuword protoIndex() const { return vcap & PCVCAP_PROTOMASK; }

    void assign(jsbytecode *kpc, jsuword kshape, jsuword vshape,
                uintN scopeIndex, uintN protoIndex, jsuword vword) {
        JS_ASSERT(kshape < SHAPE_OVERFLOW_BIT);
        JS_ASSERT(vshape < SHAPE_OVERFLOW_BIT);
        JS_ASSERT(scopeIndex <= PCVCAP_SCOPEMASK);
        JS_ASSERT(protoIndex <= PCVCAP_PROTOMASK);

        this->kpc = kpc;
        this->kshape = kshape;
        this->vcap = (vshape << PCVCAP_TAGBITS) | (scopeIndex << PCVCAP_PROTOBITS) | protoIndex;
        this->vword = vword;
    }
};





#define JS_NO_PROP_CACHE_FILL ((js::PropertyCacheEntry *) NULL + 1)

#if defined DEBUG_brendan || defined DEBUG_brendaneich
#define JS_PROPERTY_CACHE_METERING 1
#endif

struct PropertyCache
{
    enum {
        SIZE_LOG2 = 12,
        SIZE = JS_BIT(SIZE_LOG2),
        MASK = JS_BITMASK(SIZE_LOG2)
    };

    PropertyCacheEntry  table[SIZE];
    JSBool              empty;
#ifdef JS_PROPERTY_CACHE_METERING
    PropertyCacheEntry  *pctestentry;   
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

    




    static inline jsuword
    hash(jsbytecode *pc, jsuword kshape)
    {
        return ((((jsuword(pc) >> SIZE_LOG2) ^ jsuword(pc)) + kshape) & MASK);
    }

    static inline bool matchShape(JSContext *cx, JSObject *obj, uint32 shape);

    JS_ALWAYS_INLINE JS_REQUIRES_STACK void test(JSContext *cx, jsbytecode *pc,
                                                 JSObject *&obj, JSObject *&pobj,
                                                 PropertyCacheEntry *&entry, JSAtom *&atom);

    JS_REQUIRES_STACK JSAtom *fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp,
                                       JSObject **pobjp, PropertyCacheEntry *entry);

    







    JS_REQUIRES_STACK PropertyCacheEntry *fill(JSContext *cx, JSObject *obj, uintN scopeIndex,
                                               uintN protoIndex, JSObject *pobj,
                                               JSScopeProperty *sprop, JSBool adding = false);

    void purge(JSContext *cx);
    void purgeForScript(JSScript *script);

#ifdef DEBUG
    void assertEmpty();
#else
    inline void assertEmpty() {}
#endif
};




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

} 

#endif 
