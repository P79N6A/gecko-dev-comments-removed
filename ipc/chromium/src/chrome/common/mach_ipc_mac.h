



#ifndef BASE_MACH_IPC_MAC_H_
#define BASE_MACH_IPC_MAC_H_

#include <mach/mach.h>
#include <mach/message.h>
#include <servers/bootstrap.h>
#include <sys/types.h>

#include <CoreServices/CoreServices.h>

#include "base/basictypes.h"






















































#define PRINT_MACH_RESULT(result_, message_) \
  printf(message_" %s (%d)\n", mach_error_string(result_), result_ );




class MachMsgPortDescriptor : public mach_msg_port_descriptor_t {
 public:
  
  MachMsgPortDescriptor(mach_port_t in_name,
                        mach_msg_type_name_t in_disposition) {
    name = in_name;
    pad1 = 0;
    pad2 = 0;
    disposition = in_disposition;
    type = MACH_MSG_PORT_DESCRIPTOR;
  }

  
  MachMsgPortDescriptor(mach_port_t in_name) {
    name = in_name;
    pad1 = 0;
    pad2 = 0;
    disposition = MACH_MSG_TYPE_PORT_SEND;
    type = MACH_MSG_PORT_DESCRIPTOR;
  }

  
  MachMsgPortDescriptor(const MachMsgPortDescriptor& desc) {
    name = desc.name;
    pad1 = desc.pad1;
    pad2 = desc.pad2;
    disposition = desc.disposition;
    type = desc.type;
  }

  mach_port_t GetMachPort() const {
    return name;
  }

  mach_msg_type_name_t GetDisposition() const {
    return disposition;
  }

  
  
  operator mach_msg_port_descriptor_t&() {
    return *this;
  }

  
  operator mach_port_t() const {
    return GetMachPort();
  }
};



















class MachMessage {
 public:

  virtual ~MachMessage();

  
  u_int8_t *GetData() {
    return GetDataLength() > 0 ? GetDataPacket()->data : NULL;
  }

  u_int32_t GetDataLength() {
    return EndianU32_LtoN(GetDataPacket()->data_length);
  }

  
  void SetMessageID(int32_t message_id) {
    GetDataPacket()->id = EndianU32_NtoL(message_id);
  }

  int32_t GetMessageID() { return EndianU32_LtoN(GetDataPacket()->id); }

  
  
  bool AddDescriptor(const MachMsgPortDescriptor &desc);

  int GetDescriptorCount() const {
                                   return storage_->body.msgh_descriptor_count;
                                 }
  MachMsgPortDescriptor *GetDescriptor(int n);

  
  mach_port_t GetTranslatedPort(int n);

  
  bool IsSimpleMessage() const { return GetDescriptorCount() == 0; }

  
  bool SetData(const void* data, int32_t data_length);

 protected:
  
  
  MachMessage();

  
  
  MachMessage(void *storage, size_t storage_length);

  friend class ReceivePort;
  friend class MachPortSender;

  
  struct MessageDataPacket {
    int32_t  id;          
    int32_t  data_length; 
    u_int8_t data[1];     
  };

  MessageDataPacket* GetDataPacket();

  void SetDescriptorCount(int n);
  void SetDescriptor(int n, const MachMsgPortDescriptor &desc);

  
  int CalculateSize();

  
  
  size_t MaxSize() const { return storage_length_bytes_; }

 protected:
  mach_msg_header_t *Head() { return &(storage_->head); }

 private:
  struct MachMessageData {
    mach_msg_header_t  head;
    mach_msg_body_t    body;
    
    u_int8_t           padding[1024];
  };

 
 
 public:
   
  static const size_t kEmptyMessageSize = sizeof(mach_msg_header_t) +
                                          sizeof(mach_msg_body_t) +
                                          sizeof(MessageDataPacket);

 private:
  MachMessageData *storage_;
  size_t storage_length_bytes_;
  bool own_storage_;  
};









class MachReceiveMessage : public MachMessage {
 public:
  MachReceiveMessage() : MachMessage() {}
  MachReceiveMessage(void *storage, size_t storage_length)
      : MachMessage(storage, storage_length) {}

 private:
    DISALLOW_COPY_AND_ASSIGN(MachReceiveMessage);
};


class MachSendMessage : public MachMessage {
 public:
  MachSendMessage(int32_t message_id);
  MachSendMessage(void *storage, size_t storage_length, int32_t message_id);

 private:
  void Initialize(int32_t message_id);

  DISALLOW_COPY_AND_ASSIGN(MachSendMessage);
};



class ReceivePort {
 public:
  
  ReceivePort(const char *receive_port_name);

  
  
  ReceivePort(mach_port_t receive_port);

  
  ReceivePort();

  ~ReceivePort();

  
  kern_return_t WaitForMessage(MachReceiveMessage *out_message,
                               mach_msg_timeout_t timeout);

  
  mach_port_t  GetPort() const { return port_; }

 private:
  mach_port_t   port_;
  kern_return_t init_result_;

  DISALLOW_COPY_AND_ASSIGN(ReceivePort);
};



class MachPortSender {
 public:
  
  MachPortSender(const char *receive_port_name);


  
  MachPortSender(mach_port_t send_port);

  kern_return_t SendMessage(MachSendMessage &message,
                            mach_msg_timeout_t timeout);

 private:
  mach_port_t   send_port_;
  kern_return_t init_result_;

  DISALLOW_COPY_AND_ASSIGN(MachPortSender);
};

#endif 
