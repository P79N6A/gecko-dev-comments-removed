





#ifndef js_WeakMapPtr_h
#define js_WeakMapPtr_h

#include "jspubtd.h"

#include "js/TypeDecls.h"

namespace JS {







template <typename K, typename V>
class JS_PUBLIC_API(WeakMapPtr)
{
  public:
    WeakMapPtr() : ptr(nullptr) {}
    bool init(JSContext *cx);
    bool initialized() { return ptr != nullptr; }
    void destroy();
    virtual ~WeakMapPtr() { MOZ_ASSERT(!initialized()); }
    void trace(JSTracer *tracer);

    V lookup(const K &key);
    bool put(JSContext *cx, const K &key, const V &value);

    static void keyMarkCallback(JSTracer *trc, K key, void *data);

  private:
    void *ptr;

    
    WeakMapPtr(const WeakMapPtr &wmp) MOZ_DELETE;
    WeakMapPtr &operator=(const WeakMapPtr &wmp) MOZ_DELETE;
};

} 

#endif  
