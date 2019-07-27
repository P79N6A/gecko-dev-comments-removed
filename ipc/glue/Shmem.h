






#ifndef mozilla_ipc_Shmem_h
#define mozilla_ipc_Shmem_h

#include "mozilla/Attributes.h"

#include "base/basictypes.h"
#include "base/process.h"

#include "nscore.h"
#include "nsDebug.h"
#include "nsAutoPtr.h"

#include "ipc/IPCMessageUtils.h"
#include "mozilla/ipc/SharedMemory.h"



































namespace mozilla {
namespace layers {
class ShadowLayerForwarder;
}

namespace ipc {

class Shmem final
{
  friend struct IPC::ParamTraits<mozilla::ipc::Shmem>;
#ifdef DEBUG
  
  friend class mozilla::layers::ShadowLayerForwarder;
#endif

public:
  typedef int32_t id_t;
  
  typedef mozilla::ipc::SharedMemory SharedMemory;
  typedef SharedMemory::SharedMemoryType SharedMemoryType;
  struct IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead {};

  Shmem() :
    mSegment(nullptr),
    mData(nullptr),
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
    mSize = static_cast<size_t>(*PtrToSize(mSegment));
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

  bool operator==(const Shmem& aRhs) const
  {
    
    
    
    
    
    
    return mSegment == aRhs.mSegment && mId == aRhs.mId;
  }

  
  
  bool
  IsWritable() const
  {
    return mSegment != nullptr;
  }

  
  
  bool
  IsReadable() const
  {
    return mSegment != nullptr;
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

  int GetSysVID() const;

  
  id_t Id(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead) const {
    return mId;
  }

  SharedMemory* Segment(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead) const {
    return mSegment;
  }

#ifndef DEBUG
  void RevokeRights(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead)
  {
  }
#else
  void RevokeRights(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead);
#endif

  void forget(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead)
  {
    mSegment = nullptr;
    mData = nullptr;
    mSize = 0;
    mId = 0;
  }

  static already_AddRefed<Shmem::SharedMemory>
  Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
        size_t aNBytes,
        SharedMemoryType aType,
        bool aUnsafe,
        bool aProtect=false);

  
  
  
  
  IPC::Message*
  ShareTo(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
          base::ProcessId aTargetPid,
          int32_t routingId);

  
  
  
  
  IPC::Message*
  UnshareFrom(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
              base::ProcessId aTargetPid,
              int32_t routingId);

  
  
  
  
  static already_AddRefed<SharedMemory>
  OpenExisting(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
               const IPC::Message& aDescriptor,
               id_t* aId,
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

  static uint32_t*
  PtrToSize(SharedMemory* aSegment)
  {
    char* endOfSegment =
      reinterpret_cast<char*>(aSegment->memory()) + aSegment->Size();
    return reinterpret_cast<uint32_t*>(endOfSegment - sizeof(uint32_t));
  }

#else
  void AssertInvariants() const;
#endif

  SharedMemory* MOZ_NON_OWNING_REF mSegment;
  void* mData;
  size_t mSize;
  id_t mId;
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
