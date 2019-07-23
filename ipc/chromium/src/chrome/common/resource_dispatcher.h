





#ifndef CHROME_COMMON_RESOURCE_DISPATCHER_H__
#define CHROME_COMMON_RESOURCE_DISPATCHER_H__

#include <deque>
#include <queue>
#include <string>

#include "base/hash_tables.h"
#include "base/shared_memory.h"
#include "base/task.h"
#include "chrome/common/filter_policy.h"
#include "chrome/common/ipc_channel.h"
#include "webkit/glue/resource_loader_bridge.h"

struct ResourceResponseHead;




class ResourceDispatcher {
 public:
  explicit ResourceDispatcher(IPC::Message::Sender* sender);
  ~ResourceDispatcher();

  
  
  bool OnMessageReceived(const IPC::Message& message);

  
  
  
  webkit_glue::ResourceLoaderBridge* CreateBridge(const std::string& method,
    const GURL& url,
    const GURL& policy_url,
    const GURL& referrer,
    const std::string& frame_origin,
    const std::string& main_frame_origin,
    const std::string& headers,
    int load_flags,
    int origin_pid,
    ResourceType::Type resource_type,
    uint32 request_context ,
    int app_cache_context_id,
    int route_id);

  
  
  int AddPendingRequest(webkit_glue::ResourceLoaderBridge::Peer* callback,
                        ResourceType::Type resource_type);

  
  
  bool RemovePendingRequest(int request_id);

  IPC::Message::Sender* message_sender() const {
    return message_sender_;
  }

  
  void SetDefersLoading(int request_id, bool value);

 private:
  friend class ResourceDispatcherTest;

  typedef std::deque<IPC::Message*> MessageQueue;
  struct PendingRequestInfo {
    PendingRequestInfo() { }
    PendingRequestInfo(webkit_glue::ResourceLoaderBridge::Peer* peer,
                       ResourceType::Type resource_type)
        : peer(peer),
          resource_type(resource_type),
          filter_policy(FilterPolicy::DONT_FILTER),
          is_deferred(false) {
    }
    ~PendingRequestInfo() { }
    webkit_glue::ResourceLoaderBridge::Peer* peer;
    ResourceType::Type resource_type;
    FilterPolicy::Type filter_policy;
    MessageQueue deferred_message_queue;
    bool is_deferred;
  };
  typedef base::hash_map<int, PendingRequestInfo> PendingRequestList;

  
  void OnUploadProgress(const IPC::Message& message,
                        int request_id,
                        int64 position,
                        int64 size);
  void OnDownloadProgress(const IPC::Message& message,
                          int request_id, int64 position, int64 size);
  void OnReceivedResponse(int request_id, const ResourceResponseHead&);
  void OnReceivedRedirect(int request_id, const GURL& new_url);
  void OnReceivedData(const IPC::Message& message,
                      int request_id,
                      base::SharedMemoryHandle data,
                      int data_len);
  void OnRequestComplete(int request_id,
                         const URLRequestStatus& status,
                         const std::string& security_info);

  
  void DispatchMessage(const IPC::Message& message);

  
  
  void FlushDeferredMessages(int request_id);

  
  static bool IsResourceDispatcherMessage(const IPC::Message& message);

  IPC::Message::Sender* message_sender_;

  
  PendingRequestList pending_requests_;

  ScopedRunnableMethodFactory<ResourceDispatcher> method_factory_;

  DISALLOW_EVIL_CONSTRUCTORS(ResourceDispatcher);
};

#endif  
