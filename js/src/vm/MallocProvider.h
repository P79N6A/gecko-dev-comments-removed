



































#ifndef vm_MallocProvider_h
#define vm_MallocProvider_h

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"

#include "js/Utility.h"

namespace js {

template<class Client>
struct MallocProvider
{
    void *malloc_(size_t bytes) {
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(bytes);
        void *p = js_malloc(bytes);
        return MOZ_LIKELY(!!p) ? p : client->onOutOfMemory(nullptr, bytes);
    }

    void *calloc_(size_t bytes) {
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(bytes);
        void *p = js_calloc(bytes);
        return MOZ_LIKELY(!!p) ? p : client->onOutOfMemory(reinterpret_cast<void *>(1), bytes);
    }

    void *realloc_(void *p, size_t oldBytes, size_t newBytes) {
        Client *client = static_cast<Client *>(this);
        



        if (newBytes > oldBytes)
            client->updateMallocCounter(newBytes - oldBytes);
        void *p2 = js_realloc(p, newBytes);
        return MOZ_LIKELY(!!p2) ? p2 : client->onOutOfMemory(p, newBytes);
    }

    void *realloc_(void *p, size_t bytes) {
        Client *client = static_cast<Client *>(this);
        



        if (!p)
            client->updateMallocCounter(bytes);
        void *p2 = js_realloc(p, bytes);
        return MOZ_LIKELY(!!p2) ? p2 : client->onOutOfMemory(p, bytes);
    }

    template <class T>
    T *pod_malloc() {
        return (T *)malloc_(sizeof(T));
    }

    template <class T>
    T *pod_calloc() {
        return (T *)calloc_(sizeof(T));
    }

    template <class T>
    T *pod_malloc(size_t numElems) {
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return nullptr;
        }
        return (T *)malloc_(numElems * sizeof(T));
    }

    template <class T>
    T *pod_calloc(size_t numElems, JSCompartment *comp = nullptr, JSContext *cx = nullptr) {
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return nullptr;
        }
        return (T *)calloc_(numElems * sizeof(T));
    }

    JS_DECLARE_NEW_METHODS(new_, malloc_, MOZ_ALWAYS_INLINE)
};

} 

#endif
