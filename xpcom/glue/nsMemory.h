




































#ifndef nsMemory_h__
#define nsMemory_h__

#include "nsXPCOM.h"
#include "nsIMemory.h"

#define NS_MEMORY_CONTRACTID "@mozilla.org/xpcom/memory-service;1"
#define NS_MEMORY_CLASSNAME  "Global Memory Service"
#define NS_MEMORY_CID                                \
{ /* 30a04e40-38e7-11d4-8cf5-0060b0fc14a3 */         \
    0x30a04e40,                                      \
    0x38e7,                                          \
    0x11d4,                                          \
    {0x8c, 0xf5, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}











class nsMemory
{
public:
    static NS_HIDDEN_(void*) Alloc(size_t size)
        { return NS_Alloc(size); }

    static NS_HIDDEN_(void*) Realloc(void* ptr, PRSize size)
        { return NS_Realloc(ptr, size); }

    static NS_HIDDEN_(void) Free(void* ptr)
        { NS_Free(ptr); }

    static NS_COM_GLUE nsresult   HeapMinimize(PRBool aImmediate);
    static NS_COM_GLUE void*      Clone(const void* ptr, PRSize size);
    static NS_COM_GLUE nsIMemory* GetGlobalMemoryService();       
};






























#define NS_FREE_XPCOM_POINTER_ARRAY(size, array, freeFunc)                    \
    PR_BEGIN_MACRO                                                            \
        PRInt32 iter_ = PRInt32(size);                                        \
        while (--iter_ >= 0)                                                  \
            freeFunc((array)[iter_]);                                         \
        NS_Free((array));                                                     \
    PR_END_MACRO













#define NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(size, array)                    \
    NS_FREE_XPCOM_POINTER_ARRAY((size), (array), NS_Free)
















#define NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(size, array)                    \
    NS_FREE_XPCOM_POINTER_ARRAY((size), (array), NS_IF_RELEASE)






#define NS_ARRAY_LENGTH(array_) \
  (sizeof(array_)/sizeof(array_[0]))




enum nsAssignmentType {
    NS_ASSIGNMENT_COPY,   
    NS_ASSIGNMENT_DEPEND, 
    NS_ASSIGNMENT_ADOPT   
};

#endif 

