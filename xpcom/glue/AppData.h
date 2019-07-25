




#ifndef mozilla_AppData_h
#define mozilla_AppData_h

#include "nsXREAppData.h"
#include "nscore.h"
#include "nsStringGlue.h"

namespace mozilla {



class NS_COM_GLUE ScopedAppData : public nsXREAppData
{
public:
  ScopedAppData() { Zero(); this->size = sizeof(*this); }

  ScopedAppData(const nsXREAppData* aAppData);

  void Zero() { memset(this, 0, sizeof(*this)); }

  ~ScopedAppData();
};








void SetAllocatedString(const char *&str, const char *newvalue);








void SetAllocatedString(const char *&str, const nsACString &newvalue);

template<class T>
void SetStrongPtr(T *&ptr, T* newvalue)
{
  NS_IF_RELEASE(ptr);
  ptr = newvalue;
  NS_IF_ADDREF(ptr);
}

} 

#endif
