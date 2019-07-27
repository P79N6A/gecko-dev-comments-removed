




#if !defined(MediaInfo_h)
#define MediaInfo_h

#include "nsSize.h"
#include "nsRect.h"
#include "ImageTypes.h"
#include "nsString.h"

namespace mozilla {

struct TrackInfo {
  void Init(const nsAString& aId,
            const nsAString& aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage,
            bool aEnabled)
  {
    mId = aId;
    mKind = aKind;
    mLabel = aLabel;
    mLanguage = aLanguage;
    mEnabled = aEnabled;
  }

  nsString mId;
  nsString mKind;
  nsString mLabel;
  nsString mLanguage;
  bool mEnabled;
};


class VideoInfo {
private:
  VideoInfo(int32_t aWidth, int32_t aHeight, bool aHasVideo)
    : mDisplay(aWidth, aHeight)
    , mStereoMode(StereoMode::MONO)
    , mHasVideo(aHasVideo)
    , mIsHardwareAccelerated(false)
  {
  }

public:
  VideoInfo()
    : VideoInfo(0, 0, false)
  {
    
    
    mTrackInfo.Init(NS_LITERAL_STRING("2"), NS_LITERAL_STRING("main"),
    EmptyString(), EmptyString(), true);
  }

  VideoInfo(int32_t aWidth, int32_t aHeight)
    : VideoInfo(aWidth, aHeight, true)
  {
  }

  
  
  nsIntSize mDisplay;

  
  StereoMode mStereoMode;

  
  bool mHasVideo;

  TrackInfo mTrackInfo;

  bool mIsHardwareAccelerated;
};

class AudioInfo {
public:
  AudioInfo()
    : mRate(44100)
    , mChannels(2)
    , mHasAudio(false)
  {
    
    
    mTrackInfo.Init(NS_LITERAL_STRING("1"), NS_LITERAL_STRING("main"),
    EmptyString(), EmptyString(), true);
  }

  
  uint32_t mRate;

  
  uint32_t mChannels;

  
  bool mHasAudio;

  TrackInfo mTrackInfo;
};

class MediaInfo {
public:
  MediaInfo() : mIsEncrypted(false) {}

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

  bool mIsEncrypted;

  
  VideoInfo mVideo;
  AudioInfo mAudio;
};

} 

#endif 
