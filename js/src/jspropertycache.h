






#ifndef jspropertycache_h___
#define jspropertycache_h___

#include "mozilla/PodOperations.h"

#include "jsapi.h"
#include "jsprvtd.h"
#include "jstypes.h"

#include "vm/String.h"

namespace js {






class PropertyCache;

struct PropertyCacheEntry
{
    jsbytecode    *kpc;           
    Shape         *kshape;        
    Shape         *pshape;        
    Shape         *prop;          

    friend class PropertyCache;

  private:
    
    uint8_t       protoIndex;

  public:
    static const size_t MaxProtoIndex = 15;

    







    bool isOwnPropertyHit() const { return protoIndex == 0; }

    







    bool isPrototypePropertyHit() const { return protoIndex == 1; }

    void assign(jsbytecode *kpc, Shape *kshape, Shape *pshape, Shape *prop, unsigned protoIndex) {
        JS_ASSERT(protoIndex <= MaxProtoIndex);

        this->kpc = kpc;
        this->kshape = kshape;
        this->pshape = pshape;
        this->prop = prop;
        this->protoIndex = uint8_t(protoIndex);
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
        SIZE_LOG2 = 8,
        SIZE = JS_BIT(SIZE_LOG2),
        MASK = JS_BITMASK(SIZE_LOG2)
    };

    PropertyCacheEntry  table[SIZE];
    JSBool              empty;

  public:
#ifdef JS_PROPERTY_CACHE_METERING
    PropertyCacheEntry  *pctestentry;   
    uint32_t            fills;          
    uint32_t            nofills;        
    uint32_t            rofills;        
    uint32_t            disfills;       
    uint32_t            oddfills;       
    uint32_t            add2dictfills;  
    uint32_t            modfills;       
    uint32_t            brandfills;     

    uint32_t            noprotos;       
    uint32_t            longchains;     
    uint32_t            recycles;       
    uint32_t            tests;          
    uint32_t            pchits;         
    uint32_t            protopchits;    
    uint32_t            initests;       
    uint32_t            inipchits;      
    uint32_t            inipcmisses;    
    uint32_t            settests;       
    uint32_t            addpchits;      
    uint32_t            setpchits;      
    uint32_t            setpcmisses;    
    uint32_t            setmisses;      
    uint32_t            kpcmisses;      
    uint32_t            kshapemisses;   
    uint32_t            vcapmisses;     
    uint32_t            misses;         
    uint32_t            flushes;        
    uint32_t            pcpurges;       

# define PCMETER(x)     x
#else
# define PCMETER(x)     ((void)0)
#endif

    PropertyCache() {
        mozilla::PodZero(this);
    }

  private:
    static inline uintptr_t
    hash(jsbytecode *pc, const Shape *kshape)
    {
        return (((uintptr_t(pc) >> SIZE_LOG2) ^ uintptr_t(pc) ^ ((uintptr_t)kshape >> 3)) & MASK);
    }

    static inline bool matchShape(JSContext *cx, JSObject *obj, uint32_t shape);

    PropertyName *
    fullTest(JSContext *cx, jsbytecode *pc, JSObject **objp,
             JSObject **pobjp, PropertyCacheEntry *entry);

#ifdef DEBUG
    void assertEmpty();
#else
    inline void assertEmpty() {}
#endif

  public:
    JS_ALWAYS_INLINE void test(JSContext *cx, jsbytecode *pc,
                               JSObject **obj, JSObject **pobj,
                               PropertyCacheEntry **entry, PropertyName **name);

    






    JS_ALWAYS_INLINE bool testForSet(JSContext *cx, jsbytecode *pc, JSObject *obj,
                                     PropertyCacheEntry **entryp, JSObject **obj2p,
                                     PropertyName **namep);

    







    PropertyCacheEntry *fill(JSContext *cx, JSObject *obj, JSObject *pobj, js::Shape *shape);

    void purge(JSRuntime *rt);

    
    void restore(PropertyCacheEntry *entry);
};

} 

#endif 
