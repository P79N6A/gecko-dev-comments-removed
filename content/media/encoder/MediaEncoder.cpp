



#include "MediaEncoder.h"
#include "MediaDecoder.h"
#include "nsIPrincipal.h"

#ifdef MOZ_OGG
#include "OggWriter.h"
#endif
#ifdef MOZ_OPUS
#include "OpusTrackEncoder.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "MediaEncoder", ## args);
#else
#define LOG(args,...)
#endif

namespace mozilla {

#define TRACK_BUFFER_LEN 8192

namespace {

template <class String>
static bool
TypeListContains(char const *const * aTypes, const String& aType)
{
  for (int32_t i = 0; aTypes[i]; ++i) {
    if (aType.EqualsASCII(aTypes[i]))
      return true;
  }
  return false;
}

#ifdef MOZ_OGG


static const char* const gOggTypes[2] = {
  "audio/ogg",
  nullptr
};

static bool
IsOggType(const nsAString& aType)
{
  if (!MediaDecoder::IsOggEnabled()) {
    return false;
  }

  return TypeListContains(gOggTypes, aType);
}
#endif
} 

void
MediaEncoder::NotifyQueuedTrackChanges(MediaStreamGraph* aGraph,
                                       TrackID aID,
                                       TrackRate aTrackRate,
                                       TrackTicks aTrackOffset,
                                       uint32_t aTrackEvents,
                                       const MediaSegment& aQueuedMedia)
{
  
  
  if (aQueuedMedia.GetType() == MediaSegment::AUDIO) {
    mAudioEncoder->NotifyQueuedTrackChanges(aGraph, aID, aTrackRate,
                                            aTrackOffset, aTrackEvents,
                                            aQueuedMedia);

  } else {
    
  }
}

void
MediaEncoder::NotifyRemoved(MediaStreamGraph* aGraph)
{
  
  LOG("NotifyRemoved in [MediaEncoder].");
  mAudioEncoder->NotifyRemoved(aGraph);
}


already_AddRefed<MediaEncoder>
MediaEncoder::CreateEncoder(const nsAString& aMIMEType)
{
  nsAutoPtr<ContainerWriter> writer;
  nsAutoPtr<AudioTrackEncoder> audioEncoder;
  nsAutoPtr<VideoTrackEncoder> videoEncoder;
  nsRefPtr<MediaEncoder> encoder;

  if (aMIMEType.IsEmpty()) {
    
    
    const_cast<nsAString&>(aMIMEType) = NS_LITERAL_STRING("audio/ogg");
  }

  bool isAudioOnly = FindInReadable(NS_LITERAL_STRING("audio/"), aMIMEType);
#ifdef MOZ_OGG
  if (IsOggType(aMIMEType)) {
    writer = new OggWriter();
    if (!isAudioOnly) {
      
    }
#ifdef MOZ_OPUS
    audioEncoder = new OpusTrackEncoder();
#endif
  }
#endif
  
  if (!isAudioOnly) {
    NS_ENSURE_TRUE(videoEncoder, nullptr);
  }

  
  NS_ENSURE_TRUE(audioEncoder, nullptr);

  encoder = new MediaEncoder(writer.forget(), audioEncoder.forget(),
                             videoEncoder.forget(), aMIMEType);


  return encoder.forget();
}

























void
MediaEncoder::GetEncodedData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                             nsAString& aMIMEType)
{
  MOZ_ASSERT(!NS_IsMainThread());

  aMIMEType = mMIMEType;

  bool reloop = true;
  while (reloop) {
    switch (mState) {
    case ENCODE_METADDATA: {
      nsRefPtr<TrackMetadataBase> meta = mAudioEncoder->GetMetadata();
      if (meta == nullptr) {
        LOG("ERROR! AudioEncoder get null Metadata!");
        mState = ENCODE_ERROR;
        break;
      }
      nsresult rv = mWriter->SetMetadata(meta);
      if (NS_FAILED(rv)) {
       LOG("ERROR! writer can't accept audio metadata!");
       mState = ENCODE_ERROR;
       break;
      }

      rv = mWriter->GetContainerData(aOutputBufs,
                                     ContainerWriter::GET_HEADER);
      if (NS_FAILED(rv)) {
       LOG("ERROR! writer fail to generate header!");
       mState = ENCODE_ERROR;
       break;
      }

      mState = ENCODE_TRACK;
      break;
    }

    case ENCODE_TRACK: {
      EncodedFrameContainer encodedData;
      nsresult rv = mAudioEncoder->GetEncodedTrack(encodedData);
      if (NS_FAILED(rv)) {
        
        LOG("ERROR! Fail to get encoded data from encoder.");
        mState = ENCODE_ERROR;
        break;
      }
      rv = mWriter->WriteEncodedTrack(encodedData,
                                      mAudioEncoder->IsEncodingComplete() ?
                                      ContainerWriter::END_OF_STREAM : 0);
      if (NS_FAILED(rv)) {
        LOG("ERROR! Fail to write encoded track to the media container.");
        mState = ENCODE_ERROR;
        break;
      }

      rv = mWriter->GetContainerData(aOutputBufs,
                                     mAudioEncoder->IsEncodingComplete() ?
                                     ContainerWriter::FLUSH_NEEDED : 0);
      if (NS_SUCCEEDED(rv)) {
        
        reloop = false;
      }

      mState = (mAudioEncoder->IsEncodingComplete()) ? ENCODE_DONE : ENCODE_TRACK;
      break;
    }

    case ENCODE_DONE:
      LOG("MediaEncoder has been shutdown.");
      mShutdown = true;
      reloop = false;
      break;
    case ENCODE_ERROR:
      LOG("ERROR! MediaEncoder got error!");
      mShutdown = true;
      reloop = false;
      break;
    default:
      MOZ_CRASH("Invalid encode state");
    }
  }
}

}
