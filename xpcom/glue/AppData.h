





#ifndef mozilla_AppData_h
#define mozilla_AppData_h

#include "nsXREAppData.h"
#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupportsUtils.h"

namespace mozilla {



class ScopedAppData : public nsXREAppData
{
public:
  ScopedAppData()
  {
    Zero();
    this->size = sizeof(*this);
  }

  explicit ScopedAppData(const nsXREAppData* aAppData);

  void Zero() { memset(this, 0, sizeof(*this)); }

  ~ScopedAppData();
};







void SetAllocatedString(const char*& aStr, const char* aNewValue);








void SetAllocatedString(const char*& aStr, const nsACString& aNewValue);

template<class T>
void
SetStrongPtr(T*& aPtr, T* aNewValue)
{
  NS_IF_RELEASE(aPtr);
  aPtr = aNewValue;
  NS_IF_ADDREF(aPtr);
}

} 

#endif
