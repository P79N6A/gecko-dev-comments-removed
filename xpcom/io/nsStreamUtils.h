




































#ifndef nsStreamUtils_h__
#define nsStreamUtils_h__

#include "nsStringFwd.h"
#include "nsIInputStream.h"

class nsIOutputStream;
class nsIInputStreamCallback;
class nsIOutputStreamCallback;
class nsIEventTarget;











extern NS_COM nsresult
NS_NewInputStreamReadyEvent(nsIInputStreamCallback **aEvent,
                            nsIInputStreamCallback  *aNotify,
                            nsIEventTarget          *aTarget);











extern NS_COM nsresult
NS_NewOutputStreamReadyEvent(nsIOutputStreamCallback **aEvent,
                             nsIOutputStreamCallback  *aNotify,
                             nsIEventTarget           *aTarget);



enum nsAsyncCopyMode {
    NS_ASYNCCOPY_VIA_READSEGMENTS,
    NS_ASYNCCOPY_VIA_WRITESEGMENTS
};





typedef void (* nsAsyncCopyCallbackFun)(void *closure, nsresult status);











extern NS_COM nsresult
NS_AsyncCopy(nsIInputStream         *aSource,
             nsIOutputStream        *aSink,
             nsIEventTarget         *aTarget,
             nsAsyncCopyMode         aMode = NS_ASYNCCOPY_VIA_READSEGMENTS,
             PRUint32                aChunkSize = 4096,
             nsAsyncCopyCallbackFun  aCallbackFun = nsnull,
             void                   *aCallbackClosure = nsnull);
























extern NS_COM nsresult
NS_ConsumeStream(nsIInputStream *aSource, PRUint32 aMaxCount,
                 nsACString &aBuffer);














extern NS_COM PRBool
NS_InputStreamIsBuffered(nsIInputStream *aInputStream);














extern NS_COM PRBool
NS_OutputStreamIsBuffered(nsIOutputStream *aOutputStream);








extern NS_COM NS_METHOD
NS_CopySegmentToStream(nsIInputStream *aInputStream, void *aClosure,
                       const char *aFromSegment, PRUint32 aToOffset,
                       PRUint32 aCount, PRUint32 *aWriteCount);









extern NS_COM NS_METHOD
NS_CopySegmentToBuffer(nsIInputStream *aInputStream, void *aClosure,
                       const char *aFromSegment, PRUint32 aToOffset,
                       PRUint32 aCount, PRUint32 *aWriteCount);








extern NS_COM NS_METHOD
NS_DiscardSegment(nsIInputStream *aInputStream, void *aClosure,
                  const char *aFromSegment, PRUint32 aToOffset,
                  PRUint32 aCount, PRUint32 *aWriteCount);












extern NS_COM NS_METHOD
NS_WriteSegmentThunk(nsIInputStream *aInputStream, void *aClosure,
                     const char *aFromSegment, PRUint32 aToOffset,
                     PRUint32 aCount, PRUint32 *aWriteCount);

struct nsWriteSegmentThunk {
  nsIInputStream    *mStream;
  nsWriteSegmentFun  mFun;
  void              *mClosure;
};

#endif 
