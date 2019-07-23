



#include "base/compiler_specific.h"
#include "base/lock.h"
#include "base/platform_thread.h"
#include "base/simple_thread.h"
#include "base/thread_collision_warner.h"
#include "testing/gtest/include/gtest/gtest.h"


MSVC_PUSH_DISABLE_WARNING(4822)


#if defined(NDEBUG)


#undef DFAKE_MUTEX
#define DFAKE_MUTEX(obj) scoped_ptr<base::AsserterBase> obj


#define EXPECT_NDEBUG_FALSE_DEBUG_TRUE EXPECT_FALSE

#else


#define EXPECT_NDEBUG_FALSE_DEBUG_TRUE EXPECT_TRUE

#endif


namespace {




class AssertReporter : public base::AsserterBase {
 public:
  AssertReporter()
      : failed_(false) {}

  virtual void warn() {
    failed_ = true;
  }

  virtual ~AssertReporter() {}

  bool fail_state() const { return failed_; }
  void reset() { failed_ = false; }

 private:
  bool failed_;
};

}  

TEST(ThreadCollisionTest, BookCriticalSection) {
  AssertReporter* local_reporter = new AssertReporter();

  base::ThreadCollisionWarner warner(local_reporter);
  EXPECT_FALSE(local_reporter->fail_state());

  {  
    DFAKE_SCOPED_LOCK_THREAD_LOCKED(warner);
    EXPECT_FALSE(local_reporter->fail_state());
    {  
      DFAKE_SCOPED_LOCK_THREAD_LOCKED(warner);
      EXPECT_FALSE(local_reporter->fail_state());
    }
  }
}

TEST(ThreadCollisionTest, ScopedRecursiveBookCriticalSection) {
  AssertReporter* local_reporter = new AssertReporter();

  base::ThreadCollisionWarner warner(local_reporter);
  EXPECT_FALSE(local_reporter->fail_state());

  {  
    DFAKE_SCOPED_RECURSIVE_LOCK(warner);
    EXPECT_FALSE(local_reporter->fail_state());
    {  
      DFAKE_SCOPED_RECURSIVE_LOCK(warner);
      EXPECT_FALSE(local_reporter->fail_state());
    }  
  }  

  
  {  
    DFAKE_SCOPED_LOCK(warner);
    EXPECT_FALSE(local_reporter->fail_state());
  }  
}

TEST(ThreadCollisionTest, ScopedBookCriticalSection) {
  AssertReporter* local_reporter = new AssertReporter();

  base::ThreadCollisionWarner warner(local_reporter);
  EXPECT_FALSE(local_reporter->fail_state());

  {  
    DFAKE_SCOPED_LOCK(warner);
    EXPECT_FALSE(local_reporter->fail_state());
  }  

  {  
    DFAKE_SCOPED_LOCK(warner);
    EXPECT_FALSE(local_reporter->fail_state());
    {
      
      DFAKE_SCOPED_LOCK(warner);
      EXPECT_NDEBUG_FALSE_DEBUG_TRUE(local_reporter->fail_state());
      
      local_reporter->reset();
    }  
  }  

  {
    
    DFAKE_SCOPED_LOCK(warner);
    EXPECT_FALSE(local_reporter->fail_state());
  }  
}

TEST(ThreadCollisionTest, MTBookCriticalSectionTest) {
  class NonThreadSafeQueue {
   public:
    explicit NonThreadSafeQueue(base::AsserterBase* asserter)
        : push_pop_(asserter) {
    }

    void push(int value) {
      DFAKE_SCOPED_LOCK_THREAD_LOCKED(push_pop_);
    }

    int pop() {
      DFAKE_SCOPED_LOCK_THREAD_LOCKED(push_pop_);
      return 0;
    }

   private:
    DFAKE_MUTEX(push_pop_);

    DISALLOW_COPY_AND_ASSIGN(NonThreadSafeQueue);
  };

  class QueueUser : public base::DelegateSimpleThread::Delegate {
   public:
    explicit QueueUser(NonThreadSafeQueue& queue)
        : queue_(queue) {}

    virtual void Run() {
      queue_.push(0);
      queue_.pop();
    }

   private:
    NonThreadSafeQueue& queue_;
  };

  AssertReporter* local_reporter = new AssertReporter();

  NonThreadSafeQueue queue(local_reporter);

  QueueUser queue_user_a(queue);
  QueueUser queue_user_b(queue);

  base::DelegateSimpleThread thread_a(&queue_user_a, "queue_user_thread_a");
  base::DelegateSimpleThread thread_b(&queue_user_b, "queue_user_thread_b");

  thread_a.Start();
  thread_b.Start();

  thread_a.Join();
  thread_b.Join();

  EXPECT_NDEBUG_FALSE_DEBUG_TRUE(local_reporter->fail_state());
}

TEST(ThreadCollisionTest, MTScopedBookCriticalSectionTest) {
  
  
  class NonThreadSafeQueue {
   public:
    explicit NonThreadSafeQueue(base::AsserterBase* asserter)
        : push_pop_(asserter) {
    }

    void push(int value) {
      DFAKE_SCOPED_LOCK(push_pop_);
      PlatformThread::Sleep(5000);
    }

    int pop() {
      DFAKE_SCOPED_LOCK(push_pop_);
      return 0;
    }

   private:
    DFAKE_MUTEX(push_pop_);

    DISALLOW_COPY_AND_ASSIGN(NonThreadSafeQueue);
  };

  class QueueUser : public base::DelegateSimpleThread::Delegate {
   public:
    explicit QueueUser(NonThreadSafeQueue& queue)
        : queue_(queue) {}

    virtual void Run() {
      queue_.push(0);
      queue_.pop();
    }

   private:
    NonThreadSafeQueue& queue_;
  };

  AssertReporter* local_reporter = new AssertReporter();

  NonThreadSafeQueue queue(local_reporter);

  QueueUser queue_user_a(queue);
  QueueUser queue_user_b(queue);

  base::DelegateSimpleThread thread_a(&queue_user_a, "queue_user_thread_a");
  base::DelegateSimpleThread thread_b(&queue_user_b, "queue_user_thread_b");

  thread_a.Start();
  thread_b.Start();

  thread_a.Join();
  thread_b.Join();

  EXPECT_NDEBUG_FALSE_DEBUG_TRUE(local_reporter->fail_state());
}

TEST(ThreadCollisionTest, MTSynchedScopedBookCriticalSectionTest) {
  
  
  class NonThreadSafeQueue {
   public:
    explicit NonThreadSafeQueue(base::AsserterBase* asserter)
        : push_pop_(asserter) {
    }

    void push(int value) {
      DFAKE_SCOPED_LOCK(push_pop_);
      PlatformThread::Sleep(5000);
    }

    int pop() {
      DFAKE_SCOPED_LOCK(push_pop_);
      return 0;
    }

   private:
    DFAKE_MUTEX(push_pop_);

    DISALLOW_COPY_AND_ASSIGN(NonThreadSafeQueue);
  };

  
  
  class QueueUser : public base::DelegateSimpleThread::Delegate {
   public:
    QueueUser(NonThreadSafeQueue& queue, Lock& lock)
        : queue_(queue),
          lock_(lock) {}

    virtual void Run() {
      {
        AutoLock auto_lock(lock_);
        queue_.push(0);
      }
      {
        AutoLock auto_lock(lock_);
        queue_.pop();
      }
    }
   private:
    NonThreadSafeQueue& queue_;
    Lock& lock_;
  };

  AssertReporter* local_reporter = new AssertReporter();

  NonThreadSafeQueue queue(local_reporter);

  Lock lock;

  QueueUser queue_user_a(queue, lock);
  QueueUser queue_user_b(queue, lock);

  base::DelegateSimpleThread thread_a(&queue_user_a, "queue_user_thread_a");
  base::DelegateSimpleThread thread_b(&queue_user_b, "queue_user_thread_b");

  thread_a.Start();
  thread_b.Start();

  thread_a.Join();
  thread_b.Join();

  EXPECT_FALSE(local_reporter->fail_state());
}

TEST(ThreadCollisionTest, MTSynchedScopedRecursiveBookCriticalSectionTest) {
  
  
  class NonThreadSafeQueue {
   public:
    explicit NonThreadSafeQueue(base::AsserterBase* asserter)
        : push_pop_(asserter) {
    }

    void push(int) {
      DFAKE_SCOPED_RECURSIVE_LOCK(push_pop_);
      bar();
      PlatformThread::Sleep(5000);
    }

    int pop() {
      DFAKE_SCOPED_RECURSIVE_LOCK(push_pop_);
      return 0;
    }

    void bar() {
      DFAKE_SCOPED_RECURSIVE_LOCK(push_pop_);
    }

   private:
    DFAKE_MUTEX(push_pop_);

    DISALLOW_COPY_AND_ASSIGN(NonThreadSafeQueue);
  };

  
  
  class QueueUser : public base::DelegateSimpleThread::Delegate {
   public:
    QueueUser(NonThreadSafeQueue& queue, Lock& lock)
        : queue_(queue),
          lock_(lock) {}

    virtual void Run() {
      {
        AutoLock auto_lock(lock_);
        queue_.push(0);
      }
      {
        AutoLock auto_lock(lock_);
        queue_.bar();
      }
      {
        AutoLock auto_lock(lock_);
        queue_.pop();
      }
    }
   private:
    NonThreadSafeQueue& queue_;
    Lock& lock_;
  };

  AssertReporter* local_reporter = new AssertReporter();

  NonThreadSafeQueue queue(local_reporter);

  Lock lock;

  QueueUser queue_user_a(queue, lock);
  QueueUser queue_user_b(queue, lock);

  base::DelegateSimpleThread thread_a(&queue_user_a, "queue_user_thread_a");
  base::DelegateSimpleThread thread_b(&queue_user_b, "queue_user_thread_b");

  thread_a.Start();
  thread_b.Start();

  thread_a.Join();
  thread_b.Join();

  EXPECT_FALSE(local_reporter->fail_state());
}
