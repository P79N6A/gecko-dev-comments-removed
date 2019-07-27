





#ifndef EncodedBufferCache_h_
#define EncodedBufferCache_h_

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "mozilla/Mutex.h"

struct PRFileDesc;
class nsIDOMBlob;

namespace mozilla {

class ReentrantMonitor;






class EncodedBufferCache
{
public:
  explicit EncodedBufferCache(uint32_t aMaxMemoryStorage)
  : mFD(nullptr),
    mMutex("EncodedBufferCache.Data.Mutex"),
    mDataSize(0),
    mMaxMemoryStorage(aMaxMemoryStorage),
    mTempFileEnabled(false) { }
  ~EncodedBufferCache()
  {
  }
  
  
  void AppendBuffer(nsTArray<uint8_t> & aBuf);
  
  already_AddRefed<nsIDOMBlob> ExtractBlob(const nsAString &aContentType);

private:
  
  nsTArray<nsTArray<uint8_t> > mEncodedBuffers;
  
  PRFileDesc* mFD;
  
  Mutex mMutex;
  
  uint64_t mDataSize;
  
  uint32_t mMaxMemoryStorage;
  
  bool mTempFileEnabled;
};

} 

#endif
