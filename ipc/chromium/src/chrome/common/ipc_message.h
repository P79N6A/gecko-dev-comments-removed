



#ifndef CHROME_COMMON_IPC_MESSAGE_H__
#define CHROME_COMMON_IPC_MESSAGE_H__

#include <string>

#include "base/basictypes.h"
#include "base/pickle.h"

#ifndef NDEBUG
#define IPC_MESSAGE_LOG_ENABLED
#endif

#if defined(OS_POSIX)
#include "base/ref_counted.h"
#endif

#if defined(CHROMIUM_MOZILLA_BUILD)
#define IPC_MESSAGE_ENABLE_RPC
#endif

namespace base {
class FileDescriptor;
}

class FileDescriptorSet;

namespace IPC {



class Channel;
class Message;
struct LogData;

class Message : public Pickle {
 public:
#if defined(CHROMIUM_MOZILLA_BUILD)
  typedef uint32 msgid_t;
#else
  typedef uint16 msgid_t;
#endif

  
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

  virtual ~Message();

  Message();

  
  
#if !defined(CHROMIUM_MOZILLA_BUILD)
  Message(int32 routing_id, msgid_t type, PriorityValue priority);
#else
  Message(int32 routing_id, msgid_t type, PriorityValue priority,
          const char* const name="???");
#endif

  
  
  
  Message(const char* data, int data_len);

  Message(const Message& other);
  Message& operator=(const Message& other);

  PriorityValue priority() const {
    return static_cast<PriorityValue>(header()->flags & PRIORITY_MASK);
  }

  
  bool is_sync() const {
    return (header()->flags & SYNC_BIT) != 0;
  }

#if defined(IPC_MESSAGE_ENABLE_RPC)
  
  bool is_rpc() const {
    return (header()->flags & RPC_BIT) != 0;
  }
#endif

  
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

  int32 routing_id() const {
    return header()->routing;
  }

  void set_routing_id(int32 new_id) {
    header()->routing = new_id;
  }

#if defined(CHROMIUM_MOZILLA_BUILD)
  uint32 rpc_remote_stack_depth_guess() const {
    return header()->rpc_remote_stack_depth_guess;
  }

  void set_rpc_remote_stack_depth_guess(uint32 depth) {
    DCHECK(is_rpc());
    header()->rpc_remote_stack_depth_guess = depth;
  }

  uint32 rpc_local_stack_depth() const {
    return header()->rpc_local_stack_depth;
  }

  void set_rpc_local_stack_depth(uint32 depth) {
    DCHECK(is_rpc());
    header()->rpc_local_stack_depth = depth;
  }

  int32 seqno() const {
    return header()->seqno;
  }

  void set_seqno(int32 seqno) {
    header()->seqno = seqno;
  }

  const char* const name() const {
    return name_;
  }

  void set_name(const char* const name) {
    name_ = name;
  }
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
#endif

#ifdef IPC_MESSAGE_LOG_ENABLED
  
  
  void set_sent_time(int64 time);
  int64 sent_time() const;

  void set_received_time(int64 time) const;
  int64 received_time() const { return received_time_; }
  void set_output_params(const std::wstring& op) const { output_params_ = op; }
  const std::wstring& output_params() const { return output_params_; }
  
  
  
  
  void set_sync_log_data(LogData* data) const { log_data_ = data; }
  LogData* sync_log_data() const { return log_data_; }
  void set_dont_log() const { dont_log_ = true; }
  bool dont_log() const { return dont_log_; }
#endif

#if !defined(CHROMIUM_MOZILLA_BUILD)
 protected:
#endif
  friend class Channel;
  friend class MessageReplyDeserializer;
  friend class SyncMessage;

  void set_sync() {
    header()->flags |= SYNC_BIT;
  }

#if defined(IPC_MESSAGE_ENABLE_RPC)
  void set_rpc() {
    header()->flags |= RPC_BIT;
  }
#endif

#if defined(CHROMIUM_MOZILLA_BUILD) && !defined(OS_MACOSX)
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
#if defined(IPC_MESSAGE_ENABLE_RPC)
    RPC_BIT        = 0x0100,
#endif
  };

#pragma pack(push, 2)
  struct Header : Pickle::Header {
    int32 routing;  
    msgid_t type;   
#if defined(CHROMIUM_MOZILLA_BUILD)
    uint32 flags;   
#else
    uint16 flags;   
#endif
#if defined(OS_POSIX)
    uint32 num_fds; 
#endif
#if defined(CHROMIUM_MOZILLA_BUILD)
    
    uint32 rpc_remote_stack_depth_guess;
    
    uint32 rpc_local_stack_depth;
    
    int32 seqno;
#endif
  };
#pragma pack(pop)

  Header* header() {
    return headerT<Header>();
  }
  const Header* header() const {
    return headerT<Header>();
  }

#if !defined(CHROMIUM_MOZILLA_BUILD)
  void InitLoggingVariables();
#else
  void InitLoggingVariables(const char* const name="???");
#endif

#if defined(OS_POSIX)
  
  scoped_refptr<FileDescriptorSet> file_descriptor_set_;

  
  void EnsureFileDescriptorSet();

  FileDescriptorSet* file_descriptor_set() {
    EnsureFileDescriptorSet();
    return file_descriptor_set_.get();
  }
  const FileDescriptorSet* file_descriptor_set() const {
    return file_descriptor_set_.get();
  }
#endif

#if defined(CHROMIUM_MOZILLA_BUILD)
  const char* name_;
#endif

#ifdef IPC_MESSAGE_LOG_ENABLED
  
  mutable int64 received_time_;
  mutable std::wstring output_params_;
  mutable LogData* log_data_;
  mutable bool dont_log_;
#endif
};



}  

enum SpecialRoutingIDs {
  
  MSG_ROUTING_NONE = kint32min,

  
  MSG_ROUTING_CONTROL = kint32max,
};

#define IPC_REPLY_ID 0xFFF0  // Special message id for replies
#define IPC_LOGGING_ID 0xFFF1  // Special message id for logging

#endif  
