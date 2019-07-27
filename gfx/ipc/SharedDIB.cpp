




#include "SharedDIB.h"

namespace mozilla {
namespace gfx {

SharedDIB::SharedDIB() :
  mShMem(nullptr)
{
}

SharedDIB::~SharedDIB()
{
  Close();
}

nsresult
SharedDIB::Create(uint32_t aSize)
{
  Close();

  mShMem = new base::SharedMemory();
  if (!mShMem || !mShMem->Create("", false, false, aSize))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

bool
SharedDIB::IsValid()
{
  if (!mShMem)
    return false;

  return mShMem->IsHandleValid(mShMem->handle());
}

nsresult
SharedDIB::Close()
{
  delete mShMem;

  mShMem = nullptr;

  return NS_OK;
}

nsresult
SharedDIB::Attach(Handle aHandle, uint32_t aSize)
{
  Close();

  mShMem = new base::SharedMemory(aHandle, false);
  if(!mShMem)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsresult
SharedDIB::ShareToProcess(base::ProcessId aTargetPid, Handle *aNewHandle)
{
  if (!mShMem)
    return NS_ERROR_UNEXPECTED;

  if (!mShMem->ShareToProcess(aTargetPid, aNewHandle))
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}

} 
} 
