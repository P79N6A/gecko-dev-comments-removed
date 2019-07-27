












#ifndef jsalloc_h
#define jsalloc_h

#include "js/TypeDecls.h"
#include "js/Utility.h"

namespace js {

struct ContextFriendFields;


class SystemAllocPolicy
{
  public:
    template <typename T> T* pod_malloc(size_t numElems) { return js_pod_malloc<T>(numElems); }
    template <typename T> T* pod_calloc(size_t numElems) { return js_pod_calloc<T>(numElems); }
    template <typename T> T* pod_realloc(T* p, size_t oldSize, size_t newSize) {
        return js_pod_realloc<T>(p, oldSize, newSize);
    }
    void free_(void* p) { js_free(p); }
    void reportAllocOverflow() const {}
};










class TempAllocPolicy
{
    ContextFriendFields* const cx_;

    



    JS_FRIEND_API(void*) onOutOfMemory(void* p, size_t nbytes);

  public:
    MOZ_IMPLICIT TempAllocPolicy(JSContext* cx) : cx_((ContextFriendFields*) cx) {} 
    MOZ_IMPLICIT TempAllocPolicy(ContextFriendFields* cx) : cx_(cx) {}

    template <typename T>
    T* pod_malloc(size_t numElems) {
        T* p = js_pod_malloc<T>(numElems);
        if (MOZ_UNLIKELY(!p))
            p = static_cast<T*>(onOutOfMemory(nullptr, numElems * sizeof(T)));
        return p;
    }

    template <typename T>
    T* pod_calloc(size_t numElems) {
        T* p = js_pod_calloc<T>(numElems);
        if (MOZ_UNLIKELY(!p))
            p = static_cast<T*>(onOutOfMemory(reinterpret_cast<void*>(1), numElems * sizeof(T)));
        return p;
    }

    template <typename T>
    T* pod_realloc(T* prior, size_t oldSize, size_t newSize) {
        T* p2 = js_pod_realloc<T>(prior, oldSize, newSize);
        if (MOZ_UNLIKELY(!p2))
            p2 = static_cast<T*>(onOutOfMemory(prior, newSize * sizeof(T)));
        return p2;
    }

    void free_(void* p) {
        js_free(p);
    }

    JS_FRIEND_API(void) reportAllocOverflow() const;
};

} 

#endif 
