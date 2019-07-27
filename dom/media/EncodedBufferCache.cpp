





#include "EncodedBufferCache.h"
#include "mozilla/dom/File.h"
#include "nsAnonymousTemporaryFile.h"
#include "prio.h"

namespace mozilla {

void
EncodedBufferCache::AppendBuffer(nsTArray<uint8_t> & aBuf)
{
  MutexAutoLock lock(mMutex);
  mDataSize += aBuf.Length();

  mEncodedBuffers.AppendElement()->SwapElements(aBuf);

  if (!mTempFileEnabled && mDataSize > mMaxMemoryStorage) {
    nsresult rv;
    PRFileDesc* tempFD = nullptr;
    {
      
      
      MutexAutoUnlock unlock(mMutex);
      rv = NS_OpenAnonymousTemporaryFile(&tempFD);
    }
    if (!NS_FAILED(rv)) {
      
      if (mDataSize > mMaxMemoryStorage) {
        mFD = tempFD;
        mTempFileEnabled = true;
      } else {
        
        
        PR_Close(tempFD);
      }
    }
  }

  if (mTempFileEnabled) {
    
    for (uint32_t i = 0; i < mEncodedBuffers.Length(); i++) {
      int32_t amount = PR_Write(mFD, mEncodedBuffers.ElementAt(i).Elements(), mEncodedBuffers.ElementAt(i).Length());
      if (amount < 0 || size_t(amount) < mEncodedBuffers.ElementAt(i).Length()) {
        NS_WARNING("Failed to write media cache block!");
      }
    }
    mEncodedBuffers.Clear();
  }

}

already_AddRefed<dom::Blob>
EncodedBufferCache::ExtractBlob(nsISupports* aParent,
                                const nsAString &aContentType)
{
  MutexAutoLock lock(mMutex);
  nsRefPtr<dom::Blob> blob;
  if (mTempFileEnabled) {
    
    blob = dom::Blob::CreateTemporaryBlob(aParent, mFD, 0, mDataSize,
                                          aContentType);
    
    mTempFileEnabled = false;
    mDataSize = 0;
    mFD = nullptr;
  } else {
    void* blobData = malloc(mDataSize);
    NS_ASSERTION(blobData, "out of memory!!");

    if (blobData) {
      for (uint32_t i = 0, offset = 0; i < mEncodedBuffers.Length(); i++) {
        memcpy((uint8_t*)blobData + offset, mEncodedBuffers.ElementAt(i).Elements(),
               mEncodedBuffers.ElementAt(i).Length());
        offset += mEncodedBuffers.ElementAt(i).Length();
      }
      blob = dom::Blob::CreateMemoryBlob(aParent, blobData, mDataSize,
                                         aContentType);
      mEncodedBuffers.Clear();
    } else
      return nullptr;
  }
  mDataSize = 0;
  return blob.forget();
}

} 
