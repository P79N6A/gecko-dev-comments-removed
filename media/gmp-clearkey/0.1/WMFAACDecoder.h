















#ifdef TEST_DECODING

#if !defined(WMFAACDecoder_h_)
#define WMFAACDecoder_h_

class WMFAACDecoder {
public:
  WMFAACDecoder();
  ~WMFAACDecoder();

  HRESULT Init(int32_t aChannelCount,
               int32_t aSampleRate,
               BYTE* aUserData,
               UINT32 aUserDataLength);

  HRESULT Input(const uint8_t* aData,
                uint32_t aDataSize,
                Microseconds aTimestamp);

  HRESULT Output(IMFSample** aOutput);

  HRESULT Reset();

  HRESULT Drain();

  UINT32 Channels() const { return mChannels; }
  UINT32 Rate() const { return mRate; }

private:

  HRESULT GetOutputSample(IMFSample** aOutSample);
  HRESULT CreateOutputSample(IMFSample** aOutSample);
  HRESULT CreateInputSample(const uint8_t* aData,
                            uint32_t aDataSize,
                            Microseconds aTimestamp,
                            IMFSample** aOutSample);

  HRESULT SetDecoderInputType(int32_t aChannelCount,
                              int32_t aSampleRate,
                              BYTE* aUserData,
                              UINT32 aUserDataLength);
  HRESULT SetDecoderOutputType();
  HRESULT SendMFTMessage(MFT_MESSAGE_TYPE aMsg, UINT32 aData);

  MFT_INPUT_STREAM_INFO mInputStreamInfo;
  MFT_OUTPUT_STREAM_INFO mOutputStreamInfo;

  CComPtr<IMFTransform> mDecoder;

  UINT32 mChannels;
  UINT32 mRate;
};

#endif

#endif
