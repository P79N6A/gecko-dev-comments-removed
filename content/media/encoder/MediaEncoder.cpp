



#include "MediaEncoder.h"
#include "MediaDecoder.h"
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
    case ENCODE_HEADER: {
      nsTArray<uint8_t> buffer;
      nsresult rv = mAudioEncoder->GetHeader(&buffer);
      if (NS_FAILED(rv)) {
        
        mState = ENCODE_DONE;
        break;
      }

      if (!buffer.IsEmpty()) {
        rv = mWriter->WriteEncodedTrack(buffer, 0);
        if (NS_FAILED(rv)) {
          LOG("ERROR! Fail to write header to the media container.");
          mState = ENCODE_DONE;
          break;
        }

        rv = mWriter->GetContainerData(aOutputBufs,
                                       ContainerWriter::FLUSH_NEEDED);
        if (NS_SUCCEEDED(rv)) {
          
          reloop = false;
          break;
        }
      } else {
        
        mState = ENCODE_TRACK;
      }
      break;
    }

    case ENCODE_TRACK: {
      nsTArray<uint8_t> buffer;
      int encodedDuration = 0;
      nsresult rv = mAudioEncoder->GetEncodedTrack(&buffer, encodedDuration);
      if (NS_FAILED(rv)) {
        
        LOG("ERROR! Fail to get encoded data from encoder.");
        mState = ENCODE_DONE;
        break;
      }

      rv = mWriter->WriteEncodedTrack(buffer, encodedDuration,
                                      mAudioEncoder->IsEncodingComplete() ?
                                      ContainerWriter::END_OF_STREAM : 0);
      if (NS_FAILED(rv)) {
        LOG("ERROR! Fail to write encoded track to the media container.");
        mState = ENCODE_DONE;
        break;
      }

      rv = mWriter->GetContainerData(aOutputBufs,
                                     mAudioEncoder->IsEncodingComplete() ?
                                     ContainerWriter::FLUSH_NEEDED : 0);
      if (NS_SUCCEEDED(rv)) {
        
        reloop = false;
        break;
      }

      mState = (mAudioEncoder->IsEncodingComplete()) ? ENCODE_DONE : ENCODE_TRACK;
      break;
    }

    case ENCODE_DONE:
      LOG("MediaEncoder has been shutdown.");
      mShutdown = true;
      reloop = false;
      break;

    default:
      MOZ_NOT_REACHED("Invalid encode state");
      break;
    }
  }
}

}
