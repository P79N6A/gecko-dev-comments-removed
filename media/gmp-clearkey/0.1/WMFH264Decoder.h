















#if !defined(WMFH264Decoder_h_)
#define WMFH264Decoder_h_

#include "WMFUtils.h"

namespace wmf {

class WMFH264Decoder {
public:
  WMFH264Decoder();
  ~WMFH264Decoder();

  HRESULT Init();

  HRESULT Input(const uint8_t* aData,
                uint32_t aDataSize,
                Microseconds aTimestamp,
                Microseconds aDuration);

  HRESULT Output(IMFSample** aOutput);

  HRESULT Reset();

  int32_t GetFrameWidth() const;
  int32_t GetFrameHeight() const;
  const IntRect& GetPictureRegion() const;
  int32_t GetStride() const;

  HRESULT Drain();

private:

  HRESULT SetDecoderInputType();
  HRESULT SetDecoderOutputType();
  HRESULT SendMFTMessage(MFT_MESSAGE_TYPE aMsg, UINT32 aData);

  HRESULT CreateInputSample(const uint8_t* aData,
                            uint32_t aDataSize,
                            Microseconds aTimestamp,
                            Microseconds aDuration,
                            IMFSample** aOutSample);

  HRESULT CreateOutputSample(IMFSample** aOutSample);

  HRESULT GetOutputSample(IMFSample** aOutSample);
  HRESULT ConfigureVideoFrameGeometry(IMFMediaType* aMediaType);

  MFT_INPUT_STREAM_INFO mInputStreamInfo;
  MFT_OUTPUT_STREAM_INFO mOutputStreamInfo;

  CComPtr<IMFTransform> mDecoder;

  int32_t mVideoWidth;
  int32_t mVideoHeight;
  IntRect mPictureRegion;
  int32_t mStride;

};

} 

#endif
