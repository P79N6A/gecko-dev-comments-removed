






































#ifndef BASE_SIMPLE_THREAD_H_
#define BASE_SIMPLE_THREAD_H_

#include <string>
#include <queue>
#include <vector>

#include "base/basictypes.h"
#include "base/lock.h"
#include "base/waitable_event.h"
#include "base/platform_thread.h"

namespace base {



class SimpleThread : public PlatformThread::Delegate {
 public:
  class Options {
   public:
    Options() : stack_size_(0) { }
    ~Options() { }

    

    
    void set_stack_size(size_t size) { stack_size_ = size; }
    size_t stack_size() const { return stack_size_; }
   private:
    size_t stack_size_;
  };

  
  
  
  
  explicit SimpleThread(const std::string& name_prefix)
      : name_prefix_(name_prefix), name_(name_prefix),
        thread_(), event_(true, false), tid_(0), joined_(false) { }
  SimpleThread(const std::string& name_prefix, const Options& options)
      : name_prefix_(name_prefix), name_(name_prefix), options_(options),
        thread_(), event_(true, false), tid_(0), joined_(false) { }

  virtual ~SimpleThread();

  virtual void Start();
  virtual void Join();

  
  virtual void ThreadMain();

  
  virtual void Run() = 0;

  
  std::string name_prefix() { return name_prefix_; }

  
  std::string name() { return name_; }

  
  PlatformThreadId tid() { return tid_; }

  
  bool HasBeenStarted() { return event_.IsSignaled(); }

  
  bool HasBeenJoined() { return joined_; }

 private:
  const std::string name_prefix_;
  std::string name_;
  const Options options_;
  PlatformThreadHandle thread_;  
  WaitableEvent event_;          
  PlatformThreadId tid_;         
  bool joined_;                  
};

class DelegateSimpleThread : public SimpleThread {
 public:
  class Delegate {
   public:
    Delegate() { }
    virtual ~Delegate() { }
    virtual void Run() = 0;
  };

  DelegateSimpleThread(Delegate* delegate,
                       const std::string& name_prefix)
      : SimpleThread(name_prefix), delegate_(delegate) { }
  DelegateSimpleThread(Delegate* delegate,
                       const std::string& name_prefix,
                       const Options& options)
      : SimpleThread(name_prefix, options), delegate_(delegate) { }

  virtual ~DelegateSimpleThread() { }
  virtual void Run();
 private:
  Delegate* delegate_;
};










class DelegateSimpleThreadPool : public DelegateSimpleThread::Delegate {
 public:
  typedef DelegateSimpleThread::Delegate Delegate;

  DelegateSimpleThreadPool(const std::string name_prefix, int num_threads)
      : name_prefix_(name_prefix), num_threads_(num_threads),
        dry_(true, false) { }
  ~DelegateSimpleThreadPool();

  
  
  void Start();

  
  
  void JoinAll();

  
  
  void AddWork(Delegate* work, int repeat_count);
  void AddWork(Delegate* work) {
    AddWork(work, 1);
  }

  
  virtual void Run();

 private:
  const std::string name_prefix_;
  int num_threads_;
  std::vector<DelegateSimpleThread*> threads_;
  std::queue<Delegate*> delegates_;
  Lock lock_;            
  WaitableEvent dry_;    
};

}  

#endif  
