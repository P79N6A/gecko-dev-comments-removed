




























#import <iostream>
#import <mach/mach.h>
#import <servers/bootstrap.h>
#import <stdio.h>
#import <stdlib.h>
#import <sys/stat.h>
#import <unistd.h>













































class OnDemandServer {
 public:
  
  OnDemandServer()
    : server_port_(MACH_PORT_NULL),
      service_port_(MACH_PORT_NULL),
      unregister_on_cleanup_(true) {
  }

  
  kern_return_t Initialize(const char *server_command,
                           const char *service_name,
                           bool unregister_on_cleanup);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static OnDemandServer *Create(const char *server_command,
                                const char *service_name,
                                bool unregister_on_cleanup,
                                kern_return_t *out_result);

  
  
  ~OnDemandServer();

  
  
  
  void LaunchOnDemand();

  
  
  
  mach_port_t GetServicePort() { return service_port_; };

 private:
  
  OnDemandServer(const OnDemandServer&);

  
  
  void Unregister();

  name_t      service_name_;

  mach_port_t server_port_;
  mach_port_t service_port_;
  bool        unregister_on_cleanup_;
};
