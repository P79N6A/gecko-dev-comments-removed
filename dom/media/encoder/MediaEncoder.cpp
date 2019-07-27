



#include "MediaEncoder.h"
#include "MediaDecoder.h"
#include "nsIPrincipal.h"
#include "nsMimeTypes.h"
#include "mozilla/Logging.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"

#include"GeckoProfiler.h"
#include "OggWriter.h"
#include "OpusTrackEncoder.h"

#ifdef MOZ_VORBIS
#include "VorbisTrackEncoder.h"
#endif
#ifdef MOZ_WEBM_ENCODER
#include "VorbisTrackEncoder.h"
#include "VP8TrackEncoder.h"
#include "WebMWriter.h"
#endif
#ifdef MOZ_OMX_ENCODER
#include "OmxTrackEncoder.h"
#include "ISOMediaWriter.h"
#endif

#ifdef LOG
#undef LOG
#endif

PRLogModuleInfo* gMediaEncoderLog;
#define LOG(type, msg) MOZ_LOG(gMediaEncoderLog, type, msg)

namespace mozilla {

void
MediaEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                       TrackID aID,
                                       StreamTime aTrackOffset,
                                       uint32_t aTrackEvents,
                                       const MediaSegment& aQueuedMedia)
{
  
  
  if (mAudioEncoder && aQueuedMedia.GetType() == MediaSegment::AUDIO) {
    mAudioEncoder->NotifyQueuedTrackChanges(aGraph, aID,
                                            aTrackOffset, aTrackEvents,
                                            aQueuedMedia);

  } else if (mVideoEncoder && aQueuedMedia.GetType() == MediaSegment::VIDEO) {
      mVideoEncoder->NotifyQueuedTrackChanges(aGraph, aID,
                                              aTrackOffset, aTrackEvents,
                                              aQueuedMedia);
  }
}

void
MediaEncoder::NotifyEvent(MediaStreamGraph* aGraph,
                          MediaStreamListener::MediaStreamGraphEvent event)
{
  
  LOG(LogLevel::Debug, ("NotifyRemoved in [MediaEncoder]."));
  if (mAudioEncoder) {
    mAudioEncoder->NotifyEvent(aGraph, event);
  }
  if (mVideoEncoder) {
    mVideoEncoder->NotifyEvent(aGraph, event);
  }
}


already_AddRefed<MediaEncoder>
MediaEncoder::CreateEncoder(const nsAString& aMIMEType, uint8_t aTrackTypes)
{
  if (!gMediaEncoderLog) {
    gMediaEncoderLog = PR_NewLogModule("MediaEncoder");
  }
  PROFILER_LABEL("MediaEncoder", "CreateEncoder",
    js::ProfileEntry::Category::OTHER);

  nsAutoPtr<ContainerWriter> writer;
  nsAutoPtr<AudioTrackEncoder> audioEncoder;
  nsAutoPtr<VideoTrackEncoder> videoEncoder;
  nsRefPtr<MediaEncoder> encoder;
  nsString mimeType;
  if (!aTrackTypes) {
    LOG(LogLevel::Error, ("NO TrackTypes!!!"));
    return nullptr;
  }
#ifdef MOZ_WEBM_ENCODER
  else if (MediaEncoder::IsWebMEncoderEnabled() &&
          (aMIMEType.EqualsLiteral(VIDEO_WEBM) ||
          (aTrackTypes & ContainerWriter::CREATE_VIDEO_TRACK))) {
    if (aTrackTypes & ContainerWriter::CREATE_AUDIO_TRACK) {
      audioEncoder = new VorbisTrackEncoder();
      NS_ENSURE_TRUE(audioEncoder, nullptr);
    }
    videoEncoder = new VP8TrackEncoder();
    writer = new WebMWriter(aTrackTypes);
    NS_ENSURE_TRUE(writer, nullptr);
    NS_ENSURE_TRUE(videoEncoder, nullptr);
    mimeType = NS_LITERAL_STRING(VIDEO_WEBM);
  }
#endif 
#ifdef MOZ_OMX_ENCODER
  else if (MediaEncoder::IsOMXEncoderEnabled() &&
          (aMIMEType.EqualsLiteral(VIDEO_MP4) ||
          (aTrackTypes & ContainerWriter::CREATE_VIDEO_TRACK))) {
    if (aTrackTypes & ContainerWriter::CREATE_AUDIO_TRACK) {
      audioEncoder = new OmxAACAudioTrackEncoder();
      NS_ENSURE_TRUE(audioEncoder, nullptr);
    }
    videoEncoder = new OmxVideoTrackEncoder();
    writer = new ISOMediaWriter(aTrackTypes);
    NS_ENSURE_TRUE(writer, nullptr);
    NS_ENSURE_TRUE(videoEncoder, nullptr);
    mimeType = NS_LITERAL_STRING(VIDEO_MP4);
  } else if (MediaEncoder::IsOMXEncoderEnabled() &&
            (aMIMEType.EqualsLiteral(AUDIO_3GPP))) {
    audioEncoder = new OmxAMRAudioTrackEncoder();
    NS_ENSURE_TRUE(audioEncoder, nullptr);

    writer = new ISOMediaWriter(aTrackTypes, ISOMediaWriter::TYPE_FRAG_3GP);
    NS_ENSURE_TRUE(writer, nullptr);
    mimeType = NS_LITERAL_STRING(AUDIO_3GPP);
  }
#endif 
  else if (MediaDecoder::IsOggEnabled() && MediaDecoder::IsOpusEnabled() &&
           (aMIMEType.EqualsLiteral(AUDIO_OGG) ||
           (aTrackTypes & ContainerWriter::CREATE_AUDIO_TRACK))) {
    writer = new OggWriter();
    audioEncoder = new OpusTrackEncoder();
    NS_ENSURE_TRUE(writer, nullptr);
    NS_ENSURE_TRUE(audioEncoder, nullptr);
    mimeType = NS_LITERAL_STRING(AUDIO_OGG);
  }
  else {
    LOG(LogLevel::Error, ("Can not find any encoder to record this media stream"));
    return nullptr;
  }
  LOG(LogLevel::Debug, ("Create encoder result:a[%d] v[%d] w[%d] mimeType = %s.",
                      audioEncoder != nullptr, videoEncoder != nullptr,
                      writer != nullptr, mimeType.get()));
  encoder = new MediaEncoder(writer.forget(), audioEncoder.forget(),
                             videoEncoder.forget(), mimeType);
  return encoder.forget();
}

























void
MediaEncoder::GetEncodedData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                             nsAString& aMIMEType)
{
  MOZ_ASSERT(!NS_IsMainThread());

  aMIMEType = mMIMEType;
  PROFILER_LABEL("MediaEncoder", "GetEncodedData",
    js::ProfileEntry::Category::OTHER);

  bool reloop = true;
  while (reloop) {
    switch (mState) {
    case ENCODE_METADDATA: {
      LOG(LogLevel::Debug, ("ENCODE_METADDATA TimeStamp = %f", GetEncodeTimeStamp()));
      nsresult rv = CopyMetadataToMuxer(mAudioEncoder.get());
      if (NS_FAILED(rv)) {
        LOG(LogLevel::Error, ("Error! Fail to Set Audio Metadata"));
        break;
      }
      rv = CopyMetadataToMuxer(mVideoEncoder.get());
      if (NS_FAILED(rv)) {
        LOG(LogLevel::Error, ("Error! Fail to Set Video Metadata"));
        break;
      }

      rv = mWriter->GetContainerData(aOutputBufs,
                                     ContainerWriter::GET_HEADER);
      if (aOutputBufs != nullptr) {
        mSizeOfBuffer = aOutputBufs->SizeOfExcludingThis(MallocSizeOf);
      }
      if (NS_FAILED(rv)) {
       LOG(LogLevel::Error,("Error! writer fail to generate header!"));
       mState = ENCODE_ERROR;
       break;
      }
      LOG(LogLevel::Debug, ("Finish ENCODE_METADDATA TimeStamp = %f", GetEncodeTimeStamp()));
      mState = ENCODE_TRACK;
      break;
    }

    case ENCODE_TRACK: {
      LOG(LogLevel::Debug, ("ENCODE_TRACK TimeStamp = %f", GetEncodeTimeStamp()));
      EncodedFrameContainer encodedData;
      nsresult rv = NS_OK;
      rv = WriteEncodedDataToMuxer(mAudioEncoder.get());
      if (NS_FAILED(rv)) {
        LOG(LogLevel::Error, ("Error! Fail to write audio encoder data to muxer"));
        break;
      }
      LOG(LogLevel::Debug, ("Audio encoded TimeStamp = %f", GetEncodeTimeStamp()));
      rv = WriteEncodedDataToMuxer(mVideoEncoder.get());
      if (NS_FAILED(rv)) {
        LOG(LogLevel::Error, ("Fail to write video encoder data to muxer"));
        break;
      }
      LOG(LogLevel::Debug, ("Video encoded TimeStamp = %f", GetEncodeTimeStamp()));
      
      bool isAudioCompleted = (mAudioEncoder && mAudioEncoder->IsEncodingComplete()) || !mAudioEncoder;
      bool isVideoCompleted = (mVideoEncoder && mVideoEncoder->IsEncodingComplete()) || !mVideoEncoder;
      rv = mWriter->GetContainerData(aOutputBufs,
                                     isAudioCompleted && isVideoCompleted ?
                                     ContainerWriter::FLUSH_NEEDED : 0);
      if (aOutputBufs != nullptr) {
        mSizeOfBuffer = aOutputBufs->SizeOfExcludingThis(MallocSizeOf);
      }
      if (NS_SUCCEEDED(rv)) {
        
        reloop = false;
      }
      mState = (mWriter->IsWritingComplete()) ? ENCODE_DONE : ENCODE_TRACK;
      LOG(LogLevel::Debug, ("END ENCODE_TRACK TimeStamp = %f "
          "mState = %d aComplete %d vComplete %d",
          GetEncodeTimeStamp(), mState, isAudioCompleted, isVideoCompleted));
      break;
    }

    case ENCODE_DONE:
    case ENCODE_ERROR:
      LOG(LogLevel::Debug, ("MediaEncoder has been shutdown."));
      mSizeOfBuffer = 0;
      mShutdown = true;
      reloop = false;
      break;
    default:
      MOZ_CRASH("Invalid encode state");
    }
  }
}

nsresult
MediaEncoder::WriteEncodedDataToMuxer(TrackEncoder *aTrackEncoder)
{
  if (aTrackEncoder == nullptr) {
    return NS_OK;
  }
  if (aTrackEncoder->IsEncodingComplete()) {
    return NS_OK;
  }

  PROFILER_LABEL("MediaEncoder", "WriteEncodedDataToMuxer",
    js::ProfileEntry::Category::OTHER);

  EncodedFrameContainer encodedVideoData;
  nsresult rv = aTrackEncoder->GetEncodedTrack(encodedVideoData);
  if (NS_FAILED(rv)) {
    
    LOG(LogLevel::Error, ("Error! Fail to get encoded data from video encoder."));
    mState = ENCODE_ERROR;
    return rv;
  }
  rv = mWriter->WriteEncodedTrack(encodedVideoData,
                                  aTrackEncoder->IsEncodingComplete() ?
                                  ContainerWriter::END_OF_STREAM : 0);
  if (NS_FAILED(rv)) {
    LOG(LogLevel::Error, ("Error! Fail to write encoded video track to the media container."));
    mState = ENCODE_ERROR;
  }
  return rv;
}

nsresult
MediaEncoder::CopyMetadataToMuxer(TrackEncoder *aTrackEncoder)
{
  if (aTrackEncoder == nullptr) {
    return NS_OK;
  }

  PROFILER_LABEL("MediaEncoder", "CopyMetadataToMuxer",
    js::ProfileEntry::Category::OTHER);

  nsRefPtr<TrackMetadataBase> meta = aTrackEncoder->GetMetadata();
  if (meta == nullptr) {
    LOG(LogLevel::Error, ("Error! metadata = null"));
    mState = ENCODE_ERROR;
    return NS_ERROR_ABORT;
  }

  nsresult rv = mWriter->SetMetadata(meta);
  if (NS_FAILED(rv)) {
   LOG(LogLevel::Error, ("Error! SetMetadata fail"));
   mState = ENCODE_ERROR;
  }
  return rv;
}

#ifdef MOZ_WEBM_ENCODER
bool
MediaEncoder::IsWebMEncoderEnabled()
{
  return Preferences::GetBool("media.encoder.webm.enabled");
}
#endif

#ifdef MOZ_OMX_ENCODER
bool
MediaEncoder::IsOMXEncoderEnabled()
{
  return Preferences::GetBool("media.encoder.omx.enabled");
}
#endif






size_t
MediaEncoder::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t amount = 0;
  if (mState == ENCODE_TRACK) {
    amount = mSizeOfBuffer +
             (mAudioEncoder != nullptr ? mAudioEncoder->SizeOfExcludingThis(aMallocSizeOf) : 0) +
             (mVideoEncoder != nullptr ? mVideoEncoder->SizeOfExcludingThis(aMallocSizeOf) : 0);
  }
  return amount;
}

} 
