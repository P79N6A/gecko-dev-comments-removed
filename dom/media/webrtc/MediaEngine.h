



#ifndef MEDIAENGINE_H_
#define MEDIAENGINE_H_

#include "mozilla/RefPtr.h"
#include "nsIDOMFile.h"
#include "DOMMediaStream.h"
#include "MediaStreamGraph.h"
#include "mozilla/dom/MediaStreamTrackBinding.h"
#include "mozilla/dom/VideoStreamTrack.h"

namespace mozilla {

namespace dom {
class File;
}

enum {
  kVideoTrack = 1,
  kAudioTrack = 2,
  kTrackCount
};








class MediaEngineVideoSource;
class MediaEngineAudioSource;
class MediaEnginePrefs;

enum MediaEngineState {
  kAllocated,
  kStarted,
  kStopped,
  kReleased
};

class MediaEngine
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaEngine)

  static const int DEFAULT_VIDEO_FPS = 30;
  static const int DEFAULT_VIDEO_MIN_FPS = 10;
  static const int DEFAULT_43_VIDEO_WIDTH = 640;
  static const int DEFAULT_43_VIDEO_HEIGHT = 480;
  static const int DEFAULT_169_VIDEO_WIDTH = 1280;
  static const int DEFAULT_169_VIDEO_HEIGHT = 720;
  static const int DEFAULT_AUDIO_TIMER_MS = 10;

  

  virtual void EnumerateVideoDevices(dom::MediaSourceEnum,
                                     nsTArray<nsRefPtr<MediaEngineVideoSource> >*) = 0;

  

  virtual void EnumerateAudioDevices(dom::MediaSourceEnum,
                                     nsTArray<nsRefPtr<MediaEngineAudioSource> >*) = 0;

protected:
  virtual ~MediaEngine() {}
};




class MediaEngineSource : public nsISupports
{
public:
  
  
  static const unsigned int kMaxDeviceNameLength = 128;
  static const unsigned int kMaxUniqueIdLength = 256;

  virtual ~MediaEngineSource() {}

  
  virtual void GetName(nsAString&) = 0;

  
  virtual void GetUUID(nsAString&) = 0;

  
  virtual nsresult Deallocate() = 0;

  


  virtual nsresult Start(SourceMediaStream*, TrackID) = 0;

  
  virtual void SetDirectListeners(bool) = 0;

  
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime) = 0;

  
  virtual nsresult Stop(SourceMediaStream *aSource, TrackID aID) = 0;

  
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise,
                          int32_t aPlayoutDelay) = 0;

  


  virtual bool IsFake() = 0;

  
  virtual const dom::MediaSourceEnum GetMediaSource() = 0;

  
  
  class PhotoCallback {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PhotoCallback)

    
    
    virtual nsresult PhotoComplete(already_AddRefed<dom::File> aBlob) = 0;

    
    virtual nsresult PhotoError(nsresult aRv) = 0;

  protected:
    virtual ~PhotoCallback() {}
  };

  



  virtual nsresult TakePhoto(PhotoCallback* aCallback) = 0;

  
  bool IsAvailable() {
    if (mState == kAllocated || mState == kStarted) {
      return false;
    } else {
      return true;
    }
  }

  


  void SetHasFakeTracks(bool aHasFakeTracks) {
    mHasFakeTracks = aHasFakeTracks;
  }

protected:
  
  explicit MediaEngineSource(MediaEngineState aState)
    : mState(aState)
    , mHasFakeTracks(false)
  {}
  MediaEngineState mState;
  bool mHasFakeTracks;
};




class MediaEnginePrefs {
public:
  int32_t mWidth;
  int32_t mHeight;
  int32_t mFPS;
  int32_t mMinFPS;

  

  int32_t GetWidth(bool aHD = false) const {
    return mWidth? mWidth : (mHeight?
                             (mHeight * GetDefWidth(aHD)) / GetDefHeight(aHD) :
                             GetDefWidth(aHD));
  }

  int32_t GetHeight(bool aHD = false) const {
    return mHeight? mHeight : (mWidth?
                               (mWidth * GetDefHeight(aHD)) / GetDefWidth(aHD) :
                               GetDefHeight(aHD));
  }
private:
  static int32_t GetDefWidth(bool aHD = false) {
    
    
    if (aHD) {
      return MediaEngine::DEFAULT_169_VIDEO_WIDTH;
    }

    return MediaEngine::DEFAULT_43_VIDEO_WIDTH;
  }

  static int32_t GetDefHeight(bool aHD = false) {
    
    
    if (aHD) {
      return MediaEngine::DEFAULT_169_VIDEO_HEIGHT;
    }

    return MediaEngine::DEFAULT_43_VIDEO_HEIGHT;
  }
};

class MediaEngineVideoSource : public MediaEngineSource
{
public:
  virtual ~MediaEngineVideoSource() {}

  
  virtual nsresult Allocate(const dom::MediaTrackConstraints &aConstraints,
                            const MediaEnginePrefs &aPrefs) = 0;

  virtual uint32_t GetBestFitnessDistance(
      const nsTArray<const dom::MediaTrackConstraintSet*>& aConstraintSets) = 0;

protected:
  explicit MediaEngineVideoSource(MediaEngineState aState)
    : MediaEngineSource(aState) {}
  MediaEngineVideoSource()
    : MediaEngineSource(kReleased) {}
};




class MediaEngineAudioSource : public MediaEngineSource
{
public:
  virtual ~MediaEngineAudioSource() {}

  
  virtual nsresult Allocate(const dom::MediaTrackConstraints &aConstraints,
                            const MediaEnginePrefs &aPrefs) = 0;
protected:
  explicit MediaEngineAudioSource(MediaEngineState aState)
    : MediaEngineSource(aState) {}
  MediaEngineAudioSource()
    : MediaEngineSource(kReleased) {}

};

}

#endif 
