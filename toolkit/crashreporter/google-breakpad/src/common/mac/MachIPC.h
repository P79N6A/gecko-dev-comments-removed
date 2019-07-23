
































#ifndef MACH_IPC_H__
#define MACH_IPC_H__

#import <mach/mach.h>
#import <mach/message.h>
#import <servers/bootstrap.h>
#import <sys/types.h>

#import <CoreServices/CoreServices.h>






















































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
    disposition = MACH_MSG_TYPE_COPY_SEND;
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

  int GetDescriptorCount() const { return body.msgh_descriptor_count; }
  MachMsgPortDescriptor *GetDescriptor(int n);

  
  mach_port_t GetTranslatedPort(int n);

  
  bool IsSimpleMessage() const { return GetDescriptorCount() == 0; }

  
  bool SetData(void *data, int32_t data_length);

 protected:
  
  

  MachMessage() {
    memset(this, 0, sizeof(MachMessage));
  }

  friend class ReceivePort;
  friend class MachPortSender;

  
  struct MessageDataPacket {
    int32_t      id;          
    int32_t      data_length; 
    u_int8_t     data[1];     
  };

  MessageDataPacket* GetDataPacket();

  void SetDescriptorCount(int n);
  void SetDescriptor(int n, const MachMsgPortDescriptor &desc);

  
  int CalculateSize();

  mach_msg_header_t  head;
  mach_msg_body_t    body;
  u_int8_t           padding[1024]; 
};









class MachReceiveMessage : public MachMessage {
 public:
  MachReceiveMessage() : MachMessage() {};
};


class MachSendMessage : public MachMessage {
 public:
  MachSendMessage(int32_t message_id);
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
  ReceivePort(const ReceivePort&);  

  mach_port_t   port_;
  kern_return_t init_result_;
};



class MachPortSender {
 public:
  
  MachPortSender(const char *receive_port_name);


  
  MachPortSender(mach_port_t send_port);

  kern_return_t SendMessage(MachSendMessage &message,
                            mach_msg_timeout_t timeout);

 private:
  MachPortSender(const MachPortSender&);  

  mach_port_t   send_port_;
  kern_return_t init_result_;
};

#endif 
