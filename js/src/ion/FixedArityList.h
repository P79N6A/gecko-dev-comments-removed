





#ifndef ion_FixedArityList_h
#define ion_FixedArityList_h

namespace js {
namespace ion {

template <typename T, size_t Arity>
class FixedArityList
{
    T list_[Arity];

  public:
    FixedArityList()
      : list_()
    { }

    T &operator [](size_t index) {
        JS_ASSERT(index < Arity);
        return list_[index];
    }
    const T &operator [](size_t index) const {
        JS_ASSERT(index < Arity);
        return list_[index];
    }
};

template <typename T>
class FixedArityList<T, 0>
{
  public:
    T &operator [](size_t index) {
        MOZ_ASSUME_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
    const T &operator [](size_t index) const {
        MOZ_ASSUME_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
};

} 
} 

#endif 
