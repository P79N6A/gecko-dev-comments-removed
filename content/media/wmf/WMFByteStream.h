




#if !defined(WMFByteStream_h_)
#define WMFByteStream_h_

#include "WMF.h"

#include "nsISupportsImpl.h"
#include "nsCOMPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsAutoPtr.h"

namespace mozilla {

class MediaResource;






class WMFByteStream : public IMFByteStream,
                      public IMFAsyncCallback
{
public:
  WMFByteStream(MediaResource* aResource);
  ~WMFByteStream();

  nsresult Init();
  nsresult Shutdown();

  
  STDMETHODIMP QueryInterface(REFIID aIId, LPVOID *aInterface);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  
  STDMETHODIMP BeginRead(BYTE *aBuffer,
                         ULONG aLength,
                         IMFAsyncCallback *aCallback,
                         IUnknown *aCallerState);
  STDMETHODIMP BeginWrite(const BYTE *, ULONG ,
                          IMFAsyncCallback *,
                          IUnknown *);
  STDMETHODIMP Close();
  STDMETHODIMP EndRead(IMFAsyncResult* aResult, ULONG *aBytesRead);
  STDMETHODIMP EndWrite(IMFAsyncResult *, ULONG *);
  STDMETHODIMP Flush();
  STDMETHODIMP GetCapabilities(DWORD *aCapabilities);
  STDMETHODIMP GetCurrentPosition(QWORD *aPosition);
  STDMETHODIMP GetLength(QWORD *pqwLength);
  STDMETHODIMP IsEndOfStream(BOOL *aIsEndOfStream);
  STDMETHODIMP Read(BYTE *, ULONG, ULONG *);
  STDMETHODIMP Seek(MFBYTESTREAM_SEEK_ORIGIN aSeekOrigin,
                    LONGLONG aSeekOffset,
                    DWORD aSeekFlags,
                    QWORD *aCurrentPosition);
  STDMETHODIMP SetCurrentPosition(QWORD aPosition);
  STDMETHODIMP SetLength(QWORD);
  STDMETHODIMP Write(const BYTE *, ULONG, ULONG *);

  
  
  STDMETHODIMP GetParameters(DWORD*, DWORD*);
  STDMETHODIMP Invoke(IMFAsyncResult* aResult);

private:

  
  
  
  
  
  DWORD mWorkQueueId;

  
  class AsyncReadRequestState : public IUnknown {
  public:
    AsyncReadRequestState(int64_t aOffset, BYTE* aBuffer, ULONG aLength)
      : mOffset(aOffset),
        mBuffer(aBuffer),
        mBufferLength(aLength),
        mBytesRead(0)
    {}

    
    STDMETHODIMP QueryInterface(REFIID aRIID, LPVOID *aOutObject);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    int64_t mOffset;
    BYTE* mBuffer;
    ULONG mBufferLength;
    ULONG mBytesRead;

    
    nsAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD;
  };

  
  
  MediaResource* mResource;

  
  
  ReentrantMonitor mReentrantMonitor;

  
  
  
  
  int64_t mOffset;

  
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD;
};

} 

#endif
