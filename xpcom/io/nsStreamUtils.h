



#ifndef nsStreamUtils_h__
#define nsStreamUtils_h__

#include "nsStringFwd.h"
#include "nsIInputStream.h"

class nsIOutputStream;
class nsIInputStreamCallback;
class nsIOutputStreamCallback;
class nsIEventTarget;











extern nsresult
NS_NewInputStreamReadyEvent(nsIInputStreamCallback **aEvent,
                            nsIInputStreamCallback  *aNotify,
                            nsIEventTarget          *aTarget);











extern nsresult
NS_NewOutputStreamReadyEvent(nsIOutputStreamCallback **aEvent,
                             nsIOutputStreamCallback  *aNotify,
                             nsIEventTarget           *aTarget);



enum nsAsyncCopyMode {
    NS_ASYNCCOPY_VIA_READSEGMENTS,
    NS_ASYNCCOPY_VIA_WRITESEGMENTS
};





typedef void (* nsAsyncCopyProgressFun)(void *closure, uint32_t count);





typedef void (* nsAsyncCopyCallbackFun)(void *closure, nsresult status);

















extern nsresult
NS_AsyncCopy(nsIInputStream         *aSource,
             nsIOutputStream        *aSink,
             nsIEventTarget         *aTarget,
             nsAsyncCopyMode         aMode = NS_ASYNCCOPY_VIA_READSEGMENTS,
             uint32_t                aChunkSize = 4096,
             nsAsyncCopyCallbackFun  aCallbackFun = nullptr,
             void                   *aCallbackClosure = nullptr,
             bool                    aCloseSource = true,
             bool                    aCloseSink = true,
             nsISupports           **aCopierCtx = nullptr,
             nsAsyncCopyProgressFun  aProgressCallbackFun = nullptr);










extern nsresult
NS_CancelAsyncCopy(nsISupports *aCopierCtx, nsresult aReason);
























extern nsresult
NS_ConsumeStream(nsIInputStream *aSource, uint32_t aMaxCount,
                 nsACString &aBuffer);














extern bool
NS_InputStreamIsBuffered(nsIInputStream *aInputStream);














extern bool
NS_OutputStreamIsBuffered(nsIOutputStream *aOutputStream);








extern NS_METHOD
NS_CopySegmentToStream(nsIInputStream *aInputStream, void *aClosure,
                       const char *aFromSegment, uint32_t aToOffset,
                       uint32_t aCount, uint32_t *aWriteCount);









extern NS_METHOD
NS_CopySegmentToBuffer(nsIInputStream *aInputStream, void *aClosure,
                       const char *aFromSegment, uint32_t aToOffset,
                       uint32_t aCount, uint32_t *aWriteCount);








extern NS_METHOD
NS_CopySegmentToBuffer(nsIOutputStream *aOutputStream, void *aClosure,
                       char *aToSegment, uint32_t aFromOffset,
                       uint32_t aCount, uint32_t *aReadCount);








extern NS_METHOD
NS_DiscardSegment(nsIInputStream *aInputStream, void *aClosure,
                  const char *aFromSegment, uint32_t aToOffset,
                  uint32_t aCount, uint32_t *aWriteCount);












extern NS_METHOD
NS_WriteSegmentThunk(nsIInputStream *aInputStream, void *aClosure,
                     const char *aFromSegment, uint32_t aToOffset,
                     uint32_t aCount, uint32_t *aWriteCount);

struct nsWriteSegmentThunk {
  nsIInputStream    *mStream;
  nsWriteSegmentFun  mFun;
  void              *mClosure;
};

#endif 
