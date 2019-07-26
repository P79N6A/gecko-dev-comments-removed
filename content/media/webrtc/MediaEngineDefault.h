



#ifndef MEDIAENGINEDEFAULT_H_
#define MEDIAENGINEDEFAULT_H_

#include "nsITimer.h"

#include "nsCOMPtr.h"
#include "DOMMediaStream.h"
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

class MediaEngineDefault;




class MediaEngineDefaultVideoSource : public nsITimerCallback,
                                      public MediaEngineVideoSource
{
public:
  MediaEngineDefaultVideoSource(int aWidth, int aHeight, int aFPS);
  ~MediaEngineDefaultVideoSource();

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual const MediaEngineVideoOptions *GetOptions();
  virtual nsresult Allocate();
  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop(SourceMediaStream*, TrackID);
  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile);
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise) { return NS_OK; };
  virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime);
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime) {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  static const int DEFAULT_VIDEO_FPS = 60;
  static const int DEFAULT_VIDEO_MIN_FPS = 10;
  static const int DEFAULT_VIDEO_WIDTH = 640;
  static const int DEFAULT_VIDEO_HEIGHT = 480;

protected:
  friend class MediaEngineDefault;

  TrackID mTrackID;
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<layers::ImageContainer> mImageContainer;

  SourceMediaStream* mSource;
  layers::PlanarYCbCrImage* mImage;
  MediaEngineVideoOptions mOpts;
  int mCb;
  int mCr;
};

class MediaEngineDefaultAudioSource : public nsITimerCallback,
                                      public MediaEngineAudioSource
{
public:
  MediaEngineDefaultAudioSource();
  ~MediaEngineDefaultAudioSource();

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual nsresult Allocate();
  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop(SourceMediaStream*, TrackID);
  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile);
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise) { return NS_OK; };
  virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime);
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime) {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

protected:
  TrackID mTrackID;
  nsCOMPtr<nsITimer> mTimer;

  SourceMediaStream* mSource;
};

class MediaEngineDefault : public MediaEngine
{
public:
  MediaEngineDefault()
  : mMutex("mozilla::MediaEngineDefault")
  {}
  ~MediaEngineDefault() {}

  virtual void EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >*);
  virtual void EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >*);

private:
  Mutex mMutex;
  

  nsTArray<nsRefPtr<MediaEngineVideoSource> > mVSources;
  nsTArray<nsRefPtr<MediaEngineAudioSource> > mASources;
};

}

#endif 
