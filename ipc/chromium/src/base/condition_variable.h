































































#ifndef BASE_CONDITION_VARIABLE_H_
#define BASE_CONDITION_VARIABLE_H_

#include "base/lock.h"

namespace base {
  class TimeDelta;
}

class ConditionVariable {
 public:
  
  explicit ConditionVariable(Lock* user_lock);

  ~ConditionVariable();

  
  
  void Wait();
  void TimedWait(const base::TimeDelta& max_time);

  
  void Broadcast();
  
  void Signal();

 private:

#if defined(OS_WIN)

  
  
  
  
  
  
  class Event {
   public:
    
    Event();
    ~Event();

    
    void InitListElement();

    
    bool IsEmpty() const;
    void PushBack(Event* other);
    Event* PopFront();
    Event* PopBack();

    
    
    HANDLE handle() const;
    
    Event* Extract();

    
    bool IsSingleton() const;

   private:
    
    bool ValidateAsDistinct(Event* other) const;
    bool ValidateAsItem() const;
    bool ValidateAsList() const;
    bool ValidateLinks() const;

    HANDLE handle_;
    Event* next_;
    Event* prev_;
    DISALLOW_COPY_AND_ASSIGN(Event);
  };

  
  
  enum RunState { SHUTDOWN = 0, RUNNING = 64213 };

  
  Event* GetEventForWaiting();
  void RecycleEvent(Event* used_event);

  RunState run_state_;

  
  Lock internal_lock_;

  
  Lock& user_lock_;

  
  Event waiting_list_;

  
  Event recycling_list_;
  int recycling_list_size_;

  
  int allocation_counter_;

#elif defined(OS_POSIX)

  pthread_cond_t condition_;
  pthread_mutex_t* user_mutex_;

#endif

  DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

#endif  
