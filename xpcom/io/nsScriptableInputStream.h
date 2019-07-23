




































#ifndef ___nsscriptableinputstream___h_
#define ___nsscriptableinputstream___h_

#include "nsIScriptableInputStream.h"
#include "nsIScriptableStreams.h"
#include "nsIInputStream.h"
#include "nsISeekableStream.h"
#include "nsIMultiplexInputStream.h"
#include "nsIUnicharInputStream.h"
#include "nsCOMPtr.h"

#define NS_SCRIPTABLEINPUTSTREAM_CID        \
{ 0x7225c040, 0xa9bf, 0x11d3, { 0xa1, 0x97, 0x0, 0x50, 0x4, 0x1c, 0xaf, 0x44 } }

#define NS_SCRIPTABLEINPUTSTREAM_CONTRACTID "@mozilla.org/scriptableinputstream;1"
#define NS_SCRIPTABLEINPUTSTREAM_CLASSNAME "Scriptable Input Stream"

class nsScriptableInputStream : public nsIScriptableInputStream,
                                public nsIScriptableIOInputStream,
                                public nsISeekableStream,
                                public nsIMultiplexInputStream
{
public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISCRIPTABLEIOINPUTSTREAM

    
    NS_DECL_NSISEEKABLESTREAM

    
    NS_DECL_NSIMULTIPLEXINPUTSTREAM

    
    NS_IMETHOD Available(PRUint32 *aAvailable);
    NS_IMETHOD Close();
    NS_IMETHOD IsNonBlocking(PRBool *aIsNonBlocking);
    NS_IMETHOD Read(char* aData,
                    PRUint32 aCount,
                    PRUint32 *aReadCount);
    NS_IMETHOD ReadSegments(nsWriteSegmentFun aFn,
                            void* aClosure,
                            PRUint32 aCount,
                            PRUint32 *aReadCount);
    NS_IMETHOD Init(nsIInputStream* aInputStream);
    NS_IMETHOD Read(PRUint32 aCount, char** aData);

    
    nsScriptableInputStream() :
      mUnicharInputStreamHasMore(PR_TRUE) {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    ~nsScriptableInputStream() {}

    nsresult ReadFully(PRUint32 aCount, char* aBuf);

    PRBool mUnicharInputStreamHasMore;
    nsCOMPtr<nsIInputStream> mInputStream;
    nsCOMPtr<nsIUnicharInputStream> mUnicharInputStream;
};

#endif 
