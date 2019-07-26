




#if !defined(MediaInfo_h)
#define MediaInfo_h

#include "nsSize.h"
#include "nsRect.h"
#include "ImageTypes.h"

namespace mozilla {


class VideoInfo {
public:
  VideoInfo()
    : mDisplay(0,0)
    , mStereoMode(StereoMode::MONO)
    , mHasVideo(false)
  {}

  
  
  nsIntSize mDisplay;

  
  StereoMode mStereoMode;

  
  bool mHasVideo;
};

class AudioInfo {
public:
  AudioInfo()
    : mRate(44100)
    , mChannels(2)
    , mHasAudio(false)
  {}

  
  uint32_t mRate;

  
  uint32_t mChannels;

  
  bool mHasAudio;
};

class MediaInfo {
public:
  bool HasVideo() const
  {
    return mVideo.mHasVideo;
  }

  bool HasAudio() const
  {
    return mAudio.mHasAudio;
  }

  bool HasValidMedia() const
  {
    return HasVideo() || HasAudio();
  }

  VideoInfo mVideo;
  AudioInfo mAudio;
};

} 

#endif 
