



#include "VorbisTrackEncoder.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>
#include "WebMWriter.h"
#include "GeckoProfiler.h"





static const float BASE_QUALITY = 0.4f;

namespace mozilla {

#undef LOG
#ifdef PR_LOGGING
PRLogModuleInfo* gVorbisTrackEncoderLog;
#define VORBISLOG(msg, ...) PR_LOG(gVorbisTrackEncoderLog, PR_LOG_DEBUG, \
                             (msg, ##__VA_ARGS__))
#else
#define VORBISLOG(msg, ...)
#endif

VorbisTrackEncoder::VorbisTrackEncoder()
  : AudioTrackEncoder()
{
  MOZ_COUNT_CTOR(VorbisTrackEncoder);
#ifdef PR_LOGGING
  if (!gVorbisTrackEncoderLog) {
    gVorbisTrackEncoderLog = PR_NewLogModule("VorbisTrackEncoder");
  }
#endif
}

VorbisTrackEncoder::~VorbisTrackEncoder()
{
  MOZ_COUNT_DTOR(VorbisTrackEncoder);
  if (mInitialized) {
    vorbis_block_clear(&mVorbisBlock);
    vorbis_dsp_clear(&mVorbisDsp);
    vorbis_info_clear(&mVorbisInfo);
  }
}

nsresult
VorbisTrackEncoder::Init(int aChannels, int aSamplingRate)
{
  NS_ENSURE_TRUE(aChannels > 0, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aChannels <= 8, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aSamplingRate >= 8000, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aSamplingRate <= 192000, NS_ERROR_INVALID_ARG);

  
  
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mChannels = aChannels;
  mSamplingRate = aSamplingRate;

  int ret = 0;
  vorbis_info_init(&mVorbisInfo);

  ret = vorbis_encode_init_vbr(&mVorbisInfo, mChannels, mSamplingRate,
                               BASE_QUALITY);

  mInitialized = (ret == 0);

  if (mInitialized) {
    
    vorbis_analysis_init(&mVorbisDsp, &mVorbisInfo);
    vorbis_block_init(&mVorbisDsp, &mVorbisBlock);
  }

  mon.NotifyAll();

  return ret == 0 ? NS_OK : NS_ERROR_FAILURE;
}

void VorbisTrackEncoder::WriteLacing(nsTArray<uint8_t> *aOutput, int32_t aLacing)
{
  while (aLacing >= 255) {
    aLacing -= 255;
    aOutput->AppendElement(255);
  }
  aOutput->AppendElement((uint8_t)aLacing);
}

already_AddRefed<TrackMetadataBase>
VorbisTrackEncoder::GetMetadata()
{
  PROFILER_LABEL("VorbisTrackEncoder", "GetMetadata",
    js::ProfileEntry::Category::OTHER);
  {
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    while (!mCanceled && !mInitialized) {
      mon.Wait();
    }
  }

  if (mCanceled || mEncodingComplete) {
    return nullptr;
  }

  
  
  nsRefPtr<VorbisMetadata> meta = new VorbisMetadata();
  meta->mBitDepth = 32; 
  meta->mChannels = mChannels;
  meta->mSamplingFrequency = mSamplingRate;
  ogg_packet header;
  ogg_packet header_comm;
  ogg_packet header_code;
  
  vorbis_comment vorbisComment;
  vorbis_comment_init(&vorbisComment);
  vorbis_comment_add_tag(&vorbisComment, "ENCODER",
    NS_LITERAL_CSTRING("Mozilla VorbisTrackEncoder " MOZ_APP_UA_VERSION).get());
  vorbis_analysis_headerout(&mVorbisDsp, &vorbisComment,
                            &header,&header_comm, &header_code);
  vorbis_comment_clear(&vorbisComment);
  
  meta->mData.AppendElement(2);
  
  WriteLacing(&(meta->mData), header.bytes);
  WriteLacing(&(meta->mData), header_comm.bytes);

  
  meta->mData.AppendElements(header.packet, header.bytes);
  meta->mData.AppendElements(header_comm.packet, header_comm.bytes);
  meta->mData.AppendElements(header_code.packet, header_code.bytes);

  return meta.forget();
}

void
VorbisTrackEncoder::GetEncodedFrames(EncodedFrameContainer& aData)
{
  
  
  
  while (vorbis_analysis_blockout(&mVorbisDsp, &mVorbisBlock) == 1) {
    ogg_packet oggPacket;
    if (vorbis_analysis(&mVorbisBlock, &oggPacket) == 0) {
      VORBISLOG("vorbis_analysis_blockout block size %d", oggPacket.bytes);
      EncodedFrame* audiodata = new EncodedFrame();
      audiodata->SetFrameType(EncodedFrame::VORBIS_AUDIO_FRAME);
      nsTArray<uint8_t> frameData;
      frameData.AppendElements(oggPacket.packet, oggPacket.bytes);
      audiodata->SwapInFrameData(frameData);
      aData.AppendEncodedFrame(audiodata);
    }
  }
}

nsresult
VorbisTrackEncoder::GetEncodedTrack(EncodedFrameContainer& aData)
{
  if (mEosSetInEncoder) {
    return NS_OK;
  }

  PROFILER_LABEL("VorbisTrackEncoder", "GetEncodedTrack",
    js::ProfileEntry::Category::OTHER);

  nsAutoPtr<AudioSegment> sourceSegment;
  sourceSegment = new AudioSegment();
  {
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    
    while (!mCanceled && mRawSegment.GetDuration() < GetPacketDuration() &&
           !mEndOfStream) {
      mon.Wait();
    }
    VORBISLOG("GetEncodedTrack passes wait, duration is %lld\n",
      mRawSegment.GetDuration());
    if (mCanceled || mEncodingComplete) {
      return NS_ERROR_FAILURE;
    }

    sourceSegment->AppendFrom(&mRawSegment);
  }

  if (mEndOfStream && (sourceSegment->GetDuration() == 0)
      && !mEosSetInEncoder) {
    mEncodingComplete = true;
    mEosSetInEncoder = true;
    VORBISLOG("[Vorbis] Done encoding.");
    vorbis_analysis_wrote(&mVorbisDsp, 0);
    GetEncodedFrames(aData);

    return NS_OK;
  }

  
  AudioSegment::ChunkIterator iter(*sourceSegment);

  AudioDataValue **vorbisBuffer =
    vorbis_analysis_buffer(&mVorbisDsp, (int)sourceSegment->GetDuration());

  int framesCopied = 0;
  nsAutoTArray<AudioDataValue, 9600> interleavedPcm;
  nsAutoTArray<AudioDataValue, 9600> nonInterleavedPcm;
  interleavedPcm.SetLength(sourceSegment->GetDuration() * mChannels);
  nonInterleavedPcm.SetLength(sourceSegment->GetDuration() * mChannels);
  while (!iter.IsEnded()) {
    AudioChunk chunk = *iter;
    int frameToCopy = chunk.GetDuration();
    if (!chunk.IsNull()) {
      InterleaveTrackData(chunk, frameToCopy, mChannels,
                          interleavedPcm.Elements() + framesCopied * mChannels);
    } else { 
      memset(interleavedPcm.Elements() + framesCopied * mChannels, 0,
             frameToCopy * mChannels * sizeof(AudioDataValue));
    }
    framesCopied += frameToCopy;
    iter.Next();
  }
  
  DeInterleaveTrackData(interleavedPcm.Elements(), framesCopied, mChannels,
                        nonInterleavedPcm.Elements());
  
  for(uint8_t i = 0; i < mChannels; ++i) {
    memcpy(vorbisBuffer[i], nonInterleavedPcm.Elements() + framesCopied * i,
           framesCopied * sizeof(AudioDataValue));
  }

  
  
  vorbis_analysis_wrote(&mVorbisDsp, framesCopied);
  VORBISLOG("vorbis_analysis_wrote framesCopied %d\n", framesCopied);
  GetEncodedFrames(aData);

  return NS_OK;
}

} 
