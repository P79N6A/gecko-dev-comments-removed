




#if !defined(MediaInfo_h)
#define MediaInfo_h

#include "nsSize.h"
#include "nsRect.h"
#include "ImageTypes.h"
#include "nsString.h"
#include "nsTArray.h"
#include "StreamBuffer.h" 

namespace mozilla {

struct TrackInfo {
  void Init(const nsAString& aId,
            const nsAString& aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage,
            bool aEnabled,
            TrackID aOutputId = TRACK_INVALID)
  {
    mId = aId;
    mKind = aKind;
    mLabel = aLabel;
    mLanguage = aLanguage;
    mEnabled = aEnabled;
    mOutputId = aOutputId;
  }

  nsString mId;
  nsString mKind;
  nsString mLabel;
  nsString mLanguage;
  bool mEnabled;
  TrackID mOutputId;
};


class VideoInfo {
private:
  void Init(int32_t aWidth, int32_t aHeight, bool aHasVideo)
  {
    mDisplay = nsIntSize(aWidth, aHeight);
    mStereoMode = StereoMode::MONO;
    mHasVideo = aHasVideo;

    
    
    mTrackInfo.Init(NS_LITERAL_STRING("2"), NS_LITERAL_STRING("main"),
                    EmptyString(), EmptyString(), true, 2);
  }

public:
  VideoInfo()
  {
    Init(0, 0, false);
  }

  VideoInfo(int32_t aWidth, int32_t aHeight)
  {
    Init(aWidth, aHeight, true);
  }

  
  
  nsIntSize mDisplay;

  
  StereoMode mStereoMode;

  
  bool mHasVideo;

  TrackInfo mTrackInfo;
};

class AudioInfo {
public:
  AudioInfo()
    : mRate(44100)
    , mChannels(2)
    , mHasAudio(false)
  {
    
    
    mTrackInfo.Init(NS_LITERAL_STRING("1"), NS_LITERAL_STRING("main"),
                    EmptyString(), EmptyString(), true, 1);
  }

  
  uint32_t mRate;

  
  uint32_t mChannels;

  
  bool mHasAudio;

  TrackInfo mTrackInfo;
};

class EncryptionInfo {
public:
  EncryptionInfo() : mIsEncrypted(false) {}

  
  nsString mType;

  
  nsTArray<uint8_t> mInitData;

  
  bool mIsEncrypted;
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

  bool IsEncrypted() const
  {
    return mCrypto.mIsEncrypted;
  }

  bool HasValidMedia() const
  {
    return HasVideo() || HasAudio();
  }

  
  VideoInfo mVideo;
  AudioInfo mAudio;

  EncryptionInfo mCrypto;
};

} 

#endif 
