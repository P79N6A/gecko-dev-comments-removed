







































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










class PCVal
{
  private:
    enum {
        OBJECT = 0,
        SLOT = 1,
        SHAPE = 2,
        TAG = 3
    };

    jsuword v;

  public:
    bool isNull() const { return v == 0; }
    void setNull() { v = 0; }

    bool isFunObj() const { return (v & TAG) == OBJECT; }
    JSObject &toFunObj() const {
        JS_ASSERT(isFunObj());
        return *reinterpret_cast<JSObject *>(v);
    }
    void setFunObj(JSObject &obj) {
        v = reinterpret_cast<jsuword>(&obj);
    }

    bool isSlot() const { return v & SLOT; }
    uint32 toSlot() const { JS_ASSERT(isSlot()); return uint32(v) >> 1; }
    void setSlot(uint32 slot) { v = (jsuword(slot) << 1) | SLOT; }

    bool isShape() const { return (v & TAG) == SHAPE; }
    const js::Shape *toShape() const {
        JS_ASSERT(isShape());
        return reinterpret_cast<js::Shape *>(v & ~TAG);
    }
    void setShape(const js::Shape *shape) {
        JS_ASSERT(shape);
        v = reinterpret_cast<jsuword>(shape) | SHAPE;
    }
};

struct PropertyCacheEntry
{
    jsbytecode          *kpc;           
    jsuword             kshape;         
    jsuword             vcap;           
    PCVal               vword;          

    bool adding() const { return vcapTag() == 0 && kshape != vshape(); }
    bool directHit() const { return vcapTag() == 0 && kshape == vshape(); }

    jsuword vcapTag() const { return vcap & PCVCAP_TAGMASK; }
    uint32 vshape() const { return uint32(vcap >> PCVCAP_TAGBITS); }
    jsuword scopeIndex() const { return (vcap >> PCVCAP_PROTOBITS) & PCVCAP_SCOPEMASK; }
    jsuword protoIndex() const { return vcap & PCVCAP_PROTOMASK; }

    void assign(jsbytecode *kpc, jsuword kshape, jsuword vshape,
                uintN scopeIndex, uintN protoIndex, PCVal vword) {
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

class PropertyCache
{
  private:
    enum {
        SIZE_LOG2 = 12,
        SIZE = JS_BIT(SIZE_LOG2),
        MASK = JS_BITMASK(SIZE_LOG2)
    };

    PropertyCacheEntry  table[SIZE];
    JSBool              empty;

  public:
#ifdef JS_PROPERTY_CACHE_METERING
    PropertyCacheEntry  *pctestentry;   
    uint32              fills;          
    uint32              nofills;        
    uint32              rofills;        
    uint32              disfills;       
    uint32              oddfills;       
    uint32              add2dictfills;  
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

    PropertyCache() {
        PodZero(this);
    }
    
  private:
    




    static inline jsuword
    hash(jsbytecode *pc, jsuword kshape)
    {
        return ((((jsuword(pc) >> SIZE_LOG2) ^ jsuword(pc)) + kshape) & MASK);
    }

    static inline bool matchShape(JSContext *cx, JSObject *obj, uint32 shape);

    JS_REQUIRES_STACK JSAtom *fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp,
                                       JSObject **pobjp, PropertyCacheEntry *entry);

#ifdef DEBUG
    void assertEmpty();
#else
    inline void assertEmpty() {}
#endif

  public:
    JS_ALWAYS_INLINE JS_REQUIRES_STACK void test(JSContext *cx, jsbytecode *pc,
                                                 JSObject *&obj, JSObject *&pobj,
                                                 PropertyCacheEntry *&entry, JSAtom *&atom);

    






    JS_ALWAYS_INLINE bool testForSet(JSContext *cx, jsbytecode *pc, JSObject *obj,
                                     PropertyCacheEntry **entryp, JSObject **obj2p,
                                     JSAtom **atomp);

    








    JS_ALWAYS_INLINE bool testForInit(JSRuntime *rt, jsbytecode *pc, JSObject *obj,
                                      const js::Shape **shapep, PropertyCacheEntry **entryp);

    







    JS_REQUIRES_STACK PropertyCacheEntry *fill(JSContext *cx, JSObject *obj, uintN scopeIndex,
                                               JSObject *pobj, const js::Shape *shape,
                                               JSBool adding = false);

    void purge(JSContext *cx);
    void purgeForScript(JSContext *cx, JSScript *script);

    
    void restore(PropertyCacheEntry *entry);
};

} 

#endif
