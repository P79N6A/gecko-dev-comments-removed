











#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/thread_checker.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_disable.h"



#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define ENABLE_THREAD_CHECKER 1
#else
#define ENABLE_THREAD_CHECKER 0
#endif

namespace rtc {

namespace {




class ThreadCheckerClass : public ThreadChecker {
 public:
  ThreadCheckerClass() {}

  
  void DoStuff() {
    DCHECK(CalledOnValidThread());
  }

  void DetachFromThread() {
    ThreadChecker::DetachFromThread();
  }

  static void MethodOnDifferentThreadImpl();
  static void DetachThenCallFromDifferentThreadImpl();

 private:
  DISALLOW_COPY_AND_ASSIGN(ThreadCheckerClass);
};


class CallDoStuffOnThread : public Thread {
 public:
  explicit CallDoStuffOnThread(ThreadCheckerClass* thread_checker_class)
      : Thread(),
        thread_checker_class_(thread_checker_class) {
    SetName("call_do_stuff_on_thread", NULL);
  }

  virtual void Run() OVERRIDE {
    thread_checker_class_->DoStuff();
  }

  
  
  void Join() {
    Thread::Join();
  }

 private:
  ThreadCheckerClass* thread_checker_class_;

  DISALLOW_COPY_AND_ASSIGN(CallDoStuffOnThread);
};


class DeleteThreadCheckerClassOnThread : public Thread {
 public:
  explicit DeleteThreadCheckerClassOnThread(
      ThreadCheckerClass* thread_checker_class)
      : Thread(),
        thread_checker_class_(thread_checker_class) {
    SetName("delete_thread_checker_class_on_thread", NULL);
  }

  virtual void Run() OVERRIDE {
    thread_checker_class_.reset();
  }

  
  
  void Join() {
    Thread::Join();
  }

 private:
  scoped_ptr<ThreadCheckerClass> thread_checker_class_;

  DISALLOW_COPY_AND_ASSIGN(DeleteThreadCheckerClassOnThread);
};

}  

TEST(ThreadCheckerTest, CallsAllowedOnSameThread) {
  scoped_ptr<ThreadCheckerClass> thread_checker_class(
      new ThreadCheckerClass);

  
  thread_checker_class->DoStuff();

  
  thread_checker_class.reset();
}

TEST(ThreadCheckerTest, DestructorAllowedOnDifferentThread) {
  scoped_ptr<ThreadCheckerClass> thread_checker_class(
      new ThreadCheckerClass);

  
  
  DeleteThreadCheckerClassOnThread delete_on_thread(
      thread_checker_class.release());

  delete_on_thread.Start();
  delete_on_thread.Join();
}

TEST(ThreadCheckerTest, DetachFromThread) {
  scoped_ptr<ThreadCheckerClass> thread_checker_class(
      new ThreadCheckerClass);

  
  
  thread_checker_class->DetachFromThread();
  CallDoStuffOnThread call_on_thread(thread_checker_class.get());

  call_on_thread.Start();
  call_on_thread.Join();
}

#if GTEST_HAS_DEATH_TEST || !ENABLE_THREAD_CHECKER

void ThreadCheckerClass::MethodOnDifferentThreadImpl() {
  scoped_ptr<ThreadCheckerClass> thread_checker_class(
      new ThreadCheckerClass);

  
  
  CallDoStuffOnThread call_on_thread(thread_checker_class.get());

  call_on_thread.Start();
  call_on_thread.Join();
}

#if ENABLE_THREAD_CHECKER
TEST(ThreadCheckerDeathTest, MethodNotAllowedOnDifferentThreadInDebug) {
  ASSERT_DEATH({
      ThreadCheckerClass::MethodOnDifferentThreadImpl();
    }, "");
}
#else
TEST(ThreadCheckerTest, MethodAllowedOnDifferentThreadInRelease) {
  ThreadCheckerClass::MethodOnDifferentThreadImpl();
}
#endif  

void ThreadCheckerClass::DetachThenCallFromDifferentThreadImpl() {
  scoped_ptr<ThreadCheckerClass> thread_checker_class(
      new ThreadCheckerClass);

  
  
  thread_checker_class->DetachFromThread();
  CallDoStuffOnThread call_on_thread(thread_checker_class.get());

  call_on_thread.Start();
  call_on_thread.Join();

  
  
  thread_checker_class->DoStuff();
}

#if ENABLE_THREAD_CHECKER
TEST(ThreadCheckerDeathTest, DetachFromThreadInDebug) {
  ASSERT_DEATH({
    ThreadCheckerClass::DetachThenCallFromDifferentThreadImpl();
    }, "");
}
#else
TEST(ThreadCheckerTest, DetachFromThreadInRelease) {
  ThreadCheckerClass::DetachThenCallFromDifferentThreadImpl();
}
#endif  

#endif  


#undef ENABLE_THREAD_CHECKER

}  
