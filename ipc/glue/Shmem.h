







































#ifndef mozilla_ipc_Shmem_h
#define mozilla_ipc_Shmem_h

#include "base/basictypes.h"

#include "nscore.h"
#include "nsDebug.h"

#include "IPC/IPCMessageUtils.h"
#include "mozilla/ipc/SharedMemory.h"



































namespace mozilla {
namespace ipc {


class NS_FINAL_CLASS Shmem
{
  friend class IPC::ParamTraits<mozilla::ipc::Shmem>;

public:
  typedef int32 id_t;
  
  typedef mozilla::ipc::SharedMemory SharedMemory;
  typedef SharedMemory::SharedMemoryHandle SharedMemoryHandle;
  struct IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead {};

  Shmem() :
    mSegment(0),
    mData(0),
    mSize(0),
    mId(0)
  {
  }

  Shmem(const Shmem& aOther) :
    mSegment(aOther.mSegment),
    mData(aOther.mData),
    mSize(aOther.mSize),
    mId(aOther.mId)
  {
  }

#if !defined(DEBUG)
  Shmem(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
        SharedMemory* aSegment, id_t aId) :
    mSegment(aSegment),
    mData(aSegment->memory()),
    mSize(0),
    mId(aId)
  {
    mSize = *PtrToSize(mSegment);
  }
#else
  Shmem(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
        SharedMemory* aSegment, id_t aId);
#endif

  ~Shmem()
  {
    
    
    forget(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead());
  }

  Shmem& operator=(const Shmem& aRhs)
  {
    mSegment = aRhs.mSegment;
    mData = aRhs.mData;
    mSize = aRhs.mSize;
    mId = aRhs.mId;
    return *this;
  }

  
  template<typename T>
  T*
  get() const
  {
    AssertInvariants();
    AssertAligned<T>();

    return reinterpret_cast<T*>(mData);
  }

  
  
  
  
  template<typename T>
  size_t
  Size() const
  {
    AssertInvariants();
    AssertAligned<T>();

    return mSize / sizeof(T);
  }

  
  id_t Id(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead) {
    return mId;
  }

  void RevokeRights(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead)
  {
    Protect(mSegment);
  }

  void forget(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead)
  {
    mSegment = 0;
    mData = 0;
    mSize = 0;
    mId = 0;
  }

  static SharedMemory*
  Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
        size_t aNBytes,
        bool aProtect=false);

  
  
  
  static SharedMemory*
  OpenExisting(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
               SharedMemoryHandle aHandle,
               size_t aNBytes,
               bool aProtect=false);

  static void
  Dealloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
          SharedMemory* aSegment);

private:
  template<typename T>
  void AssertAligned() const
  {
    if (0 != (mSize % sizeof(T)))
      NS_RUNTIMEABORT("shmem is not T-aligned");
  }

#if !defined(DEBUG)
  void AssertInvariants() const
  { }
  static void Unprotect(SharedMemory* aSegment)
  { }
  static void Protect(SharedMemory* aSegment)
  { }

  static size_t*
  PtrToSize(SharedMemory* aSegment)
  {
    char* endOfSegment =
      reinterpret_cast<char*>(aSegment->memory()) + aSegment->Size();
    return reinterpret_cast<size_t*>(endOfSegment - sizeof(size_t));
  }

#else
  void AssertInvariants() const;

  static void Unprotect(SharedMemory* aSegment);
  static void Protect(SharedMemory* aSegment);
#endif

  static SharedMemory*
  CreateSegment(size_t aNBytes,
                SharedMemoryHandle aHandle=SharedMemory::NULLHandle());

  static void
  DestroySegment(SharedMemory* aSegment);

  static size_t
  PageAlignedSize(size_t aSize);

  SharedMemory* mSegment;
  void* mData;
  size_t mSize;
  id_t mId;

  
  static void* operator new(size_t) CPP_THROW_NEW;
  static void operator delete(void*);  
};


} 
} 


namespace IPC {

template<>
struct ParamTraits<mozilla::ipc::Shmem>
{
  typedef mozilla::ipc::Shmem paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mId);
  }

  
  

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    paramType::id_t id;
    if (!ReadParam(aMsg, aIter, &id))
      return false;
    aResult->mId = id;
    return true;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(L"(shmem segment)");
  }
};


} 


#endif 
