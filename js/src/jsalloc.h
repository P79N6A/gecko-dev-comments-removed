






#ifndef jsalloc_h_
#define jsalloc_h_

#include "jsutil.h"

namespace js {



















class SystemAllocPolicy
{
  public:
    void *malloc_(size_t bytes) { return js_malloc(bytes); }
    void *calloc_(size_t bytes) { return js_calloc(bytes); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) { return js_realloc(p, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const {}
};










class TempAllocPolicy
{
    JSContext *const cx_;

    



    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes);

  public:
    TempAllocPolicy(JSContext *cx) : cx_(cx) {}

    JSContext *context() const {
        return cx_;
    }

    void *malloc_(size_t bytes) {
        void *p = js_malloc(bytes);
        if (JS_UNLIKELY(!p))
            p = onOutOfMemory(NULL, bytes);
        return p;
    }

    void *calloc_(size_t bytes) {
        void *p = js_calloc(bytes);
        if (JS_UNLIKELY(!p))
            p = onOutOfMemory(NULL, bytes);
        return p;
    }

    void *realloc_(void *p, size_t oldBytes, size_t bytes) {
        void *p2 = js_realloc(p, bytes);
        if (JS_UNLIKELY(!p2))
            p2 = onOutOfMemory(p2, bytes);
        return p2;
    }

    void free_(void *p) {
        js_free(p);
    }

    JS_FRIEND_API(void) reportAllocOverflow() const;
};

} 

#endif 
