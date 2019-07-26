




#if !defined(WMFByteStream_h_)
#define WMFByteStream_h_

#include "WMF.h"

#include "nsISupportsImpl.h"
#include "nsCOMPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"

class nsIThreadPool;

namespace mozilla {

class MediaResource;
class ReadRequest;












class WMFByteStream MOZ_FINAL : public IMFByteStream
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

  
  
  
  void ProcessReadRequest(IMFAsyncResult* aResult,
                          ReadRequest* aRequestState);
private:

  
  
  nsresult Read(ReadRequest* aRequestState);

  
  bool IsEOS();

  
  
  nsCOMPtr<nsIThreadPool> mThreadPool;

  
  
  
  
  ReentrantMonitor mResourceMonitor;

  
  
  
  
  nsRefPtr<MediaResource> mResource;

  
  
  ReentrantMonitor mReentrantMonitor;

  
  
  
  
  int64_t mOffset;

  
  
  bool mIsShutdown;

  
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
};

} 

#endif
