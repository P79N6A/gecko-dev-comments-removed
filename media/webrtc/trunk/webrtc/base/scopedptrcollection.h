












#ifndef WEBRTC_BASE_SCOPEDPTRCOLLECTION_H_
#define WEBRTC_BASE_SCOPEDPTRCOLLECTION_H_

#include <algorithm>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"

namespace rtc {

template<class T>
class ScopedPtrCollection {
 public:
  typedef std::vector<T*> VectorT;

  ScopedPtrCollection() { }
  ~ScopedPtrCollection() {
    for (typename VectorT::iterator it = collection_.begin();
         it != collection_.end(); ++it) {
      delete *it;
    }
  }

  const VectorT& collection() const { return collection_; }
  void Reserve(size_t size) {
    collection_.reserve(size);
  }
  void PushBack(T* t) {
    collection_.push_back(t);
  }

  
  void Remove(T* t) {
    collection_.erase(std::remove(collection_.begin(), collection_.end(), t),
                      collection_.end());
  }

 private:
  VectorT collection_;

  DISALLOW_COPY_AND_ASSIGN(ScopedPtrCollection);
};

}  

#endif  
