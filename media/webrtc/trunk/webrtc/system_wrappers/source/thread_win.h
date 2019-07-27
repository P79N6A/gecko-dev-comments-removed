









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_WIN_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_THREAD_WIN_H_

#include "webrtc/system_wrappers/interface/thread_wrapper.h"

#include <windows.h>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"

namespace webrtc {

class ThreadWindows : public ThreadWrapper {
 public:
  ThreadWindows(ThreadRunFunction func, ThreadObj obj, ThreadPriority prio,
                const char* thread_name);
  virtual ~ThreadWindows();

  virtual bool Start(unsigned int& id);
  bool SetAffinity(const int* processor_numbers,
                   const unsigned int amount_of_processors);
  virtual bool Stop();
  virtual void SetNotAlive();

  static unsigned int WINAPI StartThread(LPVOID lp_parameter);

 protected:
  virtual void Run();

  void SetThreadNameHelper();
  void ThreadStoppedHelper();

  ThreadRunFunction    run_function_;
  ThreadObj            obj_;

  bool                    alive_;
  bool                    dead_;

  
  
  
  
  bool                    do_not_close_handle_;
  ThreadPriority          prio_;
  EventWrapper*           event_;
  CriticalSectionWrapper* critsect_stop_;

  HANDLE                  thread_;
  unsigned int            id_;
  char                    name_[kThreadMaxNameLength];
  bool                    set_thread_name_;

};

class ThreadWindowsUI : public ThreadWindows {
 public:
  ThreadWindowsUI(ThreadRunFunction func, ThreadObj obj, ThreadPriority prio,
                  const char* thread_name) :
  ThreadWindows(func, obj, prio, thread_name),
  hwnd_(nullptr),
  timerid_(0) {
 }

 virtual bool Start(unsigned int& id);
 virtual bool Stop();

 


 void RequestCallback();

 


 bool RequestCallbackTimer(unsigned int milliseconds);

 protected:
  virtual void Run();

 private:
  static LRESULT CALLBACK EventWindowProc(HWND, UINT, WPARAM, LPARAM);
  void NativeEventCallback();
  bool InternalInit();

  HWND hwnd_;
  UINT_PTR timerid_;
};


}  

#endif  
