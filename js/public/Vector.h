





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
    Vector(AllocPolicy alloc = AllocPolicy()) : Base(alloc) {}
    Vector(mozilla::MoveRef<Vector> vec) : Base(vec) {}
    Vector &operator=(mozilla::MoveRef<Vector> vec) {
        return Base::operator=(vec);
    }
};

} 

#endif 
