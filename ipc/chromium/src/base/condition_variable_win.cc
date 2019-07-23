



#include "base/condition_variable.h"

#include <stack>

#include "base/lock.h"
#include "base/logging.h"
#include "base/time.h"

using base::TimeDelta;

ConditionVariable::ConditionVariable(Lock* user_lock)
  : user_lock_(*user_lock),
    run_state_(RUNNING),
    allocation_counter_(0),
    recycling_list_size_(0) {
  DCHECK(user_lock);
}

ConditionVariable::~ConditionVariable() {
  AutoLock auto_lock(internal_lock_);
  run_state_ = SHUTDOWN;  

  DCHECK_EQ(recycling_list_size_, allocation_counter_);
  if (recycling_list_size_ != allocation_counter_) {  
    
    
    
    
    
    
    
    
    AutoUnlock auto_unlock(internal_lock_);
    Broadcast();  
    Sleep(10);  
    
  }  

  DCHECK_EQ(recycling_list_size_, allocation_counter_);
}

void ConditionVariable::Wait() {
  
  
  TimedWait(TimeDelta::FromMilliseconds(INFINITE));
}

void ConditionVariable::TimedWait(const TimeDelta& max_time) {
  Event* waiting_event;
  HANDLE handle;
  {
    AutoLock auto_lock(internal_lock_);
    if (RUNNING != run_state_) return;  
    waiting_event = GetEventForWaiting();
    handle = waiting_event->handle();
    DCHECK(handle);
  }  

  {
    AutoUnlock unlock(user_lock_);  
    WaitForSingleObject(handle, static_cast<DWORD>(max_time.InMilliseconds()));
    
    AutoLock auto_lock(internal_lock_);
    RecycleEvent(waiting_event);
    
  }  
}



void ConditionVariable::Broadcast() {
  std::stack<HANDLE> handles;  
  {
    AutoLock auto_lock(internal_lock_);
    if (waiting_list_.IsEmpty())
      return;
    while (!waiting_list_.IsEmpty())
      
      handles.push(waiting_list_.PopBack()->handle());
  }  
  while (!handles.empty()) {
    SetEvent(handles.top());
    handles.pop();
  }
}





void ConditionVariable::Signal() {
  HANDLE handle;
  {
    AutoLock auto_lock(internal_lock_);
    if (waiting_list_.IsEmpty())
      return;  
    
    
     handle = waiting_list_.PopBack()->handle();  
  }  
  SetEvent(handle);
}





ConditionVariable::Event* ConditionVariable::GetEventForWaiting() {
  
  Event* cv_event;
  if (0 == recycling_list_size_) {
    DCHECK(recycling_list_.IsEmpty());
    cv_event = new Event();
    cv_event->InitListElement();
    allocation_counter_++;
    
    CHECK(cv_event->handle());
  } else {
    cv_event = recycling_list_.PopFront();
    recycling_list_size_--;
  }
  waiting_list_.PushBack(cv_event);
  return cv_event;
}






void ConditionVariable::RecycleEvent(Event* used_event) {
  
  
  
  
  used_event->Extract();  
  recycling_list_.PushBack(used_event);
  recycling_list_size_++;
}





























ConditionVariable::Event::Event() : handle_(0) {
  next_ = prev_ = this;  
}

ConditionVariable::Event::~Event() {
  if (0 == handle_) {
    
    while (!IsEmpty()) {
      Event* cv_event = PopFront();
      DCHECK(cv_event->ValidateAsItem());
      delete cv_event;
    }
  }
  DCHECK(IsSingleton());
  if (0 != handle_) {
    int ret_val = CloseHandle(handle_);
    DCHECK(ret_val);
  }
}


void ConditionVariable::Event::InitListElement() {
  DCHECK(!handle_);
  handle_ = CreateEvent(NULL, false, false, NULL);
  CHECK(handle_);
}


bool ConditionVariable::Event::IsEmpty() const {
  DCHECK(ValidateAsList());
  return IsSingleton();
}

void ConditionVariable::Event::PushBack(Event* other) {
  DCHECK(ValidateAsList());
  DCHECK(other->ValidateAsItem());
  DCHECK(other->IsSingleton());
  
  other->prev_ = prev_;
  other->next_ = this;
  
  prev_->next_ = other;
  prev_ = other;
  DCHECK(ValidateAsDistinct(other));
}

ConditionVariable::Event* ConditionVariable::Event::PopFront() {
  DCHECK(ValidateAsList());
  DCHECK(!IsSingleton());
  return next_->Extract();
}

ConditionVariable::Event* ConditionVariable::Event::PopBack() {
  DCHECK(ValidateAsList());
  DCHECK(!IsSingleton());
  return prev_->Extract();
}



HANDLE ConditionVariable::Event::handle() const {
  DCHECK(ValidateAsItem());
  return handle_;
}


ConditionVariable::Event* ConditionVariable::Event::Extract() {
  DCHECK(ValidateAsItem());
  if (!IsSingleton()) {
    
    next_->prev_ = prev_;
    prev_->next_ = next_;
    
    prev_ = next_ = this;
  }
  DCHECK(IsSingleton());
  return this;
}


bool ConditionVariable::Event::IsSingleton() const {
  DCHECK(ValidateLinks());
  return next_ == this;
}


bool ConditionVariable::Event::ValidateAsDistinct(Event* other) const {
  return ValidateLinks() && other->ValidateLinks() && (this != other);
}

bool ConditionVariable::Event::ValidateAsItem() const {
  return (0 != handle_) && ValidateLinks();
}

bool ConditionVariable::Event::ValidateAsList() const {
  return (0 == handle_) && ValidateLinks();
}

bool ConditionVariable::Event::ValidateLinks() const {
  
  
  
  return (next_->prev_ == this) && (prev_->next_ == this);
}










































































































































































