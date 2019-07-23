







































#ifndef jspropcache_h___
#define jspropcache_h___






const int PROPERTY_CACHE_LOG2 = 12;
const jsuword PROPERTY_CACHE_SIZE = JS_BIT(PROPERTY_CACHE_LOG2);
const jsuword PROPERTY_CACHE_MASK = JS_BITMASK(PROPERTY_CACHE_LOG2);




const int PCVCAP_PROTOBITS = 4;
const jsuword PCVCAP_PROTOSIZE = JS_BIT(PCVCAP_PROTOBITS);
const jsuword PCVCAP_PROTOMASK = JS_BITMASK(PCVCAP_PROTOBITS);

const int PCVCAP_SCOPEBITS = 4;
const jsuword PCVCAP_SCOPESIZE = JS_BIT(PCVCAP_SCOPEBITS);
const jsuword PCVCAP_SCOPEMASK = JS_BITMASK(PCVCAP_SCOPEBITS);

const jsuword PCVCAP_TAGBITS = PCVCAP_PROTOBITS + PCVCAP_SCOPEBITS;
const jsuword PCVCAP_TAGMASK = JS_BITMASK(PCVCAP_TAGBITS);
inline jsuword PCVCAP_TAG(jsuword t) { return t & PCVCAP_TAGMASK; }

inline jsuword PCVCAP_MAKE(jsuword t, jsuword s, jsuword p)
{
    return (uint32(t) << PCVCAP_TAGBITS) | (s << PCVCAP_PROTOBITS) | p;
}

inline jsuword PCVCAP_SHAPE(jsuword t) { return t >> PCVCAP_TAGBITS; }

const jsuword SHAPE_OVERFLOW_BIT = JS_BIT(32 - PCVCAP_TAGBITS);

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





JSPropCacheEntry * const JS_NO_PROP_CACHE_FILL = (JSPropCacheEntry *) NULL + 1;

#if defined DEBUG_brendan || defined DEBUG_brendaneich
#define JS_PROPERTY_CACHE_METERING 1
#endif

class JSPropertyCache {
public:
    JS_ALWAYS_INLINE JS_REQUIRES_STACK void
    test(JSContext *cx, jsbytecode *pc, JSObject **objp,
         JSObject **pobjp, JSPropCacheEntry **entryp, JSAtom **atomp);

    JS_ALWAYS_INLINE JS_REQUIRES_STACK bool
    testForSet(JSContext *cx, jsbytecode *pc, JSObject **objp,
               JSObject **pobjp, JSPropCacheEntry **entryp, JSAtom **atomp);

    JS_ALWAYS_INLINE bool
    testForInit(JSContext *cx, jsbytecode *pc, JSObject *obj,
                JSPropCacheEntry **entryp, JSScopeProperty **spropp);

    







    JS_REQUIRES_STACK JSPropCacheEntry *
    fill(JSContext *cx, JSObject *obj, uintN scopeIndex, uintN protoIndex, JSObject *pobj,
         JSScopeProperty *sprop, JSBool adding);

    void purge(JSContext *cx);
    void purgeForScript(JSContext *cx, JSScript *script);

private:
    JS_REQUIRES_STACK JSAtom *
    fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp, JSObject **pobjp,
             JSPropCacheEntry **entryp);

    inline void assertEmpty();

    inline JSPropCacheEntry *fillEntry(jsuword khash, jsbytecode *pc, jsuword kshape,
                                       jsuword vcap, jsuword vword);

    inline JSPropCacheEntry *fillByPC(jsbytecode *pc, jsuword kshape,
                                      jsuword vshape, jsuword scopeIndex, jsuword protoIndex,
                                      jsuword vword);

    inline JSPropCacheEntry *fillByAtom(JSAtom *atom, JSObject *obj,
                                        jsuword vshape, jsuword scopeIndex, jsuword protoIndex,
                                        jsuword vword);

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
};




const jsuword PCVAL_OBJECT = 0;
const jsuword PCVAL_SLOT = 1;
const jsuword PCVAL_SPROP = 2;

const int PCVAL_TAGBITS = 2;
const jsuword PCVAL_TAGMASK = JS_BITMASK(PCVAL_TAGBITS);

inline jsuword PCVAL_TAG(jsuword v) { return v & PCVAL_TAGMASK; }
inline jsuword PCVAL_CLRTAG(jsuword v) { return v & ~PCVAL_TAGMASK; }
inline jsuword PCVAL_SETTAG(jsuword v, jsuword t) { return v | t; }

const jsuword PCVAL_NULL = 0;
inline bool PCVAL_IS_NULL(jsuword v) { return v == PCVAL_NULL; }

inline bool PCVAL_IS_OBJECT(jsuword v) { return PCVAL_TAG(v) == PCVAL_OBJECT; }
inline JSObject *PCVAL_TO_OBJECT(jsuword v) { return (JSObject *) v; }
inline jsuword OBJECT_TO_PCVAL(JSObject *obj) { return (jsuword) obj; }

inline jsval PCVAL_OBJECT_TO_JSVAL(jsuword v) { return OBJECT_TO_JSVAL(PCVAL_TO_OBJECT(v)); }
inline jsuword JSVAL_OBJECT_TO_PCVAL(jsval v) { return OBJECT_TO_PCVAL(JSVAL_TO_OBJECT(v)); }

inline bool PCVAL_IS_SLOT(jsuword v) { return (v & PCVAL_SLOT) != 0; }
inline jsuint PCVAL_TO_SLOT(jsuword v) { return (jsuint) v >> 1; }
inline jsuword SLOT_TO_PCVAL(jsuint i) { return ((jsuword) i << 1) | PCVAL_SLOT; }

inline bool PCVAL_IS_SPROP(jsuword v) { return PCVAL_TAG(v) == PCVAL_SPROP; }
inline JSScopeProperty *PCVAL_TO_SPROP(jsuword v) { return (JSScopeProperty *) PCVAL_CLRTAG(v); }

inline jsuword
SPROP_TO_PCVAL(JSScopeProperty *sprop)
{
    return PCVAL_SETTAG((jsuword) sprop, PCVAL_SPROP);
}

#endif 
