





































#include "nsError.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsBuiltinDecoder.h"
#include "nsOggReader.h"
#include "VideoUtils.h"
#include "theora/theoradec.h"
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






#define SEEK_DECODE_MARGIN 2000000







#define SEEK_FUZZ_USECS 500000

enum PageSyncResult {
  PAGE_SYNC_ERROR = 1,
  PAGE_SYNC_END_OF_RANGE= 2,
  PAGE_SYNC_OK = 3
};


static PageSyncResult
PageSync(nsMediaStream* aStream,
         ogg_sync_state* aState,
         PRBool aCachedDataOnly,
         PRInt64 aOffset,
         PRInt64 aEndOffset,
         ogg_page* aPage,
         int& aSkippedBytes);



static const int PAGE_STEP = 8192;

nsOggReader::nsOggReader(nsBuiltinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder),
    mTheoraState(nsnull),
    mVorbisState(nsnull),
    mSkeletonState(nsnull),
    mVorbisSerial(0),
    mTheoraSerial(0),
    mPageOffset(0),
    mTheoraGranulepos(-1),
    mVorbisGranulepos(-1),
    mDataOffset(0)
{
  MOZ_COUNT_CTOR(nsOggReader);
}

nsOggReader::~nsOggReader()
{
  ogg_sync_clear(&mOggState);
  MOZ_COUNT_DTOR(nsOggReader);
}

nsresult nsOggReader::Init(nsBuiltinDecoderReader* aCloneDonor) {
  PRBool init = mCodecStates.Init();
  NS_ASSERTION(init, "Failed to initialize mCodecStates");
  if (!init) {
    return NS_ERROR_FAILURE;
  }
  int ret = ogg_sync_init(&mOggState);
  NS_ENSURE_TRUE(ret == 0, NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult nsOggReader::ResetDecode()
{
  nsresult res = NS_OK;

  
  
  mTheoraGranulepos = -1;
  mVorbisGranulepos = -1;

  if (NS_FAILED(nsBuiltinDecoderReader::ResetDecode())) {
    res = NS_ERROR_FAILURE;
  }

  {
    MonitorAutoEnter mon(mMonitor);

    
    ogg_sync_reset(&mOggState);
    if (mVorbisState && NS_FAILED(mVorbisState->Reset())) {
      res = NS_ERROR_FAILURE;
    }
    if (mTheoraState && NS_FAILED(mTheoraState->Reset())) {
      res = NS_ERROR_FAILURE;
    }
  }

  return res;
}



static PRBool DoneReadingHeaders(nsTArray<nsOggCodecState*>& aBitstreams) {
  for (PRUint32 i = 0; i < aBitstreams .Length(); i++) {
    if (!aBitstreams [i]->DoneReadingHeaders()) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

nsresult nsOggReader::ReadMetadata(nsVideoInfo* aInfo)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(), "Should be on play state machine thread.");
  MonitorAutoEnter mon(mMonitor);

  
  
  

  ogg_page page;
  PRInt64 pageOffset;
  nsAutoTArray<nsOggCodecState*,4> bitstreams;
  PRBool readAllBOS = PR_FALSE;
  mDataOffset = 0;
  while (PR_TRUE) {
    if (readAllBOS && DoneReadingHeaders(bitstreams)) {
      if (mDataOffset == 0) {
        
        mDataOffset = mPageOffset;
      }
      break;
    }
    pageOffset = ReadOggPage(&page);
    if (pageOffset == -1) {
      
      break;
    }

    int ret = 0;
    int serial = ogg_page_serialno(&page);
    nsOggCodecState* codecState = 0;

    if (ogg_page_bos(&page)) {
      NS_ASSERTION(!readAllBOS, "We shouldn't encounter another BOS page");
      codecState = nsOggCodecState::Create(&page);

#ifdef DEBUG
      PRBool r =
#endif
        mCodecStates.Put(serial, codecState);
      NS_ASSERTION(r, "Failed to insert into mCodecStates");
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
          codecState->GetType() == nsOggCodecState::TYPE_SKELETON &&
          !mSkeletonState)
      {
        mSkeletonState = static_cast<nsSkeletonState*>(codecState);
      }
    } else {
      
      
      
      readAllBOS = PR_TRUE;
    }

    mCodecStates.Get(serial, &codecState);
    NS_ENSURE_TRUE(codecState, NS_ERROR_FAILURE);

    
    ret = ogg_stream_pagein(&codecState->mState, &page);
    NS_ENSURE_TRUE(ret == 0, NS_ERROR_FAILURE);

    
    ogg_packet packet;
    if (codecState->DoneReadingHeaders() && mDataOffset == 0)
    {
      
      
      
      mDataOffset = pageOffset;
      continue;
    }
    while (!codecState->DoneReadingHeaders() &&
           (ret = ogg_stream_packetout(&codecState->mState, &packet)) != 0)
    {
      if (ret == -1) {
        
        
        continue;
      }
      
      
      codecState->DecodeHeader(&packet);
    }
    if (ogg_stream_packetpeek(&codecState->mState, &packet) != 0 &&
        mDataOffset == 0)
    {
      
      
      
      
      mDataOffset = pageOffset;
    }
  }
  
  for (PRUint32 i = 0; i < bitstreams.Length(); i++) {
    nsOggCodecState* s = bitstreams[i];
    if (s != mVorbisState && s != mTheoraState && s != mSkeletonState) {
      s->Deactivate();
    }
  }

  
  
  
  
  if (mTheoraState && mTheoraState->Init()) {
    gfxIntSize sz(mTheoraState->mInfo.pic_width,
                  mTheoraState->mInfo.pic_height);
    mDecoder->SetVideoData(sz, mTheoraState->mPixelAspectRatio, nsnull, TimeStamp::Now());
  }
  if (mVorbisState) {
    mVorbisState->Init();
  }

  if (!HasAudio() && !HasVideo() && mSkeletonState) {
    
    
    mSkeletonState->Deactivate();
  }

  mInfo.mHasAudio = HasAudio();
  mInfo.mHasVideo = HasVideo();
  if (HasAudio()) {
    mInfo.mAudioRate = mVorbisState->mInfo.rate;
    mInfo.mAudioChannels = mVorbisState->mInfo.channels;
  }
  if (HasVideo()) {
    mInfo.mPixelAspectRatio = mTheoraState->mPixelAspectRatio;
    mInfo.mPicture = nsIntRect(mTheoraState->mInfo.pic_x,
                               mTheoraState->mInfo.pic_y,
                               mTheoraState->mInfo.pic_width,
                               mTheoraState->mInfo.pic_height);
    mInfo.mFrame = nsIntSize(mTheoraState->mInfo.frame_width,
                             mTheoraState->mInfo.frame_height);
    mInfo.mDisplay = nsIntSize(mInfo.mPicture.width,
                               mInfo.mPicture.height);
  }

  if (mSkeletonState && mSkeletonState->HasIndex()) {
    
    
    nsAutoTArray<PRUint32, 2> tracks;
    if (HasVideo()) {
      tracks.AppendElement(mTheoraState->mSerial);
    }
    if (HasAudio()) {
      tracks.AppendElement(mVorbisState->mSerial);
    }
    PRInt64 duration = 0;
    if (NS_SUCCEEDED(mSkeletonState->GetDuration(tracks, duration))) {
      MonitorAutoExit exitReaderMon(mMonitor);
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      mDecoder->GetStateMachine()->SetDuration(duration);
      LOG(PR_LOG_DEBUG, ("Got duration from Skeleton index %lld", duration));
    }
  }

  
  if (mVorbisState) {
    memcpy(&mVorbisInfo, &mVorbisState->mInfo, sizeof(mVorbisInfo));
    mVorbisInfo.codec_setup = NULL;
    mVorbisSerial = mVorbisState->mSerial;
  }

  if (mTheoraState) {
    memcpy(&mTheoraInfo, &mTheoraState->mInfo, sizeof(mTheoraInfo));
    mTheoraSerial = mTheoraState->mSerial;
  }

  *aInfo = mInfo;

  LOG(PR_LOG_DEBUG, ("Done loading headers, data offset %lld", mDataOffset));

  return NS_OK;
}

nsresult nsOggReader::DecodeVorbis(nsTArray<nsAutoPtr<SoundData> >& aChunks,
                                   ogg_packet* aPacket)
{
  
  if (vorbis_synthesis(&mVorbisState->mBlock, aPacket) != 0) {
    return NS_ERROR_FAILURE;
  }
  if (vorbis_synthesis_blockin(&mVorbisState->mDsp,
                               &mVorbisState->mBlock) != 0)
  {
    return NS_ERROR_FAILURE;
  }

  VorbisPCMValue** pcm = 0;
  PRInt32 samples = 0;
  PRUint32 channels = mVorbisState->mInfo.channels;
  while ((samples = vorbis_synthesis_pcmout(&mVorbisState->mDsp, &pcm)) > 0) {
    SoundDataValue* buffer = new SoundDataValue[samples * channels];
    for (PRUint32 j = 0; j < channels; ++j) {
      VorbisPCMValue* channel = pcm[j];
      for (PRUint32 i = 0; i < PRUint32(samples); ++i) {
        buffer[i*channels + j] = MOZ_CONVERT_VORBIS_SAMPLE(channel[i]);
      }
    }

    PRInt64 duration = mVorbisState->Time((PRInt64)samples);
    PRInt64 startTime = mVorbisState->Time(mVorbisGranulepos);
    SoundData* s = new SoundData(mPageOffset,
                                 startTime,
                                 duration,
                                 samples,
                                 buffer,
                                 channels);
    if (mVorbisGranulepos != -1) {
      mVorbisGranulepos += samples;
    }
    if (!aChunks.AppendElement(s)) {
      delete s;
    }
    if (vorbis_synthesis_read(&mVorbisState->mDsp, samples) != 0) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

PRBool nsOggReader::DecodeAudioData()
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on playback or decode thread.");
  NS_ASSERTION(mVorbisState!=0, "Need Vorbis state to decode audio");
  ogg_packet packet;
  packet.granulepos = -1;

  PRBool endOfStream = PR_FALSE;

  nsAutoTArray<nsAutoPtr<SoundData>, 64> chunks;
  if (mVorbisGranulepos == -1) {
    
    

    
    
    
    

    
    while (packet.granulepos <= 0 && !endOfStream) {
      if (!ReadOggPacket(mVorbisState, &packet)) {
        endOfStream = PR_TRUE;
        break;
      }
      if (packet.e_o_s != 0) {
        
        
        endOfStream = PR_TRUE;
      }

      if (NS_FAILED(DecodeVorbis(chunks, &packet))) {
        NS_WARNING("Failed to decode Vorbis packet");
      }
    }

    if (packet.granulepos > 0) {
      
      
      PRInt64 granulepos = packet.granulepos; 
      mVorbisGranulepos = packet.granulepos;
      for (int i = chunks.Length() - 1; i >= 0; --i) {
        SoundData* s = chunks[i];
        PRInt64 startGranule = granulepos - s->mSamples;
        s->mTime = mVorbisState->Time(startGranule);
        granulepos = startGranule;
      }
    }
  } else {
    
    
    if (!ReadOggPacket(mVorbisState, &packet)) {
      endOfStream = PR_TRUE;
    } else {
      
      endOfStream = packet.e_o_s != 0;

      
      if (NS_FAILED(DecodeVorbis(chunks, &packet))) {
        NS_WARNING("Failed to decode Vorbis packet");
      }

      if (packet.granulepos != -1 && packet.granulepos != mVorbisGranulepos) {
        
        
        
        mVorbisGranulepos = packet.granulepos;
      }
    }
  }

  
  
  for (PRUint32 i = 0; i < chunks.Length(); ++i) {
    mAudioQueue.Push(chunks[i].forget());
  }

  if (endOfStream) {
    
    
    
    mAudioQueue.Finish();
    return PR_FALSE;
  }

  return PR_TRUE;
}



static int
TheoraVersion(th_info* info,
              unsigned char maj,
              unsigned char min,
              unsigned char sub)
{
  ogg_uint32_t ver = (maj << 16) + (min << 8) + sub;
  ogg_uint32_t th_ver = (info->version_major << 16) +
                        (info->version_minor << 8) +
                        info->version_subminor;
  return (th_ver >= ver) ? 1 : 0;
}

#ifdef DEBUG


static PRBool
AllFrameTimesIncrease(nsTArray<nsAutoPtr<VideoData> >& aFrames)
{
  PRInt64 prevTime = -1;
  PRInt64 prevGranulepos = -1;
  for (PRUint32 i = 0; i < aFrames.Length(); i++) {
    VideoData* f = aFrames[i];
    if (f->mTime < prevTime) {
      return PR_FALSE;
    }
    prevTime = f->mTime;
    prevGranulepos = f->mTimecode;
  }

  return PR_TRUE;
}
#endif

nsresult nsOggReader::DecodeTheora(nsTArray<nsAutoPtr<VideoData> >& aFrames,
                                   ogg_packet* aPacket)
{
  int ret = th_decode_packetin(mTheoraState->mCtx, aPacket, 0);
  if (ret != 0 && ret != TH_DUPFRAME) {
    return NS_ERROR_FAILURE;
  }
  PRInt64 time = mTheoraState->StartTime(aPacket->granulepos);
  PRInt64 endTime = mTheoraState->Time(aPacket->granulepos);
  if (ret == TH_DUPFRAME) {
    VideoData* v = VideoData::CreateDuplicate(mPageOffset,
                                              time,
                                              endTime,
                                              aPacket->granulepos);
    if (!aFrames.AppendElement(v)) {
      delete v;
    }
  } else if (ret == 0) {
    th_ycbcr_buffer buffer;
    ret = th_decode_ycbcr_out(mTheoraState->mCtx, buffer);
    NS_ASSERTION(ret == 0, "th_decode_ycbcr_out failed");
    PRBool isKeyframe = th_packet_iskeyframe(aPacket) == 1;
    VideoData::YCbCrBuffer b;
    for (PRUint32 i=0; i < 3; ++i) {
      b.mPlanes[i].mData = buffer[i].data;
      b.mPlanes[i].mHeight = buffer[i].height;
      b.mPlanes[i].mWidth = buffer[i].width;
      b.mPlanes[i].mStride = buffer[i].stride;
    }

    
    
    mMonitor.AssertCurrentThreadIn();
    VideoData *v = VideoData::Create(mInfo,
                                     mDecoder->GetImageContainer(),
                                     mPageOffset,
                                     time,
                                     endTime,
                                     b,
                                     isKeyframe,
                                     aPacket->granulepos);
    if (!v) {
      
      
      NS_WARNING("Failed to allocate memory for video frame");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!aFrames.AppendElement(v)) {
      delete v;
    }
  }
  return NS_OK;
}

PRBool nsOggReader::DecodeVideoFrame(PRBool &aKeyframeSkip,
                                     PRInt64 aTimeThreshold)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or AV thread.");

  
  
  PRUint32 parsed = 0, decoded = 0;
  nsMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  
  
  
  
  
  

  nsAutoTArray<nsAutoPtr<VideoData>, 8> frames;
  ogg_packet packet;
  PRBool endOfStream = PR_FALSE;
  if (mTheoraGranulepos == -1) {
    
    
    
    
    
    do {
      if (!ReadOggPacket(mTheoraState, &packet)) {
        
        
        
        
        
        mVideoQueue.Finish();
        return PR_FALSE;
      }
      parsed++;

      if (packet.granulepos > 0) {
        
        
        
        mTheoraGranulepos = packet.granulepos;
      }

      if (DecodeTheora(frames, &packet) == NS_ERROR_OUT_OF_MEMORY) {
        NS_WARNING("Theora decode memory allocation failure!");
        return PR_FALSE;
      }

    } while (packet.granulepos <= 0 && !endOfStream);

    if (packet.granulepos > 0) {
      
      
      
      
      ogg_int64_t shift = mTheoraState->mInfo.keyframe_granule_shift;
      ogg_int64_t version_3_2_1 = TheoraVersion(&mTheoraState->mInfo,3,2,1);
      ogg_int64_t lastFrame = th_granule_frame(mTheoraState->mCtx,
                                               packet.granulepos) + version_3_2_1;
      ogg_int64_t firstFrame = lastFrame - frames.Length() + 1;

      
      
      
      
      
      
      
      ogg_int64_t keyframe = packet.granulepos >> shift;

      
      
      
      
      for (PRUint32 i = 0; i < frames.Length() - 1; ++i) {
        ogg_int64_t frame = firstFrame + i;
        ogg_int64_t granulepos;
        if (frames[i]->mKeyframe) {
          granulepos = frame << shift;
          keyframe = frame;
        } else if (frame >= keyframe &&
                   frame - keyframe < ((ogg_int64_t)1 << shift))
        {
          
          
          granulepos = (keyframe << shift) + (frame - keyframe);
        } else {
          
          
          
          ogg_int64_t k = NS_MAX(frame - (((ogg_int64_t)1 << shift) - 1), version_3_2_1);
          granulepos = (k << shift) + (frame - k);
        }
        
        
        
        
        NS_ASSERTION(granulepos >= version_3_2_1,
                     "Invalid granulepos for Theora version");

        
        
        NS_ASSERTION(i == 0 ||
                     th_granule_frame(mTheoraState->mCtx, granulepos) ==
                     th_granule_frame(mTheoraState->mCtx, frames[i-1]->mTimecode) + 1,
                     "Granulepos calculation is incorrect!");

        frames[i]->mTime = mTheoraState->StartTime(granulepos);
        frames[i]->mEndTime = mTheoraState->Time(granulepos);
        NS_ASSERTION(frames[i]->mEndTime >= frames[i]->mTime, "Frame must start before it ends.");
        frames[i]->mTimecode = granulepos;
      }
      NS_ASSERTION(AllFrameTimesIncrease(frames), "All frames must have granulepos");

      
      
      
      NS_ASSERTION(frames.Length() < 2 ||
        th_granule_frame(mTheoraState->mCtx, frames[frames.Length()-2]->mTimecode) + 1 ==
        th_granule_frame(mTheoraState->mCtx, packet.granulepos),
        "Granulepos recovery should catch up with packet.granulepos!");
    }
  } else {
    
    NS_ASSERTION(mTheoraGranulepos > 0, "We must Theora granulepos!");
    
    if (!ReadOggPacket(mTheoraState, &packet)) {
      
      
      mVideoQueue.Finish();
      return PR_FALSE;
    }
    parsed++;

    endOfStream = packet.e_o_s != 0;

    
    
    if (packet.granulepos != -1) {
      
      mTheoraGranulepos = packet.granulepos;
    } else {
      
      PRInt64 granulepos = 0;
      int shift = mTheoraState->mInfo.keyframe_granule_shift;
      
      
      if (!th_packet_iskeyframe(&packet)) {
        granulepos = mTheoraGranulepos + 1;
      } else {
        ogg_int64_t frameindex = th_granule_frame(mTheoraState->mCtx,
                                                  mTheoraGranulepos);
        ogg_int64_t granule = frameindex +
                              TheoraVersion(&mTheoraState->mInfo,3,2,1) + 1;
        NS_ASSERTION(granule > 0, "Must have positive granulepos");
        granulepos = granule << shift;
      }

      NS_ASSERTION(th_granule_frame(mTheoraState->mCtx, mTheoraGranulepos) + 1 == 
                   th_granule_frame(mTheoraState->mCtx, granulepos),
                   "Frame number must increment by 1");
      packet.granulepos = mTheoraGranulepos = granulepos;
    }

    PRInt64 time = mTheoraState->StartTime(mTheoraGranulepos);
    NS_ASSERTION(packet.granulepos != -1, "Must know packet granulepos");

    if (!aKeyframeSkip ||
        (th_packet_iskeyframe(&packet) == 1 && time >= aTimeThreshold))
    {
      if (DecodeTheora(frames, &packet) == NS_ERROR_OUT_OF_MEMORY) {
        NS_WARNING("Theora decode memory allocation failure");
        return PR_FALSE;
      }
    }
  }

  
  for (PRUint32 i = 0; i < frames.Length(); i++) {
    nsAutoPtr<VideoData> data(frames[i].forget());
    
    
    if (!mSkeletonState || mSkeletonState->IsPresentable(data->mTime)) {
      if (aKeyframeSkip && data->mKeyframe) {
        aKeyframeSkip = PR_FALSE;
      }
 
      if (!aKeyframeSkip && data->mEndTime >= aTimeThreshold) {
        mVideoQueue.Push(data.forget());
        decoded++;
      }
    }
  }

  if (endOfStream) {
    
    
    mVideoQueue.Finish();
  }

  return !endOfStream;
}

PRInt64 nsOggReader::ReadOggPage(ogg_page* aPage)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on play state machine or decode thread.");
  mMonitor.AssertCurrentThreadIn();

  int ret = 0;
  while((ret = ogg_sync_pageseek(&mOggState, aPage)) <= 0) {
    if (ret < 0) {
      
      mPageOffset += -ret;
      continue;
    }
    
    
    
    char* buffer = ogg_sync_buffer(&mOggState, 4096);
    NS_ASSERTION(buffer, "ogg_sync_buffer failed");

    
    PRUint32 bytesRead = 0;

    nsresult rv = mDecoder->GetCurrentStream()->Read(buffer, 4096, &bytesRead);
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

PRBool nsOggReader::ReadOggPacket(nsOggCodecState* aCodecState,
                                  ogg_packet* aPacket)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on play state machine or decode thread.");
  mMonitor.AssertCurrentThreadIn();

  if (!aCodecState || !aCodecState->mActive) {
    return PR_FALSE;
  }

  int ret = 0;
  while ((ret = ogg_stream_packetout(&aCodecState->mState, aPacket)) != 1) {
    ogg_page page;

    if (aCodecState->PageInFromBuffer()) {
      
      
      continue;
    }

    
    
    if (ReadOggPage(&page) == -1) {
      return PR_FALSE;
    }

    PRUint32 serial = ogg_page_serialno(&page);
    nsOggCodecState* codecState = nsnull;
    mCodecStates.Get(serial, &codecState);

    if (serial == aCodecState->mSerial) {
      
      
      ret = ogg_stream_pagein(&codecState->mState, &page);
      NS_ENSURE_TRUE(ret == 0, PR_FALSE);
    } else if (codecState && codecState->mActive) {
      
      
      
      codecState->AddToBuffer(&page);
    }
  }

  return PR_TRUE;
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

VideoData* nsOggReader::FindStartTime(PRInt64 aOffset,
                                      PRInt64& aOutStartTime)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream != nsnull, nsnull);
  
  
  NS_ASSERTION(mDataOffset > 0, "Must know mDataOffset by now");
  PRInt64 offset = NS_MAX(mDataOffset, aOffset);
  nsresult res = stream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(res, nsnull);
  return nsBuiltinDecoderReader::FindStartTime(offset, aOutStartTime);
}

PRInt64 nsOggReader::FindEndTime(PRInt64 aEndOffset)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  NS_ASSERTION(mDataOffset > 0,
               "Should have offset of first non-header page");
  PRInt64 offset = NS_MAX(mDataOffset, aEndOffset);
  PRInt64 endTime = FindEndTime(mDataOffset, offset, PR_FALSE, &mOggState);
  
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream != nsnull, -1);
  nsresult res = stream->Seek(nsISeekableStream::NS_SEEK_SET, mDataOffset);
  NS_ENSURE_SUCCESS(res, -1);
  return endTime;
}

PRInt64 nsOggReader::FindEndTime(PRInt64 aStartOffset,
                                 PRInt64 aEndOffset,
                                 PRBool aCachedDataOnly,
                                 ogg_sync_state* aState)
{
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  ogg_sync_reset(aState);

  
  
  
  
  
  
  const int step = 5000;
  PRInt64 readStartOffset = aEndOffset;
  PRInt64 readHead = aEndOffset;
  PRInt64 endTime = -1;
  PRUint32 checksumAfterSeek = 0;
  PRUint32 prevChecksumAfterSeek = 0;
  PRBool mustBackOff = PR_FALSE;
  while (PR_TRUE) {
    ogg_page page;    
    int ret = ogg_sync_pageseek(aState, &page);
    if (ret == 0) {
      
      
      if (mustBackOff || readHead == aEndOffset || readHead == aStartOffset) {
        if (endTime != -1 || readStartOffset == 0) {
          
          break;
        }
        mustBackOff = PR_FALSE;
        prevChecksumAfterSeek = checksumAfterSeek;
        checksumAfterSeek = 0;
        ogg_sync_reset(aState);
        readStartOffset = NS_MAX(static_cast<PRInt64>(0), readStartOffset - step);
        readHead = NS_MAX(aStartOffset, readStartOffset);
      }

      PRInt64 limit = NS_MIN(static_cast<PRInt64>(PR_UINT32_MAX),
                             aEndOffset - readHead);
      limit = NS_MAX(static_cast<PRInt64>(0), limit);
      limit = NS_MIN(limit, static_cast<PRInt64>(step));
      PRUint32 bytesToRead = static_cast<PRUint32>(limit);
      PRUint32 bytesRead = 0;
      char* buffer = ogg_sync_buffer(aState, bytesToRead);
      NS_ASSERTION(buffer, "Must have buffer");
      nsresult res;
      if (aCachedDataOnly) {
        res = stream->ReadFromCache(buffer, readHead, bytesToRead);
        NS_ENSURE_SUCCESS(res, -1);
        bytesRead = bytesToRead;
      } else {
        NS_ASSERTION(readHead < aEndOffset,
                     "Stream pos must be before range end");
        res = stream->Seek(nsISeekableStream::NS_SEEK_SET, readHead);
        NS_ENSURE_SUCCESS(res, -1);
        res = stream->Read(buffer, bytesToRead, &bytesRead);
        NS_ENSURE_SUCCESS(res, -1);
      }
      readHead += bytesRead;

      
      
      ret = ogg_sync_wrote(aState, bytesRead);
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
      
      
      
      
      mustBackOff = PR_TRUE;
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

  ogg_sync_reset(aState);

  return endTime;
}

nsresult nsOggReader::GetSeekRanges(nsTArray<SeekRange>& aRanges)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  mMonitor.AssertCurrentThreadIn();
  nsTArray<nsByteRange> cached;
  nsresult res = mDecoder->GetCurrentStream()->GetCachedRanges(cached);
  NS_ENSURE_SUCCESS(res, res);

  for (PRUint32 index = 0; index < aRanges.Length(); index++) {
    nsByteRange& range = cached[index];
    PRInt64 startTime = -1;
    PRInt64 endTime = -1;
    if (NS_FAILED(ResetDecode())) {
      return NS_ERROR_FAILURE;
    }
    
    PRInt64 startOffset = NS_MAX(cached[index].mStart, mDataOffset);
    PRInt64 endOffset = NS_MAX(cached[index].mEnd, mDataOffset);

    FindStartTime(startOffset, startTime);
    if (startTime != -1 &&
        ((endTime = FindEndTime(endOffset)) != -1))
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
                             PRBool aExact)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  PRInt64 so = mDataOffset;
  PRInt64 eo = mDecoder->GetCurrentStream()->GetLength();
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
  return aExact ? SeekRange() : SeekRange(so, eo, st, et);
}

nsOggReader::IndexedSeekResult nsOggReader::RollbackIndexedSeek(PRInt64 aOffset)
{
  mSkeletonState->Deactivate();
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream != nsnull, SEEK_FATAL_ERROR);
  nsresult res = stream->Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  NS_ENSURE_SUCCESS(res, SEEK_FATAL_ERROR);
  return SEEK_INDEX_FAIL;
}
 
nsOggReader::IndexedSeekResult nsOggReader::SeekToKeyframeUsingIndex(PRInt64 aTarget)
{
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream != nsnull, SEEK_FATAL_ERROR);
  if (!HasSkeleton() || !mSkeletonState->HasIndex()) {
    return SEEK_INDEX_FAIL;
  }
  
  nsAutoTArray<PRUint32, 2> tracks;
  if (HasVideo()) {
    tracks.AppendElement(mTheoraState->mSerial);
  }
  if (HasAudio()) {
    tracks.AppendElement(mVorbisState->mSerial);
  }
  nsSkeletonState::nsSeekTarget keyframe;
  if (NS_FAILED(mSkeletonState->IndexedSeekTarget(aTarget,
                                                  tracks,
                                                  keyframe)))
  {
    
    return SEEK_INDEX_FAIL;
  }

  
  PRInt64 tell = stream->Tell();

  
  if (keyframe.mKeyPoint.mOffset > stream->GetLength() ||
      keyframe.mKeyPoint.mOffset < 0)
  {
    
    return RollbackIndexedSeek(tell);
  }
  LOG(PR_LOG_DEBUG, ("Seeking using index to keyframe at offset %lld\n",
                     keyframe.mKeyPoint.mOffset));
  nsresult res = stream->Seek(nsISeekableStream::NS_SEEK_SET,
                              keyframe.mKeyPoint.mOffset);
  NS_ENSURE_SUCCESS(res, SEEK_FATAL_ERROR);
  mPageOffset = keyframe.mKeyPoint.mOffset;

  
  res = ResetDecode();
  NS_ENSURE_SUCCESS(res, SEEK_FATAL_ERROR);

  
  
  ogg_page page;
  int skippedBytes = 0;
  PageSyncResult syncres = PageSync(stream,
                                    &mOggState,
                                    PR_FALSE,
                                    mPageOffset,
                                    stream->GetLength(),
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
                                          PRInt64 aStartTime,
                                          PRInt64 aEndTime,
                                          const nsTArray<SeekRange>& aRanges,
                                          const SeekRange& aRange)
{
  LOG(PR_LOG_DEBUG, ("%p Seeking in buffered data to %lldms using bisection search", mDecoder, aTarget));

  
  
  nsresult res = SeekBisection(aTarget, aRange, 0);
  if (NS_FAILED(res) || !HasVideo()) {
    return res;
  }

  
  
  PRBool eof;
  do {
    PRBool skip = PR_FALSE;
    eof = !DecodeVideoFrame(skip, 0);
    {
      MonitorAutoExit exitReaderMon(mMonitor);
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
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
    SeekRange k = SelectSeekRange(aRanges,
                                  keyframeTime,
                                  aStartTime,
                                  aEndTime,
                                  PR_FALSE);
    res = SeekBisection(keyframeTime, k, SEEK_FUZZ_USECS);
    NS_ASSERTION(mTheoraGranulepos == -1, "SeekBisection must reset Theora decode");
    NS_ASSERTION(mVorbisGranulepos == -1, "SeekBisection must reset Vorbis decode");
  }
  return res;
}

PRBool nsOggReader::CanDecodeToTarget(PRInt64 aTarget,
                                      PRInt64 aCurrentTime)
{
  
  
  
  PRInt64 margin = HasVideo() ? mTheoraState->MaxKeyframeOffset() : SEEK_DECODE_MARGIN;
  return aTarget >= aCurrentTime &&
         aTarget - aCurrentTime < margin;
}

nsresult nsOggReader::SeekInUnbuffered(PRInt64 aTarget,
                                       PRInt64 aStartTime,
                                       PRInt64 aEndTime,
                                       const nsTArray<SeekRange>& aRanges)
{
  LOG(PR_LOG_DEBUG, ("%p Seeking in unbuffered data to %lldms using bisection search", mDecoder, aTarget));
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRInt64 keyframeOffsetMs = 0;
  if (HasVideo() && mTheoraState) {
    keyframeOffsetMs = mTheoraState->MaxKeyframeOffset();
  }
  PRInt64 seekTarget = NS_MAX(aStartTime, aTarget - keyframeOffsetMs);
  
  
  SeekRange k = SelectSeekRange(aRanges, seekTarget, aStartTime, aEndTime, PR_FALSE);
  nsresult res = SeekBisection(seekTarget, k, SEEK_FUZZ_USECS);
  NS_ASSERTION(mTheoraGranulepos == -1, "SeekBisection must reset Theora decode");
  NS_ASSERTION(mVorbisGranulepos == -1, "SeekBisection must reset Vorbis decode");
  return res;
}

nsresult nsOggReader::Seek(PRInt64 aTarget,
                           PRInt64 aStartTime,
                           PRInt64 aEndTime,
                           PRInt64 aCurrentTime)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  LOG(PR_LOG_DEBUG, ("%p About to seek to %lldms", mDecoder, aTarget));
  nsresult res;
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream != nsnull, NS_ERROR_FAILURE);

  if (aTarget == aStartTime) {
    
    
    res = stream->Seek(nsISeekableStream::NS_SEEK_SET, mDataOffset);
    NS_ENSURE_SUCCESS(res,res);

    mPageOffset = mDataOffset;
    res = ResetDecode();
    NS_ENSURE_SUCCESS(res,res);

    NS_ASSERTION(aStartTime != -1, "mStartTime should be known");
    {
      MonitorAutoExit exitReaderMon(mMonitor);
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      mDecoder->UpdatePlaybackPosition(aStartTime);
    }
  } else if (CanDecodeToTarget(aTarget, aCurrentTime)) {
    LOG(PR_LOG_DEBUG, ("%p Seek target (%lld) is close to current time (%lld), "
        "will just decode to it", mDecoder, aCurrentTime, aTarget));
  } else {
    IndexedSeekResult sres = SeekToKeyframeUsingIndex(aTarget);
    NS_ENSURE_TRUE(sres != SEEK_FATAL_ERROR, NS_ERROR_FAILURE);
    if (sres == SEEK_INDEX_FAIL) {
      
      
      
      nsAutoTArray<SeekRange, 16> ranges;
      res = GetSeekRanges(ranges);
      NS_ENSURE_SUCCESS(res,res);

      
      SeekRange r = SelectSeekRange(ranges, aTarget, aStartTime, aEndTime, PR_TRUE);

      if (!r.IsNull()) {
        
        
        res = SeekInBufferedRange(aTarget, aStartTime, aEndTime, ranges, r);
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
PageSync(nsMediaStream* aStream,
         ogg_sync_state* aState,
         PRBool aCachedDataOnly,
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

      
      PRUint32 bytesToRead =
        static_cast<PRUint32>(NS_MIN(static_cast<PRInt64>(PAGE_STEP),
                                     aEndOffset - readHead));
      if (bytesToRead <= 0) {
        return PAGE_SYNC_END_OF_RANGE;
      }
      nsresult rv = NS_OK;
      if (aCachedDataOnly) {
        rv = aStream->ReadFromCache(buffer, readHead, bytesToRead);
        NS_ENSURE_SUCCESS(rv,PAGE_SYNC_ERROR);
        bytesRead = bytesToRead;
      } else {
        rv = aStream->Seek(nsISeekableStream::NS_SEEK_SET, readHead);
        NS_ENSURE_SUCCESS(rv,PAGE_SYNC_ERROR);
        rv = aStream->Read(buffer,
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
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  nsresult res;
  nsMediaStream* stream = mDecoder->GetCurrentStream();

  if (aTarget == aRange.mTimeStart) {
    if (NS_FAILED(ResetDecode())) {
      return NS_ERROR_FAILURE;
    }
    res = stream->Seek(nsISeekableStream::NS_SEEK_SET, mDataOffset);
    NS_ENSURE_SUCCESS(res,res);
    mPageOffset = mDataOffset;
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
  ogg_int64_t previousGuess = -1;
  int backsteps = 0;
  const int maxBackStep = 10;
  NS_ASSERTION(static_cast<PRUint64>(PAGE_STEP) * pow(2.0, maxBackStep) < PR_INT32_MAX,
               "Backstep calculation must not overflow");

  
  
  
  while (PR_TRUE) {
    ogg_int64_t duration = 0;
    double target = 0;
    ogg_int64_t interval = 0;
    ogg_int64_t guess = 0;
    ogg_page page;
    int skippedBytes = 0;
    ogg_int64_t pageOffset = 0;
    ogg_int64_t pageLength = 0;
    ogg_int64_t granuleTime = -1;
    PRBool mustBackoff = PR_FALSE;

    
    
    
    while (PR_TRUE) {
  
      
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
        backsteps = NS_MIN(backsteps + 1, maxBackStep);
        
        
        mustBackoff = PR_FALSE;
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
      NS_ASSERTION(guess != previousGuess, "Guess should be differnt to previous");
      previousGuess = guess;

      hops++;
    
      
      
      
      PageSyncResult res = PageSync(stream,
                                    &mOggState,
                                    PR_FALSE,
                                    guess,
                                    endOffset,
                                    &page,
                                    skippedBytes);
      NS_ENSURE_TRUE(res != PAGE_SYNC_ERROR, NS_ERROR_FAILURE);

      
      
      pageOffset = guess + skippedBytes;
      pageLength = page.header_len + page.body_len;
      mPageOffset = pageOffset + pageLength;

      if (res == PAGE_SYNC_END_OF_RANGE) {
        
        
        
        mustBackoff = PR_TRUE;
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

        if (HasAudio() &&
            granulepos != -1 &&
            serial == mVorbisState->mSerial &&
            audioTime == -1) {
          audioTime = mVorbisState->Time(granulepos);
        }
        
        if (HasVideo() &&
            granulepos != -1 &&
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
        
      } while ((mVorbisState && audioTime == -1) ||
               (mTheoraState && videoTime == -1));

      NS_ASSERTION(mPageOffset <= endOffset, "Page read cursor should be inside range");

      if ((HasAudio() && audioTime == -1) ||
          (HasVideo() && videoTime == -1)) 
      {
        
        if (pageOffset == startOffset + startLength && mPageOffset == endOffset) {
          
          
          
          
          
          interval = 0;
          break;
        }

        
        
        mustBackoff = PR_TRUE;
        continue;
      }

      
      
      granuleTime = NS_MAX(audioTime, videoTime);
      NS_ASSERTION(granuleTime > 0, "Must get a granuletime");
      break;
    } 

    if (interval == 0) {
      
      
      SEEK_LOG(PR_LOG_DEBUG, ("Terminating seek at offset=%lld", startOffset));
      NS_ASSERTION(startTime < aTarget, "Start time must always be less than target");
      res = stream->Seek(nsISeekableStream::NS_SEEK_SET, startOffset);
      NS_ENSURE_SUCCESS(res,res);
      mPageOffset = startOffset;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      break;
    }

    SEEK_LOG(PR_LOG_DEBUG, ("Time at offset %lld is %lldms", guess, granuleTime));
    if (granuleTime < seekTarget && granuleTime > seekLowerBound) {
      
      res = stream->Seek(nsISeekableStream::NS_SEEK_SET, pageOffset);
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
      
      NS_ASSERTION(pageOffset > startOffset, "offset_start must increase");
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

  nsMediaStream* stream = mDecoder->GetCurrentStream();
  nsTArray<nsByteRange> ranges;
  nsresult res = stream->GetCachedRanges(ranges);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  
  
  ogg_sync_state state;
  ogg_sync_init(&state);
  for (PRUint32 index = 0; index < ranges.Length(); index++) {
    
    PRInt64 startOffset = NS_MAX(ranges[index].mStart, mDataOffset);
    PRInt64 endOffset = NS_MAX(ranges[index].mEnd, mDataOffset);

    
    
    
    
    PRInt64 startTime = (startOffset == mDataOffset) ? aStartTime : -1;

    
    
    
    ogg_sync_reset(&state);
    while (startTime == -1) {
      ogg_page page;
      PRInt32 discard;
      PageSyncResult res = PageSync(stream,
                                    &state,
                                    PR_TRUE,
                                    startOffset,
                                    endOffset,
                                    &page,
                                    discard);
      if (res == PAGE_SYNC_ERROR) {
        
        ogg_sync_clear(&state);
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
        startTime = nsVorbisState::Time(&mVorbisInfo, granulepos) - aStartTime;
        NS_ASSERTION(startTime > 0, "Must have positive start time");
      }
      else if (mTheoraState && serial == mTheoraSerial) {
        startTime = nsTheoraState::Time(&mTheoraInfo, granulepos) - aStartTime;
        NS_ASSERTION(startTime > 0, "Must have positive start time");
      }
      else if (IsKnownStream(serial)) {
        
        
        startOffset += page.header_len + page.body_len;
        continue;
      }
      else {
        
        
        ogg_sync_clear(&state);
        return PAGE_SYNC_ERROR;
      }
    }

    if (startTime != -1) {
      
      
      PRInt64 endTime = FindEndTime(startOffset, endOffset, PR_TRUE, &state);
      if (endTime != -1) {
        aBuffered->Add(startTime / static_cast<double>(USECS_PER_S),
                       (endTime - aStartTime) / static_cast<double>(USECS_PER_S));
      }
    }
  }

  
  ogg_sync_clear(&state);

  return NS_OK;
}

PRBool nsOggReader::IsKnownStream(PRUint32 aSerial)
{
  for (PRUint32 i = 0; i < mKnownStreams.Length(); i++) {
    PRUint32 serial = mKnownStreams[i];
    if (serial == aSerial) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}
