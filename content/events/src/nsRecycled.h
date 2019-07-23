





































#ifndef nsRecycled_h__
#define nsRecycled_h__

#include "nsAutoPtr.h"
#include "prtypes.h"



















template<class OwnerType>
class nsRecycledSingle
{
public:
  
  void* operator new(size_t sz) CPP_THROW_NEW
  {
      void* result = nsnull;
  
      if (!gPoolInUse)
      {
         if (gPoolSize >= sz)
         {
            result = gPool;
         }
         else
         {
            
            result = ::operator new(sz);
            gPool = (char*)result;
            gPoolSize = sz;
         }
         gPoolInUse = PR_TRUE;
      }
      else
      {
         if (sz > gPoolSize)
         {
            
            

            
            
            gPool.forget();
            result = ::operator new(sz);
            gPool = (char*)result;
            gPoolSize = sz;
         }
         else
         {
            result = ::operator new(sz);
         }
      }
      
     if (result) {
        memset(result, 0, sz);
     }
     
     return result;
  }

  void operator delete(void* aPtr)
  {
      if (gPool == aPtr)
      {
         gPoolInUse = PR_FALSE;
         return;
      }
      else
      {
         ::operator delete(aPtr);
      }
  }
  
protected:
  static nsAutoPtr<char> gPool;
  static size_t gPoolSize;
  static PRBool gPoolInUse;
};

template<class OwnerType> 
nsAutoPtr<char> nsRecycledSingle<OwnerType>::gPool;
template<class OwnerType> 
size_t nsRecycledSingle<OwnerType>::gPoolSize = 0;
template<class OwnerType> 
PRBool nsRecycledSingle<OwnerType>::gPoolInUse = PR_FALSE;

#endif 
