




#ifndef MediaEncoder_h_
#define MediaEncoder_h_

#include "mozilla/DebugOnly.h"
#include "TrackEncoder.h"
#include "ContainerWriter.h"
#include "MediaStreamGraph.h"

namespace mozilla {



































class MediaEncoder : public MediaStreamListener
{
public :
  enum {
    ENCODE_METADDATA,
    ENCODE_TRACK,
    ENCODE_DONE,
  };

  MediaEncoder(ContainerWriter* aWriter,
               AudioTrackEncoder* aAudioEncoder,
               VideoTrackEncoder* aVideoEncoder,
               const nsAString& aMIMEType)
    : mWriter(aWriter)
    , mAudioEncoder(aAudioEncoder)
    , mVideoEncoder(aVideoEncoder)
    , mMIMEType(aMIMEType)
    , mState(MediaEncoder::ENCODE_METADDATA)
    , mShutdown(false)
  {}

  ~MediaEncoder() {};

  



  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia);

  


  virtual void NotifyRemoved(MediaStreamGraph* aGraph);

  




  static already_AddRefed<MediaEncoder> CreateEncoder(const nsAString& aMIMEType);

  






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
  }

private:
  nsAutoPtr<ContainerWriter> mWriter;
  nsAutoPtr<AudioTrackEncoder> mAudioEncoder;
  nsAutoPtr<VideoTrackEncoder> mVideoEncoder;
  nsString mMIMEType;
  int mState;
  bool mShutdown;
};

}
#endif
