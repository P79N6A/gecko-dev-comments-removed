



#include "base/logging.h"
#include "base/simple_thread.h"
#include "base/thread_local.h"
#include "base/waitable_event.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class ThreadLocalTesterBase : public base::DelegateSimpleThreadPool::Delegate {
 public:
  typedef base::ThreadLocalPointer<ThreadLocalTesterBase> TLPType;

  ThreadLocalTesterBase(TLPType* tlp, base::WaitableEvent* done)
      : tlp_(tlp), done_(done) { }
  ~ThreadLocalTesterBase() { }

 protected:
  TLPType* tlp_;
  base::WaitableEvent* done_;
};

class SetThreadLocal : public ThreadLocalTesterBase {
 public:
  SetThreadLocal(TLPType* tlp, base::WaitableEvent* done)
      : ThreadLocalTesterBase(tlp, done), val_(NULL) { }
  ~SetThreadLocal() { }

  void set_value(ThreadLocalTesterBase* val) { val_ = val; }

  virtual void Run() {
    DCHECK(!done_->IsSignaled());
    tlp_->Set(val_);
    done_->Signal();
  }

 private:
  ThreadLocalTesterBase* val_;
};

class GetThreadLocal : public ThreadLocalTesterBase {
 public:
  GetThreadLocal(TLPType* tlp, base::WaitableEvent* done)
      : ThreadLocalTesterBase(tlp, done), ptr_(NULL) { }
  ~GetThreadLocal() { }

  void set_ptr(ThreadLocalTesterBase** ptr) { ptr_ = ptr; }

  virtual void Run() {
    DCHECK(!done_->IsSignaled());
    *ptr_ = tlp_->Get();
    done_->Signal();
  }

 private:
  ThreadLocalTesterBase** ptr_;
};

}  



TEST(ThreadLocalTest, Pointer) {
  base::DelegateSimpleThreadPool tp1("ThreadLocalTest tp1", 1);
  base::DelegateSimpleThreadPool tp2("ThreadLocalTest tp1", 1);
  tp1.Start();
  tp2.Start();

  base::ThreadLocalPointer<ThreadLocalTesterBase> tlp;

  static ThreadLocalTesterBase* const kBogusPointer =
      reinterpret_cast<ThreadLocalTesterBase*>(0x1234);

  ThreadLocalTesterBase* tls_val;
  base::WaitableEvent done(true, false);

  GetThreadLocal getter(&tlp, &done);
  getter.set_ptr(&tls_val);

  
  tls_val = kBogusPointer;
  done.Reset();
  tp1.AddWork(&getter);
  done.Wait();
  EXPECT_EQ(static_cast<ThreadLocalTesterBase*>(NULL), tls_val);

  tls_val = kBogusPointer;
  done.Reset();
  tp2.AddWork(&getter);
  done.Wait();
  EXPECT_EQ(static_cast<ThreadLocalTesterBase*>(NULL), tls_val);


  SetThreadLocal setter(&tlp, &done);
  setter.set_value(kBogusPointer);

  
  done.Reset();
  tp1.AddWork(&setter);
  done.Wait();

  tls_val = NULL;
  done.Reset();
  tp1.AddWork(&getter);
  done.Wait();
  EXPECT_EQ(kBogusPointer, tls_val);

  
  tls_val = kBogusPointer;
  done.Reset();
  tp2.AddWork(&getter);
  done.Wait();
  EXPECT_EQ(static_cast<ThreadLocalTesterBase*>(NULL), tls_val);

  
  setter.set_value(kBogusPointer + 1);

  done.Reset();
  tp2.AddWork(&setter);
  done.Wait();

  tls_val = NULL;
  done.Reset();
  tp2.AddWork(&getter);
  done.Wait();
  EXPECT_EQ(kBogusPointer + 1, tls_val);

  
  tls_val = NULL;
  done.Reset();
  tp1.AddWork(&getter);
  done.Wait();
  EXPECT_EQ(kBogusPointer, tls_val);

  tp1.JoinAll();
  tp2.JoinAll();
}

TEST(ThreadLocalTest, Boolean) {
  {
    base::ThreadLocalBoolean tlb;
    EXPECT_EQ(false, tlb.Get());

    tlb.Set(false);
    EXPECT_EQ(false, tlb.Get());

    tlb.Set(true);
    EXPECT_EQ(true, tlb.Get());
  }

  
  {
    base::ThreadLocalBoolean tlb;
    EXPECT_EQ(false, tlb.Get());
  }
}
