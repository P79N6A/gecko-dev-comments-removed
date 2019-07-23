




































#include "nsXPCOM.h"
#include "nsMemory.h"
#include "nsXPCOMPrivate.h"
#include "nsDebug.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"




NS_COM_GLUE nsresult
nsMemory::HeapMinimize(PRBool aImmediate)
{
    nsCOMPtr<nsIMemory> mem;
    nsresult rv = NS_GetMemoryManager(getter_AddRefs(mem));
    NS_ENSURE_SUCCESS(rv, rv);

    return mem->HeapMinimize(aImmediate);
}

NS_COM_GLUE void*
nsMemory::Clone(const void* ptr, PRSize size)
{
    void* newPtr = NS_Alloc(size);
    if (newPtr)
        memcpy(newPtr, ptr, size);
    return newPtr;
}

NS_COM_GLUE nsIMemory*
nsMemory::GetGlobalMemoryService()
{
    nsIMemory* mem;
    nsresult rv = NS_GetMemoryManager(&mem);
    if (NS_FAILED(rv)) return nsnull;
   
    return mem;
}



