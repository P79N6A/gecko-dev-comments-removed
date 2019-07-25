



































#ifndef mozilla_PtrAndFlag_h__
#define mozilla_PtrAndFlag_h__

#include "nsCOMPtr.h"
#include "mozilla/Types.h"

namespace mozilla {

namespace please_dont_use_this_directly {
template <class T>
class COMPtrAndFlagGetterAddRefs;
}


template <class T>
class COMPtrAndFlag
{
  
  
  class AutoFlagPersister
  {
  public:
    AutoFlagPersister(uintptr_t* aPtr)
      : mPtr(aPtr), mFlag(*aPtr & 0x1)
    {
      if (mFlag)
        *mPtr &= ~0x1;
    }

    ~AutoFlagPersister()
    {
      if (mFlag)
        *mPtr |= 0x1;
    }
  private:
    uintptr_t *mPtr;
    bool mFlag;
  };

  template <class U>
  friend class please_dont_use_this_directly::COMPtrAndFlagGetterAddRefs;

public:
  COMPtrAndFlag()
  {
    Set(nsnull, false);
  }

  COMPtrAndFlag(T* aPtr, bool aFlag)
  {
    Set(aPtr, aFlag);
  }

  COMPtrAndFlag(const nsQueryInterface aPtr, bool aFlag)
  {
    Set(aPtr, aFlag);
  }

  COMPtrAndFlag(const already_AddRefed<T>& aPtr, bool aFlag)
  {
    Set(aPtr, aFlag);
  }

  ~COMPtrAndFlag()
  {
    
    
    UnsetFlag();
  }

  COMPtrAndFlag<T>&
  operator= (const COMPtrAndFlag<T>& rhs)
  {
    Set(rhs.Ptr(), rhs.Flag());
    return *this;
  }

  void Set(T* aPtr, bool aFlag)
  {
    SetInternal(aPtr, aFlag);
  }

  void Set(const nsQueryInterface aPtr, bool aFlag)
  {
    SetInternal(aPtr, aFlag);
  }

  void Set(const already_AddRefed<T>& aPtr, bool aFlag)
  {
    SetInternal(aPtr, aFlag);
  }

  void UnsetFlag()
  {
    SetFlag(false);
  }

  void SetFlag(bool aFlag)
  {
    if (aFlag) {
      *VoidPtr() |= 0x1;
    } else {
      *VoidPtr() &= ~0x1;
    }
  }

  bool Flag() const
  {
    return *VoidPtr() & 0x1;
  }

  void SetPtr(T* aPtr)
  {
    SetInternal(aPtr);
  }

  void SetPtr(const nsQueryInterface aPtr)
  {
    SetInternal(aPtr);
  }

  void SetPtr(const already_AddRefed<T>& aPtr)
  {
    SetInternal(aPtr);
  }

  T* Ptr() const
  {
    return reinterpret_cast<T*>(*VoidPtr() & ~0x1);
  }

  void Clear()
  {
    Set(nsnull, false);
  }

private:
  template<class PtrType>
  void SetInternal(PtrType aPtr, bool aFlag)
  {
    UnsetFlag();
    mCOMPtr = aPtr;
    SetFlag(aFlag);
  }

  template<class PtrType>
  void SetInternal(PtrType aPtr)
  {
    AutoFlagPersister saveFlag(VoidPtr());
    mCOMPtr = aPtr;
  }

  uintptr_t* VoidPtr() const {
    return (uintptr_t*)(&mCOMPtr);
  }

  
  nsCOMPtr<T> mCOMPtr;
};

namespace please_dont_use_this_directly {















template <class T>
class COMPtrAndFlagGetterAddRefs
{
  public:
    explicit
    COMPtrAndFlagGetterAddRefs( COMPtrAndFlag<T>& aSmartPtr )
        : mTargetSmartPtr(aSmartPtr), mFlag(aSmartPtr.Flag())
      {
        if (mFlag)
          aSmartPtr.UnsetFlag();
      }

    ~COMPtrAndFlagGetterAddRefs()
      {
        if (mFlag)
          mTargetSmartPtr.SetFlag(true);
      }

    operator void**()
      {
        return reinterpret_cast<void**>(mTargetSmartPtr.VoidPtr());
      }

    operator T**()
      {
        return reinterpret_cast<T**>(mTargetSmartPtr.VoidPtr());
      }

    T*&
    operator*()
      {
        return *reinterpret_cast<T**>(mTargetSmartPtr.VoidPtr());
      }

  private:
    COMPtrAndFlag<T>& mTargetSmartPtr;
    bool mFlag;
};

} 

} 

template <class T>
inline
mozilla::please_dont_use_this_directly::COMPtrAndFlagGetterAddRefs<T>
getter_AddRefs( mozilla::COMPtrAndFlag<T>& aSmartPtr )
  



{
  return mozilla::please_dont_use_this_directly::COMPtrAndFlagGetterAddRefs<T>(aSmartPtr);
}


#endif
