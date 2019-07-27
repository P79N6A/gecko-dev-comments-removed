



#ifndef CHROME_COMMON_IPC_MESSAGE_H__
#define CHROME_COMMON_IPC_MESSAGE_H__

#include <string>

#include "base/basictypes.h"
#include "base/pickle.h"

#ifdef MOZ_TASK_TRACER
#include "GeckoTaskTracer.h"
#endif

#ifndef NDEBUG
#define IPC_MESSAGE_LOG_ENABLED
#endif

#if defined(OS_POSIX)
#include "nsAutoPtr.h"
#endif

namespace base {
struct FileDescriptor;
}

class FileDescriptorSet;

namespace IPC {



class Channel;
class Message;
struct LogData;

class Message : public Pickle {
 public:
  typedef uint32_t msgid_t;

  
  class Sender {
   public:
    virtual ~Sender() {}

    
    
    
    
    virtual bool Send(Message* msg) = 0;
  };

  enum PriorityValue {
    PRIORITY_LOW = 1,
    PRIORITY_NORMAL,
    PRIORITY_HIGH
  };

  enum MessageCompression {
    COMPRESSION_NONE,
    COMPRESSION_ENABLED
  };

  virtual ~Message();

  Message();

  
  
  Message(int32_t routing_id, msgid_t type, PriorityValue priority,
          MessageCompression compression = COMPRESSION_NONE,
          const char* const name="???");

  
  
  
  Message(const char* data, int data_len);

  Message(const Message& other);
  Message& operator=(const Message& other);

  PriorityValue priority() const {
    return static_cast<PriorityValue>(header()->flags & PRIORITY_MASK);
  }

  
  bool is_sync() const {
    return (header()->flags & SYNC_BIT) != 0;
  }

  
  bool is_interrupt() const {
    return (header()->flags & INTERRUPT_BIT) != 0;
  }

  
  bool is_urgent() const {
    return (header()->flags & URGENT_BIT) != 0;
  }

  
  bool is_rpc() const {
    return (header()->flags & RPC_BIT) != 0;
  }

  
  bool compress() const {
    return (header()->flags & COMPRESS_BIT) != 0;
  }

  
  void set_reply() {
    header()->flags |= REPLY_BIT;
  }

  bool is_reply() const {
    return (header()->flags & REPLY_BIT) != 0;
  }

  
  
  void set_reply_error() {
    header()->flags |= REPLY_ERROR_BIT;
  }

  bool is_reply_error() const {
    return (header()->flags & REPLY_ERROR_BIT) != 0;
  }

  
  
  
  void set_unblock(bool unblock) {
    if (unblock) {
      header()->flags |= UNBLOCK_BIT;
    } else {
      header()->flags &= ~UNBLOCK_BIT;
    }
  }

  bool should_unblock() const {
    return (header()->flags & UNBLOCK_BIT) != 0;
  }

  
  
  bool is_caller_pumping_messages() const {
    return (header()->flags & PUMPING_MSGS_BIT) != 0;
  }

  msgid_t type() const {
    return header()->type;
  }

  int32_t routing_id() const {
    return header()->routing;
  }

  void set_routing_id(int32_t new_id) {
    header()->routing = new_id;
  }

  int32_t transaction_id() const {
    return header()->txid;
  }

  void set_transaction_id(int32_t txid) {
    header()->txid = txid;
  }

  uint32_t interrupt_remote_stack_depth_guess() const {
    return header()->interrupt_remote_stack_depth_guess;
  }

  void set_interrupt_remote_stack_depth_guess(uint32_t depth) {
    DCHECK(is_interrupt());
    header()->interrupt_remote_stack_depth_guess = depth;
  }

  uint32_t interrupt_local_stack_depth() const {
    return header()->interrupt_local_stack_depth;
  }

  void set_interrupt_local_stack_depth(uint32_t depth) {
    DCHECK(is_interrupt());
    header()->interrupt_local_stack_depth = depth;
  }

  int32_t seqno() const {
    return header()->seqno;
  }

  void set_seqno(int32_t seqno) {
    header()->seqno = seqno;
  }

  const char* const name() const {
    return name_;
  }

  void set_name(const char* const name) {
    name_ = name;
  }

#if defined(OS_POSIX)
  uint32_t num_fds() const;
#endif

  template<class T>
  static bool Dispatch(const Message* msg, T* obj, void (T::*func)()) {
    (obj->*func)();
    return true;
  }

  template<class T>
  static bool Dispatch(const Message* msg, T* obj, void (T::*func)() const) {
    (obj->*func)();
    return true;
  }

  template<class T>
  static bool Dispatch(const Message* msg, T* obj,
                       void (T::*func)(const Message&)) {
    (obj->*func)(*msg);
    return true;
  }

  template<class T>
  static bool Dispatch(const Message* msg, T* obj,
                       void (T::*func)(const Message&) const) {
    (obj->*func)(*msg);
    return true;
  }

  
  static void Log(const Message* msg, std::wstring* l) {
  }

  
  
  static const char* FindNext(const char* range_start, const char* range_end) {
    return Pickle::FindNext(sizeof(Header), range_start, range_end);
  }

#if defined(OS_POSIX)
  
  

  
  bool WriteFileDescriptor(const base::FileDescriptor& descriptor);
  
  
  bool ReadFileDescriptor(void** iter, base::FileDescriptor* descriptor) const;

#if defined(OS_MACOSX)
  void set_fd_cookie(uint32_t cookie) {
    header()->cookie = cookie;
  }
  uint32_t fd_cookie() const {
    return header()->cookie;
  }
#endif
#endif

#ifdef IPC_MESSAGE_LOG_ENABLED
  
  
  void set_sent_time(int64_t time);
  int64_t sent_time() const;

  void set_received_time(int64_t time) const;
  int64_t received_time() const { return received_time_; }
  void set_output_params(const std::wstring& op) const { output_params_ = op; }
  const std::wstring& output_params() const { return output_params_; }
  
  
  
  
  void set_sync_log_data(LogData* data) const { log_data_ = data; }
  LogData* sync_log_data() const { return log_data_; }
  void set_dont_log() const { dont_log_ = true; }
  bool dont_log() const { return dont_log_; }
#endif

  friend class Channel;
  friend class MessageReplyDeserializer;
  friend class SyncMessage;

  void set_sync() {
    header()->flags |= SYNC_BIT;
  }

  void set_interrupt() {
    header()->flags |= INTERRUPT_BIT;
  }

  void set_urgent() {
    header()->flags |= URGENT_BIT;
  }

  void set_rpc() {
    header()->flags |= RPC_BIT;
  }

#if !defined(OS_MACOSX)
 protected:
#endif

  
  enum {
    PRIORITY_MASK   = 0x0003,
    SYNC_BIT        = 0x0004,
    REPLY_BIT       = 0x0008,
    REPLY_ERROR_BIT = 0x0010,
    UNBLOCK_BIT     = 0x0020,
    PUMPING_MSGS_BIT= 0x0040,
    HAS_SENT_TIME_BIT = 0x0080,
    INTERRUPT_BIT   = 0x0100,
    COMPRESS_BIT    = 0x0200,
    URGENT_BIT      = 0x0400,
    RPC_BIT         = 0x0800
  };

  struct Header : Pickle::Header {
    int32_t routing;  
    msgid_t type;   
    uint32_t flags;   
#if defined(OS_POSIX)
    uint32_t num_fds; 
# if defined(OS_MACOSX)
    uint32_t cookie;  
# endif
#endif
    union {
      
      uint32_t interrupt_remote_stack_depth_guess;

      
      int32_t txid;
    };
    
    uint32_t interrupt_local_stack_depth;
    
    int32_t seqno;
#ifdef MOZ_TASK_TRACER
    uint64_t source_event_id;
    uint64_t parent_task_id;
    mozilla::tasktracer::SourceEventType source_event_type;
#endif
  };

  Header* header() {
    return headerT<Header>();
  }
  const Header* header() const {
    return headerT<Header>();
  }

  void InitLoggingVariables(const char* const name="???");

#if defined(OS_POSIX)
  
  nsRefPtr<FileDescriptorSet> file_descriptor_set_;

  
  void EnsureFileDescriptorSet();

  FileDescriptorSet* file_descriptor_set() {
    EnsureFileDescriptorSet();
    return file_descriptor_set_.get();
  }
  const FileDescriptorSet* file_descriptor_set() const {
    return file_descriptor_set_.get();
  }
#endif

  const char* name_;

#ifdef IPC_MESSAGE_LOG_ENABLED
  
  mutable int64_t received_time_;
  mutable std::wstring output_params_;
  mutable LogData* log_data_;
  mutable bool dont_log_;
#endif
};



}  

enum SpecialRoutingIDs {
  
  MSG_ROUTING_NONE = kint32min,

  
  MSG_ROUTING_CONTROL = kint32max
};

#define IPC_REPLY_ID 0xFFF0  // Special message id for replies
#define IPC_LOGGING_ID 0xFFF1  // Special message id for logging

#endif  
