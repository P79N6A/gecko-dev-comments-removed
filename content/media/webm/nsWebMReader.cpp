





































#include "nsError.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsBuiltinDecoder.h"
#include "nsMediaStream.h"
#include "nsWebMReader.h"
#include "VideoUtils.h"
#include "nsTimeRanges.h"

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

static const unsigned NS_PER_MS = 1000000;
static const float NS_PER_S = 1e9;
static const float MS_PER_S = 1e3;




static int webm_read(void *aBuffer, size_t aLength, void *aUserData)
{
  NS_ASSERTION(aUserData, "aUserData must point to a valid nsBuiltinDecoder");
  nsBuiltinDecoder* decoder = reinterpret_cast<nsBuiltinDecoder*>(aUserData);
  nsMediaStream* stream = decoder->GetCurrentStream();
  NS_ASSERTION(stream, "Decoder has no media stream");

  nsresult rv = NS_OK;
  PRBool eof = PR_FALSE;

  char *p = static_cast<char *>(aBuffer);
  while (NS_SUCCEEDED(rv) && aLength > 0) {
    PRUint32 bytes = 0;
    rv = stream->Read(p, aLength, &bytes);
    if (bytes == 0) {
      eof = PR_TRUE;
      break;
    }
    decoder->NotifyBytesConsumed(bytes);
    aLength -= bytes;
    p += bytes;
  }

  return NS_FAILED(rv) ? -1 : eof ? 0 : 1;
}

static int webm_seek(int64_t aOffset, int aWhence, void *aUserData)
{
  NS_ASSERTION(aUserData, "aUserData must point to a valid nsBuiltinDecoder");
  nsBuiltinDecoder* decoder = reinterpret_cast<nsBuiltinDecoder*>(aUserData);
  nsMediaStream* stream = decoder->GetCurrentStream();
  NS_ASSERTION(stream, "Decoder has no media stream");
  nsresult rv = stream->Seek(aWhence, aOffset);
  return NS_SUCCEEDED(rv) ? 0 : -1;
}

static int64_t webm_tell(void *aUserData)
{
  NS_ASSERTION(aUserData, "aUserData must point to a valid nsBuiltinDecoder");
  nsBuiltinDecoder* decoder = reinterpret_cast<nsBuiltinDecoder*>(aUserData);
  nsMediaStream* stream = decoder->GetCurrentStream();
  NS_ASSERTION(stream, "Decoder has no media stream");
  return stream->Tell();
}

nsWebMReader::nsWebMReader(nsBuiltinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder),
  mContext(nsnull),
  mPacketCount(0),
  mChannels(0),
  mVideoTrack(0),
  mAudioTrack(0),
  mAudioStartMs(-1),
  mAudioSamples(0),
  mTimecodeScale(1000000),
  mHasVideo(PR_FALSE),
  mHasAudio(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsWebMReader);
}

nsWebMReader::~nsWebMReader()
{
  Cleanup();

  mVideoPackets.Reset();
  mAudioPackets.Reset();

  vpx_codec_destroy(&mVP8);

  vorbis_block_clear(&mVorbisBlock);
  vorbis_dsp_clear(&mVorbisDsp);
  vorbis_info_clear(&mVorbisInfo);
  vorbis_comment_clear(&mVorbisComment);

  MOZ_COUNT_DTOR(nsWebMReader);
}

nsresult nsWebMReader::Init()
{
  if (vpx_codec_dec_init(&mVP8, &vpx_codec_vp8_dx_algo, NULL, 0)) {
    return NS_ERROR_FAILURE;
  }

  vorbis_info_init(&mVorbisInfo);
  vorbis_comment_init(&mVorbisComment);
  memset(&mVorbisDsp, 0, sizeof(vorbis_dsp_state));
  memset(&mVorbisBlock, 0, sizeof(vorbis_block));

  return NS_OK;
}

nsresult nsWebMReader::ResetDecode()
{
  mAudioSamples = 0;
  mAudioStartMs = -1;
  nsresult res = NS_OK;
  if (NS_FAILED(nsBuiltinDecoderReader::ResetDecode())) {
    res = NS_ERROR_FAILURE;
  }

  
  
  
  vorbis_synthesis_restart(&mVorbisDsp);

  mVideoPackets.Reset();
  mAudioPackets.Reset();

  return res;
}

void nsWebMReader::Cleanup()
{
  if (mContext) {
    nestegg_destroy(mContext);
    mContext = nsnull;
  }
}

nsresult nsWebMReader::ReadMetadata()
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(), "Should be on state machine thread.");
  MonitorAutoEnter mon(mMonitor);

  nestegg_io io;
  io.read = webm_read;
  io.seek = webm_seek;
  io.tell = webm_tell;
  io.userdata = static_cast<nsBuiltinDecoder*>(mDecoder);
  int r = nestegg_init(&mContext, io, NULL);
  if (r == -1) {
    return NS_ERROR_FAILURE;
  }

  uint64_t duration = 0;
  r = nestegg_duration(mContext, &duration);
  if (r == 0) {
    MonitorAutoExit exitReaderMon(mMonitor);
    MonitorAutoEnter decoderMon(mDecoder->GetMonitor());
    mDecoder->GetStateMachine()->SetDuration(duration / NS_PER_MS);
  }

  r = nestegg_tstamp_scale(mContext, &mTimecodeScale);
  if (r == -1) {
    Cleanup();
    return NS_ERROR_FAILURE;
  }

  unsigned int ntracks = 0;
  r = nestegg_track_count(mContext, &ntracks);
  if (r == -1) {
    Cleanup();
    return NS_ERROR_FAILURE;
  }

  mInfo.mHasAudio = PR_FALSE;
  mInfo.mHasVideo = PR_FALSE;
  for (PRUint32 track = 0; track < ntracks; ++track) {
    int id = nestegg_track_codec_id(mContext, track);
    if (id == -1) {
      Cleanup();
      return NS_ERROR_FAILURE;
    }
    int type = nestegg_track_type(mContext, track);
    if (!mHasVideo && type == NESTEGG_TRACK_VIDEO) {
      nestegg_video_params params;
      r = nestegg_track_video_params(mContext, track, &params);
      if (r == -1) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      mVideoTrack = track;
      mHasVideo = PR_TRUE;
      mInfo.mHasVideo = PR_TRUE;
      mInfo.mPicture.x = params.crop_left;
      mInfo.mPicture.y = params.crop_top;
      mInfo.mPicture.width = params.width - (params.crop_right - params.crop_left);
      mInfo.mPicture.height = params.height - (params.crop_bottom - params.crop_top);
      mInfo.mFrame.width = params.width;
      mInfo.mFrame.height = params.height;
      mInfo.mPixelAspectRatio = (float(params.display_width) / params.width) /
                                (float(params.display_height) / params.height);

      
      if (mInfo.mPicture.width <= 0 || mInfo.mPicture.height <= 0) {
        mInfo.mPicture.x = 0;
        mInfo.mPicture.y = 0;
        mInfo.mPicture.width = params.width;
        mInfo.mPicture.height = params.height;
      }

      
      
      
      mInfo.mDataOffset = -1;
    }
    else if (!mHasAudio && type == NESTEGG_TRACK_AUDIO) {
      nestegg_audio_params params;
      r = nestegg_track_audio_params(mContext, track, &params);
      if (r == -1) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      mAudioTrack = track;
      mHasAudio = PR_TRUE;
      mInfo.mHasAudio = PR_TRUE;

      
      unsigned int nheaders = 0;
      r = nestegg_track_codec_data_count(mContext, track, &nheaders);
      if (r == -1 || nheaders != 3) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      for (PRUint32 header = 0; header < nheaders; ++header) {
        unsigned char* data = 0;
        size_t length = 0;

        r = nestegg_track_codec_data(mContext, track, header, &data, &length);
        if (r == -1) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }

        ogg_packet opacket = InitOggPacket(data, length, header == 0, PR_FALSE, 0);

        r = vorbis_synthesis_headerin(&mVorbisInfo,
                                      &mVorbisComment,
                                      &opacket);
        if (r < 0) {
          Cleanup();
          return NS_ERROR_FAILURE;
        }
      }

      r = vorbis_synthesis_init(&mVorbisDsp, &mVorbisInfo);
      if (r < 0) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      r = vorbis_block_init(&mVorbisDsp, &mVorbisBlock);
      if (r < 0) {
        Cleanup();
        return NS_ERROR_FAILURE;
      }

      mInfo.mAudioRate = mVorbisDsp.vi->rate;
      mInfo.mAudioChannels = mVorbisDsp.vi->channels;
      mChannels = mInfo.mAudioChannels;
    }
  }

  return NS_OK;
}

ogg_packet nsWebMReader::InitOggPacket(unsigned char* aData,
                                       size_t aLength,
                                       PRBool aBOS,
                                       PRBool aEOS,
                                       PRInt64 aGranulepos)
{
  ogg_packet packet;
  packet.packet = aData;
  packet.bytes = aLength;
  packet.b_o_s = aBOS;
  packet.e_o_s = aEOS;
  packet.granulepos = aGranulepos;
  packet.packetno = mPacketCount++;
  return packet;
}
 
PRBool nsWebMReader::DecodeAudioPacket(nestegg_packet* aPacket)
{
  mMonitor.AssertCurrentThreadIn();

  int r = 0;
  unsigned int count = 0;
  r = nestegg_packet_count(aPacket, &count);
  if (r == -1) {
    return PR_FALSE;
  }

  uint64_t tstamp = 0;
  r = nestegg_packet_tstamp(aPacket, &tstamp);
  if (r == -1) {
    nestegg_free_packet(aPacket);
    return PR_FALSE;
  }

  const PRUint32 rate = mVorbisDsp.vi->rate;
  PRUint64 tstamp_ms = tstamp / NS_PER_MS;
  if (mAudioStartMs == -1) {
    
    
    mAudioStartMs = tstamp_ms;
  }
  
  
  
  
  PRInt64 tstamp_samples = 0;
  if (!MsToSamples(tstamp_ms, rate, tstamp_samples)) {
    NS_WARNING("Int overflow converting WebM timestamp to samples");
    return PR_FALSE;
  }
  PRInt64 decoded_samples = 0;
  if (!MsToSamples(mAudioStartMs, rate, decoded_samples)) {
    NS_WARNING("Int overflow converting WebM start time to samples");
    return PR_FALSE;
  }
  if (!AddOverflow(decoded_samples, mAudioSamples, decoded_samples)) {
    NS_WARNING("Int overflow adding decoded_samples");
    return PR_FALSE;
  }
  if (tstamp_samples > decoded_samples) {
#ifdef DEBUG
    PRInt64 ms = 0;
    LOG(PR_LOG_DEBUG, ("WebMReader detected gap of %lldms, %lld samples, in audio stream\n",
      SamplesToMs(tstamp_samples - decoded_samples, rate, ms) ? ms: -1,
      tstamp_samples - decoded_samples));
#endif
    mPacketCount++;
    mAudioStartMs = tstamp_ms;
    mAudioSamples = 0;
  }

  for (PRUint32 i = 0; i < count; ++i) {
    unsigned char* data;
    size_t length;
    r = nestegg_packet_data(aPacket, i, &data, &length);
    if (r == -1) {
      nestegg_free_packet(aPacket);
      return PR_FALSE;
    }

    ogg_packet opacket = InitOggPacket(data, length, PR_FALSE, PR_FALSE, -1);

    if (vorbis_synthesis(&mVorbisBlock, &opacket) != 0) {
      nestegg_free_packet(aPacket);
      return PR_FALSE;
    }

    if (vorbis_synthesis_blockin(&mVorbisDsp,
                                 &mVorbisBlock) != 0) {
      nestegg_free_packet(aPacket);
      return PR_FALSE;
    }

    float** pcm = 0;
    PRInt32 samples = 0;
    PRInt32 total_samples = 0;
    while ((samples = vorbis_synthesis_pcmout(&mVorbisDsp, &pcm)) > 0) {
      float* buffer = new float[samples * mChannels];
      float* p = buffer;
      for (PRUint32 i = 0; i < PRUint32(samples); ++i) {
        for (PRUint32 j = 0; j < mChannels; ++j) {
          *p++ = pcm[j][i];
        }
      }

      PRInt64 duration = 0;
      if (!SamplesToMs(samples, rate, duration)) {
        NS_WARNING("Int overflow converting WebM audio duration");
        nestegg_free_packet(aPacket);
        return PR_FALSE;
      }
      PRInt64 total_duration = 0;
      if (!SamplesToMs(total_samples, rate, total_duration)) {
        NS_WARNING("Int overflow converting WebM audio total_duration");
        nestegg_free_packet(aPacket);
        return PR_FALSE;
      }
      
      PRInt64 time = tstamp_ms + total_duration;
      total_samples += samples;
      SoundData* s = new SoundData(0,
                                   time,
                                   duration,
                                   samples,
                                   buffer,
                                   mChannels);
      mAudioQueue.Push(s);
      mAudioSamples += samples;
      if (vorbis_synthesis_read(&mVorbisDsp, samples) != 0) {
        nestegg_free_packet(aPacket);
        return PR_FALSE;
      }
    }
  }

  nestegg_free_packet(aPacket);

  return PR_TRUE;
}

nestegg_packet* nsWebMReader::NextPacket(TrackType aTrackType)
{
  
  
  PacketQueue& otherPackets = 
    aTrackType == VIDEO ? mAudioPackets : mVideoPackets;

  
  PacketQueue &packets =
    aTrackType == VIDEO ? mVideoPackets : mAudioPackets;

  
  
  PRPackedBool hasType = aTrackType == VIDEO ? mHasVideo : mHasAudio;

  
  
  PRPackedBool hasOtherType = aTrackType == VIDEO ? mHasAudio : mHasVideo;

  
  PRUint32 ourTrack = aTrackType == VIDEO ? mVideoTrack : mAudioTrack;

  
  PRUint32 otherTrack = aTrackType == VIDEO ? mAudioTrack : mVideoTrack;

  nestegg_packet* packet = NULL;

  if (packets.GetSize() > 0) {
    packet = packets.PopFront();
  }
  else {
    
    
    do {
      int r = nestegg_read_packet(mContext, &packet);
      if (r <= 0) {
        return NULL;
      }

      unsigned int track = 0;
      r = nestegg_packet_track(packet, &track);
      if (r == -1) {
        nestegg_free_packet(packet);
        return NULL;
      }

      if (hasOtherType && otherTrack == track) {
        
        otherPackets.Push(packet);
        continue;
      }

      
      if (hasType && ourTrack == track) {
        break;
      }

      
      nestegg_free_packet(packet);
    } while (PR_TRUE);
  }

  return packet;
}

PRBool nsWebMReader::DecodeAudioData()
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
    "Should be on state machine thread or decode thread.");
  nestegg_packet* packet = NextPacket(AUDIO);
  if (!packet) {
    mAudioQueue.Finish();
    return PR_FALSE;
  }

  return DecodeAudioPacket(packet);
}

PRBool nsWebMReader::DecodeVideoFrame(PRBool &aKeyframeSkip,
                                      PRInt64 aTimeThreshold)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or decode thread.");
  int r = 0;
  nestegg_packet* packet = NextPacket(VIDEO);

  if (!packet) {
    mVideoQueue.Finish();
    return PR_FALSE;
  }

  unsigned int track = 0;
  r = nestegg_packet_track(packet, &track);
  if (r == -1) {
    nestegg_free_packet(packet);
    return PR_FALSE;
  }

  unsigned int count = 0;
  r = nestegg_packet_count(packet, &count);
  if (r == -1) {
    nestegg_free_packet(packet);
    return PR_FALSE;
  }

  uint64_t tstamp = 0;
  r = nestegg_packet_tstamp(packet, &tstamp);
  if (r == -1) {
    nestegg_free_packet(packet);
    return PR_FALSE;
  }

  
  
  
  
  uint64_t next_tstamp = 0;
  {
    nestegg_packet* next_packet = NextPacket(VIDEO);
    if (next_packet) {
      r = nestegg_packet_tstamp(next_packet, &next_tstamp);
      if (r == -1) {
        nestegg_free_packet(next_packet);
        return PR_FALSE;
      }
      mVideoPackets.PushFront(next_packet);
    } else {
      r = nestegg_duration(mContext, &next_tstamp);
      if (r == -1) {
        return PR_FALSE;
      }
    }
  }

  PRInt64 tstamp_ms = tstamp / NS_PER_MS;
  for (PRUint32 i = 0; i < count; ++i) {
    unsigned char* data;
    size_t length;
    r = nestegg_packet_data(packet, i, &data, &length);
    if (r == -1) {
      nestegg_free_packet(packet);
      return PR_FALSE;
    }

    vpx_codec_stream_info_t si;
    memset(&si, 0, sizeof(si));
    si.sz = sizeof(si);
    vpx_codec_peek_stream_info(&vpx_codec_vp8_dx_algo, data, length, &si);
    if ((aKeyframeSkip && !si.is_kf) || (aKeyframeSkip && si.is_kf && tstamp_ms < aTimeThreshold)) {
      aKeyframeSkip = PR_TRUE;
      break;
    }

    if (aKeyframeSkip && si.is_kf) {
      aKeyframeSkip = PR_FALSE;
    }

    if(vpx_codec_decode(&mVP8, data, length, NULL, 0)) {
      nestegg_free_packet(packet);
      return PR_FALSE;
    }

    
    
    
    if (tstamp_ms < aTimeThreshold) {
      continue;
    }

    vpx_codec_iter_t  iter = NULL;
    vpx_image_t      *img;

    while((img = vpx_codec_get_frame(&mVP8, &iter))) {
      NS_ASSERTION(mInfo.mPicture.width == static_cast<PRInt32>(img->d_w), 
                   "WebM picture width from header does not match decoded frame");
      NS_ASSERTION(mInfo.mPicture.height == static_cast<PRInt32>(img->d_h),
                   "WebM picture height from header does not match decoded frame");
      NS_ASSERTION(img->fmt == IMG_FMT_I420, "WebM image format is not I420");

      
      VideoData::YCbCrBuffer b;
      b.mPlanes[0].mData = img->planes[0];
      b.mPlanes[0].mStride = img->stride[0];
      b.mPlanes[0].mHeight = img->d_h;
      b.mPlanes[0].mWidth = img->d_w;

      b.mPlanes[1].mData = img->planes[1];
      b.mPlanes[1].mStride = img->stride[1];
      b.mPlanes[1].mHeight = img->d_h >> img->y_chroma_shift;
      b.mPlanes[1].mWidth = img->d_w >> img->x_chroma_shift;
 
      b.mPlanes[2].mData = img->planes[2];
      b.mPlanes[2].mStride = img->stride[2];
      b.mPlanes[2].mHeight = img->d_h >> img->y_chroma_shift;
      b.mPlanes[2].mWidth = img->d_w >> img->x_chroma_shift;
  
      VideoData *v = VideoData::Create(mInfo,
                                       mDecoder->GetImageContainer(),
                                       -1,
                                       tstamp_ms,
                                       next_tstamp / NS_PER_MS,
                                       b,
                                       si.is_kf,
                                       -1);
      if (!v) {
        nestegg_free_packet(packet);
        return PR_FALSE;
      }
      mVideoQueue.Push(v);
    }
  }
 
  nestegg_free_packet(packet);
  return PR_TRUE;
}

nsresult nsWebMReader::Seek(PRInt64 aTarget, PRInt64 aStartTime, PRInt64 aEndTime,
                            PRInt64 aCurrentTime)
{
  MonitorAutoEnter mon(mMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  LOG(PR_LOG_DEBUG, ("%p About to seek to %lldms", mDecoder, aTarget));
  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  int r = nestegg_track_seek(mContext, 0, aTarget * NS_PER_MS);
  if (r != 0) {
    return NS_ERROR_FAILURE;
  }
  return DecodeToTarget(aTarget);
}

void nsWebMReader::CalculateBufferedForRange(nsTimeRanges* aBuffered,
                                             PRInt64 aStartOffset, PRInt64 aEndOffset)
{
  
  PRUint32 start;
  mTimeMapping.GreatestIndexLtEq(aStartOffset, start);
  if (start == mTimeMapping.Length()) {
    return;
  }

  
  PRUint32 end;
  if (!mTimeMapping.GreatestIndexLtEq(aEndOffset, end) && end > 0) {
    
    
    end -= 1;
  }

  
  if (end <= start) {
    return;
  }

  NS_ASSERTION(mTimeMapping[start].mOffset >= aStartOffset &&
               mTimeMapping[end].mOffset <= aEndOffset,
               "Computed time range must lie within data range.");
  if (start > 0) {
    NS_ASSERTION(mTimeMapping[start - 1].mOffset <= aStartOffset,
                 "Must have found least nsWebMTimeDataOffset for start");
  }
  if (end < mTimeMapping.Length() - 1) {
    NS_ASSERTION(mTimeMapping[end + 1].mOffset >= aEndOffset,
                 "Must have found greatest nsWebMTimeDataOffset for end");
  }

  float startTime = mTimeMapping[start].mTimecode * mTimecodeScale / NS_PER_S;
  float endTime = mTimeMapping[end].mTimecode * mTimecodeScale / NS_PER_S;
  aBuffered->Add(startTime, endTime);
}

nsresult nsWebMReader::GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  nsMediaStream* stream = mDecoder->GetCurrentStream();

  
  if (stream->IsDataCachedToEndOfStream(0)) {
    uint64_t duration = 0;
    if (nestegg_duration(mContext, &duration) == 0) {
      aBuffered->Add(aStartTime / MS_PER_S, duration / NS_PER_S);
    }
  } else {
    PRInt64 startOffset = stream->GetNextCachedData(0);
    while (startOffset >= 0) {
      PRInt64 endOffset = stream->GetCachedDataEnd(startOffset);
      NS_ASSERTION(startOffset < endOffset, "Cached range invalid");

      CalculateBufferedForRange(aBuffered, startOffset, endOffset);

      
      startOffset = stream->GetNextCachedData(endOffset);
      NS_ASSERTION(startOffset == -1 || startOffset > endOffset,
                   "Next cached range invalid");
    }
  }

  return NS_OK;
}

void nsWebMReader::NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset)
{
  PRUint32 idx;
  if (!mRangeParsers.GreatestIndexLtEq(aOffset, idx)) {
    
    
    
    
    
    if (idx != mRangeParsers.Length() && mRangeParsers[idx].mStartOffset <= aOffset) {
      
      if (aOffset + aLength <= mRangeParsers[idx].mCurrentOffset) {
        return;
      }

      
      PRInt64 adjust = mRangeParsers[idx].mCurrentOffset - aOffset;
      NS_ASSERTION(adjust >= 0, "Overlap detection bug.");
      aBuffer += adjust;
      aLength -= PRUint32(adjust);
    } else {
      mRangeParsers.InsertElementAt(idx, nsWebMBufferedParser(aOffset));
    }
  }

  mRangeParsers[idx].Append(reinterpret_cast<const unsigned char*>(aBuffer), aLength, mTimeMapping);

  
  PRUint32 i = 0;
  while (i + 1 < mRangeParsers.Length()) {
    if (mRangeParsers[i].mCurrentOffset >= mRangeParsers[i + 1].mStartOffset) {
      mRangeParsers[i + 1].mStartOffset = mRangeParsers[i].mStartOffset;
      mRangeParsers.RemoveElementAt(i);
    } else {
      i += 1;
    }
  }
}
