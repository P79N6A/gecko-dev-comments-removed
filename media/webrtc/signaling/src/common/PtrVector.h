



#ifndef PtrVector_h
#define PtrVector_h

#include <mozilla/Move.h>
#include <vector>

namespace mozilla
{




template <class T> class PtrVector
{
public:
  PtrVector() = default;
  PtrVector(const PtrVector&) = delete;
  PtrVector(PtrVector&& aOther)
    : values(Move(aOther.values))
  {}
  PtrVector& operator=(const PtrVector&) = delete;
  PtrVector& operator=(PtrVector&& aOther)
  {
    Swap(values, aOther.values);
    return *this;
  }

  ~PtrVector()
  {
    for (T* value : values) { delete value; }
  }

  std::vector<T*> values;
};

} 

#endif 

