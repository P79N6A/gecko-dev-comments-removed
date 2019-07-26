





#ifndef mozilla_plugins_MiniShmBase_h
#define mozilla_plugins_MiniShmBase_h

#include "base/basictypes.h"

#include "nsDebug.h"

#include <windows.h>

namespace mozilla {
namespace plugins {





class ScopedMappedFileView
{
public:
  explicit
  ScopedMappedFileView(LPVOID aView)
    : mView(aView)
  {
  }

  ~ScopedMappedFileView()
  {
    Close();
  }

  void
  Close()
  {
    if (mView) {
      ::UnmapViewOfFile(mView);
      mView = nullptr;
    }
  }

  void
  Set(LPVOID aView)
  {
    Close();
    mView = aView;
  }

  LPVOID
  Get() const
  {
    return mView;
  }

  LPVOID
  Take()
  {
    LPVOID result = mView;
    mView = nullptr;
    return result;
  }

  operator LPVOID()
  {
    return mView;
  }

  bool
  IsValid() const
  {
    return (mView);
  }

private:
  DISALLOW_COPY_AND_ASSIGN(ScopedMappedFileView);

  LPVOID mView;
};

class MiniShmBase;

class MiniShmObserver
{
public:
  




  virtual void OnMiniShmEvent(MiniShmBase *aMiniShmObj) = 0;
  






  virtual void OnMiniShmConnect(MiniShmBase *aMiniShmObj) { }
};





class MiniShmBase
{
public:
  












  template<typename T> nsresult
  GetWritePtr(T*& aPtr)
  {
    if (!mWriteHeader) {
      return NS_ERROR_NOT_INITIALIZED;
    }
    if (sizeof(T) > mPayloadMaxLen ||
        T::identifier <= RESERVED_CODE_LAST) {
      return NS_ERROR_ILLEGAL_VALUE;
    }
    mWriteHeader->mId = T::identifier;
    mWriteHeader->mPayloadLen = sizeof(T);
    aPtr = reinterpret_cast<T*>(mWriteHeader + 1);
    return NS_OK;
  }

  














  template<typename T> nsresult
  GetReadPtr(const T*& aPtr)
  {
    if (!mReadHeader) {
      return NS_ERROR_NOT_INITIALIZED;
    }
    if (mReadHeader->mId != T::identifier ||
        sizeof(T) != mReadHeader->mPayloadLen) {
      return NS_ERROR_ILLEGAL_VALUE;
    }
    aPtr = reinterpret_cast<const T*>(mReadHeader + 1);
    return NS_OK;
  }

  




  virtual nsresult
  Send() = 0;

protected:
  




  enum ReservedCodes
  {
    RESERVED_CODE_INIT = 0,
    RESERVED_CODE_INIT_COMPLETE = 1,
    RESERVED_CODE_LAST = RESERVED_CODE_INIT_COMPLETE
  };

  struct MiniShmHeader
  {
    unsigned int  mId;
    unsigned int  mPayloadLen;
  };

  struct MiniShmInit
  {
    enum identifier_t
    {
      identifier = RESERVED_CODE_INIT
    };
    HANDLE    mParentEvent;
    HANDLE    mParentGuard;
    HANDLE    mChildEvent;
    HANDLE    mChildGuard;
  };

  struct MiniShmInitComplete
  {
    enum identifier_t
    {
      identifier = RESERVED_CODE_INIT_COMPLETE
    };
    bool      mSucceeded;
  };

  MiniShmBase()
    : mObserver(nullptr),
      mWriteHeader(nullptr),
      mReadHeader(nullptr),
      mPayloadMaxLen(0)
  {
  }
  virtual ~MiniShmBase()
  { }

  virtual void
  OnEvent()
  {
    if (mObserver) {
      mObserver->OnMiniShmEvent(this);
    }
  }

  virtual void
  OnConnect()
  {
    if (mObserver) {
      mObserver->OnMiniShmConnect(this);
    }
  }

  nsresult
  SetView(LPVOID aView, const unsigned int aSize, bool aIsChild)
  {
    if (!aView || aSize <= 2 * sizeof(MiniShmHeader)) {
      return NS_ERROR_ILLEGAL_VALUE;
    }
    
    if (aIsChild) {
      mReadHeader = static_cast<MiniShmHeader*>(aView);
      mWriteHeader = reinterpret_cast<MiniShmHeader*>(static_cast<char*>(aView)
                                                      + aSize / 2U);
    } else {
      mWriteHeader = static_cast<MiniShmHeader*>(aView);
      mReadHeader = reinterpret_cast<MiniShmHeader*>(static_cast<char*>(aView)
                                                     + aSize / 2U);
    }
    mPayloadMaxLen = aSize / 2U - sizeof(MiniShmHeader);
    return NS_OK;
  }

  inline void
  SetObserver(MiniShmObserver *aObserver) { mObserver = aObserver; }

  











  template<typename T> nsresult
  GetWritePtrInternal(T*& aPtr)
  {
    if (!mWriteHeader) {
      return NS_ERROR_NOT_INITIALIZED;
    }
    if (sizeof(T) > mPayloadMaxLen ||
        T::identifier > RESERVED_CODE_LAST) {
      return NS_ERROR_ILLEGAL_VALUE;
    }
    mWriteHeader->mId = T::identifier;
    mWriteHeader->mPayloadLen = sizeof(T);
    aPtr = reinterpret_cast<T*>(mWriteHeader + 1);
    return NS_OK;
  }

  static VOID CALLBACK
  SOnEvent(PVOID aContext, BOOLEAN aIsTimer)
  {
    MiniShmBase* object = static_cast<MiniShmBase*>(aContext);
    object->OnEvent();
  }

private:
  MiniShmObserver*  mObserver;
  MiniShmHeader*    mWriteHeader;
  MiniShmHeader*    mReadHeader;
  unsigned int      mPayloadMaxLen;

  DISALLOW_COPY_AND_ASSIGN(MiniShmBase);
};

} 
} 

#endif

