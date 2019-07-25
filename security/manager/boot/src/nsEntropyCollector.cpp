




#include "prlog.h"
#include "nsEntropyCollector.h"
#include "nsMemory.h"
#include "nsAlgorithm.h"

nsEntropyCollector::nsEntropyCollector()
:mBytesCollected(0), mWritePointer(mEntropyCache)
{
  
  
  
  
  
  memset(mEntropyCache, 0, sizeof(mEntropyCache));
}

nsEntropyCollector::~nsEntropyCollector()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsEntropyCollector,
                              nsIEntropyCollector,
                              nsIBufEntropyCollector)

NS_IMETHODIMP
nsEntropyCollector::RandomUpdate(void *new_entropy, int32_t bufLen)
{
  if (bufLen > 0) {
    if (mForwardTarget) {
      return mForwardTarget->RandomUpdate(new_entropy, bufLen);
    }
    else {
      const unsigned char *InputPointer = (const unsigned char *)new_entropy;
      const unsigned char *PastEndPointer = mEntropyCache + entropy_buffer_size;

      
      int32_t bytes_wanted = NS_MIN(bufLen, int32_t(entropy_buffer_size));

      
      mBytesCollected = NS_MIN(int32_t(entropy_buffer_size),
                               mBytesCollected + bytes_wanted);

      
      
      while (bytes_wanted > 0) {

        
        const int32_t space_to_end = PastEndPointer - mWritePointer;

        
        const int32_t this_time = NS_MIN(space_to_end, bytes_wanted);

        
        for (int32_t i = 0; i < this_time; ++i) {

          unsigned int old = *mWritePointer;

          
          
          *mWritePointer++ = ((old << 1) | (old >> 7)) ^ *InputPointer++;
        }

        PR_ASSERT(mWritePointer <= PastEndPointer);
        PR_ASSERT(mWritePointer >= mEntropyCache);

        
        if (PastEndPointer == mWritePointer) {
          
          mWritePointer = mEntropyCache;
        }

        
        bytes_wanted -= this_time;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEntropyCollector::ForwardTo(nsIEntropyCollector *aCollector)
{
  NS_PRECONDITION(!mForwardTarget, "|ForwardTo| should only be called once.");

  mForwardTarget = aCollector;
  mForwardTarget->RandomUpdate(mEntropyCache, mBytesCollected);
  mBytesCollected = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsEntropyCollector::DontForward()
{
  mForwardTarget = nullptr;
  return NS_OK;
}
