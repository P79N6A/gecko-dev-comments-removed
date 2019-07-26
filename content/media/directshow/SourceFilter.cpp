





#include "SourceFilter.h"
#include "MediaResource.h"
#include "mozilla/RefPtr.h"
#include "DirectShowUtils.h"
#include "MP3FrameParser.h"
#include "prlog.h"
#include <algorithm>

using namespace mozilla::media;

namespace mozilla {




#if defined(PR_LOGGING) && defined (DEBUG_SOURCE_TRACE)
PRLogModuleInfo* GetDirectShowLog();
#define LOG(...) PR_LOG(GetDirectShowLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

static HRESULT
DoGetInterface(IUnknown* aUnknown, void** aInterface)
{
  if (!aInterface)
    return E_POINTER;
  *aInterface = aUnknown;
  aUnknown->AddRef();
  return S_OK;
}


class ReadRequest {
public:

  ReadRequest(IMediaSample* aSample,
              DWORD_PTR aDwUser,
              uint32_t aOffset,
              uint32_t aCount)
    : mSample(aSample),
      mDwUser(aDwUser),
      mOffset(aOffset),
      mCount(aCount)
  {
    MOZ_COUNT_CTOR(ReadRequest);
  }

  ~ReadRequest() {
    MOZ_COUNT_DTOR(ReadRequest);
  }

  RefPtr<IMediaSample> mSample;
  DWORD_PTR mDwUser;
  uint32_t mOffset;
  uint32_t mCount;
};






class MediaResourcePartition {
public:
  MediaResourcePartition(MediaResource* aResource,
                         int64_t aDataStart)
    : mResource(aResource),
      mDataOffset(aDataStart)
  {}

  int64_t GetLength() {
    int64_t len = mResource->GetLength();
    if (len == -1) {
      return len;
    }
    return std::max<int64_t>(0, len - mDataOffset);
  }
  nsresult ReadAt(int64_t aOffset, char* aBuffer,
                  uint32_t aCount, uint32_t* aBytes)
  {
    return mResource->ReadAt(aOffset + mDataOffset,
                             aBuffer,
                             aCount,
                             aBytes);
  }
  int64_t GetCachedDataEnd() {
    int64_t tell = mResource->Tell();
    int64_t dataEnd = mResource->GetCachedDataEnd(tell) - mDataOffset;
    return dataEnd;
  }
private:
  
  RefPtr<MediaResource> mResource;
  int64_t mDataOffset;
};



















class DECLSPEC_UUID("18e5cfb2-1015-440c-a65c-e63853235894")
OutputPin : public IAsyncReader,
            public BasePin
{
public:

  OutputPin(MediaResource* aMediaResource,
            SourceFilter* aParent,
            CriticalSection& aFilterLock,
            int64_t aMP3DataStart);
  virtual ~OutputPin();

  
  
  STDMETHODIMP_(ULONG) AddRef() MOZ_OVERRIDE { return BasePin::AddRef(); }
  STDMETHODIMP_(ULONG) Release() MOZ_OVERRIDE { return BasePin::Release(); }
  STDMETHODIMP QueryInterface(REFIID iid, void** ppv) MOZ_OVERRIDE;

  
  
  HRESULT CheckMediaType(const MediaType* aMediaType) MOZ_OVERRIDE;

  
  HRESULT GetMediaType(int aPosition, MediaType* aMediaType) MOZ_OVERRIDE;

  
  HRESULT BreakConnect(void) MOZ_OVERRIDE;

  
  HRESULT CheckConnect(IPin* aPin) MOZ_OVERRIDE;


  

  
  
  STDMETHODIMP RequestAllocator(IMemAllocator* aPreferred,
                                ALLOCATOR_PROPERTIES* aProps,
                                IMemAllocator** aActual) MOZ_OVERRIDE;

  
  
  STDMETHODIMP Request(IMediaSample* aSample, DWORD_PTR aUserData) MOZ_OVERRIDE;

  
  
  
  STDMETHODIMP WaitForNext(DWORD aTimeout,
                           IMediaSample** aSamples,
                           DWORD_PTR* aUserData) MOZ_OVERRIDE;

  
  
  
  STDMETHODIMP SyncReadAligned(IMediaSample* aSample) MOZ_OVERRIDE;

  
  
  
  STDMETHODIMP SyncRead(LONGLONG aPosition, LONG aLength, BYTE* aBuffer) MOZ_OVERRIDE;

  
  STDMETHODIMP Length(LONGLONG* aTotal, LONGLONG* aAvailable) MOZ_OVERRIDE;

  
  STDMETHODIMP BeginFlush(void) MOZ_OVERRIDE;
  STDMETHODIMP EndFlush(void) MOZ_OVERRIDE;

  uint32_t GetAndResetBytesConsumedCount();

private:

  
  
  CriticalSection& mPinLock;

  
  Signal mSignal;

  
  SourceFilter* mParentSource;

  MediaResourcePartition mResource;

  
  
  int32_t mFlushCount;

  
  
  uint32_t mBytesConsumed;

  
  
  nsDeque mPendingReads;

  
  
  
  bool mQueriedForAsyncReader;

};


#ifdef __CRT_UUID_DECL
}
__CRT_UUID_DECL(mozilla::OutputPin, 0x18e5cfb2,0x1015,0x440c,0xa6,0x5c,0xe6,0x38,0x53,0x23,0x58,0x94);
namespace mozilla {
#endif

OutputPin::OutputPin(MediaResource* aResource,
                     SourceFilter* aParent,
                     CriticalSection& aFilterLock,
                     int64_t aMP3DataStart)
  : BasePin(static_cast<BaseFilter*>(aParent),
            &aFilterLock,
            L"MozillaOutputPin",
            PINDIR_OUTPUT),
    mPinLock(aFilterLock),
    mSignal(&mPinLock),
    mParentSource(aParent),
    mResource(aResource, aMP3DataStart),
    mFlushCount(0),
    mBytesConsumed(0),
    mQueriedForAsyncReader(false)
{
  MOZ_COUNT_CTOR(OutputPin);
  LOG("OutputPin::OutputPin()");
}

OutputPin::~OutputPin()
{
  MOZ_COUNT_DTOR(OutputPin);
  LOG("OutputPin::~OutputPin()");
}

HRESULT
OutputPin::BreakConnect()
{
  mQueriedForAsyncReader = false;
  return BasePin::BreakConnect();
}

STDMETHODIMP
OutputPin::QueryInterface(REFIID aIId, void** aInterface)
{
  if (aIId == IID_IAsyncReader) {
    mQueriedForAsyncReader = true;
    return DoGetInterface(static_cast<IAsyncReader*>(this), aInterface);
  }

  if (aIId == __uuidof(OutputPin)) {
    AddRef();
    *aInterface = this;
    return S_OK;
  }

  return BasePin::QueryInterface(aIId, aInterface);
}

HRESULT
OutputPin::CheckConnect(IPin* aPin)
{
  
  
  return mQueriedForAsyncReader ? S_OK : S_FALSE;
}

HRESULT
OutputPin::CheckMediaType(const MediaType* aMediaType)
{
  const MediaType *myMediaType = mParentSource->GetMediaType();

  if (IsEqualGUID(aMediaType->majortype, myMediaType->majortype) &&
      IsEqualGUID(aMediaType->subtype, myMediaType->subtype) &&
      IsEqualGUID(aMediaType->formattype, myMediaType->formattype))
  {
    LOG("OutputPin::CheckMediaType() Match: major=%s minor=%s TC=%d FSS=%d SS=%u",
        GetDirectShowGuidName(aMediaType->majortype),
        GetDirectShowGuidName(aMediaType->subtype),
        aMediaType->TemporalCompression(),
        aMediaType->bFixedSizeSamples,
        aMediaType->SampleSize());
    return S_OK;
  }

  LOG("OutputPin::CheckMediaType() Failed to match: major=%s minor=%s TC=%d FSS=%d SS=%u",
      GetDirectShowGuidName(aMediaType->majortype),
      GetDirectShowGuidName(aMediaType->subtype),
      aMediaType->TemporalCompression(),
      aMediaType->bFixedSizeSamples,
      aMediaType->SampleSize());
  return S_FALSE;
}

HRESULT
OutputPin::GetMediaType(int aPosition, MediaType* aMediaType)
{
  if (!aMediaType)
    return E_POINTER;

  if (aPosition == 0) {
    aMediaType->Assign(mParentSource->GetMediaType());
    return S_OK;
  }
  return VFW_S_NO_MORE_ITEMS;
}

static inline bool
IsPowerOf2(int32_t x) {
  return ((-x & x) != x);
}

STDMETHODIMP
OutputPin::RequestAllocator(IMemAllocator* aPreferred,
                            ALLOCATOR_PROPERTIES* aProps,
                            IMemAllocator** aActual)
{
  
  if (!aPreferred) return E_POINTER;
  if (!aProps) return E_POINTER;
  if (!aActual) return E_POINTER;

  
  
  ALLOCATOR_PROPERTIES props;
  memcpy(&props, aProps, sizeof(ALLOCATOR_PROPERTIES));
  if (aProps->cbAlign == 0 || IsPowerOf2(aProps->cbAlign)) {
    props.cbAlign = 4;
  }

  
  
  
  
  
  
  if (props.cBuffers > BaseFilter::sMaxNumBuffers) {
    props.cBuffers = BaseFilter::sMaxNumBuffers;
  }

  
  
  ALLOCATOR_PROPERTIES actualProps;
  HRESULT hr;

  if (aPreferred) {
    
    hr = aPreferred->SetProperties(&props, &actualProps);
    if (SUCCEEDED(hr)) {
      aPreferred->AddRef();
      *aActual = aPreferred;
      return S_OK;
    }
  }

  

  
  
  nsRefPtr<IMemAllocator> allocator;
  hr = CoCreateInstance(CLSID_MemoryAllocator,
                        0,
                        CLSCTX_INPROC_SERVER,
                        IID_IMemAllocator,
                        getter_AddRefs(allocator));
  if(FAILED(hr) || (allocator == nullptr)) {
    NS_WARNING("Can't create our own DirectShow allocator.");
    return hr;
  }

  
  hr = allocator->SetProperties(&props, &actualProps);
  if (SUCCEEDED(hr)) {
    
    
    allocator.forget(aActual);
    return S_OK;
  }

  NS_WARNING("Failed to pick an allocator");
  return hr;
}

STDMETHODIMP
OutputPin::Request(IMediaSample* aSample, DWORD_PTR aDwUser)
{
  if (!aSample) return E_FAIL;

  CriticalSectionAutoEnter lock(*mLock);
  NS_ASSERTION(!mFlushCount, "Request() while flushing");

  if (mFlushCount)
    return VFW_E_WRONG_STATE;

  REFERENCE_TIME refStart = 0, refEnd = 0;
  if (FAILED(aSample->GetTime(&refStart, &refEnd))) {
    NS_WARNING("Sample incorrectly timestamped");
    return VFW_E_SAMPLE_TIME_NOT_SET;
  }

  
  uint32_t start = (uint32_t)(refStart / 10000000);
  uint32_t end = (uint32_t)(refEnd / 10000000);

  uint32_t numBytes = end - start;

  ReadRequest* request = new ReadRequest(aSample,
                                         aDwUser,
                                         start,
                                         numBytes);
  
  

  
  mPendingReads.Push(request);

  
  
  mSignal.Notify();

  return S_OK;
}

STDMETHODIMP
OutputPin::WaitForNext(DWORD aTimeout,
                       IMediaSample** aOutSample,
                       DWORD_PTR* aOutDwUser)
{
  NS_ASSERTION(aTimeout == 0 || aTimeout == INFINITE,
               "Oops, we don't handle this!");

  *aOutSample = nullptr;
  *aOutDwUser = 0;

  LONGLONG offset = 0;
  LONG count = 0;
  BYTE* buf = nullptr;

  {
    CriticalSectionAutoEnter lock(*mLock);

    
    while (aTimeout && mPendingReads.GetSize() == 0 && !mFlushCount) {
      
      
      
      
      mSignal.Wait();
    }

    nsAutoPtr<ReadRequest> request(reinterpret_cast<ReadRequest*>(mPendingReads.PopFront()));
    if (!request)
      return VFW_E_WRONG_STATE;

    *aOutSample = request->mSample;
    *aOutDwUser = request->mDwUser;

    offset = request->mOffset;
    count = request->mCount;
    buf = nullptr;
    request->mSample->GetPointer(&buf);
    NS_ASSERTION(buf != nullptr, "Invalid buffer!");

    if (mFlushCount) {
      return VFW_E_TIMEOUT;
    }
  }

  return SyncRead(offset, count, buf);
}

STDMETHODIMP
OutputPin::SyncReadAligned(IMediaSample* aSample)
{
  {
    
    CriticalSectionAutoEnter lock(*mLock);
    if (mFlushCount) {
      return S_FALSE;
    }
  }

  if (!aSample)
    return E_FAIL;

  REFERENCE_TIME lStart = 0, lEnd = 0;
  if (FAILED(aSample->GetTime(&lStart, &lEnd))) {
    NS_WARNING("Sample incorrectly timestamped");
    return VFW_E_SAMPLE_TIME_NOT_SET;
  }

  
  int32_t start = (int32_t)(lStart / 10000000);
  int32_t end = (int32_t)(lEnd / 10000000);

  int32_t numBytes = end - start;

  
  
  int64_t streamLength = mResource.GetLength();
  if (streamLength != -1) {
    
    
    if (start > streamLength) {
      return VFW_E_BADALIGN;
    }

    
    
    if ((start + numBytes) > streamLength) {
      numBytes = (uint32_t)(streamLength - start);
    }
  }

  BYTE* buf=0;
  aSample->GetPointer(&buf);

  return SyncRead(start, numBytes, buf);
}

STDMETHODIMP
OutputPin::SyncRead(LONGLONG aPosition,
                    LONG aLength,
                    BYTE* aBuffer)
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(aPosition >= 0, E_FAIL);
  NS_ENSURE_TRUE(aLength > 0, E_FAIL);
  NS_ENSURE_TRUE(aBuffer, E_POINTER);

  LOG("OutputPin::SyncRead(%lld, %d)", aPosition, aLength);
  {
    
    CriticalSectionAutoEnter lock(*mLock);
    if (mFlushCount) {
      return S_FALSE;
    }
  }

  
  LONG totalBytesRead = 0;
  while (totalBytesRead < aLength) {
    BYTE* readBuffer = aBuffer + totalBytesRead;
    uint32_t bytesRead = 0;
    LONG length = aLength - totalBytesRead;
    nsresult rv = mResource.ReadAt(aPosition + totalBytesRead,
                                   reinterpret_cast<char*>(readBuffer),
                                   length,
                                   &bytesRead);
    if (NS_FAILED(rv)) {
      return E_FAIL;
    }
    totalBytesRead += bytesRead;
    if (bytesRead == 0) {
      break;
    }
  }
  if (totalBytesRead > 0) {
    CriticalSectionAutoEnter lock(*mLock);
    mBytesConsumed += totalBytesRead;
  }
  return (totalBytesRead == aLength) ? S_OK : S_FALSE;
}

STDMETHODIMP
OutputPin::Length(LONGLONG* aTotal, LONGLONG* aAvailable)
{
  HRESULT hr = S_OK;
  int64_t length = mResource.GetLength();
  if (length == -1) {
    hr = VFW_S_ESTIMATED;
    
    *aTotal = INT32_MAX;
  } else {
    *aTotal = length;
  }
  if (aAvailable) {
    *aAvailable = mResource.GetCachedDataEnd();
  }

  LOG("OutputPin::Length() len=%lld avail=%lld", *aTotal, *aAvailable);

  return hr;
}

STDMETHODIMP
OutputPin::BeginFlush()
{
  CriticalSectionAutoEnter lock(*mLock);
  mFlushCount++;
  mSignal.Notify();
  return S_OK;
}

STDMETHODIMP
OutputPin::EndFlush(void)
{
  CriticalSectionAutoEnter lock(*mLock);
  mFlushCount--;
  return S_OK;
}

uint32_t
OutputPin::GetAndResetBytesConsumedCount()
{
  CriticalSectionAutoEnter lock(*mLock);
  uint32_t bytesConsumed = mBytesConsumed;
  mBytesConsumed = 0;
  return bytesConsumed;
}

SourceFilter::SourceFilter(const GUID& aMajorType,
                                               const GUID& aSubType)
  : BaseFilter(L"MozillaDirectShowSource", __uuidof(SourceFilter))
{
  MOZ_COUNT_CTOR(SourceFilter);
  mMediaType.majortype = aMajorType;
  mMediaType.subtype = aSubType;

  LOG("SourceFilter Constructor(%s, %s)",
      GetDirectShowGuidName(aMajorType),
      GetDirectShowGuidName(aSubType));
}

SourceFilter::~SourceFilter()
{
  MOZ_COUNT_DTOR(SourceFilter);
  LOG("SourceFilter Destructor()");
}

BasePin*
SourceFilter::GetPin(int n)
{
  if (n == 0) {
    NS_ASSERTION(mOutputPin != 0, "GetPin with no pin!");
    return static_cast<BasePin*>(mOutputPin);
  } else {
    return nullptr;
  }
}


const MediaType*
SourceFilter::GetMediaType() const
{
  return &mMediaType;
}

static uint32_t
Read(MediaResource* aResource,
     const int64_t aOffset,
     char* aBuffer,
     const uint32_t aBytesToRead)
{
  uint32_t totalBytesRead = 0;
  while (totalBytesRead < aBytesToRead) {
    uint32_t bytesRead = 0;
    nsresult rv = aResource->ReadAt(aOffset + totalBytesRead,
                                    aBuffer+totalBytesRead,
                                    aBytesToRead-totalBytesRead,
                                    &bytesRead);
    if (NS_FAILED(rv) || bytesRead == 0) {
      
      break;
    }
    totalBytesRead += bytesRead;
  }
  return totalBytesRead;
}




static nsresult
GetMP3DataOffset(MediaResource* aResource, int64_t* aOutOffset)
{
  MP3FrameParser parser;
  int64_t offset = 0;
  const uint32_t len = 1024;
  char buffer[len];
  do {
    uint32_t bytesRead = Read(aResource, offset, buffer, len);
    if (bytesRead == 0) {
      break;
    }
    parser.Parse(buffer, bytesRead, offset);
    offset += bytesRead;
  } while (parser.GetMP3Offset() == -1 && parser.IsMP3());

  if (!parser.IsMP3() || parser.GetMP3Offset() == -1) {
    return NS_ERROR_FAILURE;
  }

  *aOutOffset = parser.GetMP3Offset();
  return NS_OK;
}

nsresult
SourceFilter::Init(MediaResource* aResource)
{
  LOG("SourceFilter::Init()");

  
  
  
  
  int64_t mp3DataOffset = 0;
  nsresult rv = GetMP3DataOffset(aResource, &mp3DataOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG("First MP3 sync/data frame lies at offset %lld", mp3DataOffset);

  mOutputPin = new OutputPin(aResource,
                             this,
                             mLock,
                             mp3DataOffset);
  NS_ENSURE_TRUE(mOutputPin != nullptr, NS_ERROR_FAILURE);

  return NS_OK;
}

uint32_t
SourceFilter::GetAndResetBytesConsumedCount()
{
  return mOutputPin->GetAndResetBytesConsumedCount();
}


} 
