






#ifndef jsion_ion_fixed_arity_list_h__
#define jsion_ion_fixed_arity_list_h__

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
        JS_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
    const T &operator [](size_t index) const {
        JS_NOT_REACHED("no items");
        static T *operand = NULL;
        return *operand;
    }
};

} 
} 

#endif 

