





































#include "nsError.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsBuiltinDecoder.h"
#include "nsOggReader.h"
#include "VideoUtils.h"
#include "theora/theoradec.h"

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



static const int PAGE_STEP = 8192;

nsOggReader::nsOggReader(nsBuiltinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder),
    mTheoraState(nsnull),
    mVorbisState(nsnull),
    mPageOffset(0),
    mCallbackPeriod(0),
    mTheoraGranulepos(-1),
    mVorbisGranulepos(-1)
{
  MOZ_COUNT_CTOR(nsOggReader);
}

nsOggReader::~nsOggReader()
{
  ogg_sync_clear(&mOggState);
  MOZ_COUNT_DTOR(nsOggReader);
}

nsresult nsOggReader::Init() {
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


nsresult nsOggReader::ReadMetadata(nsVideoInfo& aInfo)
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
      PRBool r = mCodecStates.Put(serial, codecState);
      NS_ASSERTION(r, "Failed to insert into mCodecStates");
      bitstreams.AppendElement(codecState);
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
    if (s != mVorbisState && s != mTheoraState) {
      s->Deactivate();
    }
  }

  
  
  
  float aspectRatio = 0;
  if (mTheoraState) {
    if (mTheoraState->Init()) {
      mCallbackPeriod = mTheoraState->mFrameDuration;
      aspectRatio = mTheoraState->mAspectRatio;
      gfxIntSize sz(mTheoraState->mInfo.pic_width,
                    mTheoraState->mInfo.pic_height);
      mDecoder->SetVideoData(sz, mTheoraState->mAspectRatio, nsnull);
    } else {
      mTheoraState = nsnull;
    }
  }
  if (mVorbisState) {
    mVorbisState->Init();
  }

  aInfo.mHasAudio = HasAudio();
  aInfo.mHasVideo = HasVideo();
  aInfo.mCallbackPeriod = mCallbackPeriod;
  if (HasAudio()) {
    aInfo.mAudioRate = mVorbisState->mInfo.rate;
    aInfo.mAudioChannels = mVorbisState->mInfo.channels;
  }
  if (HasVideo()) {
    aInfo.mFramerate = mTheoraState->mFrameRate;
    aInfo.mAspectRatio = mTheoraState->mAspectRatio;
    aInfo.mPicture.width = mTheoraState->mInfo.pic_width;
    aInfo.mPicture.height = mTheoraState->mInfo.pic_height;
    aInfo.mPicture.x = mTheoraState->mInfo.pic_x;
    aInfo.mPicture.y = mTheoraState->mInfo.pic_y;
    aInfo.mFrame.width = mTheoraState->mInfo.frame_width;
    aInfo.mFrame.height = mTheoraState->mInfo.frame_height;
  }
  aInfo.mDataOffset = mDataOffset;

  LOG(PR_LOG_DEBUG, ("Done loading headers, data offset %lld", mDataOffset));

  return NS_OK;
}

nsresult nsOggReader::DecodeVorbis(nsTArray<SoundData*>& aChunks,
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

  float** pcm = 0;
  PRUint32 samples = 0;
  PRUint32 channels = mVorbisState->mInfo.channels;
  while ((samples = vorbis_synthesis_pcmout(&mVorbisState->mDsp, &pcm)) > 0) {
    if (samples > 0) {
      float* buffer = new float[samples * channels];
      float* p = buffer;
      for (PRUint32 i = 0; i < samples; ++i) {
        for (PRUint32 j = 0; j < channels; ++j) {
          *p++ = pcm[j][i];
        }
      }

      PRInt64 duration = mVorbisState->Time((PRInt64)samples);
      PRInt64 startTime = (mVorbisGranulepos != -1) ?
        mVorbisState->Time(mVorbisGranulepos) : -1;
      SoundData* s = new SoundData(mPageOffset,
                                   startTime,
                                   duration,
                                   samples,
                                   buffer,
                                   channels);
      if (mVorbisGranulepos != -1) {
        mVorbisGranulepos += samples;
      }
      aChunks.AppendElement(s);
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

  nsAutoTArray<SoundData*, 64> chunks;
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
    mAudioQueue.Push(chunks[i]);
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
AllFrameTimesIncrease(nsTArray<VideoData*>& aFrames)
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

static void Clear(nsTArray<VideoData*>& aFrames) {
  for (PRUint32 i = 0; i < aFrames.Length(); ++i) {
    delete aFrames[i];
  }
  aFrames.Clear();
}

nsresult nsOggReader::DecodeTheora(nsTArray<VideoData*>& aFrames,
                                   ogg_packet* aPacket)
{
  int ret = th_decode_packetin(mTheoraState->mCtx, aPacket, 0);
  if (ret != 0 && ret != TH_DUPFRAME) {
    return NS_ERROR_FAILURE;
  }
  PRInt64 time = (aPacket->granulepos != -1)
    ? mTheoraState->StartTime(aPacket->granulepos) : -1;
  if (ret == TH_DUPFRAME) {
    aFrames.AppendElement(VideoData::CreateDuplicate(mPageOffset,
                                                     time,
                                                     aPacket->granulepos));
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
    VideoData *v = VideoData::Create(mPageOffset,
                                     time,
                                     b,
                                     isKeyframe,
                                     aPacket->granulepos);
    if (!v) {
      NS_WARNING("Failed to allocate memory for video frame");
      Clear(aFrames);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aFrames.AppendElement(v);
  }
  return NS_OK;
}

PRBool nsOggReader::DecodeVideoFrame(PRBool &aKeyframeSkip,
                                     PRInt64 aTimeThreshold)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or AV thread.");
  
  
  
  
  
  

  nsAutoTArray<VideoData*, 8> frames;
  ogg_packet packet;
  PRBool endOfStream = PR_FALSE;
  if (mTheoraGranulepos == -1) {
    
    
    
    
    
    do {
      if (!ReadOggPacket(mTheoraState, &packet)) {
        
        
        
        
        
        mVideoQueue.Finish();
        return PR_FALSE;
      }

      if (packet.granulepos > 0) {
        
        
        
        mTheoraGranulepos = packet.granulepos;
      }
    
      if (DecodeTheora(frames, &packet) == NS_ERROR_OUT_OF_MEMORY) {
        NS_WARNING("Theora decode memory allocation failure!");
        return PR_FALSE;
      }

    } while (packet.granulepos <= 0 && !endOfStream);

    if (packet.granulepos > 0) {
      
      
      PRInt64 succGranulepos = packet.granulepos;
      int version_3_2_1 = TheoraVersion(&mTheoraState->mInfo,3,2,1);
      int shift = mTheoraState->mInfo.keyframe_granule_shift;
      for (int i = frames.Length() - 2; i >= 0; --i) {
        PRInt64 granulepos = succGranulepos;
        if (frames[i]->mKeyframe) {
          
          
          ogg_int64_t frame_index = th_granule_frame(mTheoraState->mCtx,
                                                     granulepos);
          granulepos = (frame_index + version_3_2_1 - 1) << shift;
          
          
          
          
          NS_ASSERTION((version_3_2_1 && granulepos > 0) ||
                       granulepos >= 0, "Should have positive granulepos");
        } else {
          
          
          if (frames[i+1]->mKeyframe) {
            
            
            
            
            
            
            
            ogg_int64_t frameno = th_granule_frame(mTheoraState->mCtx,
                                                   granulepos);
            ogg_int64_t max_offset = NS_MIN((frameno - 1),
                                         (ogg_int64_t)(1 << shift) - 1);
            ogg_int64_t granule = frameno +
                                  TheoraVersion(&mTheoraState->mInfo,3,2,1) -
                                  1 - max_offset;
            NS_ASSERTION(granule > 0, "Must have positive granulepos");
            granulepos = (granule << shift) + max_offset;
          } else {
            
            
            
            --granulepos;
          }
        }
        
        
        NS_ASSERTION(th_granule_frame(mTheoraState->mCtx, succGranulepos) ==
                     th_granule_frame(mTheoraState->mCtx, granulepos) + 1,
                     "Granulepos calculation is incorrect!");
        frames[i]->mTime = mTheoraState->StartTime(granulepos);
        frames[i]->mTimecode = granulepos;
        succGranulepos = granulepos;
        NS_ASSERTION(frames[i]->mTime < frames[i+1]->mTime, "Times should increase");      
      }
      NS_ASSERTION(AllFrameTimesIncrease(frames), "All frames must have granulepos");
    }
  } else {
    
    NS_ASSERTION(mTheoraGranulepos > 0, "We must Theora granulepos!");
    
    if (!ReadOggPacket(mTheoraState, &packet)) {
      
      
      mVideoQueue.Finish();
      return PR_FALSE;
    }

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
    nsAutoPtr<VideoData> data(frames[i]);
    if (!aKeyframeSkip || (aKeyframeSkip && frames[i]->mKeyframe)) {
      mVideoQueue.Push(data.forget());
      if (aKeyframeSkip && frames[i]->mKeyframe) {
        aKeyframeSkip = PR_FALSE;
      }
    } else {
      frames[i] = nsnull;
      data = nsnull;
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

PRInt64 nsOggReader::FindEndTime(PRInt64 aEndOffset)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(), "Should be on state machine thread.");

  nsMediaStream* stream = mDecoder->GetCurrentStream();
  ogg_sync_reset(&mOggState);

  stream->Seek(nsISeekableStream::NS_SEEK_SET, aEndOffset);

  
  
  
  
  
  
  const int step = 5000;
  PRInt64 offset = aEndOffset;
  PRInt64 endTime = -1;
  PRUint32 checksumAfterSeek = 0;
  PRUint32 prevChecksumAfterSeek = 0;
  PRBool mustBackOff = PR_FALSE;
  while (PR_TRUE) {
    {
      MonitorAutoExit exitReaderMon(mMonitor);
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return -1;
      }
    }
    ogg_page page;    
    int ret = ogg_sync_pageseek(&mOggState, &page);
    if (ret == 0) {
      
      
      if (mustBackOff || stream->Tell() == aEndOffset) {
        if (endTime != -1) {
          
          break;
        }
        mustBackOff = PR_FALSE;
        prevChecksumAfterSeek = checksumAfterSeek;
        checksumAfterSeek = 0;
        ogg_sync_reset(&mOggState);
        offset = NS_MAX(static_cast<PRInt64>(0), offset - step);
        stream->Seek(nsISeekableStream::NS_SEEK_SET, offset);
      }
      NS_ASSERTION(stream->Tell() < aEndOffset,
                   "Stream pos must be before range end");

      PRInt64 limit = NS_MIN(static_cast<PRInt64>(PR_UINT32_MAX),
                             aEndOffset - stream->Tell());
      limit = NS_MAX(static_cast<PRInt64>(0), limit);
      limit = NS_MIN(limit, static_cast<PRInt64>(step));
      PRUint32 bytesToRead = static_cast<PRUint32>(limit);
      PRUint32 bytesRead = 0;
      char* buffer = ogg_sync_buffer(&mOggState,
                                     bytesToRead);
      NS_ASSERTION(buffer, "Must have buffer");
      stream->Read(buffer, bytesToRead, &bytesRead);

      
      
      ret = ogg_sync_wrote(&mOggState, bytesRead);
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
      
      
      
      break;
    }

    PRInt64 t = codecState ? codecState->Time(granulepos) : -1;
    if (t != -1) {
      endTime = t;
    }
  }

  ogg_sync_reset(&mOggState);

  return endTime;
}

nsresult nsOggReader::Seek(PRInt64 aTarget, PRInt64 aStartTime, PRInt64 aEndTime)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  LOG(PR_LOG_DEBUG, ("%p About to seek to %lldms", mDecoder, aTarget));
  nsMediaStream* stream = mDecoder->GetCurrentStream();

  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  if (aTarget == aStartTime) {
    stream->Seek(nsISeekableStream::NS_SEEK_SET, mDataOffset);
    mPageOffset = mDataOffset;
    NS_ASSERTION(aStartTime != -1, "mStartTime should be known");
    {
      MonitorAutoExit exitReaderMon(mMonitor);
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      mDecoder->UpdatePlaybackPosition(aStartTime);
    }
  } else {

    
    nsAutoTArray<ByteRange, 16> ranges;
    stream->Pin();
    if (NS_FAILED(GetBufferedBytes(ranges))) {
      stream->Unpin();
      return NS_ERROR_FAILURE;
    }

    
    
    
    ByteRange r = GetSeekRange(ranges, aTarget, aStartTime, aEndTime, PR_TRUE);
    nsresult res = NS_ERROR_FAILURE;
    if (!r.IsNull()) {
      
      res = SeekBisection(aTarget, r, 0);

      if (NS_SUCCEEDED(res) && HasVideo()) {
        
        
        PRBool eof;
        do {
          PRBool skip = PR_FALSE;
          eof = !DecodeVideoFrame(skip, 0);
          {
            MonitorAutoExit exitReaderMon(mMonitor);
            MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
            if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
              stream->Unpin();
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
          ByteRange k = GetSeekRange(ranges,
                                     keyframeTime,
                                     aStartTime,
                                     aEndTime,
                                     PR_FALSE);
          res = SeekBisection(keyframeTime, k, 500);
          NS_ASSERTION(mTheoraGranulepos == -1, "SeekBisection must reset Theora decode");
          NS_ASSERTION(mVorbisGranulepos == -1, "SeekBisection must reset Vorbis decode");
        }
      }
    }

    stream->Unpin();

    if (NS_FAILED(res)) {
      
      
      

      
      
      
      
      
      
      
      
      
      
      
      PRInt64 keyframeOffsetMs = 0;
      if (HasVideo() && mTheoraState) {
        keyframeOffsetMs = mTheoraState->MaxKeyframeOffset();
      }
      PRInt64 seekTarget = NS_MAX(aStartTime, aTarget - keyframeOffsetMs);

      ByteRange k = GetSeekRange(ranges, seekTarget, aStartTime, aEndTime, PR_FALSE);
      res = SeekBisection(seekTarget, k, 500);

      NS_ENSURE_SUCCESS(res, res);
      NS_ASSERTION(mTheoraGranulepos == -1, "SeekBisection must reset Theora decode");
      NS_ASSERTION(mVorbisGranulepos == -1, "SeekBisection must reset Vorbis decode");
    }
  }

  
  
  if (HasVideo()) {
    nsAutoPtr<VideoData> video;
    PRBool eof = PR_FALSE;
    PRInt64 startTime = -1;
    video = nsnull;
    while (HasVideo() && !eof) {
      while (mVideoQueue.GetSize() == 0 && !eof) {
        PRBool skip = PR_FALSE;
        eof = !DecodeVideoFrame(skip, 0);
        {
          MonitorAutoExit exitReaderMon(mMonitor);
          MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
          if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      if (mVideoQueue.GetSize() == 0) {
        break;
      }
      video = mVideoQueue.PeekFront();
      
      
      if (video && video->mTime + mCallbackPeriod < aTarget) {
        if (startTime == -1) {
          startTime = video->mTime;
        }
        mVideoQueue.PopFront();
        video = nsnull;
      } else {
        video.forget();
        break;
      }
    }
    {
      MonitorAutoExit exitReaderMon(mMonitor);
      MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
      if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
        return NS_ERROR_FAILURE;
      }
    }
    SEEK_LOG(PR_LOG_DEBUG, ("First video frame after decode is %lld", startTime));
  }

  if (HasAudio()) {
    
    nsAutoPtr<SoundData> audio;
    bool eof = PR_FALSE;
    while (HasAudio() && !eof) {
      while (!eof && mAudioQueue.GetSize() == 0) {
        eof = !DecodeAudioData();
        {
          MonitorAutoExit exitReaderMon(mMonitor);
          MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
          if (mDecoder->GetDecodeState() == nsBuiltinDecoderStateMachine::DECODER_STATE_SHUTDOWN) {
            return NS_ERROR_FAILURE;
          }
        }
      }
      audio = mAudioQueue.PeekFront();
      if (audio && audio->mTime + audio->mDuration <= aTarget) {
        mAudioQueue.PopFront();
        audio = nsnull;
      } else {
        audio.forget();
        break;
      }
    }
  }

  return NS_OK;
}

enum PageSyncResult {
  PAGE_SYNC_ERROR = 1,
  PAGE_SYNC_END_OF_RANGE= 2,
  PAGE_SYNC_OK = 3
};


static PageSyncResult
PageSync(ogg_sync_state* aState,
         nsMediaStream* aStream,
         PRInt64 aEndOffset,
         ogg_page* aPage,
         int& aSkippedBytes)
{
  aSkippedBytes = 0;
  
  int ret = 0;
  PRUint32 bytesRead = 0;
  while (ret <= 0) {
    ret = ogg_sync_pageseek(aState, aPage);
    if (ret == 0) {
      char* buffer = ogg_sync_buffer(aState, PAGE_STEP);
      NS_ASSERTION(buffer, "Must have a buffer");

      
      PRInt64 bytesToRead = NS_MIN(static_cast<PRInt64>(PAGE_STEP),
                                   aEndOffset - aStream->Tell());
      if (bytesToRead <= 0) {
        return PAGE_SYNC_END_OF_RANGE;
      }
      nsresult rv = aStream->Read(buffer,
                                  static_cast<PRUint32>(bytesToRead),
                                  &bytesRead);
      if (NS_FAILED(rv)) {
        return PAGE_SYNC_ERROR;
      }

      if (bytesRead == 0 && NS_SUCCEEDED(rv)) {
        
        return PAGE_SYNC_END_OF_RANGE;
      }

      
      
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
                                    const ByteRange& aRange,
                                    PRUint32 aFuzz)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  nsMediaStream* stream = mDecoder->GetCurrentStream();

  if (aTarget == aRange.mTimeStart) {
    if (NS_FAILED(ResetDecode())) {
      return NS_ERROR_FAILURE;
    }
    stream->Seek(nsISeekableStream::NS_SEEK_SET, mDataOffset);
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
  int backsteps = 1;
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
    int backoff = 0;
    ogg_int64_t granuleTime = -1;
    PRInt64 oldPageOffset = 0;

    
    
    while (PR_TRUE) {
  
      
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }

      
      duration = endTime - startTime;
      target = (double)(seekTarget - startTime) / (double)duration;
      interval = endOffset - startOffset - startLength;
      guess = startOffset + startLength +
              (ogg_int64_t)((double)interval * target) - backoff;
      guess = NS_MIN(guess, endOffset - PAGE_STEP);
      guess = NS_MAX(guess, startOffset + startLength);

      if (interval == 0 || guess == previousGuess) {
        interval = 0;
        
        
        
        break;
      }

      NS_ASSERTION(guess >= startOffset + startLength, "Guess must be after range start");
      NS_ASSERTION(guess < endOffset, "Guess must be before range end");
      NS_ASSERTION(guess != previousGuess, "Guess should be differnt to previous");
      previousGuess = guess;

      SEEK_LOG(PR_LOG_DEBUG, ("Seek loop offset_start=%lld start_end=%lld "
                              "offset_guess=%lld offset_end=%lld interval=%lld "
                              "target=%lf time_start=%lld time_end=%lld",
                              startOffset, (startOffset+startLength), guess,
                              endOffset, interval, target, startTime, endTime));
      hops++;
      stream->Seek(nsISeekableStream::NS_SEEK_SET, guess);
    
      
      
      
      
      PageSyncResult res = PageSync(&mOggState,
                                    stream,
                                    endOffset,
                                    &page,
                                    skippedBytes);
      if (res == PAGE_SYNC_ERROR) {
        return NS_ERROR_FAILURE;
      }

      
      
      pageOffset = guess + skippedBytes;
      pageLength = page.header_len + page.body_len;
      mPageOffset = pageOffset + pageLength;

      if (mPageOffset == endOffset || res == PAGE_SYNC_END_OF_RANGE) {
        
        
        
        backsteps = NS_MIN(backsteps + 1, maxBackStep);
        backoff = PAGE_STEP * pow(2.0, backsteps);
        continue;
      }

      NS_ASSERTION(mPageOffset < endOffset, "Page read cursor should be inside range");

      
      
      ogg_int64_t audioTime = -1;
      ogg_int64_t videoTime = -1;
      int ret;
      oldPageOffset = mPageOffset;
      while ((mVorbisState && audioTime == -1) ||
             (mTheoraState && videoTime == -1)) {
      
        
        PRUint32 serial = ogg_page_serialno(&page);
        nsOggCodecState* codecState = nsnull;
        mCodecStates.Get(serial, &codecState);
        if (codecState && codecState->mActive) {
          ret = ogg_stream_pagein(&codecState->mState, &page);
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

        mPageOffset += page.header_len + page.body_len;
        if (ReadOggPage(&page) == -1) {
          break;
        }
      }

      if ((HasAudio() && audioTime == -1) ||
          (HasVideo() && videoTime == -1)) 
      {
        backsteps = NS_MIN(backsteps + 1, maxBackStep);
        backoff = PAGE_STEP * pow(2.0, backsteps);
        continue;
      }

      
      
      granuleTime = NS_MAX(audioTime, videoTime);
      NS_ASSERTION(granuleTime > 0, "Must get a granuletime");
      break;
    }

    if (interval == 0) {
      
      
      SEEK_LOG(PR_LOG_DEBUG, ("Seek loop (interval == 0) break"));
      NS_ASSERTION(startTime < aTarget, "Start time must always be less than target");
      stream->Seek(nsISeekableStream::NS_SEEK_SET, startOffset);
      mPageOffset = startOffset;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      break;
    }

    SEEK_LOG(PR_LOG_DEBUG, ("Time at offset %lld is %lldms", guess, granuleTime));
    if (granuleTime < seekTarget && granuleTime > seekLowerBound) {
      
      stream->Seek(nsISeekableStream::NS_SEEK_SET, oldPageOffset);
      mPageOffset = oldPageOffset;
      if (NS_FAILED(ResetDecode())) {
        return NS_ERROR_FAILURE;
      }
      break;
    }

    if (granuleTime >= seekTarget) {
      
      ogg_int64_t old_offset_end = endOffset;
      endOffset = pageOffset;
      NS_ASSERTION(endOffset < old_offset_end, "offset_end must decrease");
      endTime = granuleTime;
    } else if (granuleTime < seekTarget) {
      
      ogg_int64_t old_offset_start = startOffset;
      startOffset = pageOffset;
      startLength = pageLength;
      NS_ASSERTION(startOffset > old_offset_start, "offset_start must increase");
      startTime = granuleTime;
    }
    NS_ASSERTION(startTime < seekTarget, "Must be before seek target");
    NS_ASSERTION(endTime >= seekTarget, "End must be after seek target");
  }

  SEEK_LOG(PR_LOG_DEBUG, ("Seek complete in %d bisections.", hops));

  return NS_OK;
}


