



#ifndef PtrVector_h
#define PtrVector_h

#include <vector>

namespace mozilla
{




template <class T> class PtrVector
{
public:
  PtrVector() = default;
  PtrVector(const PtrVector&) = delete;
  PtrVector(PtrVector&& aOther)
    : values(std::move(aOther.values))
  {}
  PtrVector& operator=(const PtrVector&) = delete;
  PtrVector& operator=(PtrVector&& aOther)
  {
    std::swap(values, aOther.values);
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

