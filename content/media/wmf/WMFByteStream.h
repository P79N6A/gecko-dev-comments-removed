




#if !defined(WMFByteStream_h_)
#define WMFByteStream_h_

#include "WMF.h"

#include "nsISupportsImpl.h"
#include "nsCOMPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"

class nsIThreadPool;

namespace mozilla {

class MediaResource;
class ReadRequest;












class WMFByteStream MOZ_FINAL : public IMFByteStream
                              , public IMFAttributes
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

  
  STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT* pValue);
  STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType);
  STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult);
  STDMETHODIMP Compare(IMFAttributes* pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL* pbResult);
  STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32* punValue);
  STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64* punValue);
  STDMETHODIMP GetDouble(REFGUID guidKey, double* pfValue);
  STDMETHODIMP GetGUID(REFGUID guidKey, GUID* pguidValue);
  STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32* pcchLength);
  STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength);
  STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength);
  STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32* pcbBlobSize);
  STDMETHODIMP GetBlob(REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize);
  STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize);
  STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID* ppv);
  STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value);
  STDMETHODIMP DeleteItem(REFGUID guidKey);
  STDMETHODIMP DeleteAllItems();
  STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue);
  STDMETHODIMP SetUINT64(REFGUID guidKey,UINT64 unValue);
  STDMETHODIMP SetDouble(REFGUID guidKey, double fValue);
  STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue);
  STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue);
  STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize);
  STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown* pUnknown);
  STDMETHODIMP LockStore();
  STDMETHODIMP UnlockStore();
  STDMETHODIMP GetCount(UINT32* pcItems);
  STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue);
  STDMETHODIMP CopyAllItems(IMFAttributes* pDest);

  
  
  
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

  
  
  RefPtr<IMFAttributes> mAttributes;

  
  
  bool mIsShutdown;

  
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
};

} 

#endif
