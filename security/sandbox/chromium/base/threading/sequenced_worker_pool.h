



#ifndef BASE_THREADING_SEQUENCED_WORKER_POOL_H_
#define BASE_THREADING_SEQUENCED_WORKER_POOL_H_

#include <cstddef>
#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/task_runner.h"

namespace tracked_objects {
class Location;
}  

namespace base {

class MessageLoopProxy;

template <class T> class DeleteHelper;

class SequencedTaskRunner;














































class BASE_EXPORT SequencedWorkerPool : public TaskRunner {
 public:
  
  
  enum WorkerShutdown {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    CONTINUE_ON_SHUTDOWN,

    
    
    
    
    
    
    
    
    SKIP_ON_SHUTDOWN,

    
    
    
    
    
    
    
    
    
    
    BLOCK_SHUTDOWN,
  };

  
  
  class SequenceToken {
   public:
    SequenceToken() : id_(0) {}
    ~SequenceToken() {}

    bool Equals(const SequenceToken& other) const {
      return id_ == other.id_;
    }

    
    bool IsValid() const {
      return id_ != 0;
    }

   private:
    friend class SequencedWorkerPool;

    explicit SequenceToken(int id) : id_(id) {}

    int id_;
  };

  
  class TestingObserver {
   public:
    virtual ~TestingObserver() {}
    virtual void OnHasWork() = 0;
    virtual void WillWaitForShutdown() = 0;
    virtual void OnDestruct() = 0;
  };

  
  
  
  static SequenceToken GetSequenceTokenForCurrentThread();

  
  
  

  
  
  SequencedWorkerPool(size_t max_threads,
                      const std::string& thread_name_prefix);

  
  
  SequencedWorkerPool(size_t max_threads,
                      const std::string& thread_name_prefix,
                      TestingObserver* observer);

  
  
  SequenceToken GetSequenceToken();

  
  
  
  
  SequenceToken GetNamedSequenceToken(const std::string& name);

  
  
  
  
  scoped_refptr<SequencedTaskRunner> GetSequencedTaskRunner(
      SequenceToken token);

  
  
  
  
  scoped_refptr<SequencedTaskRunner> GetSequencedTaskRunnerWithShutdownBehavior(
      SequenceToken token,
      WorkerShutdown shutdown_behavior);

  
  
  
  
  scoped_refptr<TaskRunner> GetTaskRunnerWithShutdownBehavior(
      WorkerShutdown shutdown_behavior);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool PostWorkerTask(const tracked_objects::Location& from_here,
                      const Closure& task);

  
  
  
  
  
  
  
  
  
  bool PostDelayedWorkerTask(const tracked_objects::Location& from_here,
                             const Closure& task,
                             TimeDelta delay);

  
  bool PostWorkerTaskWithShutdownBehavior(
      const tracked_objects::Location& from_here,
      const Closure& task,
      WorkerShutdown shutdown_behavior);

  
  
  
  
  
  
  
  
  
  
  
  bool PostSequencedWorkerTask(SequenceToken sequence_token,
                               const tracked_objects::Location& from_here,
                               const Closure& task);

  
  
  bool PostNamedSequencedWorkerTask(const std::string& token_name,
                                    const tracked_objects::Location& from_here,
                                    const Closure& task);

  
  
  
  
  
  
  
  
  
  bool PostDelayedSequencedWorkerTask(
      SequenceToken sequence_token,
      const tracked_objects::Location& from_here,
      const Closure& task,
      TimeDelta delay);

  
  
  bool PostSequencedWorkerTaskWithShutdownBehavior(
      SequenceToken sequence_token,
      const tracked_objects::Location& from_here,
      const Closure& task,
      WorkerShutdown shutdown_behavior);

  
  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       const Closure& task,
                       TimeDelta delay) override;
  bool RunsTasksOnCurrentThread() const override;

  
  
  bool IsRunningSequenceOnCurrentThread(SequenceToken sequence_token) const;

  
  
  
  
  
  
  
  
  
  void FlushForTesting();

  
  void SignalHasWorkForTesting();

  
  
  
  
  
  void Shutdown() { Shutdown(0); }

  
  
  
  
  
  
  
  void Shutdown(int max_new_blocking_tasks_after_shutdown);

  
  
  
  
  bool IsShutdownInProgress();

 protected:
  ~SequencedWorkerPool() override;

  void OnDestruct() const override;

 private:
  friend class RefCountedThreadSafe<SequencedWorkerPool>;
  friend class DeleteHelper<SequencedWorkerPool>;

  class Inner;
  class Worker;

  const scoped_refptr<MessageLoopProxy> constructor_message_loop_;

  
  
  const scoped_ptr<Inner> inner_;

  DISALLOW_COPY_AND_ASSIGN(SequencedWorkerPool);
};

}  

#endif
