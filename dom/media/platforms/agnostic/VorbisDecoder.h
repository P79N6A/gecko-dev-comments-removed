




#if !defined(VorbisDecoder_h_)
#define VorbisDecoder_h_

#include "PlatformDecoderModule.h"

#ifdef MOZ_TREMOR
#include "tremor/ivorbiscodec.h"
#else
#include "vorbis/codec.h"
#endif

namespace mozilla {

class VorbisDataDecoder : public MediaDataDecoder
{
public:
  VorbisDataDecoder(const AudioInfo& aConfig,
                FlushableTaskQueue* aTaskQueue,
                MediaDataDecoderCallback* aCallback);
  ~VorbisDataDecoder();

  nsresult Init() override;
  nsresult Input(MediaRawData* aSample) override;
  nsresult Flush() override;
  nsresult Drain() override;
  nsresult Shutdown() override;

  
  static bool IsVorbis(const nsACString& aMimeType);

private:
  nsresult DecodeHeader(const unsigned char* aData, size_t aLength);

  void Decode (MediaRawData* aSample);
  int DoDecode (MediaRawData* aSample);
  void DoDrain ();

  const AudioInfo& mInfo;
  RefPtr<FlushableTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  
  vorbis_info mVorbisInfo;
  vorbis_comment mVorbisComment;
  vorbis_dsp_state mVorbisDsp;
  vorbis_block mVorbisBlock;

  int64_t mPacketCount;
  int64_t mFrames;
};

} 
#endif
