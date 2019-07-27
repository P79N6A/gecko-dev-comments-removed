



#ifndef PtrVector_h
#define PtrVector_h

#include <vector>

namespace mozilla
{




template <class T> class PtrVector
{
public:
  ~PtrVector()
  {
    for (T* value : values) { delete value; }
  }

  std::vector<T*> values;
};

} 

#endif 

