





#ifndef mozilla_net_AutoClose_h
#define mozilla_net_AutoClose_h

#include "nsCOMPtr.h"

namespace mozilla { namespace net {




template <typename T>
class AutoClose
{
public:
  AutoClose() { } 
  ~AutoClose(){
    Close();
  }

  operator bool() const
  {
    return mPtr;
  }

  already_AddRefed<T> forget()
  {
    return mPtr.forget();
  }

  void takeOver(AutoClose<T> & rhs)
  {
    Close();
    mPtr = rhs.mPtr.forget();
  }

  
  void operator=(const nsQueryInterfaceWithError rhs)
  {
    Close();
    mPtr = rhs;
  }

  void CloseAndRelease()
  {
    Close();
    mPtr = nsnull;
  }

  T* operator->() const
  {
    return mPtr.operator->();
  }

private:
  void Close()
  {
    if (mPtr) {
      mPtr->Close();
    }
  }

  void operator=(const AutoClose<T> &) MOZ_DELETE;
  AutoClose(const AutoClose<T> &) MOZ_DELETE;

  nsCOMPtr<T> mPtr;
};

} } 

#endif 
