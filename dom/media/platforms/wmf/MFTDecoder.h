





#if !defined(MFTDecoder_h_)
#define MFTDecoder_h_

#include "WMF.h"
#include "mozilla/RefPtr.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsIThread.h"

namespace mozilla {

class MFTDecoder final {
  ~MFTDecoder();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MFTDecoder)

  MFTDecoder();

  
  
  
  
  
  HRESULT Create(const GUID& aMFTClsID);

  
  
  
  
  
  
  
  typedef HRESULT (*ConfigureOutputCallback)(IMFMediaType* aOutputType, void* aData);
  HRESULT SetMediaTypes(IMFMediaType* aInputType,
                        IMFMediaType* aOutputType,
                        ConfigureOutputCallback aCallback = nullptr,
                        void* aData = nullptr);

  
  TemporaryRef<IMFAttributes> GetAttributes();

  
  
  HRESULT GetOutputMediaType(RefPtr<IMFMediaType>& aMediaType);

  
  
  
  
  
  HRESULT Input(const uint8_t* aData,
                uint32_t aDataSize,
                int64_t aTimestampUsecs);
  HRESULT Input(IMFSample* aSample);

  
  
  
  
  
  
  
  
  
  
  
  
  HRESULT Output(RefPtr<IMFSample>* aOutput);

  
  
  HRESULT Flush();

  
  HRESULT SendMFTMessage(MFT_MESSAGE_TYPE aMsg, ULONG_PTR aData);

private:

  HRESULT SetDecoderOutputType(ConfigureOutputCallback aCallback, void* aData);

  HRESULT CreateInputSample(const uint8_t* aData,
                            uint32_t aDataSize,
                            int64_t aTimestampUsecs,
                            RefPtr<IMFSample>* aOutSample);

  HRESULT CreateOutputSample(RefPtr<IMFSample>* aOutSample);

  MFT_INPUT_STREAM_INFO mInputStreamInfo;
  MFT_OUTPUT_STREAM_INFO mOutputStreamInfo;

  RefPtr<IMFTransform> mDecoder;

  RefPtr<IMFMediaType> mOutputType;

  
  bool mMFTProvidesOutputSamples;

  
  bool mDiscontinuity;
};

} 

#endif
