



#ifndef MEDIAENGINE_H_
#define MEDIAENGINE_H_

#include "nsIDOMFile.h"
#include "DOMMediaStream.h"
#include "MediaStreamGraph.h"

namespace mozilla {








class MediaEngineVideoSource;
class MediaEngineAudioSource;

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

  

  virtual void EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >*) = 0;

  

  virtual void EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >*) = 0;
};




class MediaEngineSource : public nsISupports
{
public:
  virtual ~MediaEngineSource() {}

  
  virtual void GetName(nsAString&) = 0;

  
  virtual void GetUUID(nsAString&) = 0;

  
  virtual nsresult Allocate() = 0;

  
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




enum MediaEngineVideoCodecType {
  kVideoCodecH263,
  kVideoCodecVP8,
  kVideoCodecI420
};

struct MediaEngineVideoOptions {
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mMaxFPS;
  MediaEngineVideoCodecType codecType;
};

class MediaEngineVideoSource : public MediaEngineSource
{
public:
  virtual ~MediaEngineVideoSource() {}

  

  virtual const MediaEngineVideoOptions *GetOptions() = 0;
};




class MediaEngineAudioSource : public MediaEngineSource
{
public:
  virtual ~MediaEngineAudioSource() {}
};

}

#endif 
