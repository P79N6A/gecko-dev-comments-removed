



#ifndef MEDIAENGINEDEFAULT_H_
#define MEDIAENGINEDEFAULT_H_

#include "prmem.h"
#include "nsITimer.h"

#include "nsCOMPtr.h"
#include "nsDOMMediaStream.h"
#include "nsComponentManagerUtils.h"

#include "VideoUtils.h"
#include "MediaEngine.h"
#include "VideoSegment.h"
#include "AudioSegment.h"
#include "StreamBuffer.h"
#include "MediaStreamGraph.h"

namespace mozilla {

namespace layers {
class ImageContainer;
class PlanarYCbCrImage;
}




class MediaEngineDefaultVideoSource : public nsITimerCallback,
                                      public MediaEngineVideoSource
{
public:
  MediaEngineDefaultVideoSource();
  ~MediaEngineDefaultVideoSource();

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual MediaEngineVideoOptions GetOptions();
  virtual nsresult Allocate();

  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop();
  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

protected:
  TrackID mTrackID;
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<layers::ImageContainer> mImageContainer;

  MediaEngineState mState;
  SourceMediaStream* mSource;
  layers::PlanarYCbCrImage* mImage;
};

class MediaEngineDefaultAudioSource : public nsITimerCallback,
                                      public MediaEngineAudioSource
{
public:
  MediaEngineDefaultAudioSource() : mTimer(nullptr), mState(kReleased) {}
  ~MediaEngineDefaultAudioSource(){};

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual nsresult Allocate();

  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop();
  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

protected:
  TrackID mTrackID;
  nsCOMPtr<nsITimer> mTimer;

  MediaEngineState mState;
  SourceMediaStream* mSource;
};

class MediaEngineDefault : public MediaEngine
{
public:
  MediaEngineDefault() {
    mVSource = new MediaEngineDefaultVideoSource();
    mASource = new MediaEngineDefaultAudioSource();
  }
  ~MediaEngineDefault() {}

  virtual void EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >*);
  virtual void EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >*);

private:
  nsRefPtr<MediaEngineVideoSource> mVSource;
  nsRefPtr<MediaEngineAudioSource> mASource;
};

}

#endif 
