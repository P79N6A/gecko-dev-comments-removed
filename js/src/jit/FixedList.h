





#ifndef jit_FixedList_h
#define jit_FixedList_h

#include <stddef.h>

#include "jit/Ion.h"
#include "jit/IonAllocPolicy.h"

namespace js {
namespace jit {


template <typename T>
class FixedList
{
    T *list_;
    size_t length_;

  private:
    FixedList(const FixedList&); 
    void operator= (const FixedList*); 

  public:
    FixedList()
      : list_(nullptr), length_(0)
    { }

    
    bool init(TempAllocator &alloc, size_t length) {
        length_ = length;
        if (length == 0)
            return true;

        if (length & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
            return false;
        list_ = (T *)alloc.allocate(length * sizeof(T));
        return list_ != nullptr;
    }

    size_t length() const {
        return length_;
    }

    void shrink(size_t num) {
        JS_ASSERT(num < length_);
        length_ -= num;
    }

    bool growBy(TempAllocator &alloc, size_t num) {
        size_t newlength = length_ + num;
        if (newlength < length_)
            return false;
        if (newlength & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
            return false;
        T *list = (T *)alloc.allocate((length_ + num) * sizeof(T));
        if (!list)
            return false;

        for (size_t i = 0; i < length_; i++)
            list[i] = list_[i];

        length_ += num;
        list_ = list;
        return true;
    }

    T &operator[](size_t index) {
        JS_ASSERT(index < length_);
        return list_[index];
    }
    const T &operator [](size_t index) const {
        JS_ASSERT(index < length_);
        return list_[index];
    }
};

} 
} 

#endif 
