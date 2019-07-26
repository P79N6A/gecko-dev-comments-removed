































#ifndef CLIENT_IOS_HANDLER_IOS_EXCEPTION_MINIDUMP_GENERATOR_H_
#define CLIENT_IOS_HANDLER_IOS_EXCEPTION_MINIDUMP_GENERATOR_H_

#include <Foundation/Foundation.h>

#include "client/mac/handler/minidump_generator.h"

namespace google_breakpad {

class IosExceptionMinidumpGenerator : public MinidumpGenerator {
 public:
  explicit IosExceptionMinidumpGenerator(NSException *exception);
  virtual ~IosExceptionMinidumpGenerator();

 protected:
  virtual bool WriteExceptionStream(MDRawDirectory *exception_stream);
  virtual bool WriteThreadStream(mach_port_t thread_id, MDRawThread *thread);

 private:

  
  uint32_t GetPCFromException();

  
  bool WriteCrashingContext(MDLocationDescriptor *register_location);

  NSArray *return_addresses_;
};

}  

#endif  
