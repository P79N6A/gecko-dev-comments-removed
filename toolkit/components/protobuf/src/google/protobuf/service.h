


































































































#ifndef GOOGLE_PROTOBUF_SERVICE_H__
#define GOOGLE_PROTOBUF_SERVICE_H__

#include <string>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {


class Service;
class RpcController;
class RpcChannel;


class Descriptor;            
class ServiceDescriptor;     
class MethodDescriptor;      
class Message;               






class LIBPROTOBUF_EXPORT Service {
 public:
  inline Service() {}
  virtual ~Service();

  
  
  
  enum ChannelOwnership {
    STUB_OWNS_CHANNEL,
    STUB_DOESNT_OWN_CHANNEL
  };

  
  virtual const ServiceDescriptor* GetDescriptor() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void CallMethod(const MethodDescriptor* method,
                          RpcController* controller,
                          const Message* request,
                          Message* response,
                          Closure* done) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual const Message& GetRequestPrototype(
    const MethodDescriptor* method) const = 0;
  virtual const Message& GetResponsePrototype(
    const MethodDescriptor* method) const = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Service);
};









class LIBPROTOBUF_EXPORT RpcController {
 public:
  inline RpcController() {}
  virtual ~RpcController();

  
  
  

  
  
  virtual void Reset() = 0;

  
  
  
  
  virtual bool Failed() const = 0;

  
  virtual string ErrorText() const = 0;

  
  
  
  
  
  virtual void StartCancel() = 0;

  
  
  

  
  
  
  
  
  virtual void SetFailed(const string& reason) = 0;

  
  
  
  virtual bool IsCanceled() const = 0;

  
  
  
  
  
  
  
  virtual void NotifyOnCancel(Closure* callback) = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RpcController);
};









class LIBPROTOBUF_EXPORT RpcChannel {
 public:
  inline RpcChannel() {}
  virtual ~RpcChannel();

  
  
  
  
  
  virtual void CallMethod(const MethodDescriptor* method,
                          RpcController* controller,
                          const Message* request,
                          Message* response,
                          Closure* done) = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RpcChannel);
};

}  

}  
#endif  
