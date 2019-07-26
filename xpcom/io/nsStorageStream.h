












#ifndef _nsStorageStream_h_
#define _nsStorageStream_h_

#include "nsIStorageStream.h"
#include "nsIOutputStream.h"
#include "nsMemory.h"
#include "mozilla/Attributes.h"

#define NS_STORAGESTREAM_CID                       \
{ /* 669a9795-6ff7-4ed4-9150-c34ce2971b63 */       \
  0x669a9795,                                      \
  0x6ff7,                                          \
  0x4ed4,                                          \
  {0x91, 0x50, 0xc3, 0x4c, 0xe2, 0x97, 0x1b, 0x63} \
}

#define NS_STORAGESTREAM_CONTRACTID "@mozilla.org/storagestream;1"

class nsSegmentedBuffer;

class nsStorageStream MOZ_FINAL : public nsIStorageStream,
                                  public nsIOutputStream
{
public:
    nsStorageStream();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTORAGESTREAM
    NS_DECL_NSIOUTPUTSTREAM

    friend class nsStorageInputStream;

private:
    ~nsStorageStream();

    nsSegmentedBuffer* mSegmentedBuffer;
    uint32_t           mSegmentSize;       
                                           
    uint32_t           mSegmentSizeLog2;   
    bool               mWriteInProgress;   
    int32_t            mLastSegmentNum;    
    char*              mWriteCursor;       
    char*              mSegmentEnd;        
                                           
    uint32_t           mLogicalLength;     

    NS_METHOD Seek(int32_t aPosition);
    uint32_t SegNum(uint32_t aPosition)    {return aPosition >> mSegmentSizeLog2;}
    uint32_t SegOffset(uint32_t aPosition) {return aPosition & (mSegmentSize - 1);}
};

#endif 
