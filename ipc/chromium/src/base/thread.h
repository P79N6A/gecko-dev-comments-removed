



#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_

#include <string>

#include "base/message_loop.h"
#include "base/platform_thread.h"

namespace base {






class Thread : PlatformThread::Delegate {
 public:
  struct Options {
    
    MessageLoop::Type message_loop_type;

    
    
    
    size_t stack_size;

    Options() : message_loop_type(MessageLoop::TYPE_DEFAULT), stack_size(0) {}
    Options(MessageLoop::Type type, size_t size)
        : message_loop_type(type), stack_size(size) {}
  };

  
  
  explicit Thread(const char *name);

  
  
  
  
  
  virtual ~Thread();

  
  
  
  
  
  
  
  bool Start();

  
  
  
  
  
  
  bool StartWithOptions(const Options& options);

  
  
  
  
  
  
  
  
  
  
  
  void Stop();

  
  
  
  
  
  
  
  
  
  
  
  void StopSoon();

  
  
  
  
  
  
  
  
  MessageLoop* message_loop() const { return message_loop_; }

  
  const std::string &thread_name() { return name_; }

  
  PlatformThreadHandle thread_handle() { return thread_; }

  
  PlatformThreadId thread_id() const { return thread_id_; }

  
  
  bool IsRunning() const { return thread_id_ != 0; }

 protected:
  
  virtual void Init() {}

  
  virtual void CleanUp() {}

  static void SetThreadWasQuitProperly(bool flag);
  static bool GetThreadWasQuitProperly();

 private:
  
  virtual void ThreadMain();

  
  
  bool thread_was_started() const { return startup_data_ != NULL; }

  
  struct StartupData;
  StartupData* startup_data_;

  
  PlatformThreadHandle thread_;

  
  
  MessageLoop* message_loop_;

  
  PlatformThreadId thread_id_;

  
  std::string name_;

  friend class ThreadQuitTask;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  

#endif  
