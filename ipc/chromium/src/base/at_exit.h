



#ifndef BASE_AT_EXIT_H_
#define BASE_AT_EXIT_H_

#include <stack>

#include "base/basictypes.h"
#include "base/lock.h"

namespace base {















class AtExitManager {
 protected:
  
  
  
  
  AtExitManager(bool shadow);

 public:
  typedef void (*AtExitCallbackType)(void*);

  AtExitManager();

  
  
  ~AtExitManager();

  
  
  static void RegisterCallback(AtExitCallbackType func, void* param);

  
  
  static void ProcessCallbacksNow();

 private:
  struct CallbackAndParam {
    CallbackAndParam(AtExitCallbackType func, void* param)
        : func_(func), param_(param) { }
    AtExitCallbackType func_;
    void* param_;
  };

  Lock lock_;
  std::stack<CallbackAndParam> stack_;
  AtExitManager* next_manager_;  

  DISALLOW_COPY_AND_ASSIGN(AtExitManager);
};

}  

#endif  
