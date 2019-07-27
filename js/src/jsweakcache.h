





#ifndef jsweakcache_h
#define jsweakcache_h

#include "jscntxt.h"
#include "gc/Marking.h"
#include "js/HashTable.h"
#include "vm/Runtime.h"

namespace js {







template <class Key, class Value,
          class HashPolicy = DefaultHasher<Key>,
          class AllocPolicy = RuntimeAllocPolicy>
class WeakCache : public HashMap<Key, Value, HashPolicy, AllocPolicy> {
  private:
    typedef HashMap<Key, Value, HashPolicy, AllocPolicy> Base;
    typedef typename Base::Range Range;
    typedef typename Base::Enum Enum;

  public:
    explicit WeakCache(JSRuntime* rt) : Base(rt) { }
    explicit WeakCache(JSContext* cx) : Base(cx->runtime()) { }

  public:
    
    void sweep(FreeOp* fop) {
        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            
            
            Key k(e.front().key);
            bool isKeyDying = gc::IsAboutToBeFinalized(&k);

            if (isKeyDying || gc::IsAboutToBeFinalized(e.front().value)) {
                e.removeFront();
            } else {
                
                
                
                
            }
        }

#if DEBUG
        
        
        for (Range r = Base::all(); !r.empty(); r.popFront()) {
            Key k(r.front().key);

            MOZ_ASSERT(!gc::IsAboutToBeFinalized(&k));
            MOZ_ASSERT(!gc::IsAboutToBeFinalized(r.front().value));

            
            MOZ_ASSERT(k == r.front().key);
        }
#endif
    }
};



template <class Key, class Value,
          class HashPolicy = DefaultHasher<Key>,
          class AllocPolicy = RuntimeAllocPolicy>
class WeakValueCache : public HashMap<Key, Value, HashPolicy, AllocPolicy>
{
  public:
    typedef HashMap<Key, Value, HashPolicy, AllocPolicy> Base;
    typedef typename Base::Range Range;
    typedef typename Base::Enum Enum;

    explicit WeakValueCache(JSRuntime* rt) : Base(rt) { }
    explicit WeakValueCache(JSContext* cx) : Base(cx->runtime()) { }

  public:
    
    void sweep(FreeOp* fop) {
        
        for (Enum e(*this); !e.empty(); e.popFront()) {
            if (gc::IsAboutToBeFinalized(&e.front().value()))
                e.removeFront();
        }

#if DEBUG
        
        
        for (Range r = Base::all(); !r.empty(); r.popFront())
            MOZ_ASSERT(!gc::IsAboutToBeFinalized(&r.front().value()));
#endif
    }
};

} 

#endif 
