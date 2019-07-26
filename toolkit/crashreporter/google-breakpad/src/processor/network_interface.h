

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_NETWORK_INTERFACE_H_
#define GOOGLE_BREAKPAD_PROCESSOR_NETWORK_INTERFACE_H_
namespace google_breakpad {

class NetworkInterface {
 public:
  
  
  
  
  virtual bool Init(bool listen) = 0;

  
  
  virtual bool Send(const char *data, size_t length) = 0;

  
  
  
  virtual bool WaitToReceive(int timeout) = 0;

  
  
  virtual bool Receive(char *buffer, size_t buffer_size, ssize_t &received) = 0;
};

}  
#endif  
