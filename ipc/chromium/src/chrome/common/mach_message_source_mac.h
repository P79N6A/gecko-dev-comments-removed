



#ifndef CHROME_COMMON_MACH_MESSAGE_SOURCE_MAC_H_
#define CHROME_COMMON_MACH_MESSAGE_SOURCE_MAC_H_

#include <CoreServices/CoreServices.h>

#include "base/scoped_cftyperef.h"






















class MachMessageSource {
 public:
  
  
  
  class MachPortListener {
   public:
    virtual void OnMachMessageReceived(void* mach_msg, size_t size) = 0;
  };

  
  
  MachMessageSource(mach_port_t port,
                    MachPortListener* listener,
                    bool* success);
  ~MachMessageSource();

 private:
  
  static void OnReceiveMachMessage(CFMachPortRef port, void* msg, CFIndex size,
                                   void* closure);

  scoped_cftyperef<CFRunLoopSourceRef> machport_runloop_ref_;
  DISALLOW_COPY_AND_ASSIGN(MachMessageSource);
};

#endif 
