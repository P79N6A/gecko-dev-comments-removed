









#ifndef WEBRTC_BASE_NATSOCKETFACTORY_H_
#define WEBRTC_BASE_NATSOCKETFACTORY_H_

#include <string>
#include <map>
#include <set>

#include "webrtc/base/natserver.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/socketserver.h"

namespace rtc {

const size_t kNATEncodedIPv4AddressSize = 8U;
const size_t kNATEncodedIPv6AddressSize = 20U;


class NATInternalSocketFactory {
 public:
  virtual ~NATInternalSocketFactory() {}
  virtual AsyncSocket* CreateInternalSocket(int family, int type,
      const SocketAddress& local_addr, SocketAddress* nat_addr) = 0;
};




class NATSocketFactory : public SocketFactory, public NATInternalSocketFactory {
 public:
  NATSocketFactory(SocketFactory* factory, const SocketAddress& nat_addr);

  
  virtual Socket* CreateSocket(int type);
  virtual Socket* CreateSocket(int family, int type);
  virtual AsyncSocket* CreateAsyncSocket(int type);
  virtual AsyncSocket* CreateAsyncSocket(int family, int type);

  
  virtual AsyncSocket* CreateInternalSocket(int family, int type,
      const SocketAddress& local_addr, SocketAddress* nat_addr);

 private:
  SocketFactory* factory_;
  SocketAddress nat_addr_;
  DISALLOW_EVIL_CONSTRUCTORS(NATSocketFactory);
};
















class NATSocketServer : public SocketServer, public NATInternalSocketFactory {
 public:
  class Translator;
  
  class TranslatorMap : private std::map<SocketAddress, Translator*> {
   public:
    ~TranslatorMap();
    Translator* Get(const SocketAddress& ext_ip);
    Translator* Add(const SocketAddress& ext_ip, Translator*);
    void Remove(const SocketAddress& ext_ip);
    Translator* FindClient(const SocketAddress& int_ip);
  };

  
  class Translator {
   public:
    Translator(NATSocketServer* server, NATType type,
               const SocketAddress& int_addr, SocketFactory* ext_factory,
               const SocketAddress& ext_addr);

    SocketFactory* internal_factory() { return internal_factory_.get(); }
    SocketAddress internal_address() const {
      return nat_server_->internal_address();
    }
    SocketAddress internal_tcp_address() const {
      return SocketAddress();  
    }

    Translator* GetTranslator(const SocketAddress& ext_ip);
    Translator* AddTranslator(const SocketAddress& ext_ip,
                              const SocketAddress& int_ip, NATType type);
    void RemoveTranslator(const SocketAddress& ext_ip);

    bool AddClient(const SocketAddress& int_ip);
    void RemoveClient(const SocketAddress& int_ip);

    
    Translator* FindClient(const SocketAddress& int_ip);

   private:
    NATSocketServer* server_;
    scoped_ptr<SocketFactory> internal_factory_;
    scoped_ptr<NATServer> nat_server_;
    TranslatorMap nats_;
    std::set<SocketAddress> clients_;
  };

  explicit NATSocketServer(SocketServer* ss);

  SocketServer* socketserver() { return server_; }
  MessageQueue* queue() { return msg_queue_; }

  Translator* GetTranslator(const SocketAddress& ext_ip);
  Translator* AddTranslator(const SocketAddress& ext_ip,
                            const SocketAddress& int_ip, NATType type);
  void RemoveTranslator(const SocketAddress& ext_ip);

  
  virtual Socket* CreateSocket(int type);
  virtual Socket* CreateSocket(int family, int type);

  virtual AsyncSocket* CreateAsyncSocket(int type);
  virtual AsyncSocket* CreateAsyncSocket(int family, int type);

  virtual void SetMessageQueue(MessageQueue* queue) {
    msg_queue_ = queue;
    server_->SetMessageQueue(queue);
  }
  virtual bool Wait(int cms, bool process_io) {
    return server_->Wait(cms, process_io);
  }
  virtual void WakeUp() {
    server_->WakeUp();
  }

  
  virtual AsyncSocket* CreateInternalSocket(int family, int type,
      const SocketAddress& local_addr, SocketAddress* nat_addr);

 private:
  SocketServer* server_;
  MessageQueue* msg_queue_;
  TranslatorMap nats_;
  DISALLOW_EVIL_CONSTRUCTORS(NATSocketServer);
};


size_t PackAddressForNAT(char* buf, size_t buf_size,
                         const SocketAddress& remote_addr);
size_t UnpackAddressFromNAT(const char* buf, size_t buf_size,
                            SocketAddress* remote_addr);
}  

#endif  
