





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

  explicit operator bool() const
  {
    return mPtr;
  }

  already_AddRefed<T> forget()
  {
    return mPtr.forget();
  }

  void takeOver(nsCOMPtr<T> & rhs)
  {
    Close();
    mPtr = rhs.forget();
  }

  void takeOver(AutoClose<T> & rhs)
  {
    Close();
    mPtr = rhs.mPtr.forget();
  }

  void CloseAndRelease()
  {
    Close();
    mPtr = nullptr;
  }

  T* operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN
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

  void operator=(const AutoClose<T> &) = delete;
  AutoClose(const AutoClose<T> &) = delete;

  nsCOMPtr<T> mPtr;
};

} } 

#endif 
