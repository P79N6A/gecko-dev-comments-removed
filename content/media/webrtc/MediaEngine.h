



#ifndef MEDIAENGINE_H_
#define MEDIAENGINE_H_

#include "nsIDOMFile.h"
#include "DOMMediaStream.h"
#include "MediaStreamGraph.h"

namespace mozilla {








class MediaEngineVideoSource;
class MediaEngineAudioSource;
struct MediaEnginePrefs;

enum MediaEngineState {
  kAllocated,
  kStarted,
  kStopped,
  kReleased
};


enum {
  kVideoTrack = 1,
  kAudioTrack = 2
};

class MediaEngine
{
public:
  virtual ~MediaEngine() {}

  static const int DEFAULT_VIDEO_FPS = 30;
  static const int DEFAULT_VIDEO_MIN_FPS = 10;
  static const int DEFAULT_VIDEO_WIDTH = 640;
  static const int DEFAULT_VIDEO_HEIGHT = 480;

  

  virtual void EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >*) = 0;

  

  virtual void EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >*) = 0;
};




class MediaEngineSource : public nsISupports
{
public:
  virtual ~MediaEngineSource() {}

  
  virtual void GetName(nsAString&) = 0;

  
  virtual void GetUUID(nsAString&) = 0;

  
  virtual nsresult Allocate(const MediaEnginePrefs &aPrefs) = 0;

  
  virtual nsresult Deallocate() = 0;

  


  virtual nsresult Start(SourceMediaStream*, TrackID) = 0;

  



  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile) = 0;

  
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime) = 0;

  
  virtual nsresult Stop(SourceMediaStream *aSource, TrackID aID) = 0;

  
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise) = 0;

  
  bool IsAvailable() {
    if (mState == kAllocated || mState == kStarted) {
      return false;
    } else {
      return true;
    }
  }

  


protected:
  MediaEngineState mState;
};




struct MediaEnginePrefs {
  int32_t mWidth;
  int32_t mHeight;
  int32_t mFPS;
  int32_t mMinFPS;
};

class MediaEngineVideoSource : public MediaEngineSource
{
public:
  virtual ~MediaEngineVideoSource() {}
};




class MediaEngineAudioSource : public MediaEngineSource
{
public:
  virtual ~MediaEngineAudioSource() {}
};

}

#endif 
