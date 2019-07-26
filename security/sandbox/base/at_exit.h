



#ifndef BASE_AT_EXIT_H_
#define BASE_AT_EXIT_H_

#include <stack>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/callback.h"
#include "base/synchronization/lock.h"

namespace base {















class BASE_EXPORT AtExitManager {
 public:
  typedef void (*AtExitCallbackType)(void*);

  AtExitManager();

  
  
  ~AtExitManager();

  
  
  static void RegisterCallback(AtExitCallbackType func, void* param);

  
  static void RegisterTask(base::Closure task);

  
  
  static void ProcessCallbacksNow();

 protected:
  
  
  
  
  explicit AtExitManager(bool shadow);

 private:
  base::Lock lock_;
  std::stack<base::Closure> stack_;
  AtExitManager* next_manager_;  

  DISALLOW_COPY_AND_ASSIGN(AtExitManager);
};

#if defined(UNIT_TEST)
class ShadowingAtExitManager : public AtExitManager {
 public:
  ShadowingAtExitManager() : AtExitManager(true) {}
};
#endif  

}  

#endif  
