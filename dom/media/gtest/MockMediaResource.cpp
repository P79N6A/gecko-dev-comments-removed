



#include "MockMediaResource.h"

#include <sys/types.h>
#include <sys/stat.h>

namespace mozilla
{

MockMediaResource::MockMediaResource(const char* aFileName)
  : mFileName(aFileName)
  , mContentType(NS_LITERAL_CSTRING("video/mp4"))
{
}

nsresult
MockMediaResource::Open(nsIStreamListener** aStreamListener)
{
  mFileHandle = fopen(mFileName, "rb");
  if (mFileHandle == nullptr) {
    printf_stderr("Can't open %s\n", mFileName);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

MockMediaResource::~MockMediaResource()
{
  if (mFileHandle != nullptr) {
    fclose(mFileHandle);
  }
}

nsresult
MockMediaResource::ReadAt(int64_t aOffset, char* aBuffer, uint32_t aCount,
                          uint32_t* aBytes)
{
  if (mFileHandle == nullptr) {
    return NS_ERROR_FAILURE;
  }

  
  if (mEntry++) {
    MOZ_ASSERT(false);
    return NS_ERROR_FAILURE;
  }

  fseek(mFileHandle, aOffset, SEEK_SET);
  *aBytes = (uint32_t) fread(aBuffer, aCount, 1, mFileHandle);

  mEntry--;

  return ferror(mFileHandle) ? NS_ERROR_FAILURE : NS_OK;
}

int64_t
MockMediaResource::GetLength()
{
  return fseek(mFileHandle, 0, SEEK_END);
}

void
MockMediaResource::MockClearBufferedRanges()
{
  mRanges.Clear();
}

void
MockMediaResource::MockAddBufferedRange(int64_t aStart, int64_t aEnd)
{
  mRanges.AppendElement(MediaByteRange(aStart, aEnd));
}

int64_t
MockMediaResource::GetNextCachedData(int64_t aOffset)
{
  if (!aOffset) {
    return mRanges.Length() ? mRanges[0].mStart : -1;
  }
  for (size_t i = 0; i < mRanges.Length(); i++) {
    if (aOffset == mRanges[i].mStart) {
      ++i;
      return i < mRanges.Length() ? mRanges[i].mStart : -1;
    }
  }
  return -1;
}

int64_t
MockMediaResource::GetCachedDataEnd(int64_t aOffset)
{
  for (size_t i = 0; i < mRanges.Length(); i++) {
    if (aOffset == mRanges[i].mStart) {
      return mRanges[i].mEnd;
    }
  }
  return -1;
}

nsresult
MockMediaResource::GetCachedRanges(nsTArray<MediaByteRange>& aRanges)
{
  aRanges.Clear();
  aRanges.AppendElements(mRanges);
  return NS_OK;
}
}
