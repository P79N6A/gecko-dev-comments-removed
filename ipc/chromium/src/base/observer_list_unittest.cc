



#include "base/message_loop.h"
#include "base/observer_list.h"
#include "base/observer_list_threadsafe.h"
#include "base/platform_thread.h"
#include "base/ref_counted.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;

namespace {

class ObserverListTest : public testing::Test {
};

class Foo {
 public:
  virtual void Observe(int x) = 0;
  virtual ~Foo() {}
};

class Adder : public Foo {
 public:
  explicit Adder(int scaler) : total(0), scaler_(scaler) {}
  virtual void Observe(int x) {
    total += x * scaler_;
  }
  virtual ~Adder() { }
  int total;
 private:
  int scaler_;
};

class Disrupter : public Foo {
 public:
  Disrupter(ObserverList<Foo>* list, Foo* doomed)
      : list_(list), doomed_(doomed) { }
  virtual ~Disrupter() { }
  virtual void Observe(int x) {
    list_->RemoveObserver(doomed_);
  }
 private:
  ObserverList<Foo>* list_;
  Foo* doomed_;
};

class ThreadSafeDisrupter : public Foo {
 public:
  ThreadSafeDisrupter(ObserverListThreadSafe<Foo>* list, Foo* doomed)
      : list_(list), doomed_(doomed) { }
  virtual ~ThreadSafeDisrupter() { }
  virtual void Observe(int x) {
    list_->RemoveObserver(doomed_);
  }
 private:
  ObserverListThreadSafe<Foo>* list_;
  Foo* doomed_;
};

class AddInObserve : public Foo {
 public:
  AddInObserve(ObserverList<Foo>* observer_list)
      : added(false),
        observer_list(observer_list),
        adder(1) {
  }
  virtual void Observe(int x) {
    if (!added) {
      added = true;
      observer_list->AddObserver(&adder);
    }
  }

  bool added;
  ObserverList<Foo>* observer_list;
  Adder adder;
};


class ObserverListThreadSafeTest : public testing::Test {
};

static const int kThreadRunTime = 10000;  




class AddRemoveThread : public PlatformThread::Delegate,
                        public Foo {
 public:
  AddRemoveThread(ObserverListThreadSafe<Foo>* list, bool notify)
      : list_(list),
        in_list_(false),
        start_(Time::Now()),
        count_observes_(0),
        count_addtask_(0),
        do_notifies_(notify) {
    factory_ = new ScopedRunnableMethodFactory<AddRemoveThread>(this);
  }

  virtual ~AddRemoveThread() {
    delete factory_;
  }

  void ThreadMain() {
    loop_ = new MessageLoop();  
    loop_->PostTask(FROM_HERE,
      factory_->NewRunnableMethod(&AddRemoveThread::AddTask));
    loop_->Run();
    
    
    delete loop_;
    loop_ = reinterpret_cast<MessageLoop*>(0xdeadbeef);
    delete this;
  }

  
  
  void AddTask() {
    count_addtask_++;

    if ((Time::Now() - start_).InMilliseconds() > kThreadRunTime) {
      LOG(INFO) << "DONE!";
      return;
    }

    if (!in_list_) {
      list_->AddObserver(this);
      in_list_ = true;
    }

    if (do_notifies_) {
      list_->Notify(&Foo::Observe, 10);
    }

    loop_->PostDelayedTask(FROM_HERE,
      factory_->NewRunnableMethod(&AddRemoveThread::AddTask), 0);
  }

  void Quit() {
    loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask());
  }

  virtual void Observe(int x) {
    count_observes_++;

    
    
    DCHECK(in_list_);

    
    EXPECT_EQ(loop_, MessageLoop::current());

    list_->RemoveObserver(this);
    in_list_ = false;
  }

 private:
  ObserverListThreadSafe<Foo>* list_;
  MessageLoop* loop_;
  bool in_list_;        
                        
  Time start_;          

  int count_observes_;  
  int count_addtask_;   
  bool do_notifies_;    

  ScopedRunnableMethodFactory<AddRemoveThread>* factory_;
};

}  

TEST(ObserverListTest, BasicTest) {
  ObserverList<Foo> observer_list;
  Adder a(1), b(-1), c(1), d(-1);
  Disrupter evil(&observer_list, &c);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  FOR_EACH_OBSERVER(Foo, observer_list, Observe(10));

  observer_list.AddObserver(&evil);
  observer_list.AddObserver(&c);
  observer_list.AddObserver(&d);

  FOR_EACH_OBSERVER(Foo, observer_list, Observe(10));

  EXPECT_EQ(a.total, 20);
  EXPECT_EQ(b.total, -20);
  EXPECT_EQ(c.total, 0);
  EXPECT_EQ(d.total, -10);
}

TEST(ObserverListThreadSafeTest, BasicTest) {
  MessageLoop loop;

  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);
  Adder a(1);
  Adder b(-1);
  Adder c(1);
  Adder d(-1);
  ThreadSafeDisrupter evil(observer_list.get(), &c);

  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  observer_list->Notify(&Foo::Observe, 10);
  loop.RunAllPending();

  observer_list->AddObserver(&evil);
  observer_list->AddObserver(&c);
  observer_list->AddObserver(&d);

  observer_list->Notify(&Foo::Observe, 10);
  loop.RunAllPending();

  EXPECT_EQ(a.total, 20);
  EXPECT_EQ(b.total, -20);
  EXPECT_EQ(c.total, 0);
  EXPECT_EQ(d.total, -10);
}







static void ThreadSafeObserverHarness(int num_threads,
                                      bool cross_thread_notifies) {
  MessageLoop loop;

  const int kMaxThreads = 15;
  num_threads = num_threads > kMaxThreads ? kMaxThreads : num_threads;

  scoped_refptr<ObserverListThreadSafe<Foo> > observer_list(
      new ObserverListThreadSafe<Foo>);
  Adder a(1);
  Adder b(-1);
  Adder c(1);
  Adder d(-1);

  observer_list->AddObserver(&a);
  observer_list->AddObserver(&b);

  AddRemoveThread* threaded_observer[kMaxThreads];
  PlatformThreadHandle threads[kMaxThreads];
  for (int index = 0; index < num_threads; index++) {
    threaded_observer[index] = new AddRemoveThread(observer_list.get(), false);
    EXPECT_TRUE(PlatformThread::Create(0,
                threaded_observer[index], &threads[index]));
  }

  Time start = Time::Now();
  while (true) {
    if ((Time::Now() - start).InMilliseconds() > kThreadRunTime)
      break;

    observer_list->Notify(&Foo::Observe, 10);

    loop.RunAllPending();
  }

  for (int index = 0; index < num_threads; index++) {
    threaded_observer[index]->Quit();
    PlatformThread::Join(threads[index]);
  }
}

TEST(ObserverListThreadSafeTest, CrossThreadObserver) {
  
  
  ThreadSafeObserverHarness(7, false);
}

TEST(ObserverListThreadSafeTest, CrossThreadNotifications) {
  
  
  ThreadSafeObserverHarness(3, true);
}

TEST(ObserverListTest, Existing) {
  ObserverList<Foo> observer_list(ObserverList<Foo>::NOTIFY_EXISTING_ONLY);
  Adder a(1);
  AddInObserve b(&observer_list);

  observer_list.AddObserver(&a);
  observer_list.AddObserver(&b);

  FOR_EACH_OBSERVER(Foo, observer_list, Observe(1));

  EXPECT_TRUE(b.added);
  
  
  EXPECT_EQ(0, b.adder.total);

  
  FOR_EACH_OBSERVER(Foo, observer_list, Observe(1));
  EXPECT_EQ(1, b.adder.total);
}
