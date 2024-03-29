





#include "nsXPCOM.h"
#include "nsMemory.h"
#include "nsIMemory.h"
#include "nsXPCOMPrivate.h"
#include "nsDebug.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"




nsresult
nsMemory::HeapMinimize(bool aImmediate)
{
  nsCOMPtr<nsIMemory> mem;
  nsresult rv = NS_GetMemoryManager(getter_AddRefs(mem));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return mem->HeapMinimize(aImmediate);
}

void*
nsMemory::Clone(const void* aPtr, size_t aSize)
{
  void* newPtr = NS_Alloc(aSize);
  if (newPtr) {
    memcpy(newPtr, aPtr, aSize);
  }
  return newPtr;
}

nsIMemory*
nsMemory::GetGlobalMemoryService()
{
  nsIMemory* mem;
  nsresult rv = NS_GetMemoryManager(&mem);
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  return mem;
}



