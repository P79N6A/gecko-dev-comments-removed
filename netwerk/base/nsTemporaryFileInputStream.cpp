




#include "nsTemporaryFileInputStream.h"
#include "nsStreamUtils.h"
#include <algorithm>

NS_IMPL_ISUPPORTS(nsTemporaryFileInputStream, nsIInputStream)

nsTemporaryFileInputStream::nsTemporaryFileInputStream(FileDescOwner* aFileDescOwner, uint64_t aStartPos, uint64_t aEndPos)
  : mFileDescOwner(aFileDescOwner),
    mStartPos(aStartPos),
    mEndPos(aEndPos),
    mClosed(false)
{ 
  NS_ASSERTION(aStartPos <= aEndPos, "StartPos should less equal than EndPos!");
}

NS_IMETHODIMP
nsTemporaryFileInputStream::Close()
{
  mClosed = true;
  return NS_OK;
}

NS_IMETHODIMP
nsTemporaryFileInputStream::Available(uint64_t * bytesAvailable)
{
  if (mClosed)
    return NS_BASE_STREAM_CLOSED;

  NS_ASSERTION(mStartPos <= mEndPos, "StartPos should less equal than EndPos!");

  *bytesAvailable = mEndPos - mStartPos;
  return NS_OK;
}

NS_IMETHODIMP
nsTemporaryFileInputStream::Read(char* buffer, uint32_t count, uint32_t* bytesRead)
{
  return ReadSegments(NS_CopySegmentToBuffer, buffer, count, bytesRead);
}

NS_IMETHODIMP
nsTemporaryFileInputStream::ReadSegments(nsWriteSegmentFun writer,
                                         void *            closure,
                                         uint32_t          count,
                                         uint32_t *        result)
{
  NS_ASSERTION(result, "null ptr");
  NS_ASSERTION(mStartPos <= mEndPos, "bad stream state");
  *result = 0;

  if (mClosed) {
    return NS_BASE_STREAM_CLOSED;
  }

  mozilla::MutexAutoLock lock(mFileDescOwner->FileMutex());
  PR_Seek64(mFileDescOwner->mFD, mStartPos, PR_SEEK_SET);

  
  count = std::min(count, uint32_t(mEndPos - mStartPos));

  char buf[4096];
  while (*result < count) {
    uint32_t bufCount = std::min(count - *result, (uint32_t) sizeof(buf));
    int32_t bytesRead = PR_Read(mFileDescOwner->mFD, buf, bufCount);
    if (bytesRead < 0) {
      return NS_ErrorAccordingToNSPR();
    }

    int32_t bytesWritten = 0;
    while (bytesWritten < bytesRead) {
      uint32_t writerCount = 0;
      nsresult rv = writer(this, closure, buf + bytesWritten, *result,
                           bytesRead - bytesWritten, &writerCount);
      if (NS_FAILED(rv) || writerCount == 0) {
        
        
        
        
        
        
        return NS_OK;
      }
      NS_ASSERTION(writerCount <= (uint32_t) (bytesRead - bytesWritten),
                   "writer should not write more than we asked it to write");
      bytesWritten += writerCount;
      *result += writerCount;
      mStartPos += writerCount;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTemporaryFileInputStream::IsNonBlocking(bool * nonBlocking)
{
  *nonBlocking = false;
  return NS_OK;
}

