
















#include "webrtc/system_wrappers/source/condition_variable_event_win.h"
#include "webrtc/system_wrappers/source/critical_section_win.h"

namespace webrtc {

ConditionVariableEventWin::ConditionVariableEventWin() : eventID_(WAKEALL_0) {
  memset(&num_waiters_[0], 0, sizeof(num_waiters_));

  InitializeCriticalSection(&num_waiters_crit_sect_);

  events_[WAKEALL_0] = CreateEvent(NULL,  
                                   TRUE,  
                                   FALSE,  
                                   NULL);  

  events_[WAKEALL_1] = CreateEvent(NULL,  
                                   TRUE,  
                                   FALSE,  
                                   NULL);  

  events_[WAKE] = CreateEvent(NULL,  
                              FALSE,  
                              FALSE,  
                              NULL);  
}

ConditionVariableEventWin::~ConditionVariableEventWin() {
  CloseHandle(events_[WAKE]);
  CloseHandle(events_[WAKEALL_1]);
  CloseHandle(events_[WAKEALL_0]);

  DeleteCriticalSection(&num_waiters_crit_sect_);
}

void ConditionVariableEventWin::SleepCS(CriticalSectionWrapper& crit_sect) {
  SleepCS(crit_sect, INFINITE);
}

bool ConditionVariableEventWin::SleepCS(CriticalSectionWrapper& crit_sect,
                                        unsigned long max_time_in_ms) {
  EnterCriticalSection(&num_waiters_crit_sect_);

  
  
  const EventWakeUpType eventID =
      (WAKEALL_0 == eventID_) ? WAKEALL_1 : WAKEALL_0;

  ++(num_waiters_[eventID]);
  LeaveCriticalSection(&num_waiters_crit_sect_);

  CriticalSectionWindows* cs =
      static_cast<CriticalSectionWindows*>(&crit_sect);
  LeaveCriticalSection(&cs->crit);
  HANDLE events[2];
  events[0] = events_[WAKE];
  events[1] = events_[eventID];
  const DWORD result = WaitForMultipleObjects(2,  
                                              events,
                                              FALSE,  
                                              max_time_in_ms);

  const bool ret_val = (result != WAIT_TIMEOUT);

  EnterCriticalSection(&num_waiters_crit_sect_);
  --(num_waiters_[eventID]);

  
  
  const bool last_waiter = (result == WAIT_OBJECT_0 + 1) &&
      (num_waiters_[eventID] == 0);
  LeaveCriticalSection(&num_waiters_crit_sect_);

  if (last_waiter) {
    
    
    ResetEvent(events_[eventID]);
  }

  EnterCriticalSection(&cs->crit);
  return ret_val;
}

void ConditionVariableEventWin::Wake() {
  EnterCriticalSection(&num_waiters_crit_sect_);
  const bool have_waiters = (num_waiters_[WAKEALL_0] > 0) ||
      (num_waiters_[WAKEALL_1] > 0);
  LeaveCriticalSection(&num_waiters_crit_sect_);

  if (have_waiters) {
    SetEvent(events_[WAKE]);
  }
}

void ConditionVariableEventWin::WakeAll() {
  EnterCriticalSection(&num_waiters_crit_sect_);

  
  eventID_ = (WAKEALL_0 == eventID_) ? WAKEALL_1 : WAKEALL_0;

  
  const EventWakeUpType eventID = eventID_;
  const bool have_waiters = num_waiters_[eventID] > 0;
  LeaveCriticalSection(&num_waiters_crit_sect_);

  if (have_waiters) {
    SetEvent(events_[eventID]);
  }
}

}  
