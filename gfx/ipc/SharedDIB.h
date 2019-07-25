




#ifndef gfx_SharedDIB_h__
#define gfx_SharedDIB_h__

#include "base/shared_memory.h"
#include "prtypes.h"
#include "nscore.h"

namespace mozilla {
namespace gfx {

class SharedDIB
{
public:
  typedef base::SharedMemoryHandle Handle;

public:
  SharedDIB();
  ~SharedDIB();

  
  nsresult Create(uint32_t aSize);

  
  nsresult Close();

  
  bool IsValid();

  
  
  
  nsresult Attach(Handle aHandle, uint32_t aSize);

  
  nsresult ShareToProcess(base::ProcessHandle aChildProcess, Handle *aChildHandle);

protected:
  base::SharedMemory *mShMem;
};

} 
} 

#endif
