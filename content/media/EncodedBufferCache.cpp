





#include "EncodedBufferCache.h"
#include "nsAnonymousTemporaryFile.h"
#include "nsLocalFile.h"

namespace mozilla {

void
EncodedBufferCache::AppendBuffer(nsTArray<uint8_t> & aBuf)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mDataSize += aBuf.Length();

  mEncodedBuffers.AppendElement()->SwapElements(aBuf);

  if (!mTempFileEnabled && mDataSize > mMaxMemoryStorage) {
    nsresult rv = NS_OpenAnonymousTemporaryFile(&mFD);
    if (!NS_FAILED(rv)) {
      mTempFileEnabled = true;
    }
  }

  if (mTempFileEnabled) {
    
    for (uint32_t i = 0; i < mEncodedBuffers.Length(); i++) {
      int64_t amount = PR_Write(mFD, mEncodedBuffers.ElementAt(i).Elements(), mEncodedBuffers.ElementAt(i).Length());
      if (amount <  mEncodedBuffers.ElementAt(i).Length()) {
        NS_WARNING("Failed to write media cache block!");
      }
    }
    mEncodedBuffers.Clear();
  }

}

already_AddRefed<nsIDOMBlob>
EncodedBufferCache::ExtractBlob(const nsAString &aContentType)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  nsCOMPtr<nsIDOMBlob> blob;
  if (mTempFileEnabled) {
    
    blob = new nsDOMTemporaryFileBlob(mFD, 0, mDataSize, aContentType);
    
    mTempFileEnabled = false;
    mDataSize = 0;
    mFD = nullptr;
  } else {
    void* blobData = moz_malloc(mDataSize);
    NS_ASSERTION(blobData, "out of memory!!");

    if (blobData) {
      for (uint32_t i = 0, offset = 0; i < mEncodedBuffers.Length(); i++) {
        memcpy((uint8_t*)blobData + offset, mEncodedBuffers.ElementAt(i).Elements(),
               mEncodedBuffers.ElementAt(i).Length());
        offset += mEncodedBuffers.ElementAt(i).Length();
      }
      blob = new nsDOMMemoryFile(blobData, mDataSize,
                                 aContentType);
      mEncodedBuffers.Clear();
    } else
      return nullptr;
  }
  mDataSize = 0;
  return blob.forget();
}

} 
