




#ifndef MediaEncoder_h_
#define MediaEncoder_h_

#include "mozilla/DebugOnly.h"
#include "TrackEncoder.h"
#include "ContainerWriter.h"
#include "MediaStreamGraph.h"
#include "nsIMemoryReporter.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {



































class MediaEncoder : public MediaStreamListener
{
public :
  enum {
    ENCODE_METADDATA,
    ENCODE_TRACK,
    ENCODE_DONE,
    ENCODE_ERROR,
  };

  MediaEncoder(ContainerWriter* aWriter,
               AudioTrackEncoder* aAudioEncoder,
               VideoTrackEncoder* aVideoEncoder,
               const nsAString& aMIMEType)
    : mWriter(aWriter)
    , mAudioEncoder(aAudioEncoder)
    , mVideoEncoder(aVideoEncoder)
    , mStartTime(TimeStamp::Now())
    , mMIMEType(aMIMEType)
    , mSizeOfBuffer(0)
    , mState(MediaEncoder::ENCODE_METADDATA)
    , mShutdown(false)
  {}

  ~MediaEncoder() {};

  



  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) MOZ_OVERRIDE;

  


  virtual void NotifyEvent(MediaStreamGraph* aGraph,
                           MediaStreamListener::MediaStreamGraphEvent event) MOZ_OVERRIDE;

  




  static already_AddRefed<MediaEncoder> CreateEncoder(const nsAString& aMIMEType,
                                                      uint8_t aTrackTypes = ContainerWriter::CREATE_AUDIO_TRACK);
  






  void GetEncodedData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                      nsAString& aMIMEType);

  



  bool IsShutdown()
  {
    return mShutdown;
  }

  


  void Cancel()
  {
    if (mAudioEncoder) {
      mAudioEncoder->NotifyCancel();
    }
    if (mVideoEncoder) {
      mVideoEncoder->NotifyCancel();
    }
  }

  bool HasError()
  {
    return mState == ENCODE_ERROR;
  }

#ifdef MOZ_WEBM_ENCODER
  static bool IsWebMEncoderEnabled();
#endif

#ifdef MOZ_OMX_ENCODER
  static bool IsOMXEncoderEnabled();
#endif

  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)
  



  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  
  nsresult WriteEncodedDataToMuxer(TrackEncoder *aTrackEncoder);
  
  nsresult CopyMetadataToMuxer(TrackEncoder* aTrackEncoder);
  nsAutoPtr<ContainerWriter> mWriter;
  nsAutoPtr<AudioTrackEncoder> mAudioEncoder;
  nsAutoPtr<VideoTrackEncoder> mVideoEncoder;
  TimeStamp mStartTime;
  nsString mMIMEType;
  int64_t mSizeOfBuffer;
  int mState;
  bool mShutdown;
  
  double GetEncodeTimeStamp()
  {
    TimeDuration decodeTime;
    decodeTime = TimeStamp::Now() - mStartTime;
    return decodeTime.ToMilliseconds();
  }
};

}
#endif
