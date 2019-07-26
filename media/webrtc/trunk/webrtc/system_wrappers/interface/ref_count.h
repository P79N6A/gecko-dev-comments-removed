









#ifndef SYSTEM_WRAPPERS_INTERFACE_REF_COUNT_H_
#define SYSTEM_WRAPPERS_INTERFACE_REF_COUNT_H_

#include "system_wrappers/interface/atomic32.h"

namespace webrtc {


























template <class T>
class RefCountImpl : public T {
 public:
  RefCountImpl() : ref_count_(0) {}

  template<typename P>
  explicit RefCountImpl(P p) : T(p), ref_count_(0) {}

  template<typename P1, typename P2>
  RefCountImpl(P1 p1, P2 p2) : T(p1, p2), ref_count_(0) {}

  template<typename P1, typename P2, typename P3>
  RefCountImpl(P1 p1, P2 p2, P3 p3) : T(p1, p2, p3), ref_count_(0) {}

  template<typename P1, typename P2, typename P3, typename P4>
  RefCountImpl(P1 p1, P2 p2, P3 p3, P4 p4) : T(p1, p2, p3, p4), ref_count_(0) {}

  template<typename P1, typename P2, typename P3, typename P4, typename P5>
  RefCountImpl(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
      : T(p1, p2, p3, p4, p5), ref_count_(0) {}

  virtual int32_t AddRef() {
    return ++ref_count_;
  }

  virtual int32_t Release() {
    int32_t ref_count;
    ref_count = --ref_count_;
    if (ref_count == 0)
      delete this;
    return ref_count;
  }

 protected:
  Atomic32 ref_count_;
};

}  

#endif  
