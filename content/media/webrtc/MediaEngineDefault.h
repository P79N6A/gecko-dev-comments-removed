



#ifndef MEDIAENGINEDEFAULT_H_
#define MEDIAENGINEDEFAULT_H_

#include "nsITimer.h"

#include "nsCOMPtr.h"
#include "DOMMediaStream.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Monitor.h"

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

class MediaEngineDefault;




class MediaEngineDefaultVideoSource : public nsITimerCallback,
                                      public MediaEngineVideoSource
{
public:
  MediaEngineDefaultVideoSource();

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual nsresult Allocate(const VideoTrackConstraintsN &aConstraints,
                            const MediaEnginePrefs &aPrefs);
  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop(SourceMediaStream*, TrackID);
  virtual void SetDirectListeners(bool aHasDirectListeners) {};
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise,
                          int32_t aPlayoutDelay) { return NS_OK; };
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime);

  virtual bool IsFake() {
    return true;
  }

  virtual const MediaSourceType GetMediaSource() {
    return MediaSourceType::Camera;
  }

  virtual nsresult TakePhoto(PhotoCallback* aCallback)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

protected:
  ~MediaEngineDefaultVideoSource();

  friend class MediaEngineDefault;

  TrackID mTrackID;
  nsCOMPtr<nsITimer> mTimer;
  
  
  
  
  Monitor mMonitor;
  nsRefPtr<layers::Image> mImage;

  nsRefPtr<layers::ImageContainer> mImageContainer;

  MediaEnginePrefs mOpts;
  int mCb;
  int mCr;
};

class SineWaveGenerator;

class MediaEngineDefaultAudioSource : public nsITimerCallback,
                                      public MediaEngineAudioSource
{
public:
  MediaEngineDefaultAudioSource();

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual nsresult Allocate(const AudioTrackConstraintsN &aConstraints,
                            const MediaEnginePrefs &aPrefs);
  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop(SourceMediaStream*, TrackID);
  virtual void SetDirectListeners(bool aHasDirectListeners) {};
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise,
                          int32_t aPlayoutDelay) { return NS_OK; };
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime) {}

  virtual bool IsFake() {
    return true;
  }

  virtual const MediaSourceType GetMediaSource() {
    return MediaSourceType::Microphone;
  }

  virtual nsresult TakePhoto(PhotoCallback* aCallback)
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

protected:
  ~MediaEngineDefaultAudioSource();

  TrackID mTrackID;
  nsCOMPtr<nsITimer> mTimer;

  SourceMediaStream* mSource;
  nsAutoPtr<SineWaveGenerator> mSineGenerator;
};


class MediaEngineDefault : public MediaEngine
{
public:
  MediaEngineDefault()
  : mMutex("mozilla::MediaEngineDefault")
  {}

  virtual void EnumerateVideoDevices(MediaSourceType,
                                     nsTArray<nsRefPtr<MediaEngineVideoSource> >*);
  virtual void EnumerateAudioDevices(MediaSourceType,
                                     nsTArray<nsRefPtr<MediaEngineAudioSource> >*);

private:
  ~MediaEngineDefault() {}

  Mutex mMutex;
  

  nsTArray<nsRefPtr<MediaEngineVideoSource> > mVSources;
  nsTArray<nsRefPtr<MediaEngineAudioSource> > mASources;
};

}

#endif 
