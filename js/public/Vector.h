





#ifndef js_Vector_h
#define js_Vector_h

#include "mozilla/Vector.h"


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4345)
#endif

namespace js {

class TempAllocPolicy;
























template <typename T,
          size_t MinInlineCapacity = 0,
          class AllocPolicy = TempAllocPolicy>
class Vector
  : public mozilla::VectorBase<T,
                               MinInlineCapacity,
                               AllocPolicy,
                               Vector<T, MinInlineCapacity, AllocPolicy> >
{
    typedef typename mozilla::VectorBase<T, MinInlineCapacity, AllocPolicy, Vector> Base;

  public:
    explicit Vector(AllocPolicy alloc = AllocPolicy()) : Base(alloc) {}
    Vector(Vector &&vec) : Base(mozilla::Move(vec)) {}
    Vector &operator=(Vector &&vec) {
        return Base::operator=(mozilla::Move(vec));
    }
};

} 

#endif 
