





#ifndef EncodedBufferCache_h_
#define EncodedBufferCache_h_

#include "nsTArray.h"
#include "mozilla/ReentrantMonitor.h"
#include "prio.h"
#include "nsDOMFile.h"

namespace mozilla {

class ReentrantMonitor;






class EncodedBufferCache
{
public:
  EncodedBufferCache(uint32_t aMaxMemoryStorage)
  : mFD(nullptr),
    mReentrantMonitor("EncodedBufferCache.Data.Monitor"),
    mDataSize(0),
    mMaxMemoryStorage(aMaxMemoryStorage),
    mTempFileEnabled(false) { }
  ~EncodedBufferCache()
  {
    NS_ASSERTION(mDataSize == 0, "still has data in EncodedBuffers!");
  }
  
  
  void AppendBuffer(nsTArray<uint8_t> & aBuf);
  
  already_AddRefed<nsIDOMBlob> ExtractBlob(const nsAString &aContentType);

private:
  
  nsTArray<nsTArray<uint8_t> > mEncodedBuffers;
  
  PRFileDesc* mFD;
  
  ReentrantMonitor mReentrantMonitor;
  
  uint64_t mDataSize;
  
  uint32_t mMaxMemoryStorage;
  
  bool mTempFileEnabled;
};

} 

#endif
