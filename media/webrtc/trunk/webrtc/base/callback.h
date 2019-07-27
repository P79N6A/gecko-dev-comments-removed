




























































#ifndef WEBRTC_BASE_CALLBACK_H_
#define WEBRTC_BASE_CALLBACK_H_

#include "webrtc/base/logging.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/scoped_ref_ptr.h"

namespace rtc {

template <class R>
class Callback0 {
 public:
  
  Callback0() {}
  template <class T> Callback0(const T& functor)
      : helper_(new RefCountedObject< HelperImpl<T> >(functor)) {}
  R operator()() {
    if (empty())
      return R();
    return helper_->Run();
  }
  bool empty() const { return !helper_; }

 private:
  struct Helper : RefCountInterface {
    virtual ~Helper() {}
    virtual R Run() = 0;
  };
  template <class T> struct HelperImpl : Helper {
    explicit HelperImpl(const T& functor) : functor_(functor) {}
    virtual R Run() {
      return functor_();
    }
    T functor_;
  };
  scoped_refptr<Helper> helper_;
};

template <class R,
          class P1>
class Callback1 {
 public:
  
  Callback1() {}
  template <class T> Callback1(const T& functor)
      : helper_(new RefCountedObject< HelperImpl<T> >(functor)) {}
  R operator()(P1 p1) {
    if (empty())
      return R();
    return helper_->Run(p1);
  }
  bool empty() const { return !helper_; }

 private:
  struct Helper : RefCountInterface {
    virtual ~Helper() {}
    virtual R Run(P1 p1) = 0;
  };
  template <class T> struct HelperImpl : Helper {
    explicit HelperImpl(const T& functor) : functor_(functor) {}
    virtual R Run(P1 p1) {
      return functor_(p1);
    }
    T functor_;
  };
  scoped_refptr<Helper> helper_;
};

template <class R,
          class P1,
          class P2>
class Callback2 {
 public:
  
  Callback2() {}
  template <class T> Callback2(const T& functor)
      : helper_(new RefCountedObject< HelperImpl<T> >(functor)) {}
  R operator()(P1 p1, P2 p2) {
    if (empty())
      return R();
    return helper_->Run(p1, p2);
  }
  bool empty() const { return !helper_; }

 private:
  struct Helper : RefCountInterface {
    virtual ~Helper() {}
    virtual R Run(P1 p1, P2 p2) = 0;
  };
  template <class T> struct HelperImpl : Helper {
    explicit HelperImpl(const T& functor) : functor_(functor) {}
    virtual R Run(P1 p1, P2 p2) {
      return functor_(p1, p2);
    }
    T functor_;
  };
  scoped_refptr<Helper> helper_;
};

template <class R,
          class P1,
          class P2,
          class P3>
class Callback3 {
 public:
  
  Callback3() {}
  template <class T> Callback3(const T& functor)
      : helper_(new RefCountedObject< HelperImpl<T> >(functor)) {}
  R operator()(P1 p1, P2 p2, P3 p3) {
    if (empty())
      return R();
    return helper_->Run(p1, p2, p3);
  }
  bool empty() const { return !helper_; }

 private:
  struct Helper : RefCountInterface {
    virtual ~Helper() {}
    virtual R Run(P1 p1, P2 p2, P3 p3) = 0;
  };
  template <class T> struct HelperImpl : Helper {
    explicit HelperImpl(const T& functor) : functor_(functor) {}
    virtual R Run(P1 p1, P2 p2, P3 p3) {
      return functor_(p1, p2, p3);
    }
    T functor_;
  };
  scoped_refptr<Helper> helper_;
};

template <class R,
          class P1,
          class P2,
          class P3,
          class P4>
class Callback4 {
 public:
  
  Callback4() {}
  template <class T> Callback4(const T& functor)
      : helper_(new RefCountedObject< HelperImpl<T> >(functor)) {}
  R operator()(P1 p1, P2 p2, P3 p3, P4 p4) {
    if (empty())
      return R();
    return helper_->Run(p1, p2, p3, p4);
  }
  bool empty() const { return !helper_; }

 private:
  struct Helper : RefCountInterface {
    virtual ~Helper() {}
    virtual R Run(P1 p1, P2 p2, P3 p3, P4 p4) = 0;
  };
  template <class T> struct HelperImpl : Helper {
    explicit HelperImpl(const T& functor) : functor_(functor) {}
    virtual R Run(P1 p1, P2 p2, P3 p3, P4 p4) {
      return functor_(p1, p2, p3, p4);
    }
    T functor_;
  };
  scoped_refptr<Helper> helper_;
};

template <class R,
          class P1,
          class P2,
          class P3,
          class P4,
          class P5>
class Callback5 {
 public:
  
  Callback5() {}
  template <class T> Callback5(const T& functor)
      : helper_(new RefCountedObject< HelperImpl<T> >(functor)) {}
  R operator()(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
    if (empty())
      return R();
    return helper_->Run(p1, p2, p3, p4, p5);
  }
  bool empty() const { return !helper_; }

 private:
  struct Helper : RefCountInterface {
    virtual ~Helper() {}
    virtual R Run(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) = 0;
  };
  template <class T> struct HelperImpl : Helper {
    explicit HelperImpl(const T& functor) : functor_(functor) {}
    virtual R Run(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
      return functor_(p1, p2, p3, p4, p5);
    }
    T functor_;
  };
  scoped_refptr<Helper> helper_;
};
}  

#endif  
