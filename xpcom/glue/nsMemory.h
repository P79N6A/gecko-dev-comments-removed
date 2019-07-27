





#ifndef nsMemory_h__
#define nsMemory_h__

#include "nsXPCOM.h"

class nsIMemory;

#define NS_MEMORY_CONTRACTID "@mozilla.org/xpcom/memory-service;1"
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
  static NS_HIDDEN_(void*) Alloc(size_t aSize)
  {
    return NS_Alloc(aSize);
  }

  static NS_HIDDEN_(void*) Realloc(void* aPtr, size_t aSize)
  {
    return NS_Realloc(aPtr, aSize);
  }

  static NS_HIDDEN_(void) Free(void* aPtr)
  {
    NS_Free(aPtr);
  }

  static nsresult   HeapMinimize(bool aImmediate);
  static void*      Clone(const void* aPtr, size_t aSize);
  static nsIMemory* GetGlobalMemoryService();       
};






























#define NS_FREE_XPCOM_POINTER_ARRAY(size, array, freeFunc)                    \
    PR_BEGIN_MACRO                                                            \
        int32_t iter_ = int32_t(size);                                        \
        while (--iter_ >= 0)                                                  \
            freeFunc((array)[iter_]);                                         \
        NS_Free((array));                                                     \
    PR_END_MACRO













#define NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(size, array)                    \
    NS_FREE_XPCOM_POINTER_ARRAY((size), (array), NS_Free)
















#define NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(size, array)                    \
    NS_FREE_XPCOM_POINTER_ARRAY((size), (array), NS_IF_RELEASE)





namespace mozilla {
template<class T>
struct AlignmentTestStruct
{
  char c;
  T t;
};
}

#define NS_ALIGNMENT_OF(t_) \
  (sizeof(mozilla::AlignmentTestStruct<t_>) - sizeof(t_))




enum nsAssignmentType
{
  NS_ASSIGNMENT_COPY,   
  NS_ASSIGNMENT_DEPEND, 
  NS_ASSIGNMENT_ADOPT   
};

#endif 

