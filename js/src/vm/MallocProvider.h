







































#ifndef vm_MallocProvider_h
#define vm_MallocProvider_h

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include "mozilla/UniquePtr.h"

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

    template <class T>
    T *pod_malloc() {
        return (T *)malloc_(sizeof(T));
    }

    template <class T>
    T *pod_calloc() {
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(sizeof(T));
        T *p = js_pod_calloc<T>();
        if (MOZ_UNLIKELY(!p)) {
            client->onOutOfMemory(reinterpret_cast<void *>(1), sizeof(T));
            return nullptr;
        }
        return p;
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
    mozilla::UniquePtr<T[], JS::FreePolicy>
    make_pod_array(size_t numElems) {
        return mozilla::UniquePtr<T[], JS::FreePolicy>(pod_malloc<T>(numElems));
    }

    template <class T>
    T *
    pod_calloc(size_t numElems) {
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            Client *client = static_cast<Client *>(this);
            client->reportAllocationOverflow();
            return nullptr;
        }
        Client *client = static_cast<Client *>(this);
        client->updateMallocCounter(numElems * sizeof(T));
        T *p = js_pod_calloc<T>(numElems);
        if (MOZ_UNLIKELY(!p)) {
            client->onOutOfMemory(reinterpret_cast<void *>(1), sizeof(T));
            return nullptr;
        }
        return p;
    }

    template <class T>
    mozilla::UniquePtr<T[], JS::FreePolicy>
    make_zeroed_pod_array(size_t numElems) {
        return mozilla::UniquePtr<T[], JS::FreePolicy>(pod_calloc<T>(numElems));
    }

    template <class T>
    T *pod_realloc(T *prior, size_t oldSize, size_t newSize) {
        Client *client = static_cast<Client *>(this);
        T *p = js_pod_realloc(prior, oldSize, newSize);
        if (MOZ_LIKELY(p)) {
            
            
            if (newSize > oldSize)
                client->updateMallocCounter((newSize - oldSize) * sizeof(T));
            return p;
        }
        if (newSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            client->reportAllocationOverflow();
            return nullptr;
        }
        client->onOutOfMemory(prior, newSize * sizeof(T));
        return nullptr;
    }

    JS_DECLARE_NEW_METHODS(new_, malloc_, MOZ_ALWAYS_INLINE)
    JS_DECLARE_MAKE_METHODS(make_unique, new_, MOZ_ALWAYS_INLINE)
};

} 

#endif
