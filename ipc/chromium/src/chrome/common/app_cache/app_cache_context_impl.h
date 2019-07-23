



#ifndef CHROME_COMMON_APP_CACHE_APP_CACHE_CONTEXT_IMPL_H_
#define CHROME_COMMON_APP_CACHE_APP_CACHE_CONTEXT_IMPL_H_

#include "base/id_map.h"
#include "chrome/common/ipc_message.h"
#include "webkit/glue/webappcachecontext.h"


class AppCacheContextImpl : public WebAppCacheContext {
 public:
  
  static AppCacheContextImpl* FromContextId(int id);

  AppCacheContextImpl(IPC::Message::Sender* sender);
  virtual ~AppCacheContextImpl();

  
  virtual int GetContextID() { return context_id_; }
  virtual int64 GetAppCacheID() { return app_cache_id_; }
  virtual void Initialize(WebAppCacheContext::ContextType context_type,
                          WebAppCacheContext* opt_parent);
  virtual void SelectAppCacheWithoutManifest(
                                     const GURL& document_url,
                                     int64 cache_document_was_loaded_from);
  virtual void SelectAppCacheWithManifest(
                                     const GURL& document_url,
                                     int64 cache_document_was_loaded_from,
                                     const GURL& manifest_url);

  
  void OnAppCacheSelected(int select_request_id, int64 app_cache_id);

 private:
  void UnInitializeContext();

  int context_id_;
  int64 app_cache_id_;
  int pending_select_request_id_;
  IPC::Message::Sender* sender_;

  static IDMap<AppCacheContextImpl> all_contexts;
};

#endif  
