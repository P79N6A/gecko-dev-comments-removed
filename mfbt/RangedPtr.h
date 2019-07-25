







































#ifndef mozilla_RangedPtr_h_
#define mozilla_RangedPtr_h_

#include "mozilla/Util.h"

#ifdef __cplusplus

namespace mozilla {




















template <typename T>
class RangedPtr
{
    T* ptr;

#ifdef DEBUG
    T* const rangeStart;
    T* const rangeEnd;
#endif

    void checkSanity() {
        MOZ_ASSERT(rangeStart <= ptr);
        MOZ_ASSERT(ptr <= rangeEnd);
    }

    
    RangedPtr<T> create(T *ptr) const {
#ifdef DEBUG
        return RangedPtr<T>(ptr, rangeStart, rangeEnd);
#else
        return RangedPtr<T>(ptr, NULL, size_t(0));
#endif
    }

  public:
    RangedPtr(T* p, T* start, T* end)
      : ptr(p)
#ifdef DEBUG
      , rangeStart(start), rangeEnd(end)
#endif
    {
        MOZ_ASSERT(rangeStart <= rangeEnd);
        checkSanity();
    }
    RangedPtr(T* p, T* start, size_t length)
      : ptr(p)
#ifdef DEBUG
      , rangeStart(start), rangeEnd(start + length)
#endif
    {
        MOZ_ASSERT(length <= size_t(-1) / sizeof(T));
        MOZ_ASSERT(uintptr_t(rangeStart) + length * sizeof(T) >= uintptr_t(rangeStart));
        checkSanity();
    }

    
    RangedPtr(T* p, size_t length)
      : ptr(p)
#ifdef DEBUG
      , rangeStart(p), rangeEnd(p + length)
#endif
    {
        MOZ_ASSERT(length <= size_t(-1) / sizeof(T));
        MOZ_ASSERT(uintptr_t(rangeStart) + length * sizeof(T) >= uintptr_t(rangeStart));
        checkSanity();
    }

    T* get() const {
        return ptr;
    }

    









    RangedPtr<T>& operator=(const RangedPtr<T>& other) {
        MOZ_ASSERT(rangeStart == other.rangeStart);
        MOZ_ASSERT(rangeEnd == other.rangeEnd);
        ptr = other.ptr;
        checkSanity();
        return *this;
    }

    RangedPtr<T> operator+(size_t inc) {
        MOZ_ASSERT(inc <= size_t(-1) / sizeof(T));
        MOZ_ASSERT(ptr + inc > ptr);
        return create(ptr + inc);
    }

    RangedPtr<T> operator-(size_t dec) {
        MOZ_ASSERT(dec <= size_t(-1) / sizeof(T));
        MOZ_ASSERT(ptr - dec < ptr);
        return create(ptr - dec);
    }

    



    template <typename U>
    RangedPtr<T>& operator=(U* p) {
        *this = create(p);
        return *this;
    }

    template <typename U>
    RangedPtr<T>& operator=(const RangedPtr<U>& p) {
        MOZ_ASSERT(rangeStart <= p.ptr);
        MOZ_ASSERT(p.ptr <= rangeEnd);
        ptr = p.ptr;
        checkSanity();
        return *this;
    }

    RangedPtr<T>& operator++() {
        return (*this += 1);
    }

    RangedPtr<T> operator++(int) {
        RangedPtr<T> rcp = *this;
        ++*this;
        return rcp;
    }

    RangedPtr<T>& operator--() {
        return (*this -= 1);
    }

    RangedPtr<T> operator--(int) {
        RangedPtr<T> rcp = *this;
        --*this;
        return rcp;
    }

    RangedPtr<T>& operator+=(size_t inc) {
        this->operator=<T>(*this + inc);
        return *this;
    }

    RangedPtr<T>& operator-=(size_t dec) {
        this->operator=<T>(*this - dec);
        return *this;
    }

    T& operator[](int index) const {
        MOZ_ASSERT(size_t(index > 0 ? index : -index) <= size_t(-1) / sizeof(T));
        return *create(ptr + index);
    }

    T& operator*() const {
        return *ptr;
    }

    template <typename U>
    bool operator==(const RangedPtr<U>& other) const {
        return ptr == other.ptr;
    }
    template <typename U>
    bool operator!=(const RangedPtr<U>& other) const {
        return !(*this == other);
    }

    template<typename U>
    bool operator==(const U* u) const {
        return ptr == u;
    }
    template<typename U>
    bool operator!=(const U* u) const {
        return !(*this == u);
    }

    template <typename U>
    bool operator<(const RangedPtr<U>& other) const {
        return ptr < other.ptr;
    }
    template <typename U>
    bool operator<=(const RangedPtr<U>& other) const {
        return ptr <= other.ptr;
    }

    template <typename U>
    bool operator>(const RangedPtr<U>& other) const {
        return ptr > other.ptr;
    }
    template <typename U>
    bool operator>=(const RangedPtr<U>& other) const {
        return ptr >= other.ptr;
    }

    size_t operator-(const RangedPtr<T>& other) const {
        MOZ_ASSERT(ptr >= other.ptr);
        return PointerRangeSize(other.ptr, ptr);
    }

  private:
    RangedPtr();
    T* operator&();
    operator T*() const;
};

} 

#endif 

#endif  
