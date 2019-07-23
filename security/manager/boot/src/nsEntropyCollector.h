





































#ifndef nsEntropyCollector_h___
#define nsEntropyCollector_h___

#include "nsIEntropyCollector.h"
#include "nsIBufEntropyCollector.h"
#include "nsCOMPtr.h"

#define NS_ENTROPYCOLLECTOR_CID \
 { /* 34587f4a-be18-43c0-9112-b782b08c0add */       \
  0x34587f4a, 0xbe18, 0x43c0,                       \
 {0x91, 0x12, 0xb7, 0x82, 0xb0, 0x8c, 0x0a, 0xdd} }

class nsEntropyCollector : public nsIBufEntropyCollector
{
  public:
    nsEntropyCollector();
    virtual ~nsEntropyCollector();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIENTROPYCOLLECTOR
    NS_DECL_NSIBUFENTROPYCOLLECTOR

    enum { entropy_buffer_size = 1024 };

  protected:
    unsigned char mEntropyCache[entropy_buffer_size];
    PRInt32 mBytesCollected;
    unsigned char *mWritePointer;
    nsCOMPtr<nsIEntropyCollector> mForwardTarget;
};

#endif
