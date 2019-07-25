













































#ifndef _nsStorageStream_h_
#define _nsStorageStream_h_

#include "nsIStorageStream.h"
#include "nsIOutputStream.h"
#include "nsMemory.h"

#define NS_STORAGESTREAM_CID                       \
{ /* 669a9795-6ff7-4ed4-9150-c34ce2971b63 */       \
  0x669a9795,                                      \
  0x6ff7,                                          \
  0x4ed4,                                          \
  {0x91, 0x50, 0xc3, 0x4c, 0xe2, 0x97, 0x1b, 0x63} \
}

#define NS_STORAGESTREAM_CONTRACTID "@mozilla.org/storagestream;1"
#define NS_STORAGESTREAM_CLASSNAME "Storage Stream"

class nsSegmentedBuffer;

class nsStorageStream : public nsIStorageStream,
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
    PRUint32           mSegmentSize;       
                                           
    PRUint32           mSegmentSizeLog2;   
    PRBool             mWriteInProgress;   
    PRInt32            mLastSegmentNum;    
    char*              mWriteCursor;       
    char*              mSegmentEnd;        
                                           
    PRUint32           mLogicalLength;     

    NS_METHOD Seek(PRInt32 aPosition);
    PRUint32 SegNum(PRUint32 aPosition)    {return aPosition >> mSegmentSizeLog2;}
    PRUint32 SegOffset(PRUint32 aPosition) {return aPosition & (mSegmentSize - 1);}
};

#endif 
