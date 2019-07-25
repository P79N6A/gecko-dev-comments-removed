







































#ifndef jspropertycache_h___
#define jspropertycache_h___

#include "jsapi.h"
#include "jsprvtd.h"
#include "jstypes.h"

namespace js {








enum {
    PCINDEX_PROTOBITS = 4,
    PCINDEX_PROTOSIZE = JS_BIT(PCINDEX_PROTOBITS),
    PCINDEX_PROTOMASK = JS_BITMASK(PCINDEX_PROTOBITS),

    PCINDEX_SCOPEBITS = 4,
    PCINDEX_SCOPESIZE = JS_BIT(PCINDEX_SCOPEBITS),
    PCINDEX_SCOPEMASK = JS_BITMASK(PCINDEX_SCOPEBITS)
};

struct PropertyCacheEntry
{
    jsbytecode          *kpc;           
    const Shape         *kshape;        
    const Shape         *pshape;        
    const Shape         *prop;          
    uint16              vindex;         


    bool directHit() const { return vindex == 0; }

    void assign(jsbytecode *kpc, const Shape *kshape, const Shape *pshape,
                const Shape *prop, uintN scopeIndex, uintN protoIndex) {
        JS_ASSERT(scopeIndex <= PCINDEX_SCOPEMASK);
        JS_ASSERT(protoIndex <= PCINDEX_PROTOMASK);

        this->kpc = kpc;
        this->kshape = kshape;
        this->pshape = pshape;
        this->prop = prop;
        this->vindex = (scopeIndex << PCINDEX_PROTOBITS) | protoIndex;
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
    hash(jsbytecode *pc, const Shape *kshape)
    {
        return (((jsuword(pc) >> SIZE_LOG2) ^ jsuword(pc) ^ ((jsuword)kshape >> 3)) & MASK);
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

    







    JS_REQUIRES_STACK PropertyCacheEntry *fill(JSContext *cx, JSObject *obj, uintN scopeIndex,
                                               JSObject *pobj, const js::Shape *shape);

    void purge(JSContext *cx);

    
    void restore(PropertyCacheEntry *entry);
};

} 

#endif 
