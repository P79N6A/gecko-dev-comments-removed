





#include "chrome/common/resource_dispatcher.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/message_loop.h"
#include "base/shared_memory.h"
#include "base/string_util.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/security_filter_peer.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "webkit/glue/resource_type.h"
#include "webkit/glue/webkit_glue.h"




#ifdef LOG_RESOURCE_REQUESTS
# define RESOURCE_LOG(stuff) LOG(INFO) << stuff
#else
# define RESOURCE_LOG(stuff)
#endif


static int MakeRequestID() {
  
  
  
  static int next_request_id = 0;
  return next_request_id++;
}



namespace webkit_glue {

class IPCResourceLoaderBridge : public ResourceLoaderBridge {
 public:
  IPCResourceLoaderBridge(ResourceDispatcher* dispatcher,
                          const std::string& method,
                          const GURL& url,
                          const GURL& policy_url,
                          const GURL& referrer,
                          const std::string& frame_origin,
                          const std::string& main_frame_origin,
                          const std::string& headers,
                          int load_flags,
                          int origin_pid,
                          ResourceType::Type resource_type,
                          uint32 request_context,
                          int app_cache_context_id,
                          int route_id);
  virtual ~IPCResourceLoaderBridge();

  
  virtual void AppendDataToUpload(const char* data, int data_len);
  virtual void AppendFileRangeToUpload(const FilePath& path,
                                       uint64 offset, uint64 length);
  virtual void SetUploadIdentifier(int64 identifier);
  virtual bool Start(Peer* peer);
  virtual void Cancel();
  virtual void SetDefersLoading(bool value);
  virtual void SyncLoad(SyncLoadResponse* response);

#ifdef LOG_RESOURCE_REQUESTS
  const std::string& url() const { return url_; }
#endif

 private:
  ResourceLoaderBridge::Peer* peer_;

  
  
  ResourceDispatcher* dispatcher_;

  
  
  ViewHostMsg_Resource_Request request_;

  
  int request_id_;

  
  int route_id_;

#ifdef LOG_RESOURCE_REQUESTS
  
  std::string url_;
#endif
};

IPCResourceLoaderBridge::IPCResourceLoaderBridge(
    ResourceDispatcher* dispatcher,
    const std::string& method,
    const GURL& url,
    const GURL& policy_url,
    const GURL& referrer,
    const std::string& frame_origin,
    const std::string& main_frame_origin,
    const std::string& headers,
    int load_flags,
    int origin_pid,
    ResourceType::Type resource_type,
    uint32 request_context,
    int app_cache_context_id,
    int route_id)
    : peer_(NULL),
      dispatcher_(dispatcher),
      request_id_(-1),
      route_id_(route_id) {
  DCHECK(dispatcher_) << "no resource dispatcher";
  request_.method = method;
  request_.url = url;
  request_.policy_url = policy_url;
  request_.referrer = referrer;
  request_.frame_origin = frame_origin;
  request_.main_frame_origin = main_frame_origin;
  request_.headers = headers;
  request_.load_flags = load_flags;
  request_.origin_pid = origin_pid;
  request_.resource_type = resource_type;
  request_.request_context = request_context;
  request_.app_cache_context_id = app_cache_context_id;

#ifdef LOG_RESOURCE_REQUESTS
  url_ = url.possibly_invalid_spec();
#endif
}

IPCResourceLoaderBridge::~IPCResourceLoaderBridge() {
  
  
  if (request_id_ >= 0) {
    
    
    dispatcher_->RemovePendingRequest(request_id_);
  }
}

void IPCResourceLoaderBridge::AppendDataToUpload(const char* data,
                                                 int data_len) {
  DCHECK(request_id_ == -1) << "request already started";

  
  if (data_len == 0)
    return;

  if (!request_.upload_data)
    request_.upload_data = new net::UploadData();
  request_.upload_data->AppendBytes(data, data_len);
}

void IPCResourceLoaderBridge::AppendFileRangeToUpload(
    const FilePath& path, uint64 offset, uint64 length) {
  DCHECK(request_id_ == -1) << "request already started";

  if (!request_.upload_data)
    request_.upload_data = new net::UploadData();
  request_.upload_data->AppendFileRange(path, offset, length);
}

void IPCResourceLoaderBridge::SetUploadIdentifier(int64 identifier) {
  DCHECK(request_id_ == -1) << "request already started";

  if (!request_.upload_data)
    request_.upload_data = new net::UploadData();
  request_.upload_data->set_identifier(identifier);
}


bool IPCResourceLoaderBridge::Start(Peer* peer) {
  if (request_id_ != -1) {
    NOTREACHED() << "Starting a request twice";
    return false;
  }

  RESOURCE_LOG("Starting request for " << url_);

  peer_ = peer;

  
  request_id_ = dispatcher_->AddPendingRequest(peer_, request_.resource_type);

  return dispatcher_->message_sender()->Send(
      new ViewHostMsg_RequestResource(route_id_, request_id_, request_));
}

void IPCResourceLoaderBridge::Cancel() {
  if (request_id_ < 0) {
    NOTREACHED() << "Trying to cancel an unstarted request";
    return;
  }

  RESOURCE_LOG("Canceling request for " << url_);

  dispatcher_->message_sender()->Send(
      new ViewHostMsg_CancelRequest(route_id_, request_id_));

  
  
  
}

void IPCResourceLoaderBridge::SetDefersLoading(bool value) {
  if (request_id_ < 0) {
    NOTREACHED() << "Trying to (un)defer an unstarted request";
    return;
  }

  dispatcher_->SetDefersLoading(request_id_, value);
}

void IPCResourceLoaderBridge::SyncLoad(SyncLoadResponse* response) {
  if (request_id_ != -1) {
    NOTREACHED() << "Starting a request twice";
    response->status.set_status(URLRequestStatus::FAILED);
    return;
  }

  RESOURCE_LOG("Making sync request for " << url_);

  request_id_ = MakeRequestID();

  SyncLoadResult result;
  IPC::Message* msg = new ViewHostMsg_SyncLoad(route_id_, request_id_,
                                               request_, &result);
  if (!dispatcher_->message_sender()->Send(msg)) {
    response->status.set_status(URLRequestStatus::FAILED);
    return;
  }

  response->status = result.status;
  response->url = result.final_url;
  response->headers = result.headers;
  response->mime_type = result.mime_type;
  response->charset = result.charset;
  response->data.swap(result.data);
}

}  



ResourceDispatcher::ResourceDispatcher(IPC::Message::Sender* sender)
    : message_sender_(sender),
      ALLOW_THIS_IN_INITIALIZER_LIST(method_factory_(this)) {
}

ResourceDispatcher::~ResourceDispatcher() {
}



bool ResourceDispatcher::OnMessageReceived(const IPC::Message& message) {
  if (!IsResourceDispatcherMessage(message)) {
    return false;
  }

  int request_id;

  void* iter = NULL;
  if (!message.ReadInt(&iter, &request_id)) {
    NOTREACHED() << "malformed resource message";
    return true;
  }

  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    
    
    DLOG(WARNING) << "Got response for a nonexistant or finished request";
    return true;
  }

  PendingRequestInfo& request_info = it->second;
  if (request_info.is_deferred) {
    request_info.deferred_message_queue.push_back(new IPC::Message(message));
    return true;
  }
  
  if (!request_info.deferred_message_queue.empty())
    FlushDeferredMessages(request_id);

  DispatchMessage(message);
  return true;
}

void ResourceDispatcher::OnDownloadProgress(
    const IPC::Message& message, int request_id, int64 position, int64 size) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
     DLOG(WARNING) << "Got download progress for a nonexistant or "
         " finished requests";
     return;
  }

  PendingRequestInfo& request_info = it->second;

  RESOURCE_LOG("Dispatching download progress for " <<
               request_info.peer->GetURLForDebugging());
  request_info.peer->OnDownloadProgress(position, size);

  
  IPC::Message::Sender* sender = message_sender();
  if (sender) {
    sender->Send(
        new ViewHostMsg_DownloadProgress_ACK(message.routing_id(), request_id));
  }
}

void ResourceDispatcher::OnUploadProgress(
    const IPC::Message& message, int request_id, int64 position, int64 size) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    
    
    DLOG(WARNING) << "Got upload progress for a nonexistant or "
        "finished request";
    return;
  }

  PendingRequestInfo& request_info = it->second;

  RESOURCE_LOG("Dispatching upload progress for " <<
               request_info.peer->GetURLForDebugging());
  request_info.peer->OnUploadProgress(position, size);

  
  message_sender()->Send(
      new ViewHostMsg_UploadProgress_ACK(message.routing_id(), request_id));
}

void ResourceDispatcher::OnReceivedResponse(
    int request_id, const ResourceResponseHead& response_head) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    
    
    DLOG(WARNING) << "Got response for a nonexistant or finished request";
    return;
  }

  PendingRequestInfo& request_info = it->second;
  request_info.filter_policy = response_head.filter_policy;
  webkit_glue::ResourceLoaderBridge::Peer* peer = request_info.peer;
  if (request_info.filter_policy != FilterPolicy::DONT_FILTER) {
    
    webkit_glue::ResourceLoaderBridge::Peer* new_peer =
        SecurityFilterPeer::CreateSecurityFilterPeer(
            NULL, peer,
            request_info.resource_type, response_head.mime_type,
            request_info.filter_policy,
            net::ERR_INSECURE_RESPONSE);
    if (new_peer) {
      request_info.peer = new_peer;
      peer = new_peer;
    }
  }

  RESOURCE_LOG("Dispatching response for " << peer->GetURLForDebugging());
  peer->OnReceivedResponse(response_head, false);
}

void ResourceDispatcher::OnReceivedData(const IPC::Message& message,
                                        int request_id,
                                        base::SharedMemoryHandle shm_handle,
                                        int data_len) {
  
  message_sender()->Send(
      new ViewHostMsg_DataReceived_ACK(message.routing_id(), request_id));

  const bool shm_valid = base::SharedMemory::IsHandleValid(shm_handle);
  DCHECK((shm_valid && data_len > 0) || (!shm_valid && !data_len));
  base::SharedMemory shared_mem(shm_handle, true);  

  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    
    
    DLOG(WARNING) << "Got data for a nonexistant or finished request";
    return;
  }

  PendingRequestInfo& request_info = it->second;

  if (data_len > 0 && shared_mem.Map(data_len)) {
    RESOURCE_LOG("Dispatching " << data_len << " bytes for " <<
                 request_info.peer->GetURLForDebugging());
    const char* data = static_cast<char*>(shared_mem.memory());
    request_info.peer->OnReceivedData(data, data_len);
  }
}

void ResourceDispatcher::OnReceivedRedirect(int request_id,
                                            const GURL& new_url) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    
    
    DLOG(WARNING) << "Got data for a nonexistant or finished request";
    return;
  }

  PendingRequestInfo& request_info = it->second;

  RESOURCE_LOG("Dispatching redirect for " <<
               request_info.peer->GetURLForDebugging());
  request_info.peer->OnReceivedRedirect(new_url);
}

void ResourceDispatcher::OnRequestComplete(int request_id,
                                           const URLRequestStatus& status,
                                           const std::string& security_info) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    
    
    DLOG(WARNING) << "Got 'complete' for a nonexistant or finished request";
    return;
  }

  PendingRequestInfo& request_info = it->second;
  webkit_glue::ResourceLoaderBridge::Peer* peer = request_info.peer;

  RESOURCE_LOG("Dispatching complete for " <<
               request_info.peer->GetURLForDebugging());

  if (status.status() == URLRequestStatus::CANCELED &&
      status.os_error() != net::ERR_ABORTED) {
    
    SecurityFilterPeer* new_peer =
        SecurityFilterPeer::CreateSecurityFilterPeerForDeniedRequest(
            request_info.resource_type,
            request_info.peer,
            status.os_error());
    if (new_peer) {
      request_info.peer = new_peer;
      peer = new_peer;
    }
  }

  
  
  
  peer->OnCompletedRequest(status, security_info);

  webkit_glue::NotifyCacheStats();
}

int ResourceDispatcher::AddPendingRequest(
    webkit_glue::ResourceLoaderBridge::Peer* callback,
    ResourceType::Type resource_type) {
  
  int id = MakeRequestID();
  pending_requests_[id] = PendingRequestInfo(callback, resource_type);
  return id;
}

bool ResourceDispatcher::RemovePendingRequest(int request_id) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end())
    return false;
  pending_requests_.erase(it);
  return true;
}

void ResourceDispatcher::SetDefersLoading(int request_id, bool value) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end()) {
    NOTREACHED() << "unknown request";
    return;
  }
  PendingRequestInfo& request_info = it->second;
  if (value) {
    request_info.is_deferred = value;
  } else if (request_info.is_deferred) {
    request_info.is_deferred = false;
    MessageLoop::current()->PostTask(FROM_HERE,
        method_factory_.NewRunnableMethod(
            &ResourceDispatcher::FlushDeferredMessages, request_id));
  }
}

void ResourceDispatcher::DispatchMessage(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(ResourceDispatcher, message)
    IPC_MESSAGE_HANDLER(ViewMsg_Resource_UploadProgress, OnUploadProgress)
    IPC_MESSAGE_HANDLER(ViewMsg_Resource_DownloadProgress, OnDownloadProgress)
    IPC_MESSAGE_HANDLER(ViewMsg_Resource_ReceivedResponse, OnReceivedResponse)
    IPC_MESSAGE_HANDLER(ViewMsg_Resource_ReceivedRedirect, OnReceivedRedirect)
    IPC_MESSAGE_HANDLER(ViewMsg_Resource_DataReceived, OnReceivedData)
    IPC_MESSAGE_HANDLER(ViewMsg_Resource_RequestComplete, OnRequestComplete)
  IPC_END_MESSAGE_MAP()
}

void ResourceDispatcher::FlushDeferredMessages(int request_id) {
  PendingRequestList::iterator it = pending_requests_.find(request_id);
  if (it == pending_requests_.end())  
    return;
  PendingRequestInfo& request_info = it->second;
  if (request_info.is_deferred)
    return;
  
  
  MessageQueue q;
  q.swap(request_info.deferred_message_queue);
  while (!q.empty()) {
    IPC::Message* m = q.front();
    q.pop_front();
    DispatchMessage(*m);
    delete m;
  }
}

webkit_glue::ResourceLoaderBridge* ResourceDispatcher::CreateBridge(
    const std::string& method,
    const GURL& url,
    const GURL& policy_url,
    const GURL& referrer,
    const std::string& frame_origin,
    const std::string& main_frame_origin,
    const std::string& headers,
    int flags,
    int origin_pid,
    ResourceType::Type resource_type,
    uint32 request_context,
    int app_cache_context_id,
    int route_id) {
  return new webkit_glue::IPCResourceLoaderBridge(this, method, url, policy_url,
                                                  referrer, frame_origin,
                                                  main_frame_origin, headers,
                                                  flags, origin_pid,
                                                  resource_type,
                                                  request_context,
                                                  app_cache_context_id,
                                                  route_id);
}

bool ResourceDispatcher::IsResourceDispatcherMessage(
    const IPC::Message& message) {
  switch (message.type()) {
    case ViewMsg_Resource_DownloadProgress::ID:
    case ViewMsg_Resource_UploadProgress::ID:
    case ViewMsg_Resource_ReceivedResponse::ID:
    case ViewMsg_Resource_ReceivedRedirect::ID:
    case ViewMsg_Resource_DataReceived::ID:
    case ViewMsg_Resource_RequestComplete::ID:
      return true;

    default:
      break;
  }

  return false;
}
