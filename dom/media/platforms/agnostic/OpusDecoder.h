




#if !defined(OpusDecoder_h_)
#define OpusDecoder_h_

#include "OpusParser.h"
#include "PlatformDecoderModule.h"

#include "nsAutoPtr.h"

namespace mozilla {

class OpusDataDecoder : public MediaDataDecoder
{
public:
  OpusDataDecoder(const AudioInfo& aConfig,
                  FlushableTaskQueue* aTaskQueue,
                  MediaDataDecoderCallback* aCallback);
  ~OpusDataDecoder();

  nsresult Init() override;
  nsresult Input(MediaRawData* aSample) override;
  nsresult Flush() override;
  nsresult Drain() override;
  nsresult Shutdown() override;

  
  static bool IsOpus(const nsACString& aMimeType);

private:
  nsresult DecodeHeader(const unsigned char* aData, size_t aLength);

  void Decode (MediaRawData* aSample);
  int DoDecode (MediaRawData* aSample);
  void DoDrain ();

  const AudioInfo& mInfo;
  RefPtr<FlushableTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  
  nsAutoPtr<OpusParser> mOpusParser;
  OpusMSDecoder* mOpusDecoder;

  uint16_t mSkip;        
  bool mDecodedHeader;

  
  
  
  bool mPaddingDiscarded;
  int64_t mFrames;
};

} 
#endif
