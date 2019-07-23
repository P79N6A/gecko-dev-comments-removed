



#ifndef CHROME_COMMON_APP_CACHE_APP_CACHE_DISPATCHER_HOST_H_
#define CHROME_COMMON_APP_CACHE_APP_CACHE_DISPATCHER_HOST_H_

#include "base/id_map.h"
#include "chrome/common/ipc_message.h"
#include "webkit/glue/webappcachecontext.h"

class GURL;





class AppCacheDispatcherHost {
 public:
  AppCacheDispatcherHost() : sender_(NULL) {}
  ~AppCacheDispatcherHost();
  void Initialize(IPC::Message::Sender* sender);
  bool OnMessageReceived(const IPC::Message& msg, bool* msg_is_ok);

 private:
  
  void OnContextCreated(WebAppCacheContext::ContextType context_type,
                        int context_id, int opt_parent_id);
  void OnContextDestroyed(int context_id);
  void OnSelectAppCache(int context_id,
                        int select_request_id,
                        const GURL& document_url,
                        int64 cache_document_was_loaded_from,
                        const GURL& opt_manifest_url);

  bool Send(IPC::Message* msg) {
    return sender_->Send(msg);
  }

  IPC::Message::Sender* sender_;
};

#endif  
