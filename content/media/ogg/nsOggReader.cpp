




#include "nsError.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsBuiltinDecoder.h"
#include "nsOggReader.h"
#include "VideoUtils.h"
#include "theora/theoradec.h"
#ifdef MOZ_OPUS
#include "opus/opus.h"
#endif
#include "nsTimeRanges.h"
#include "mozilla/TimeStamp.h"

using namespace mozilla;




#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#ifdef SEEK_LOGGING
#define SEEK_LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define SEEK_LOG(type, msg)
#endif
#else
#define LOG(type, msg)
#define SEEK_LOG(type, msg)
#endif







static const PRUint32 SEEK_FUZZ_USECS = 500000;



static const PRInt64 SEEK_OPUS_PREROLL = 80 * USECS_PER_MS;

enum PageSyncResult {
  PAGE_SYNC_ERROR = 1,
  PAGE_SYNC_END_OF_RANGE= 2,
  PAGE_SYNC_OK = 3
};


static PageSyncResult
PageSync(MediaResource* aResource,
         ogg_sync_state* aState,
         bool aCachedDataOnly,
         PRInt64 aOffset,
         PRInt64 aEndOffset,
         ogg_page* aPage,
         int& aSkippedBytes);



static const int PAGE_STEP = 8192;

nsOggReader::nsOggReader(nsBuiltinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder),
    mTheoraState(nsnull),
    mVorbisState(nsnull),
    mOpusState(nsnull),
    mOpusEnabled(nsHTMLMediaElement::IsOpusEnabled()),
    mSkeletonState(nsnull),
    mVorbisSerial(0),
    mOpusSerial(0),
    mTheoraSerial(0),
    mOpusPreSkip(0),
    mPageOffset(0)
{
  MOZ_COUNT_CTOR(nsOggReader);
  memset(&mTheoraInfo, 0, sizeof(mTheoraInfo));
}

nsOggReader::~nsOggReader()
{
  ogg_sync_clear(&mOggState);
  MOZ_COUNT_DTOR(nsOggReader);
}

nsresult nsOggReader::Init(nsBuiltinDecoderReader* aCloneDonor) {
  mCodecStates.Init();
  int ret = ogg_sync_init(&mOggState);
  NS_ENSURE_TRUE(ret == 0, NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult nsOggReader::ResetDecode()
{
  return ResetDecode(false);
}

nsresult nsOggReader::ResetDecode(bool start)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  nsresult res = NS_OK;

  if (NS_FAILED(nsBuiltinDecoderReader::ResetDecode())) {
    res = NS_ERROR_FAILURE;
  }

  
  ogg_sync_reset(&mOggState);
  if (mVorbisState && NS_FAILED(mVorbisState->Reset())) {
    res = NS_ERROR_FAILURE;
  }
  if (mOpusState && NS_FAILED(mOpusState->Reset(start))) {
    res = NS_ERROR_FAILURE;
  }
  if (mTheoraState && NS_FAILED(mTheoraState->Reset())) {
    res = NS_ERROR_FAILURE;
  }

  return res;
}

bool nsOggReader::ReadHeaders(nsOggCodecState* aState)
{
  while (!aState->DoneReadingHeaders()) {
    ogg_packet* packet = NextOggPacket(aState);
    
    if (!packet || !aState->DecodeHeader(packet)) {
      aState->Deactivate();
      return false;
    }
  }
  return aState->Init();
}

void nsOggReader::BuildSerialList(nsTArray<PRUint32>& aTracks)
{
  if (HasVideo()) {
    aTracks.AppendElement(mTheoraState->mSerial);
  }
  if (HasAudio()) {
    if (mVorbisState) {
      aTracks.AppendElement(mVorbisState->mSerial);
    } else if(mOpusState) {
      aTracks.AppendElement(mOpusState->mSerial);
    }
  }
}

nsresult nsOggReader::ReadMetadata(nsVideoInfo* aInfo)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  
  

  ogg_page page;
  nsAutoTArray<nsOggCodecState*,4> bitstreams;
  bool readAllBOS = false;
  while (!readAllBOS) {
    PRInt64 pageOffset = ReadOggPage(&page);
    if (pageOffset == -1) {
      
      break;
    }

    int serial = ogg_page_serialno(&page);
    nsOggCodecState* codecState = 0;

    if (!ogg_page_bos(&page)) {
      
      
      
      readAllBOS = true;
    } else if (!mCodecStates.Get(serial, nsnull)) {
      
      
      
      codecState = nsOggCodecState::Create(&page);
      mCodecStates.Put(serial, codecState);
      bitstreams.AppendElement(codecState);
      mKnownStreams.AppendElement(serial);
      if (codecState &&
          codecState->GetType() == nsOggCodecState::TYPE_VORBIS &&
          !mVorbisState)
      {
        
        
        mVorbisState = static_cast<nsVorbisState*>(codecState);
      }
      if (codecState &&
          codecState->GetType() == nsOggCodecState::TYPE_THEORA &&
          !mTheoraState)
      {
        
        
        mTheoraState = static_cast<nsTheoraState*>(codecState);
      }
      if (codecState &&
          codecState->GetType() == nsOggCodecState::TYPE_OPUS &&
          !mOpusState)
      {
        if (mOpusEnabled) {
          mOpusState = static_cast<nsOpusState*>(codecState);
        } else {
          NS_WARNING("Opus decoding disabled."
                     " See media.opus.enabled in about:config");
        }
      }
      if (codecState &&
          codecState->GetType() == nsOggCodecState::TYPE_SKELETON &&
          !mSkeletonState)
      {
        mSkeletonState = static_cast<nsSkeletonState*>(codecState);
      }
    }

    mCodecStates.Get(serial, &codecState);
    NS_ENSURE_TRUE(codecState, NS_ERROR_FAILURE);

    if (NS_FAILED(codecState->PageIn(&page))) {
      return NS_ERROR_FAILURE;
    }
  }

  
  
  

  
  for (PRUint32 i = 0; i < bitstreams.Length(); i++) {
    nsOggCodecState* s = bitstreams[i];
    if (s != mVorbisState && s != mOpusState &&
        s != mTheoraState && s != mSkeletonState) {
      s->Deactivate();
    }
  }

  if (mTheoraState && ReadHeaders(mTheoraState)) {
    nsIntRect picture = nsIntRect(mTheoraState->mInfo.pic_x,
                                  mTheoraState->mInfo.pic_y,
                                  mTheoraState->mInfo.pic_width,
                                  mTheoraState->mInfo.pic_height);

    nsIntSize displaySize = nsIntSize(mTheoraState->mInfo.pic_width,
                                      mTheoraState->mInfo.pic_height);

    
    
    ScaleDisplayByAspectRatio(displaySize, mTheoraState->mPixelAspectRatio);

    nsIntSize frameSize(mTheoraState->mInfo.frame_width,
                        mTheoraState->mInfo.frame_height);
    if (nsVideoInfo::ValidateVideoRegion(frameSize, picture, displaySize)) {
      
      mInfo.mHasVideo = true;
      mInfo.mDisplay = displaySize;
      mPicture = picture;

      VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
      if (container) {
        container->SetCurrentFrame(gfxIntSize(displaySize.width, displaySize.height),
                                   nsnull,
                                   TimeStamp::Now());
      }

      
      memcpy(&mTheoraInfo, &mTheoraState->mInfo, sizeof(mTheoraInfo));
      mTheoraSerial = mTheoraState->mSerial;
    }
  }

  if (mVorbisState && ReadHeaders(mVorbisState)) {
    mInfo.mHasAudio = true;
    mInfo.mAudioRate = mVorbisState->mInfo.rate;
    mInfo.mAudioChannels = mVorbisState->mInfo.channels;
    
    memcpy(&mVorbisInfo, &mVorbisState->mInfo, sizeof(mVorbisInfo));
    mVorbisInfo.codec_setup = NULL;
    mVorbisSerial = mVorbisState->mSerial;
  } else {
    memset(&mVorbisInfo, 0, sizeof(mVorbisInfo));
  }
#ifdef MOZ_OPUS
  if (mOpusState && ReadHeaders(mOpusState)) {
    mInfo.mHasAudio = true;
    mInfo.mAudioRate = mOpusState->mRate;
    mInfo.mAudioChannels = mOpusState->mChannels;
    mOpusSerial = mOpusState->mSerial;
    mOpusPreSkip = mOpusState->mPreSkip;
  }
#endif
  if (mSkeletonState) {
    if (!HasAudio() && !HasVideo()) {
      
      
      mSkeletonState->Deactivate();
    } else if (ReadHeaders(mSkeletonState) && mSkeletonState->HasIndex()) {
      
      
      nsAutoTArray<PRUint32, 2> tracks;
      BuildSerialList(tracks);
      PRInt64 duration = 0;
      if (NS_SUCCEEDED(mSkeletonState->GetDuration(tracks, duration))) {
        ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
        mDecoder->GetStateMachine()->SetDuration(duration);
        LOG(PR_LOG_DEBUG, ("Got duration from Skeleton index %lld", duration));
      }
    }
  }

  if (HasAudio() || HasVideo()) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

    MediaResource* resource = mDecoder->GetResource();
    if (mDecoder->GetStateMachine()->GetDuration() == -1 &&
        mDecoder->GetStateMachine()->GetState() != nsDecoderStateMachine::DECODER_STATE_SHUTDOWN &&
        resource->GetLength() >= 0 &&
        mDecoder->GetStateMachine()->IsSeekable())
    {
      
      
      PRInt64 length = resource->GetLength();
      NS_ASSERTION(length > 0, "Must have a content length to get end time");

      PRInt64 endTime = 0;
      {
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        endTime = RangeEndTime(length);
      }
      if (endTime != -1) {
        mDecoder->GetStateMachine()->SetEndTime(endTime);
        LOG(PR_LOG_DEBUG, ("Got Ogg duration from seeking to end %lld", endTime));
      }
    }
  } else {
    return NS_ERROR_FAILURE;
  }
  *aInfo = mInfo;

  return NS_OK;
}

nsresult nsOggReader::DecodeVorbis(ogg_packet* aPacket) {
  NS_ASSERTION(aPacket->granulepos != -1, "Must know vorbis granulepos!");

  if (vorbis_synthesis(&mVorbisState->mBlock, aPacket) != 0) {
    return NS_ERROR_FAILURE;
  }
  if (vorbis_synthesis_blockin(&mVorbisState->mDsp,
                               &mVorbisState->mBlock) != 0)
  {
    return NS_ERROR_FAILURE;
  }

  VorbisPCMValue** pcm = 0;
  PRInt32 frames = 0;
  PRUint32 channels = mVorbisState->mInfo.channels;
  ogg_int64_t endFrame = aPacket->granulepos;
  while ((frames = vorbis_synthesis_pcmout(&mVorbisState->mDsp, &pcm)) > 0) {
    mVorbisState->ValidateVorbisPacketSamples(aPacket, frames);
    nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[frames * channels]);
    for (PRUint32 j = 0; j < channels; ++j) {
      VorbisPCMValue* channel = pcm[j];
      for (PRUint32 i = 0; i < PRUint32(frames); ++i) {
        buffer[i*channels + j] = MOZ_CONVERT_VORBIS_SAMPLE(channel[i]);
      }
    }

    PRInt64 duration = mVorbisState->Time((PRInt64)frames);
    PRInt64 startTime = mVorbisState->Time(endFrame - frames);
    mAudioQueue.Push(new AudioData(mPageOffset,
                                   startTime,
                                   duration,
                                   frames,
                                   buffer.forget(),
                                   channels));
    endFrame -= frames;
    if (vorbis_synthesis_read(&mVorbisState->mDsp, frames) != 0) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}
#ifdef MOZ_OPUS
nsresult nsOggReader::DecodeOpus(ogg_packet* aPacket) {
  NS_ASSERTION(aPacket->granulepos != -1, "Must know opus granulepos!");

  
  PRInt32 frames = opus_decoder_get_nb_samples(mOpusState->mDecoder,
                                               aPacket->packet,
                                               aPacket->bytes);
  if (frames <= 0)
    return NS_ERROR_FAILURE;
  PRUint32 channels = mOpusState->mChannels;
  nsAutoArrayPtr<AudioDataValue> buffer(new AudioDataValue[frames * channels]);

  
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  int ret = opus_decode_float(mOpusState->mDecoder,
                              aPacket->packet, aPacket->bytes,
                              buffer, frames, false);
#else
  int ret = opus_decode(mOpusState->mDecoder,
                        aPacket->packet, aPacket->bytes,
                        buffer, frames, false);
#endif
  if (ret < 0)
    return NS_ERROR_FAILURE;
  NS_ASSERTION(ret == frames, "Opus decoded too few audio samples");

  PRInt64 endFrame = aPacket->granulepos;
  PRInt64 startFrame;
  
  if (aPacket->e_o_s && mOpusState->mPrevPacketGranulepos != -1) {
    startFrame = mOpusState->mPrevPacketGranulepos;
    frames = static_cast<PRInt32>(NS_MAX(static_cast<PRInt64>(0),
                                         NS_MIN(endFrame - startFrame,
                                                static_cast<PRInt64>(frames))));
  } else {
    startFrame = endFrame - frames;
  }

  
  if (mOpusState->mSkip > 0) {
    PRInt32 skipFrames = NS_MIN(mOpusState->mSkip, frames);
    if (skipFrames == frames) {
      
      mOpusState->mSkip -= frames;
      LOG(PR_LOG_DEBUG, ("Opus decoder skipping %d frames"
                         " (whole packet)", frames));
      return NS_OK;
    }
    PRInt32 keepFrames = frames - skipFrames;
    int samples = keepFrames * channels;
    nsAutoArrayPtr<AudioDataValue> trimBuffer(new AudioDataValue[samples]);
    for (int i = 0; i < samples; i++)
      trimBuffer[i] = buffer[skipFrames*channels + i];

    startFrame = endFrame - keepFrames;
    frames = keepFrames;
    buffer = trimBuffer;

    mOpusState->mSkip -= skipFrames;
    LOG(PR_LOG_DEBUG, ("Opus decoder skipping %d frames", skipFrames));
  }
  
  
  mOpusState->mPrevPacketGranulepos = endFrame;

  
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  if (mOpusState->mGain != 1.0f) {
    float gain = mOpusState->mGain;
    int samples = frames * channels;
    for (int i = 0; i < samples; i++) {
      buffer[i] *= gain;
    }
  }
#else
  if (mOpusState->mGain_Q16 != 65536) {
    PRInt64 gain_Q16 = mOpusState->mGain_Q16;
    int samples = frames * channels;
    for (int i = 0; i < samples; i++) {
      PRInt32 val = static_cast<PRInt32>((gain_Q16*buffer[i] + 32768)>>16);
      buffer[i] = static_cast<AudioDataValue>(MOZ_CLIP_TO_15(val));
    }
  }
#endif

  LOG(PR_LOG_DEBUG, ("Opus decoder pushing %d frames", frames));
  PRInt64 startTime = mOpusState->Time(startFrame);
  PRInt64 endTime = mOpusState->Time(endFrame);
  mAudioQueue.Push(new AudioData(mPageOffset,
                                 startTime,
                                 endTime - startTime,
                                 frames,
                                 buffer.forget(),
                                 channels));
  return NS_OK;
}
#endif 

bool nsOggReader::DecodeAudioData()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  NS_ASSERTION(mVorbisState != nsnull || mOpusState != nsnull,
    "Need audio codec state to decode audio");

  
  ogg_packet* packet = 0;
  nsOggCodecState* codecState;
  if (mVorbisState)
    codecState = static_cast<nsOggCodecState*>(mVorbisState);
  else
    codecState = static_cast<nsOggCodecState*>(mOpusState);
  do {
    if (packet) {
      nsOggCodecState::ReleasePacket(packet);
    }
    packet = NextOggPacket(codecState);
  } while (packet && codecState->IsHeader(packet));

  if (!packet) {
    mAudioQueue.Finish();
    return false;
  }

  NS_ASSERTION(packet && packet->granulepos != -1,
    "Must have packet with known granulepos");
  nsAutoRef<ogg_packet> autoRelease(packet);
  if (mVorbisState) {
    DecodeVorbis(packet);
#ifdef MOZ_OPUS
  } else if (mOpusState) {
    DecodeOpus(packet);
#endif
  }

  if (packet->e_o_s) {
    
    
    
    mAudioQueue.Finish();
    return false;
  }

  return true;
}

nsresult nsOggReader::DecodeTheora(ogg_packet* aPacket, PRInt64 aTimeThreshold)
{
  NS_ASSERTION(aPacket->granulepos >= TheoraVersion(&mTheoraState->mInfo,3,2,1),
    "Packets must have valid granulepos and packetno");

  int ret = th_decode_packetin(mTheoraState->mCtx, aPacket, 0);
  if (ret != 0 && ret != TH_DUPFRAME) {
    return NS_ERROR_FAILURE;
  }
  PRInt64 time = mTheoraState->StartTime(aPacket->granulepos);

  
  
  
  
  if (mSkeletonState && !mSkeletonState->IsPresentable(time)) {
    return NS_OK;
  }

  PRInt64 endTime = mTheoraState->Time(aPacket->granulepos);
  if (endTime < aTimeThreshold) {
    
    
    return NS_OK;
  }

  if (ret == TH_DUPFRAME) {
    VideoData* v = VideoData::CreateDuplicate(mPageOffset,
                                              time,
                                              endTime,
                                              aPacket->granulepos);
    mVideoQueue.Push(v);
  } else if (ret == 0) {
    th_ycbcr_buffer buffer;
    ret = th_decode_ycbcr_out(mTheoraState->mCtx, buffer);
    NS_ASSERTION(ret == 0, "th_decode_ycbcr_out failed");
    bool isKeyframe = th_packet_iskeyframe(aPacket) == 1;
    VideoData::YCbCrBuffer b;
    for (PRUint32 i=0; i < 3; ++i) {
      b.mPlanes[i].mData = buffer[i].data;
      b.mPlanes[i].mHeight = buffer[i].height;
      b.mPlanes[i].mWidth = buffer[i].width;
      b.mPlanes[i].mStride = buffer[i].stride;
      b.mPlanes[i].mOffset = b.mPlanes[i].mSkip = 0;
    }

    VideoData *v = VideoData::Create(mInfo,
                                     mDecoder->GetImageContainer(),
                                     mPageOffset,
                                     time,
                                     endTime,
                                     b,
                                     isKeyframe,
                                     aPacket->granulepos,
                                     mPicture);
    if (!v) {
      
      
      NS_WARNING("Failed to allocate memory for video frame");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mVideoQueue.Push(v);
  }
  return NS_OK;
}

bool nsOggReader::DecodeVideoFrame(bool &aKeyframeSkip,
                                     PRInt64 aTimeThreshold)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  
  
  PRUint32 parsed = 0, decoded = 0;
  nsMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  
  ogg_packet* packet = 0;
  do {
    if (packet) {
      nsOggCodecState::ReleasePacket(packet);
    }
    packet = NextOggPacket(mTheoraState);
  } while (packet && mTheoraState->IsHeader(packet));
  if (!packet) {
    mVideoQueue.Finish();
    return false;
  }
  nsAutoRef<ogg_packet> autoRelease(packet);

  parsed++;
  NS_ASSERTION(packet && packet->granulepos != -1,
                "Must know first packet's granulepos");
  bool eos = packet->e_o_s;
  PRInt64 frameEndTime = mTheoraState->Time(packet->granulepos);
  if (!aKeyframeSkip ||
     (th_packet_iskeyframe(packet) && frameEndTime >= aTimeThreshold))
  {
    aKeyframeSkip = false;
    nsresult res = DecodeTheora(packet, aTimeThreshold);
    decoded++;
    if (NS_FAILED(res)) {
      return false;
    }
  }

  if (eos) {
    
    
    mVideoQueue.Finish();
    return false;
  }

  return true;
}

PRInt64 nsOggReader::ReadOggPage(ogg_page* aPage)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  int ret = 0;
  while((ret = ogg_sync_pageseek(&mOggState, aPage)) <= 0) {
    if (ret < 0) {
      
      mPageOffset += -ret;
      continue;
    }
    
    
    
    char* buffer = ogg_sync_buffer(&mOggState, 4096);
    NS_ASSERTION(buffer, "ogg_sync_buffer failed");

    
    PRUint32 bytesRead = 0;

    nsresult rv = mDecoder->GetResource()->Read(buffer, 4096, &bytesRead);
    if (NS_FAILED(rv) || (bytesRead == 0 && ret == 0)) {
      
      return -1;
    }

    mDecoder->NotifyBytesConsumed(bytesRead);
    
    
    ret = ogg_sync_wrote(&mOggState, bytesRead);
    NS_ENSURE_TRUE(ret == 0, -1);    
  }
  PRInt64 offset = mPageOffset;
  mPageOffset += aPage->header_len + aPage->body_len;
  
  return offset;
}

ogg_packet* nsOggReader::NextOggPacket(nsOggCodecState* aCodecState)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  if (!aCodecState || !aCodecState->mActive) {
    return nsnull;
  }

  ogg_packet* packet;
  while ((packet = aCodecState->PacketOut()) == nsnull) {
    
    
    ogg_page page;
    if (ReadOggPage(&page) == -1) {
      return nsnull;
    }

    PRUint32 serial = ogg_page_serialno(&page);
    nsOggCodecState* codecState = nsnull;
    mCodecStates.Get(serial, &codecState);
    if (codecState && NS_FAILED(codecState->PageIn(&page))) {
      return nsnull;
    }
  }

  return packet;
}


static ogg_uint32_t
GetChecksum(ogg_page* page)
{
  if (page == 0 || page->header == 0 || page->header_len < 25) {
    return 0;
  }
  const unsigned char* p = page->header + 22;
  PRUint32 c =  p[0] +
               (p[1] << 8) + 
               (p[2] << 16) +
               (p[3] << 24);
  return c;
}

PRInt64 nsOggReader::RangeStartTime(PRInt64 aOffset)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource != nsnull, nsnull);
  nsresult res = resource->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  NS_ENSURE_SUCCESS(res, nsnull);
  PRInt64 startTime = 0;
  nsBuiltinDecoderReader::FindStartTime(startTime);
  return startTime;
}

struct nsAutoOggSyncState {
  nsAutoOggSyncState() {
    ogg_sync_init(&mState);
  }
  ~nsAutoOggSyncState() {
    ogg_sync_clear(&mState);
  }
  ogg_sync_state mState;
};

PRInt64 nsOggReader::RangeEndTime(PRInt64 aEndOffset)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or decode thread.");

  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource != nsnull, -1);
  PRInt64 position = resource->Tell();
  PRInt64 endTime = RangeEndTime(0, aEndOffset, false);
  nsresult res = resource->Seek(nsISeekableStream::NS_SEEK_SET, position);
  NS_ENSURE_SUCCESS(res, -1);
  return endTime;
}

PRInt64 nsOggReader::RangeEndTime(PRInt64 aStartOffset,
                                  PRInt64 aEndOffset,
                                  bool aCachedDataOnly)
{
  MediaResource* resource = mDecoder->GetResource();
  nsAutoOggSyncState sync;

  
  
  
  
  
  
  const int step = 5000;
  PRInt64 readStartOffset = aEndOffset;
  PRInt64 readHead = aEndOffset;
  PRInt64 endTime = -1;
  PRUint32 checksumAfterSeek = 0;
  PRUint32 prevChecksumAfterSeek = 0;
  bool mustBackOff = false;
  while (true) {
    ogg_page page;    
    int ret = ogg_sync_pageseek(&sync.mState, &page);
    if (ret == 0) {
      
      
      if (mustBackOff || readHead == aEndOffset || readHead == aStartOffset) {
        if (endTime != -1 || readStartOffset == 0) {
          
          break;
        }
        mustBackOff = false;
        prevChecksumAfterSeek = checksumAfterSeek;
        checksumAfterSeek = 0;
        ogg_sync_reset(&sync.mState);
        readStartOffset = NS_MAX(static_cast<PRInt64>(0), readStartOffset - step);
        readHead = NS_MAX(aStartOffset, readStartOffset);
      }

      PRInt64 limit = NS_MIN(static_cast<PRInt64>(PR_UINT32_MAX),
                             aEndOffset - readHead);
      limit = NS_MAX(static_cast<PRInt64>(0), limit);
      limit = NS_MIN(limit, static_cast<PRInt64>(step));
      PRUint32 bytesToRead = static_cast<PRUint32>(limit);
      PRUint32 bytesRead = 0;
      char* buffer = ogg_sync_buffer(&sync.mState, bytesToRead);
      NS_ASSERTION(buffer, "Must have buffer");
      nsresult res;
      if (aCachedDataOnly) {
        res = resource->ReadFromCache(buffer, readHead, bytesToRead);
        NS_ENSURE_SUCCESS(res, -1);
        bytesRead = bytesToRead;
      } else {
        NS_ASSERTION(readHead < aEndOffset,
                     "resource pos must be before range end");
        res = resource->Seek(nsISeekableStream::NS_SEEK_SET, readHead);
        NS_ENSURE_SUCCESS(res, -1);
        res = resource->Read(buffer, bytesToRead, &bytesRead);
        NS_ENSURE_SUCCESS(res, -1);
      }
      readHead += bytesRead;

      
      
      ret = ogg_sync_wrote(&sync.mState, bytesRead);
      if (ret != 0) {
        endTime = -1;
        break;
      }

      continue;
    }

    if (ret < 0 || ogg_page_granulepos(&page) < 0) {
      continue;
    }

    PRUint32 checksum = GetChecksum(&page);
    if (checksumAfterSeek == 0) {
      
      
      
      
      checksumAfterSeek = checksum;
    }
    if (checksum == prevChecksumAfterSeek) {
      
      
      
      
      mustBackOff = true;
      continue;
    }

    PRInt64 granulepos = ogg_page_granulepos(&page);
    int serial = ogg_page_serialno(&page);

    nsOggCodecState* codecState = nsnull;
    mCodecStates.Get(serial, &codecState);

    if (!codecState) {
      
      
      
      endTime = -1;
      break;
    }

    PRInt64 t = codecState->Time(granulepos);
    if (t != -1) {
      endTime = t;
    }
  }

  return endTime;
}

nsresult nsOggReader::GetSeekRanges(nsTArray<SeekRange>& aRanges)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  nsTArray<MediaByteRange> cached;
  nsresult res = mDecoder->GetResource()->GetCachedRanges(cached);
  NS_ENSURE_SUCCESS(res, res);

  for (PRUint32 index = 0; index < cached.Length(); index++) {
    MediaByteRange& range = cached[index];
    PRInt64 startTime = -1;
    PRInt64 endTime = -1;
    if (NS_FAILED(ResetDecode())) {
      return NS_ERROR_FAILURE;
    }
    PRInt64 startOffset = range.mStart;
    PRInt64 endOffset = range.mEnd;
    startTime = RangeStartTime(startOffset);
    if (startTime != -1 &&
        ((endTime = RangeEndTime(endOffset)) != -1))
    {
      NS_ASSERTION(startTime < endTime,
                   "Start time must be before end time");
      aRanges.AppendElement(SeekRange(startOffset,
                                      endOffset,
                                      startTime,
                                      endTime));
     }
  }
  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsOggReader::SeekRange
nsOggReader::SelectSeekRange(const nsTArray<SeekRange>& ranges,
                             PRInt64 aTarget,
                             PRInt64 aStartTime,
                             PRInt64 aEndTime,
                             bool aExact)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  PRInt64 so = 0;
  PRInt64 eo = mDecoder->GetResource()->GetLength();
  PRInt64 st = aStartTime;
  PRInt64 et = aEndTime;
  for (PRUint32 i = 0; i < ranges.Length(); i++) {
    const SeekRange &r = ranges[i];
    if (r.mTimeStart < aTarget) {
      so = r.mOffsetStart;
      st = r.mTimeStart;
    }
    if (r.mTimeEnd >= aTarget && r.mTimeEnd < et) {
      eo = r.mOffsetEnd;
      et = r.mTimeEnd;
    }

    if (r.mTimeStart < aTarget && aTarget <= r.mTimeEnd) {
      
      return ranges[i];
    }
  }
  if (aExact || eo == -1) {
    return SeekRange();
  }
  return SeekRange(so, eo, st, et);
}

nsOggReader::IndexedSeekResult nsOggReader::RollbackIndexedSeek(PRInt64 aOffset)
{
  mSkeletonState->Deactivate();
  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource != nsnull, SEEK_FATAL_ERROR);
  nsresult res = resource->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  NS_ENSURE_SUCCESS(res, SEEK_FATAL_ERROR);
  return SEEK_INDEX_FAIL;
}
 
nsOggReader::IndexedSeekResult nsOggReader::SeekToKeyframeUsingIndex(PRInt64 aTarget)
{
  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource != nsnull, SEEK_FATAL_ERROR);
  if (!HasSkeleton() || !mSkeletonState->HasIndex()) {
    return SEEK_INDEX_FAIL;
  }
  
  nsAutoTArray<PRUint32, 2> tracks;
  BuildSerialList(tracks);
  nsSkeletonState::nsSeekTarget keyframe;
  if (NS_FAILED(mSkeletonState->IndexedSeekTarget(aTarget,
                                                  tracks,
                                                  keyframe)))
  {
    
    return SEEK_INDEX_FAIL;
  }

  
  PRInt64 tell = resource->Tell();

  
  if (keyframe.mKeyPoint.mOffset > resource->GetLength() ||
      keyframe.mKeyPoint.mOffset < 0)
  {
    
    return RollbackIndexedSeek(tell);
  }
  LOG(PR_LOG_DEBUG, ("Seeking using index to keyframe at offset %lld\n",
                     keyframe.mKeyPoint.mOffset));
  nsresult res = resource->Seek(nsISeekableStream::NS_SEEK_SET,
                              keyframe.mKeyPoint.mOffset);
  NS_ENSURE_SUCCESS(res, SEEK_FATAL_ERROR);
  mPageOffset = keyframe.mKeyPoint.mOffset;

  
  res = ResetDecode();
  NS_ENSURE_SUCCESS(res, SEEK_FATAL_ERROR);

  
  
  ogg_page page;
  int skippedBytes = 0;
  PageSyncResult syncres = PageSync(resource,
                                    &mOggState,
                                    false,
                                    mPageOffset,
                                    resource->GetLength(),
                                    &page,
                                    skippedBytes);
  NS_ENSURE_TRUE(syncres != PAGE_SYNC_ERROR, SEEK_FATAL_ERROR);
  if (syncres != PAGE_SYNC_OK || skippedBytes != 0) {
    LOG(PR_LOG_DEBUG, ("Indexed-seek failure: Ogg Skeleton Index is invalid "
                       "or sync error after seek"));
    return RollbackIndexedSeek(tell);
  }
  PRUint32 serial = ogg_page_serialno(&page);
  if (serial != keyframe.mSerial) {
    
    
    return RollbackIndexedSeek(tell);
  }
  nsOggCodecState* codecState = nsnull;
  mCodecStates.Get(serial, &codecState);
  if (codecState &&
      codecState->mActive &&
      ogg_stream_pagein(&codecState->mState, &page) != 0)
  {
    
    
    return RollbackIndexedSeek(tell);
  }      
  mPageOffset = keyframe.mKeyPoint.mOffset + page.header_len + page.body_len;
  return SEEK_OK;
}

nsresult nsOggReader::SeekInBufferedRange(PRInt64 aTarget,
                                          PRInt64 aAdjustedTarget,
                                          PRInt64 aStartTime,
                                          PRInt64 aEndTime,
                                          const nsTArray<SeekRange>& aRanges,
                                          const SeekRange& aRange)
{
  LOG(PR_LOG_DEBUG, ("%p Seeking in buffered data to %lld using bisection search", mDecoder, aTarget));
  nsresult res = NS_OK;
  if (HasVideo() || aAdjustedTarget >= aTarget) {
    
    
    nsresult res = SeekBisection(aTarget, aRange, 0);
    if (NS_FAILED(res) || !HasVideo()) {
      return res;
    }

    
    
    bool eof;
    do {
      bool skip = false;
      eof = !DecodeVideoFrame(skip, 0);
      {
        ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
        if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
          return NS_ERROR_FAILURE;
        }
      }
    } while (!eof &&
             mVideoQueue.GetSize() == 0);

    VideoData* video = mVideoQueue.PeekFront();
    if (video && !video->mKeyframe) {
      
      
      NS_ASSERTION(video->mTimecode != -1, "Must have a granulepos");
      int shift = mTheoraState->mInfo.keyframe_granule_shift;
      PRInt64 keyframeGranulepos = (video->mTimecode >> shift) << shift;
      PRInt64 keyframeTime = mTheoraState->StartTime(keyframeGranulepos);
      SEEK_LOG(PR_LOG_DEBUG, ("Keyframe for %lld is at %lld, seeking back to it",
                              video->mTime, keyframeTime));
      aAdjustedTarget = NS_MIN(aAdjustedTarget, keyframeTime);
    }
  }
  if (aAdjustedTarget < aTarget) {
    SeekRange k = SelectSeekRange(aRanges,
                                  aAdjustedTarget,
                                  aStartTime,
                                  aEndTime,
                                  false);
    res = SeekBisection(aAdjustedTarget, k, SEEK_FUZZ_USECS);
  }
  return res;
}

nsresult nsOggReader::SeekInUnbuffered(PRInt64 aTarget,
                                       PRInt64 aStartTime,
                                       PRInt64 aEndTime,
                                       const nsTArray<SeekRange>& aRanges)
{
  LOG(PR_LOG_DEBUG, ("%p Seeking in unbuffered data to %lld using bisection search", mDecoder, aTarget));
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRInt64 keyframeOffsetMs = 0;
  if (HasVideo() && mTheoraState) {
    keyframeOffsetMs = mTheoraState->MaxKeyframeOffset();
  }
  
  if (HasAudio() && mOpusState) {
    keyframeOffsetMs = NS_MAX(keyframeOffsetMs, SEEK_OPUS_PREROLL);
  }
  PRInt64 seekTarget = NS_MAX(aStartTime, aTarget - keyframeOffsetMs);
  
  
  SeekRange k = SelectSeekRange(aRanges, seekTarget, aStartTime, aEndTime, false);
  return SeekBisection(seekTarget, k, SEEK_FUZZ_USECS);
}

nsresult nsOggReader::Seek(PRInt64 aTarget,
                           PRInt64 aStartTime,
                           PRInt64 aEndTime,
                           PRInt64 aCurrentTime)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  LOG(PR_LOG_DEBUG, ("%p About to seek to %lld", mDecoder, aTarget));
  nsresult res;
  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource != nsnull, NS_ERROR_FAILURE);
  PRInt64 adjustedTarget = aTarget;
  if (HasAudio() && mOpusState){
    adjustedTarget = NS_MAX(aStartTime, aTarget - SEEK_OPUS_PREROLL);
  }

  if (adjustedTarget == aStartTime) {
    
    
    res = resource->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    NS_ENSURE_SUCCESS(res,res);

    mPageOffset = 0;
    res = ResetDecode(true);
    NS_ENSURE_SUCCESS(res,res);

    NS_ASSERTION(aStartTime != -1, "mStartTime should be known");
    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mDecoder->UpdatePlaybackPosition(aStartTime);
    }
  } else {
    
    
    
    IndexedSeekResult sres = SeekToKeyframeUsingIndex(adjustedTarget);
    NS_ENSURE_TRUE(sres != SEEK_FATAL_ERROR, NS_ERROR_FAILURE);
    if (sres == SEEK_INDEX_FAIL) {
      
      
      
      nsAutoTArray<SeekRange, 16> ranges;
      res = GetSeekRanges(ranges);
      NS_ENSURE_SUCCESS(res,res);

      
      SeekRange r = SelectSeekRange(ranges, aTarget, aStartTime, aEndTime, true);

      if (!r.IsNull()) {
        
        
        res = SeekInBufferedRange(aTarget, adjustedTarget, aStartTime, aEndTime, ranges, r);
        NS_ENSURE_SUCCESS(res,res);
      } else {
        
        
        
        res = SeekInUnbuffered(aTarget, aStartTime, aEndTime, ranges);
        NS_ENSURE_SUCCESS(res,res);
      }
    }
  }

  
  
  
  return DecodeToTarget(aTarget);
}


static PageSyncResult
PageSync(MediaResource* aResource,
         ogg_sync_state* aState,
         bool aCachedDataOnly,
         PRInt64 aOffset,
         PRInt64 aEndOffset,
         ogg_page* aPage,
         int& aSkippedBytes)
{
  aSkippedBytes = 0;
  
  int ret = 0;
  PRUint32 bytesRead = 0;
  PRInt64 readHead = aOffset;
  while (ret <= 0) {
    ret = ogg_sync_pageseek(aState, aPage);
    if (ret == 0) {
      char* buffer = ogg_sync_buffer(aState, PAGE_STEP);
      NS_ASSERTION(buffer, "Must have a buffer");

      
      PRInt64 bytesToRead = NS_MIN(static_cast<PRInt64>(PAGE_STEP),
                                   aEndOffset - readHead);
      NS_ASSERTION(bytesToRead <= PR_UINT32_MAX, "bytesToRead range check");
      if (bytesToRead <= 0) {
        return PAGE_SYNC_END_OF_RANGE;
      }
      nsresult rv = NS_OK;
      if (aCachedDataOnly) {
        rv = aResource->ReadFromCache(buffer, readHead,
                                      static_cast<PRUint32>(bytesToRead));
        NS_ENSURE_SUCCESS(rv,PAGE_SYNC_ERROR);
        bytesRead = static_cast<PRUint32>(bytesToRead);
      } else {
        rv = aResource->Seek(nsISeekableStream::NS_SEEK_SET, readHead);
        NS_ENSURE_SUCCESS(rv,PAGE_SYNC_ERROR);
        rv = aResource->Read(buffer,
                             static_cast<PRUint32>(bytesToRead),
                             &bytesRead);
        NS_ENSURE_SUCCESS(rv,PAGE_SYNC_ERROR);
      }
      if (bytesRead == 0 && NS_SUCCEEDED(rv)) {
        
        return PAGE_SYNC_END_OF_RANGE;
      }
      readHead += bytesRead;

      
      
      ret = ogg_sync_wrote(aState, bytesRead);
      NS_ENSURE_TRUE(ret == 0, PAGE_SYNC_ERROR);    
      continue;
    }

    if (ret < 0) {
      NS_ASSERTION(aSkippedBytes >= 0, "Offset >= 0");
      aSkippedBytes += -ret;
      NS_ASSERTION(aSkippedBytes >= 0, "Offset >= 0");
      continue;
    }
  }
  
  return PAGE_SYNC_OK;
}

nsresult nsOggReader::SeekBisection(PRInt64 aTarget,
                                    const SeekRange& aRange,
                                    PRUint32 aFuzz)
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
  nsresult res;
  MediaResource* resource = mDecoder->GetResource();

  if (aTarget == aRange.mTimeStart) {
    if (NS_FAILED(ResetDecode())) {
      return NS_ERROR_FAILURE;
    }
    res = resource->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    NS_ENSURE_SUCCESS(res,res);
    mPageOffset = 0;
    return NS_OK;
  }

  
  
  ogg_int64_t startOffset = aRange.mOffsetStart;
  ogg_int64_t startTime = aRange.mTimeStart;
  ogg_int64_t startLength = 0; 
  ogg_int64_t endOffset = aRange.mOffsetEnd;
  ogg_int64_t endTime = aRange.mTimeEnd;

  ogg_int64_t seekTarget = aTarget;
  PRInt64 seekLowerBound = NS_MAX(static_cast<PRInt64>(0), aTarget - aFuzz);
  int hops = 0;
  DebugOnly<ogg_int64_t> previousGuess = -1;
  int backsteps = 0;
  const int maxBackStep = 10;
  NS_ASSERTION(static_cast<PRUint64>(PAGE_STEP) * pow(2.0, maxBackStep) < PR_INT32_MAX,
               "Backstep calculation must not overflow");

  
  
  
  while (true) {
    ogg_int64_t duration = 0;
    double target = 0;
    ogg_int64_t interval = 0;
    ogg_int64_t guess = 0;
    ogg_page page;
    int skippedBytes = 0;
    ogg_int64_t pageOffset = 0;
    ogg_int64_t pageLength = 0;
    ogg_int64_t granuleTime = -1;
    bool mustBackoff = false;

    
    
    
    while (true) {
  
      
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }

      interval = endOffset - startOffset - startLength;
      if (interval == 0) {
        
        
        
        SEEK_LOG(PR_LOG_DEBUG, ("Interval narrowed, terminating bisection."));
        break;
      }

      
      duration = endTime - startTime;
      target = (double)(seekTarget - startTime) / (double)duration;
      guess = startOffset + startLength +
              static_cast<ogg_int64_t>((double)interval * target);
      guess = NS_MIN(guess, endOffset - PAGE_STEP);
      if (mustBackoff) {
        
        
        
        
        SEEK_LOG(PR_LOG_DEBUG, ("Backing off %d bytes, backsteps=%d",
          static_cast<PRInt32>(PAGE_STEP * pow(2.0, backsteps)), backsteps));
        guess -= PAGE_STEP * static_cast<ogg_int64_t>(pow(2.0, backsteps));

        if (guess <= startOffset) {
          
          
          
          
          
          interval = 0;
          break;
        }

        backsteps = NS_MIN(backsteps + 1, maxBackStep);
        
        
        mustBackoff = false;
      } else {
        backsteps = 0;
      }
      guess = NS_MAX(guess, startOffset + startLength);

      SEEK_LOG(PR_LOG_DEBUG, ("Seek loop start[o=%lld..%lld t=%lld] "
                              "end[o=%lld t=%lld] "
                              "interval=%lld target=%lf guess=%lld",
                              startOffset, (startOffset+startLength), startTime,
                              endOffset, endTime, interval, target, guess));

      NS_ASSERTION(guess >= startOffset + startLength, "Guess must be after range start");
      NS_ASSERTION(guess < endOffset, "Guess must be before range end");
      NS_ASSERTION(guess != previousGuess, "Guess should be different to previous");
      previousGuess = guess;

      hops++;
    
      
      
      
      PageSyncResult res = PageSync(resource,
                                    &mOggState,
                                    false,
                                    guess,
                                    endOffset,
                                    &page,
                                    skippedBytes);
      NS_ENSURE_TRUE(res != PAGE_SYNC_ERROR, NS_ERROR_FAILURE);

      
      
      pageOffset = guess + skippedBytes;
      pageLength = page.header_len + page.body_len;
      mPageOffset = pageOffset + pageLength;

      if (res == PAGE_SYNC_END_OF_RANGE) {
        
        
        
        mustBackoff = true;
        SEEK_LOG(PR_LOG_DEBUG, ("Hit the end of range, backing off"));
        continue;
      }

      
      
      ogg_int64_t audioTime = -1;
      ogg_int64_t videoTime = -1;
      do {
        
        PRUint32 serial = ogg_page_serialno(&page);
        nsOggCodecState* codecState = nsnull;
        mCodecStates.Get(serial, &codecState);
        if (codecState && codecState->mActive) {
          int ret = ogg_stream_pagein(&codecState->mState, &page);
          NS_ENSURE_TRUE(ret == 0, NS_ERROR_FAILURE);
        }

        ogg_int64_t granulepos = ogg_page_granulepos(&page);

        if (HasAudio() && granulepos > 0 && audioTime == -1) {
          if (mVorbisState && serial == mVorbisState->mSerial) {
            audioTime = mVorbisState->Time(granulepos);
#ifdef MOZ_OPUS
          } else if (mOpusState && serial == mOpusState->mSerial) {
            audioTime = mOpusState->Time(granulepos);
#endif
          }
        }
        
        if (HasVideo() &&
            granulepos > 0 &&
            serial == mTheoraState->mSerial &&
            videoTime == -1) {
          videoTime = mTheoraState->StartTime(granulepos);
        }

        if (mPageOffset == endOffset) {
          
          break;
        }

        if (ReadOggPage(&page) == -1) {
          break;
        }
        
      } while ((HasAudio() && audioTime == -1) ||
               (HasVideo() && videoTime == -1));

      NS_ASSERTION(mPageOffset <= endOffset, "Page read cursor should be inside range");

      if ((HasAudio() && audioTime == -1) ||
          (HasVideo() && videoTime == -1)) 
      {
        
        if (pageOffset == startOffset + startLength && mPageOffset == endOffset) {
          
          
          
          
          
          interval = 0;
          break;
        }

        
        
        mustBackoff = true;
        continue;
      }

      
      
      granuleTime = NS_MAX(audioTime, videoTime);
      NS_ASSERTION(granuleTime > 0, "Must get a granuletime");
      break;
    } 

    if (interval == 0) {
      
      
      SEEK_LOG(PR_LOG_DEBUG, ("Terminating seek at offset=%lld", startOffset));
      NS_ASSERTION(startTime < aTarget, "Start time must always be less than target");
      res = resource->Seek(nsISeekableStream::NS_SEEK_SET, startOffset);
      NS_ENSURE_SUCCESS(res,res);
      mPageOffset = startOffset;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      break;
    }

    SEEK_LOG(PR_LOG_DEBUG, ("Time at offset %lld is %lld", guess, granuleTime));
    if (granuleTime < seekTarget && granuleTime > seekLowerBound) {
      
      res = resource->Seek(nsISeekableStream::NS_SEEK_SET, pageOffset);
      NS_ENSURE_SUCCESS(res,res);
      mPageOffset = pageOffset;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      SEEK_LOG(PR_LOG_DEBUG, ("Terminating seek at offset=%lld", mPageOffset));
      break;
    }

    if (granuleTime >= seekTarget) {
      
      NS_ASSERTION(pageOffset < endOffset, "offset_end must decrease");
      endOffset = pageOffset;
      endTime = granuleTime;
    } else if (granuleTime < seekTarget) {
      
      NS_ASSERTION(pageOffset >= startOffset + startLength,
        "Bisection point should be at or after end of first page in interval");
      startOffset = pageOffset;
      startLength = pageLength;
      startTime = granuleTime;
    }
    NS_ASSERTION(startTime < seekTarget, "Must be before seek target");
    NS_ASSERTION(endTime >= seekTarget, "End must be after seek target");
  }

  SEEK_LOG(PR_LOG_DEBUG, ("Seek complete in %d bisections.", hops));

  return NS_OK;
}

nsresult nsOggReader::GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime)
{
  
  
  
  
  if (!mInfo.mHasVideo && !mInfo.mHasAudio) {
    
    return NS_OK;
  }

  MediaResource* resource = mDecoder->GetResource();
  nsTArray<MediaByteRange> ranges;
  nsresult res = resource->GetCachedRanges(ranges);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  
  
  nsAutoOggSyncState sync;
  for (PRUint32 index = 0; index < ranges.Length(); index++) {
    
    PRInt64 startOffset = ranges[index].mStart;
    PRInt64 endOffset = ranges[index].mEnd;

    
    
    
    
    PRInt64 startTime = (startOffset == 0) ? aStartTime : -1;

    
    
    
    ogg_sync_reset(&sync.mState);
    while (startTime == -1) {
      ogg_page page;
      PRInt32 discard;
      PageSyncResult res = PageSync(resource,
                                    &sync.mState,
                                    true,
                                    startOffset,
                                    endOffset,
                                    &page,
                                    discard);
      if (res == PAGE_SYNC_ERROR) {
        return NS_ERROR_FAILURE;
      } else if (res == PAGE_SYNC_END_OF_RANGE) {
        
        
        break;
      }

      PRInt64 granulepos = ogg_page_granulepos(&page);
      if (granulepos == -1) {
        
        
        startOffset += page.header_len + page.body_len;
        continue;
      }

      PRUint32 serial = ogg_page_serialno(&page);
      if (mVorbisState && serial == mVorbisSerial) {
        startTime = nsVorbisState::Time(&mVorbisInfo, granulepos);
        NS_ASSERTION(startTime > 0, "Must have positive start time");
      }
      else if (mOpusState && serial == mOpusSerial) {
        startTime = nsOpusState::Time(mOpusPreSkip, granulepos);
        NS_ASSERTION(startTime > 0, "Must have positive start time");
      }
      else if (mTheoraState && serial == mTheoraSerial) {
        startTime = nsTheoraState::Time(&mTheoraInfo, granulepos);
        NS_ASSERTION(startTime > 0, "Must have positive start time");
      }
      else if (IsKnownStream(serial)) {
        
        
        startOffset += page.header_len + page.body_len;
        continue;
      }
      else {
        
        
        return PAGE_SYNC_ERROR;
      }
    }

    if (startTime != -1) {
      
      
      PRInt64 endTime = RangeEndTime(startOffset, endOffset, true);
      if (endTime != -1) {
        aBuffered->Add((startTime - aStartTime) / static_cast<double>(USECS_PER_S),
                       (endTime - aStartTime) / static_cast<double>(USECS_PER_S));
      }
    }
  }

  return NS_OK;
}

bool nsOggReader::IsKnownStream(PRUint32 aSerial)
{
  for (PRUint32 i = 0; i < mKnownStreams.Length(); i++) {
    PRUint32 serial = mKnownStreams[i];
    if (serial == aSerial) {
      return true;
    }
  }

  return false;
}
