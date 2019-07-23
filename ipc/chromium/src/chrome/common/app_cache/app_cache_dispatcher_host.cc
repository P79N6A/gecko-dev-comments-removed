



#include "chrome/common/app_cache/app_cache_dispatcher_host.h"

#include "chrome/common/render_messages.h"

AppCacheDispatcherHost::~AppCacheDispatcherHost() {
  if (sender_) {
    
    
  }
}

void AppCacheDispatcherHost::Initialize(IPC::Message::Sender* sender) {
  DCHECK(sender);
  sender_ = sender;
  
  
}

bool AppCacheDispatcherHost::OnMessageReceived(const IPC::Message& msg,
                                               bool *msg_ok) {
  DCHECK(sender_);
  *msg_ok = true;
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(AppCacheDispatcherHost, msg, *msg_ok)
    IPC_MESSAGE_HANDLER(AppCacheMsg_ContextCreated, OnContextCreated);
    IPC_MESSAGE_HANDLER(AppCacheMsg_ContextDestroyed, OnContextDestroyed);
    IPC_MESSAGE_HANDLER(AppCacheMsg_SelectAppCache, OnSelectAppCache);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()
  return handled;
}

void AppCacheDispatcherHost::OnContextCreated(
                                 WebAppCacheContext::ContextType context_type,
                                 int context_id,
                                 int opt_parent_id) {
  
  DCHECK(context_id != WebAppCacheContext::kNoAppCacheContextId);
}

void AppCacheDispatcherHost::OnContextDestroyed(int context_id) {
  
  DCHECK(context_id != WebAppCacheContext::kNoAppCacheContextId);
}

void AppCacheDispatcherHost::OnSelectAppCache(
                                int context_id,
                                int select_request_id,
                                const GURL& document_url,
                                int64 cache_document_was_loaded_from,
                                const GURL& opt_manifest_url) {
  
  DCHECK(context_id != WebAppCacheContext::kNoAppCacheContextId);
  Send(new AppCacheMsg_AppCacheSelected(context_id, select_request_id,
                                        WebAppCacheContext::kNoAppCacheId));
}
