



















#include "condition_variable_win.h"
#include "critical_section_win.h"
#include "trace.h"

namespace webrtc {

bool ConditionVariableWindows::win_support_condition_variables_primitive_ =
    false;
static HMODULE library = NULL;

PInitializeConditionVariable  PInitializeConditionVariable_;
PSleepConditionVariableCS     PSleepConditionVariableCS_;
PWakeConditionVariable        PWakeConditionVariable_;
PWakeAllConditionVariable     PWakeAllConditionVariable_;

typedef void (WINAPI *PInitializeConditionVariable)(PCONDITION_VARIABLE);
typedef BOOL (WINAPI *PSleepConditionVariableCS)(PCONDITION_VARIABLE,
                                                 PCRITICAL_SECTION, DWORD);
typedef void (WINAPI *PWakeConditionVariable)(PCONDITION_VARIABLE);
typedef void (WINAPI *PWakeAllConditionVariable)(PCONDITION_VARIABLE);

ConditionVariableWindows::ConditionVariableWindows()
    : eventID_(WAKEALL_0) {
  if (!library) {
    
    library = LoadLibrary(TEXT("Kernel32.dll"));
    if (library) {
      WEBRTC_TRACE(kTraceStateInfo, kTraceUtility, -1, "Loaded Kernel.dll");

      PInitializeConditionVariable_ =
          (PInitializeConditionVariable) GetProcAddress(
              library, "InitializeConditionVariable");
      PSleepConditionVariableCS_ = (PSleepConditionVariableCS) GetProcAddress(
          library, "SleepConditionVariableCS");
      PWakeConditionVariable_ = (PWakeConditionVariable) GetProcAddress(
          library, "WakeConditionVariable");
      PWakeAllConditionVariable_ = (PWakeAllConditionVariable) GetProcAddress(
          library, "WakeAllConditionVariable");

      if (PInitializeConditionVariable_ && PSleepConditionVariableCS_
          && PWakeConditionVariable_ && PWakeAllConditionVariable_) {
        WEBRTC_TRACE(
            kTraceStateInfo, kTraceUtility, -1,
            "Loaded native condition variables");
        win_support_condition_variables_primitive_ = true;
      }
    }
  }

  if (win_support_condition_variables_primitive_) {
    PInitializeConditionVariable_(&condition_variable_);

    events_[WAKEALL_0] = NULL;
    events_[WAKEALL_1] = NULL;
    events_[WAKE] = NULL;

  } else {
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
}

ConditionVariableWindows::~ConditionVariableWindows() {
  if (!win_support_condition_variables_primitive_) {
    CloseHandle(events_[WAKE]);
    CloseHandle(events_[WAKEALL_1]);
    CloseHandle(events_[WAKEALL_0]);

    DeleteCriticalSection(&num_waiters_crit_sect_);
  }
}

void ConditionVariableWindows::SleepCS(CriticalSectionWrapper& crit_sect) {
  SleepCS(crit_sect, INFINITE);
}

bool ConditionVariableWindows::SleepCS(CriticalSectionWrapper& crit_sect,
                                       unsigned long max_time_in_ms) {
  CriticalSectionWindows* cs =
      reinterpret_cast<CriticalSectionWindows*>(&crit_sect);

  if (win_support_condition_variables_primitive_) {
    BOOL ret_val = PSleepConditionVariableCS_(
        &condition_variable_, &(cs->crit), max_time_in_ms);
    return (ret_val == 0) ? false : true;
  } else {
    EnterCriticalSection(&num_waiters_crit_sect_);

    
    
    const EventWakeUpType eventID =
        (WAKEALL_0 == eventID_) ? WAKEALL_1 : WAKEALL_0;

    ++(num_waiters_[eventID]);
    LeaveCriticalSection(&num_waiters_crit_sect_);

    LeaveCriticalSection(&cs->crit);
    HANDLE events[2];
    events[0] = events_[WAKE];
    events[1] = events_[eventID];
    const DWORD result = WaitForMultipleObjects(2,  
        events, FALSE,  
        max_time_in_ms);

    const bool ret_val = (result != WAIT_TIMEOUT);

    EnterCriticalSection(&num_waiters_crit_sect_);
    --(num_waiters_[eventID]);

    
    
    const bool last_waiter = (result == WAIT_OBJECT_0 + 1)
        && (num_waiters_[eventID] == 0);
    LeaveCriticalSection(&num_waiters_crit_sect_);

    if (last_waiter) {
      
      
      ResetEvent(events_[eventID]);
    }

    EnterCriticalSection(&cs->crit);
    return ret_val;
  }
}

void ConditionVariableWindows::Wake() {
  if (win_support_condition_variables_primitive_) {
    PWakeConditionVariable_(&condition_variable_);
  } else {
    EnterCriticalSection(&num_waiters_crit_sect_);
    const bool have_waiters = (num_waiters_[WAKEALL_0] > 0)
        || (num_waiters_[WAKEALL_1] > 0);
    LeaveCriticalSection(&num_waiters_crit_sect_);

    if (have_waiters) {
      SetEvent(events_[WAKE]);
    }
  }
}

void ConditionVariableWindows::WakeAll() {
  if (win_support_condition_variables_primitive_) {
    PWakeAllConditionVariable_(&condition_variable_);
  } else {
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

} 
