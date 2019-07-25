






























#ifndef _GOOGLE_BREAKPAD_PROCESSOR_UDP_NETWORK_H_
#define _GOOGLE_BREAKPAD_PROCESSOR_UDP_NETWORK_H_

#include <sys/socket.h>

#include <string>

#include "processor/network_interface.h"

namespace google_breakpad {

class UDPNetwork : public NetworkInterface {
 public:
  
  
  UDPNetwork(const std::string address,
             unsigned short port,
             bool ip4only = false)
    : server_(address),
      port_(port),
      ip4only_(ip4only),
      socket_(-1) {};

  ~UDPNetwork();

  virtual bool Init(bool listen);
  virtual bool Send(const char *data, size_t length);
  virtual bool WaitToReceive(int timeout);
  virtual bool Receive(char *buffer, size_t buffer_size, ssize_t &received);

  unsigned short port() { return port_; }

 private:
  std::string server_;
  unsigned short port_;
  bool ip4only_;
  struct sockaddr_storage address_;
  int socket_;
};

}  
#endif  
