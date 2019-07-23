





































#include "SharedDIB.h"

namespace mozilla {
namespace gfx {

SharedDIB::SharedDIB() :
  mShMem(nsnull)
{
}

SharedDIB::~SharedDIB()
{
  Close();
}

nsresult
SharedDIB::Create(PRUint32 aSize)
{
  Close();

  mShMem = new base::SharedMemory();
  if (!mShMem || !mShMem->Create(std::wstring(L""), false, false, aSize))
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (!mShMem->Map(0))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

bool
SharedDIB::IsValid()
{
  if (!mShMem)
    return false;

  return (mShMem->handle() != mShMem->NULLHandle() ? true : false);
}

nsresult
SharedDIB::Close()
{
  if (mShMem)
    delete mShMem;

  mShMem = nsnull;

  return NS_OK;
}

nsresult
SharedDIB::Attach(Handle aHandle, PRUint32 aSize)
{
  Close();

  mShMem = new base::SharedMemory(aHandle, false);
  if(!mShMem)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mShMem->Map(aSize))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
SharedDIB::ShareToProcess(base::ProcessHandle aChildProcess, Handle *aChildHandle)
{
  if (!mShMem)
    return NS_ERROR_UNEXPECTED;

  if (!mShMem->ShareToProcess(aChildProcess, aChildHandle))
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}

} 
} 
