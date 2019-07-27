






#ifndef mozilla_ipc_SharedMemoryBasic_mach_h
#define mozilla_ipc_SharedMemoryBasic_mach_h

#include "base/file_descriptor_posix.h"
#include "base/process.h"

#include "SharedMemory.h"
#include <mach/port.h>






class MachPortSender;
class ReceivePort;

namespace mozilla {
namespace ipc {

class SharedMemoryBasic final : public SharedMemory
{
public:
  typedef mach_port_t Handle;

  static void SetupMachMemory(pid_t pid,
                              ReceivePort* listen_port,
                              MachPortSender* listen_port_ack,
                              MachPortSender* send_port,
                              ReceivePort* send_port_ack,
                              bool pidIsParent);

  static void CleanupForPid(pid_t pid);

  SharedMemoryBasic();

  explicit SharedMemoryBasic(Handle aHandle);

  virtual bool Create(size_t aNbytes) override;

  virtual bool Map(size_t nBytes) override;

  virtual void* memory() const override
  {
    return mMemory;
  }

  virtual SharedMemoryType Type() const override
  {
    return TYPE_BASIC;
  }

  static Handle NULLHandle()
  {
    return Handle();
  }


  static bool IsHandleValid(const Handle &aHandle);

  bool ShareToProcess(base::ProcessId aProcessId,
                      Handle* aNewHandle);

private:
  ~SharedMemoryBasic();

  void Unmap();
  void Destroy();
  mach_port_t mPort;
  
  void *mMemory;
};

} 
} 

#endif 
