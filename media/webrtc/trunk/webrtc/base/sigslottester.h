













#ifndef WEBRTC_BASE_SIGSLOTTESTER_H_
#define WEBRTC_BASE_SIGSLOTTESTER_H_

























#include "webrtc/base/constructormagic.h"
#include "webrtc/base/sigslot.h"

namespace rtc {







template <class A1, class C1>
class SigslotTester1 : public sigslot::has_slots<> {
 public:
  SigslotTester1(sigslot::signal1<A1>* signal,
                C1* capture1)
      : callback_count_(0),
      capture1_(capture1) {
    signal->connect(this, &SigslotTester1::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1) {
    callback_count_++;
    *capture1_ = arg1;
  }

  int callback_count_;
  C1* capture1_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester1);
};

template <class A1, class A2, class C1, class C2>
class SigslotTester2 : public sigslot::has_slots<> {
 public:
  SigslotTester2(sigslot::signal2<A1, A2>* signal,
                C1* capture1, C2* capture2)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2) {
    signal->connect(this, &SigslotTester2::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester2);
};

template <class A1, class A2, class A3, class C1, class C2, class C3>
class SigslotTester3 : public sigslot::has_slots<> {
 public:
  SigslotTester3(sigslot::signal3<A1, A2, A3>* signal,
                C1* capture1, C2* capture2, C3* capture3)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2), capture3_(capture3) {
    signal->connect(this, &SigslotTester3::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2, A3 arg3) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
    *capture3_ = arg3;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;
  C3* capture3_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester3);
};

template <class A1, class A2, class A3, class A4, class C1, class C2, class C3,
    class C4>
class SigslotTester4 : public sigslot::has_slots<> {
 public:
  SigslotTester4(sigslot::signal4<A1, A2, A3, A4>* signal,
                C1* capture1, C2* capture2, C3* capture3, C4* capture4)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2), capture3_(capture3),
          capture4_(capture4) {
    signal->connect(this, &SigslotTester4::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2, A3 arg3, A4 arg4) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
    *capture3_ = arg3;
    *capture4_ = arg4;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;
  C3* capture3_;
  C4* capture4_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester4);
};

template <class A1, class A2, class A3, class A4, class A5, class C1, class C2,
    class C3, class C4, class C5>
class SigslotTester5 : public sigslot::has_slots<> {
 public:
  SigslotTester5(sigslot::signal5<A1, A2, A3, A4, A5>* signal,
                C1* capture1, C2* capture2, C3* capture3, C4* capture4,
                    C5* capture5)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2), capture3_(capture3),
          capture4_(capture4), capture5_(capture5) {
    signal->connect(this, &SigslotTester5::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
    *capture3_ = arg3;
    *capture4_ = arg4;
    *capture5_ = arg5;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;
  C3* capture3_;
  C4* capture4_;
  C5* capture5_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester5);
};
}  

#endif  
