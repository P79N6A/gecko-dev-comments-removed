





#include "MP4Stream.h"
#include "MediaResource.h"

namespace mozilla {

MP4Stream::MP4Stream(MediaResource* aResource) : mResource(aResource)
{
  MOZ_COUNT_CTOR(MP4Stream);
  MOZ_ASSERT(aResource);
}

MP4Stream::~MP4Stream()
{
  MOZ_COUNT_DTOR(MP4Stream);
}

bool
MP4Stream::ReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                  size_t* aBytesRead)
{
  uint32_t sum = 0;
  uint32_t bytesRead = 0;
  do {
    uint64_t offset = aOffset + sum;
    char* buffer = reinterpret_cast<char*>(aBuffer) + sum;
    uint32_t toRead = aCount - sum;
    nsresult rv = mResource->ReadAt(offset, buffer, toRead, &bytesRead);
    if (NS_FAILED(rv)) {
      return false;
    }
    sum += bytesRead;
  } while (sum < aCount && bytesRead > 0);
  *aBytesRead = sum;
  return true;
}

bool
MP4Stream::CachedReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                        size_t* aBytesRead)
{
  nsresult rv = mResource->ReadFromCache(reinterpret_cast<char*>(aBuffer),
                                         aOffset, aCount);
  if (NS_FAILED(rv)) {
    *aBytesRead = 0;
    return false;
  }
  *aBytesRead = aCount;
  return true;
}

bool
MP4Stream::Length(int64_t* aSize)
{
  if (mResource->GetLength() < 0)
    return false;
  *aSize = mResource->GetLength();
  return true;
}
}
