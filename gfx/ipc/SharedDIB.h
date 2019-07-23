





































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

  
  nsresult Create(PRUint32 aSize);

  
  nsresult Close();

  
  bool IsValid();

  
  
  
  nsresult Attach(Handle aHandle, PRUint32 aSize);

  
  nsresult ShareToProcess(base::ProcessHandle aChildProcess, Handle *aChildHandle);

protected:
  base::SharedMemory *mShMem;
};

} 
} 

#endif
