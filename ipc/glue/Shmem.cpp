







































#include <math.h>

#include "Shmem.h"

#include "ProtocolUtils.h"
#include "SharedMemoryBasic.h"
#include "SharedMemorySysV.h"

#include "nsAutoPtr.h"


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
  return segment.forget();
}

static void
DestroySegment(SharedMemory* aSegment)
{
  
  delete aSegment;
}

static size_t
PageAlignedSize(size_t aSize)
{
  size_t pageSize = SharedMemory::SystemPageSize();
  size_t nPagesNeeded = int(ceil(double(aSize) / double(pageSize)));
  return pageSize * nPagesNeeded;
}


#if defined(DEBUG)

static const char sMagic[] =
    "This little piggy went to market.\n"
    "This little piggy stayed at home.\n"
    "This little piggy has roast beef,\n"
    "This little piggy had none.\n"
    "And this little piggy cried \"Wee! Wee! Wee!\" all the way home";


struct Header {
  size_t mSize;
  char mMagic[sizeof(sMagic)];
};

static void
GetSections(Shmem::SharedMemory* aSegment,
            char** aFrontSentinel,
            char** aData,
            char** aBackSentinel)
{
  NS_ABORT_IF_FALSE(aSegment && aFrontSentinel && aData && aBackSentinel,
                    "NULL param(s)");

  *aFrontSentinel = reinterpret_cast<char*>(aSegment->memory());
  NS_ABORT_IF_FALSE(*aFrontSentinel, "NULL memory()");

  size_t pageSize = Shmem::SharedMemory::SystemPageSize();
  *aData = *aFrontSentinel + pageSize;

  *aBackSentinel = *aFrontSentinel + aSegment->Size() - pageSize;
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

  char* frontSentinel;
  char* data;
  char* backSentinel;
  GetSections(aSegment, &frontSentinel, &data, &backSentinel);

  
  char check = *frontSentinel;
  (void)check;

  Header* header = reinterpret_cast<Header*>(frontSentinel);
  NS_ABORT_IF_FALSE(!strncmp(header->mMagic, sMagic, sizeof(sMagic)),
                      "invalid segment");
  mSize = header->mSize;

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
  checkMappingFront = checkMappingBack; 
}

void
Shmem::RevokeRights(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead)
{
  AssertInvariants();
  Protect(mSegment);
}


Shmem::SharedMemory*
Shmem::Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
             size_t aNBytes,
             SharedMemoryType aType,
             bool aProtect)
{
  size_t pageSize = SharedMemory::SystemPageSize();
  SharedMemory* segment = nsnull;
  
  size_t segmentSize = PageAlignedSize(aNBytes + 2*pageSize);

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

  char *frontSentinel;
  char *data;
  char *backSentinel;
  GetSections(segment, &frontSentinel, &data, &backSentinel);

  
  Header* header = reinterpret_cast<Header*>(frontSentinel);
  memcpy(header->mMagic, sMagic, sizeof(sMagic));
  header->mSize = aNBytes;

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
  
  size_t segmentSize = PageAlignedSize(size + 2*pageSize);

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

  if (aProtect)
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
  char *frontSentinel;
  char *data;
  char *backSentinel;
  GetSections(aSegment, &frontSentinel, &data, &backSentinel);

  aSegment->Protect(frontSentinel, pageSize, RightsWrite | RightsRead);
  Header* header = reinterpret_cast<Header*>(frontSentinel);
  memset(header->mMagic, 0, sizeof(sMagic));
  header->mSize = 0;

  DestroySegment(aSegment);
}


#else  


Shmem::SharedMemory*
Shmem::Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
             size_t aNBytes, 
             SharedMemoryType aType,
             bool )
{
  SharedMemory *segment = nsnull;

  if (aType == SharedMemory::TYPE_BASIC)
    segment = CreateSegment(PageAlignedSize(aNBytes + sizeof(size_t)),
                            SharedMemoryBasic::NULLHandle());
#ifdef MOZ_HAVE_SHAREDMEMORYSYSV
  else if (aType == SharedMemory::TYPE_SYSV)
    segment = CreateSegment(PageAlignedSize(aNBytes + sizeof(size_t)),
                            SharedMemorySysV::NULLHandle());
#endif
  else
    
    NS_ABORT();

  if (!segment)
    return 0;

  *PtrToSize(segment) = aNBytes;

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
  size_t segmentSize = PageAlignedSize(size + sizeof(size_t));

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

  
  if (size != *PtrToSize(segment))
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

} 
} 
