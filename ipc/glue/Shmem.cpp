







































#include "Shmem.h"

#include "ProtocolUtils.h"
#include "SharedMemoryBasic.h"
#include "SharedMemorySysV.h"

#include "nsAutoPtr.h"
#include "mozilla/unused.h"


namespace mozilla {
namespace ipc {

class ShmemCreated : public IPC::Message
{
private:
  typedef Shmem::id_t id_t;

public:
  ShmemCreated(int32 routingId,
               const id_t& aIPDLId,
               const size_t& aSize,
               const SharedMemoryBasic::Handle& aHandle) :
    IPC::Message(routingId, SHMEM_CREATED_MESSAGE_TYPE, PRIORITY_NORMAL)
  {
    IPC::WriteParam(this, aIPDLId);
    IPC::WriteParam(this, aSize);
    IPC::WriteParam(this, int32(SharedMemory::TYPE_BASIC)),
    IPC::WriteParam(this, aHandle);
  }

  
  
  
  

  static bool
  ReadInfo(const Message* msg, void** iter,
           id_t* aIPDLId,
           size_t* aSize,
           SharedMemory::SharedMemoryType* aType)
  {
    if (!IPC::ReadParam(msg, iter, aIPDLId) ||
        !IPC::ReadParam(msg, iter, aSize) ||
        !IPC::ReadParam(msg, iter, reinterpret_cast<int32*>(aType)))
      return false;
    return true;
  }

  static bool
  ReadHandle(const Message* msg, void** iter,
             SharedMemoryBasic::Handle* aHandle)
  {
    if (!IPC::ReadParam(msg, iter, aHandle))
      return false;
    msg->EndRead(*iter);
    return true;
  }

#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  ShmemCreated(int32 routingId,
               const id_t& aIPDLId,
               const size_t& aSize,
               const SharedMemorySysV::Handle& aHandle) :
    IPC::Message(routingId, SHMEM_CREATED_MESSAGE_TYPE, PRIORITY_NORMAL)
  {
    IPC::WriteParam(this, aIPDLId);
    IPC::WriteParam(this, aSize);
    IPC::WriteParam(this, int32(SharedMemory::TYPE_SYSV)),
    IPC::WriteParam(this, aHandle);
  }

  static bool
  ReadHandle(const Message* msg, void** iter,
             SharedMemorySysV::Handle* aHandle)
  {
    if (!IPC::ReadParam(msg, iter, aHandle))
      return false;
    msg->EndRead(*iter);
    return true;
  }
#endif

  void Log(const std::string& aPrefix,
           FILE* aOutf) const
  {
    fputs("(special ShmemCreated msg)", aOutf);
  }
};

class ShmemDestroyed : public IPC::Message
{
private:
  typedef Shmem::id_t id_t;

public:
  ShmemDestroyed(int32 routingId,
                 const id_t& aIPDLId) :
    IPC::Message(routingId, SHMEM_DESTROYED_MESSAGE_TYPE, PRIORITY_NORMAL)
  {
    IPC::WriteParam(this, aIPDLId);
  }
};


#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
static Shmem::SharedMemory*
CreateSegment(size_t aNBytes, SharedMemorySysV::Handle aHandle)
{
  nsAutoPtr<SharedMemory> segment;

  if (SharedMemorySysV::IsHandleValid(aHandle)) {
    segment = new SharedMemorySysV(aHandle);
  }
  else {
    segment = new SharedMemorySysV();

    if (!segment->Create(aNBytes))
      return 0;
  }
  if (!segment->Map(aNBytes))
    return 0;

  segment->AddRef();
  return segment.forget();
}
#endif

static Shmem::SharedMemory*
CreateSegment(size_t aNBytes, SharedMemoryBasic::Handle aHandle)
{
  nsAutoPtr<SharedMemory> segment;

  if (SharedMemoryBasic::IsHandleValid(aHandle)) {
    segment = new SharedMemoryBasic(aHandle);
  }
  else {
    segment = new SharedMemoryBasic();

    if (!segment->Create(aNBytes))
      return 0;
  }
  if (!segment->Map(aNBytes))
    return 0;

  segment->AddRef();
  return segment.forget();
}

static void
DestroySegment(SharedMemory* aSegment)
{
  
  if (aSegment)
    aSegment->Release();
}


#if defined(DEBUG)

static const char sMagic[] =
    "This little piggy went to market.\n"
    "This little piggy stayed at home.\n"
    "This little piggy has roast beef,\n"
    "This little piggy had none.\n"
    "And this little piggy cried \"Wee! Wee! Wee!\" all the way home";


struct Header {
  
  
  uint32 mSize;
  uint32 mUnsafe;
  char mMagic[sizeof(sMagic)];
};

static void
GetSections(Shmem::SharedMemory* aSegment,
            Header** aHeader,
            char** aFrontSentinel,
            char** aData,
            char** aBackSentinel)
{
  NS_ABORT_IF_FALSE(aSegment && aFrontSentinel && aData && aBackSentinel,
                    "NULL param(s)");

  *aFrontSentinel = reinterpret_cast<char*>(aSegment->memory());
  NS_ABORT_IF_FALSE(*aFrontSentinel, "NULL memory()");

  *aHeader = reinterpret_cast<Header*>(*aFrontSentinel);

  size_t pageSize = Shmem::SharedMemory::SystemPageSize();
  *aData = *aFrontSentinel + pageSize;

  *aBackSentinel = *aFrontSentinel + aSegment->Size() - pageSize;
}

static Header*
GetHeader(Shmem::SharedMemory* aSegment)
{
  Header* header;
  char* dontcare;
  GetSections(aSegment, &header, &dontcare, &dontcare, &dontcare);
  return header;
}

static void
Protect(SharedMemory* aSegment)
{
  NS_ABORT_IF_FALSE(aSegment, "NULL segment");
  aSegment->Protect(reinterpret_cast<char*>(aSegment->memory()),
                    aSegment->Size(),
                    RightsNone);
}

static void
Unprotect(SharedMemory* aSegment)
{
  NS_ABORT_IF_FALSE(aSegment, "NULL segment");
  aSegment->Protect(reinterpret_cast<char*>(aSegment->memory()),
                    aSegment->Size(),
                    RightsRead | RightsWrite);
}


















































Shmem::Shmem(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
             SharedMemory* aSegment, id_t aId) :
    mSegment(aSegment),
    mData(0),
    mSize(0)
{
  NS_ABORT_IF_FALSE(mSegment, "NULL segment");
  NS_ABORT_IF_FALSE(aId != 0, "invalid ID");

  Unprotect(mSegment);

  Header* header;
  char* frontSentinel;
  char* data;
  char* backSentinel;
  GetSections(aSegment, &header, &frontSentinel, &data, &backSentinel);

  
  char check = *frontSentinel;
  (void)check;

  NS_ABORT_IF_FALSE(!strncmp(header->mMagic, sMagic, sizeof(sMagic)),
                      "invalid segment");
  mSize = static_cast<size_t>(header->mSize);

  size_t pageSize = SharedMemory::SystemPageSize();
  
  
  mSegment->Protect(frontSentinel, pageSize, RightsNone);
  mSegment->Protect(backSentinel, pageSize, RightsNone);

  
  mData = data;
  mId = aId;
}

void
Shmem::AssertInvariants() const
{
  NS_ABORT_IF_FALSE(mSegment, "NULL segment");
  NS_ABORT_IF_FALSE(mData, "NULL data pointer");
  NS_ABORT_IF_FALSE(mSize > 0, "invalid size");
  
  
  char checkMappingFront = *reinterpret_cast<char*>(mData);
  char checkMappingBack = *(reinterpret_cast<char*>(mData) + mSize - 1);

  
  unused << checkMappingFront;
  unused << checkMappingBack;
}

void
Shmem::RevokeRights(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead)
{
  AssertInvariants();

  size_t pageSize = SharedMemory::SystemPageSize();
  Header* header = GetHeader(mSegment);

  
  mSegment->Protect(reinterpret_cast<char*>(header), pageSize, RightsRead);

  if (!header->mUnsafe) {
    Protect(mSegment);
  } else {
    mSegment->Protect(reinterpret_cast<char*>(header), pageSize, RightsNone);
  }
}


Shmem::SharedMemory*
Shmem::Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
             size_t aNBytes,
             SharedMemoryType aType,
             bool aUnsafe,
             bool aProtect)
{
  NS_ASSERTION(aNBytes <= PR_UINT32_MAX, "Will truncate shmem segment size!");
  NS_ABORT_IF_FALSE(!aProtect || !aUnsafe, "protect => !unsafe");

  size_t pageSize = SharedMemory::SystemPageSize();
  SharedMemory* segment = nsnull;
  
  size_t segmentSize = SharedMemory::PageAlignedSize(aNBytes + 2*pageSize);

  if (aType == SharedMemory::TYPE_BASIC)
    segment = CreateSegment(segmentSize, SharedMemoryBasic::NULLHandle());
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  else if (aType == SharedMemory::TYPE_SYSV)
    segment = CreateSegment(segmentSize, SharedMemorySysV::NULLHandle());
#endif
  else
    NS_RUNTIMEABORT("unknown shmem type");

  if (!segment)
    return 0;

  Header* header;
  char *frontSentinel;
  char *data;
  char *backSentinel;
  GetSections(segment, &header, &frontSentinel, &data, &backSentinel);

  

  
  
  
  NS_ABORT_IF_FALSE(sizeof(Header) <= pageSize,
                    "Shmem::Header has gotten too big");
  memcpy(header->mMagic, sMagic, sizeof(sMagic));
  header->mSize = static_cast<uint32>(aNBytes);
  header->mUnsafe = aUnsafe;

  if (aProtect)
    Protect(segment);

  return segment;
}


Shmem::SharedMemory*
Shmem::OpenExisting(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
                    const IPC::Message& aDescriptor,
                    id_t* aId,
                    bool aProtect)
{
  if (SHMEM_CREATED_MESSAGE_TYPE != aDescriptor.type())
    NS_RUNTIMEABORT("expected 'shmem created' message");

  void* iter = 0;
  SharedMemory::SharedMemoryType type;
  size_t size;
  if (!ShmemCreated::ReadInfo(&aDescriptor, &iter, aId, &size, &type))
    return 0;

  SharedMemory* segment = 0;
  size_t pageSize = SharedMemory::SystemPageSize();
  
  size_t segmentSize = SharedMemory::PageAlignedSize(size + 2*pageSize);

  if (SharedMemory::TYPE_BASIC == type) {
    SharedMemoryBasic::Handle handle;
    if (!ShmemCreated::ReadHandle(&aDescriptor, &iter, &handle))
      return 0;

    if (!SharedMemoryBasic::IsHandleValid(handle))
      NS_RUNTIMEABORT("trying to open invalid handle");
    segment = CreateSegment(segmentSize, handle);
  }
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  else if (SharedMemory::TYPE_SYSV == type) {
    SharedMemorySysV::Handle handle;
    if (!ShmemCreated::ReadHandle(&aDescriptor, &iter, &handle))
      return 0;

    if (!SharedMemorySysV::IsHandleValid(handle))
      NS_RUNTIMEABORT("trying to open invalid handle");
    segment = CreateSegment(segmentSize, handle);
  }
#endif
  else {
    NS_RUNTIMEABORT("unknown shmem type");
  }

  if (!segment)
    return 0;

  
  
  Header* header = GetHeader(segment);
  if (!header->mUnsafe && aProtect)
    Protect(segment);

  return segment;
}


void
Shmem::Dealloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
               SharedMemory* aSegment)
{
  if (!aSegment)
    return;

  size_t pageSize = SharedMemory::SystemPageSize();
  Header* header;
  char *frontSentinel;
  char *data;
  char *backSentinel;
  GetSections(aSegment, &header, &frontSentinel, &data, &backSentinel);

  aSegment->Protect(frontSentinel, pageSize, RightsWrite | RightsRead);
  memset(header->mMagic, 0, sizeof(sMagic));
  header->mSize = 0;
  header->mUnsafe = false;          

  DestroySegment(aSegment);
}


#else  


Shmem::SharedMemory*
Shmem::Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
             size_t aNBytes, 
             SharedMemoryType aType,
             bool ,
             bool )
{
  SharedMemory *segment = nsnull;

  if (aType == SharedMemory::TYPE_BASIC)
    segment = CreateSegment(SharedMemory::PageAlignedSize(aNBytes + sizeof(uint32)),
                            SharedMemoryBasic::NULLHandle());
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  else if (aType == SharedMemory::TYPE_SYSV)
    segment = CreateSegment(SharedMemory::PageAlignedSize(aNBytes + sizeof(uint32)),
                            SharedMemorySysV::NULLHandle());
#endif
  else
    
    NS_ABORT();

  if (!segment)
    return 0;

  *PtrToSize(segment) = static_cast<uint32>(aNBytes);

  return segment;
}


Shmem::SharedMemory*
Shmem::OpenExisting(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
                    const IPC::Message& aDescriptor,
                    id_t* aId,
                    bool )
{
  if (SHMEM_CREATED_MESSAGE_TYPE != aDescriptor.type())
    NS_RUNTIMEABORT("expected 'shmem created' message");

  SharedMemory::SharedMemoryType type;
  void* iter = 0;
  size_t size;
  if (!ShmemCreated::ReadInfo(&aDescriptor, &iter, aId, &size, &type))
    return 0;

  SharedMemory* segment = 0;
  size_t segmentSize = SharedMemory::PageAlignedSize(size + sizeof(size_t));

  if (SharedMemory::TYPE_BASIC == type) {
    SharedMemoryBasic::Handle handle;
    if (!ShmemCreated::ReadHandle(&aDescriptor, &iter, &handle))
      return 0;

    if (!SharedMemoryBasic::IsHandleValid(handle))
      NS_RUNTIMEABORT("trying to open invalid handle");

    segment = CreateSegment(segmentSize, handle);
  }
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  else if (SharedMemory::TYPE_SYSV == type) {
    SharedMemorySysV::Handle handle;
    if (!ShmemCreated::ReadHandle(&aDescriptor, &iter, &handle))
      return 0;

    if (!SharedMemorySysV::IsHandleValid(handle))
      NS_RUNTIMEABORT("trying to open invalid handle");
    segment = CreateSegment(segmentSize, handle);
  }
#endif
  else {
    NS_RUNTIMEABORT("unknown shmem type");
  }

  if (!segment)
    return 0;

  
  if (size != static_cast<size_t>(*PtrToSize(segment)))
    NS_RUNTIMEABORT("Alloc() segment size disagrees with OpenExisting()'s");

  return segment;
}


void
Shmem::Dealloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
               SharedMemory* aSegment)
{
  DestroySegment(aSegment);
}

#endif  

int
Shmem::GetSysVID() const
{
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  AssertInvariants();

  if (mSegment->Type() != SharedMemory::TYPE_SYSV)
    NS_RUNTIMEABORT("Can't call GetSysVID() on a non-SysV Shmem!");

  SharedMemorySysV* seg = static_cast<SharedMemorySysV*>(mSegment);
  return seg->GetHandle();
#else
  NS_RUNTIMEABORT("Can't call GetSysVID() with no support for SysV shared memory!");
  return -1;                    
#endif
}

IPC::Message*
Shmem::ShareTo(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
               base::ProcessHandle aProcess,
               int32 routingId)
{
  AssertInvariants();

  if (SharedMemory::TYPE_BASIC == mSegment->Type()) {
    SharedMemoryBasic* seg = static_cast<SharedMemoryBasic*>(mSegment);
    SharedMemoryBasic::Handle handle;
    if (!seg->ShareToProcess(aProcess, &handle))
      return 0;

    return new ShmemCreated(routingId, mId, mSize, handle);
  }
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  else if (SharedMemory::TYPE_SYSV == mSegment->Type()) {
    SharedMemorySysV* seg = static_cast<SharedMemorySysV*>(mSegment);
    return new ShmemCreated(routingId, mId, mSize, seg->GetHandle());
  }
#endif
  else {
    NS_RUNTIMEABORT("unknown shmem type (here?!)");
  }

  return 0;
}

IPC::Message*
Shmem::UnshareFrom(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
                   base::ProcessHandle aProcess,
                   int32 routingId)
{
  AssertInvariants();
  return new ShmemDestroyed(routingId, mId);
}

} 
} 
