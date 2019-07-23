







































#include <math.h>

#include "Shmem.h"

#include "nsAutoPtr.h"


#if defined(DEBUG)
static const char sMagic[] =
    "This little piggy went to market.\n"
    "This little piggy stayed at home.\n"
    "This little piggy has roast beef,\n"
    "This little piggy had none.\n"
    "And this little piggy cried \"Wee! Wee! Wee!\" all the way home";
#endif

namespace mozilla {
namespace ipc {


#if defined(DEBUG)

namespace {

struct Header
{
  size_t mSize;
  char mMagic[sizeof(sMagic)];
};

void
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
Shmem::Protect(SharedMemory* aSegment)
{
  NS_ABORT_IF_FALSE(aSegment, "NULL segment");
  aSegment->Protect(reinterpret_cast<char*>(aSegment->memory()),
                    aSegment->Size(),
                    RightsNone);
}

void
Shmem::Unprotect(SharedMemory* aSegment)
{
  NS_ABORT_IF_FALSE(aSegment, "NULL segment");
  aSegment->Protect(reinterpret_cast<char*>(aSegment->memory()),
                    aSegment->Size(),
                    RightsRead | RightsWrite);
}

Shmem::SharedMemory*
Shmem::Alloc(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
             size_t aNBytes,
             bool aProtect)
{
  size_t pageSize = SharedMemory::SystemPageSize();
  
  SharedMemory* segment = CreateSegment(PageAlignedSize(aNBytes + 2*pageSize));
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
                    SharedMemoryHandle aHandle,
                    size_t aNBytes,
                    bool aProtect)
{
  if (!SharedMemory::IsHandleValid(aHandle))
    NS_RUNTIMEABORT("trying to open invalid handle");

  size_t pageSize = SharedMemory::SystemPageSize();
  
  SharedMemory* segment = CreateSegment(PageAlignedSize(aNBytes + 2*pageSize),
                                        aHandle);
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
             bool )
{
  SharedMemory* segment =
    CreateSegment(PageAlignedSize(aNBytes + sizeof(size_t)));
  if (!segment)
    return 0;

  *PtrToSize(segment) = aNBytes;

  return segment;
}

Shmem::SharedMemory*
Shmem::OpenExisting(IHadBetterBeIPDLCodeCallingThis_OtherwiseIAmADoodyhead,
                    SharedMemoryHandle aHandle,
                    size_t aNBytes,
                    bool )
{
  SharedMemory* segment =
    CreateSegment(PageAlignedSize(aNBytes + sizeof(size_t)), aHandle);
  if (!segment)
    return 0;

  
  if (aNBytes != *PtrToSize(segment))
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


Shmem::SharedMemory*
Shmem::CreateSegment(size_t aNBytes, SharedMemoryHandle aHandle)
{
  nsAutoPtr<SharedMemory> segment;

  if (SharedMemory::IsHandleValid(aHandle)) {
    segment = new SharedMemory(aHandle);
  }
  else {
    segment = new SharedMemory();
    if (!segment->Create("", false, false, aNBytes))
      return 0;
  }
  if (!segment->Map(aNBytes))
    return 0;
  return segment.forget();
}

void
Shmem::DestroySegment(SharedMemory* aSegment)
{
  
  delete aSegment;
}

size_t
Shmem::PageAlignedSize(size_t aSize)
{
  size_t pageSize = SharedMemory::SystemPageSize();
  size_t nPagesNeeded = int(ceil(double(aSize) / double(pageSize)));
  return pageSize * nPagesNeeded;
}


} 
} 
