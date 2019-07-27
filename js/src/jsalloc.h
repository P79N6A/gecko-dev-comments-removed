












#ifndef jsalloc_h
#define jsalloc_h

#include "js/TypeDecls.h"
#include "js/Utility.h"

namespace js {

struct ContextFriendFields;


class SystemAllocPolicy
{
  public:
    void *malloc_(size_t bytes) { return js_malloc(bytes); }
    template <typename T> T *pod_calloc(size_t numElems) { return js_pod_calloc<T>(numElems); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) { return js_realloc(p, bytes); }
    void free_(void *p) { js_free(p); }
    void reportAllocOverflow() const {}
};










class TempAllocPolicy
{
    ContextFriendFields *const cx_;

    



    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes);

  public:
    MOZ_IMPLICIT TempAllocPolicy(JSContext *cx) : cx_((ContextFriendFields *) cx) {} 
    MOZ_IMPLICIT TempAllocPolicy(ContextFriendFields *cx) : cx_(cx) {}

    void *malloc_(size_t bytes) {
        void *p = js_malloc(bytes);
        if (MOZ_UNLIKELY(!p))
            p = onOutOfMemory(nullptr, bytes);
        return p;
    }

    template <typename T>
    T *pod_calloc(size_t numElems) {
        T *p = js_pod_calloc<T>(numElems);
        if (MOZ_UNLIKELY(!p))
            p = (T *)onOutOfMemory(reinterpret_cast<void *>(1), numElems * sizeof(T));
        return p;
    }

    void *realloc_(void *p, size_t oldBytes, size_t bytes) {
        void *p2 = js_realloc(p, bytes);
        if (MOZ_UNLIKELY(!p2))
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
