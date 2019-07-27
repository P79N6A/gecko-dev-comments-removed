









#ifndef WEBRTC_BASE_WORKER_H_
#define WEBRTC_BASE_WORKER_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/messagehandler.h"

namespace rtc {

class Thread;












class Worker : private MessageHandler {
 public:
  Worker();

  
  virtual ~Worker();

  
  
  bool StartWork();
  
  
  bool StopWork();

 protected:
  
  
  void HaveWork();

  
  
  virtual void OnStart() = 0;
  
  virtual void OnHaveWork() = 0;
  
  
  virtual void OnStop() = 0;

 private:
  
  virtual void OnMessage(Message *msg);

  
  Thread *worker_thread_;

  DISALLOW_COPY_AND_ASSIGN(Worker);
};

}  

#endif  
