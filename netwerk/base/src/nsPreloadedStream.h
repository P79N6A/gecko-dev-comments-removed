



















































#ifndef nsPreloadedStream_h__
#define nsPreloadedStream_h__

#include "nsIAsyncInputStream.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace net {

class nsPreloadedStream : public nsIAsyncInputStream
{
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIASYNCINPUTSTREAM

    nsPreloadedStream(nsIAsyncInputStream *aStream, 
                      const char *data, PRUint32 datalen);
private:
    ~nsPreloadedStream();

    nsCOMPtr<nsIAsyncInputStream> mStream;

    char *mBuf;
    PRUint32 mOffset;
    PRUint32 mLen;
};
        
} 
} 

#endif
