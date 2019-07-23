



#ifndef CHROME_COMMON_REF_COUNTED_UTIL_H__
#define CHROME_COMMON_REF_COUNTED_UTIL_H__

#include "base/ref_counted.h"
#include <vector>



template<class T>
class RefCountedVector :
    public base::RefCountedThreadSafe<RefCountedVector<T> > {
 public:
  RefCountedVector() {}
  RefCountedVector(const std::vector<T>& initializer)
      : data(initializer) {}

  std::vector<T> data;

  DISALLOW_EVIL_CONSTRUCTORS(RefCountedVector<T>);
};



typedef RefCountedVector<unsigned char> RefCountedBytes;

#endif  
