



#ifndef CHROME_COMMON_APP_CACHE_APP_CACHE_DISPATCHER_H_
#define CHROME_COMMON_APP_CACHE_APP_CACHE_DISPATCHER_H_

#include "base/basictypes.h"
#include "chrome/common/ipc_message.h"





class AppCacheDispatcher {
 public:
  bool OnMessageReceived(const IPC::Message& msg);

 private:
  
  void OnAppCacheSelected(int context_id,
                          int select_request_id,
                          int64 app_cache_id);
};

#endif  
