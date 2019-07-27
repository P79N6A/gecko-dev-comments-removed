







































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
    template <class T>
    T* pod_malloc() {
        return pod_malloc<T>(1);
    }

    template <class T>
    T* pod_malloc(size_t numElems) {
        T* p = js_pod_malloc<T>(numElems);
        if (MOZ_LIKELY(p)) {
            client()->updateMallocCounter(numElems * sizeof(T));
            return p;
        }
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        return (T*)client()->onOutOfMemory(nullptr, numElems * sizeof(T));
    }

    template <class T, class U>
    T* pod_malloc_with_extra(size_t numExtra) {
        if (MOZ_UNLIKELY(numExtra & mozilla::tl::MulOverflowMask<sizeof(U)>::value)) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        size_t bytes = sizeof(T) + numExtra * sizeof(U);
        if (MOZ_UNLIKELY(bytes < sizeof(T))) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        T* p = reinterpret_cast<T*>(js_pod_malloc<uint8_t>(bytes));
        if (MOZ_LIKELY(p)) {
            client()->updateMallocCounter(bytes);
            return p;
        }
        return (T*)client()->onOutOfMemory(nullptr, bytes);
    }

    template <class T>
    mozilla::UniquePtr<T[], JS::FreePolicy>
    make_pod_array(size_t numElems) {
        return mozilla::UniquePtr<T[], JS::FreePolicy>(pod_malloc<T>(numElems));
    }

    template <class T>
    T* pod_calloc() {
        return pod_calloc<T>(1);
    }

    template <class T>
    T* pod_calloc(size_t numElems) {
        T* p = js_pod_calloc<T>(numElems);
        if (MOZ_LIKELY(p)) {
            client()->updateMallocCounter(numElems * sizeof(T));
            return p;
        }
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        return (T*)client()->onOutOfMemory(reinterpret_cast<void*>(1), numElems * sizeof(T));
    }

    template <class T, class U>
    T* pod_calloc_with_extra(size_t numExtra) {
        if (MOZ_UNLIKELY(numExtra & mozilla::tl::MulOverflowMask<sizeof(U)>::value)) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        size_t bytes = sizeof(T) + numExtra * sizeof(U);
        if (MOZ_UNLIKELY(bytes < sizeof(T))) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        T* p = reinterpret_cast<T*>(js_pod_calloc<uint8_t>(bytes));
        if (MOZ_LIKELY(p)) {
            client()->updateMallocCounter(bytes);
            return p;
        }
        return (T*)client()->onOutOfMemory(reinterpret_cast<void*>(1), bytes);
    }

    template <class T>
    mozilla::UniquePtr<T[], JS::FreePolicy>
    make_zeroed_pod_array(size_t numElems)
    {
        return mozilla::UniquePtr<T[], JS::FreePolicy>(pod_calloc<T>(numElems));
    }

    template <class T>
    T* pod_realloc(T* prior, size_t oldSize, size_t newSize) {
        T* p = js_pod_realloc(prior, oldSize, newSize);
        if (MOZ_LIKELY(p)) {
            
            
            if (newSize > oldSize)
                client()->updateMallocCounter((newSize - oldSize) * sizeof(T));
            return p;
        }
        if (newSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value) {
            client()->reportAllocationOverflow();
            return nullptr;
        }
        return (T*)client()->onOutOfMemory(prior, newSize * sizeof(T));
    }

    JS_DECLARE_NEW_METHODS(new_, pod_malloc<uint8_t>, MOZ_ALWAYS_INLINE)
    JS_DECLARE_MAKE_METHODS(make_unique, new_, MOZ_ALWAYS_INLINE)

  private:
    Client* client() { return static_cast<Client*>(this); }
};

} 

#endif 
