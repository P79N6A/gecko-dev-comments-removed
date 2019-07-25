




























#include "client/mac/crash_generation/crash_generation_client.h"

#include "client/mac/crash_generation/crash_generation_server.h"
#include "common/mac/MachIPC.h"

namespace google_breakpad {

bool CrashGenerationClient::RequestDumpForException(
    int exception_type,
    int exception_code,
    int exception_subcode,
    mach_port_t crashing_thread) {
  
  
  ReceivePort acknowledge_port;

  MachSendMessage message(kDumpRequestMessage);
  message.AddDescriptor(mach_task_self());            
  message.AddDescriptor(crashing_thread);             
  message.AddDescriptor(mach_thread_self());          
  message.AddDescriptor(acknowledge_port.GetPort());  

  ExceptionInfo info;
  info.exception_type = exception_type;
  info.exception_code = exception_code;
  info.exception_subcode = exception_subcode;
  message.SetData(&info, sizeof(info));

  const mach_msg_timeout_t kSendTimeoutMs = 2 * 1000;
  kern_return_t result = sender_.SendMessage(message, kSendTimeoutMs);
  if (result != KERN_SUCCESS)
    return false;

  
  
  const mach_msg_timeout_t kReceiveTimeoutMs = 5 * 1000;
  MachReceiveMessage acknowledge_message;
  result = acknowledge_port.WaitForMessage(&acknowledge_message,
					   kReceiveTimeoutMs);
  return result == KERN_SUCCESS;
}

}  
