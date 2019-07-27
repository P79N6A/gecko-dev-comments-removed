




#if !defined(MediaInfo_h)
#define MediaInfo_h

#include "mozilla/UniquePtr.h"
#include "nsRect.h"
#include "nsRefPtr.h"
#include "nsSize.h"
#include "nsString.h"
#include "nsTArray.h"
#include "ImageTypes.h"
#include "MediaData.h"
#include "StreamBuffer.h" 

namespace mozilla {

class AudioInfo;
class VideoInfo;
class TextInfo;

class TrackInfo {
public:
  enum TrackType {
    kUndefinedTrack,
    kAudioTrack,
    kVideoTrack,
    kTextTrack
  };
  TrackInfo(TrackType aType,
            const nsAString& aId,
            const nsAString& aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage,
            bool aEnabled,
            TrackID aTrackId = TRACK_INVALID)
    : mId(aId)
    , mKind(aKind)
    , mLabel(aLabel)
    , mLanguage(aLanguage)
    , mEnabled(aEnabled)
    , mTrackId(aTrackId)
    , mDuration(0)
    , mMediaTime(0)
    , mType(aType)
  {
    MOZ_COUNT_CTOR(TrackInfo);
  }

  
  void Init(TrackType aType,
            const nsAString& aId,
            const nsAString& aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage,
            bool aEnabled,
            TrackID aTrackId = TRACK_INVALID)
  {
    mId = aId;
    mKind = aKind;
    mLabel = aLabel;
    mLanguage = aLanguage;
    mEnabled = aEnabled;
    mTrackId = aTrackId;
    mType = aType;
  }

  
  nsString mId;
  nsString mKind;
  nsString mLabel;
  nsString mLanguage;
  bool mEnabled;

  TrackID mTrackId;

  nsAutoCString mMimeType;
  int64_t mDuration;
  int64_t mMediaTime;
  CryptoTrack mCrypto;

  virtual AudioInfo* GetAsAudioInfo()
  {
    return nullptr;
  }
  virtual VideoInfo* GetAsVideoInfo()
  {
    return nullptr;
  }
  virtual TextInfo* GetAsTextInfo()
  {
    return nullptr;
  }
  virtual const AudioInfo* GetAsAudioInfo() const
  {
    return nullptr;
  }
  virtual const VideoInfo* GetAsVideoInfo() const
  {
    return nullptr;
  }
  virtual const TextInfo* GetAsTextInfo() const
  {
    return nullptr;
  }

  bool IsAudio() const
  {
    return !!GetAsAudioInfo();
  }
  bool IsVideo() const
  {
    return !!GetAsVideoInfo();
  }
  bool IsText() const
  {
    return !!GetAsTextInfo();
  }
  TrackType GetType() const
  {
    return mType;
  }

  bool virtual IsValid() const = 0;

  virtual UniquePtr<TrackInfo> Clone() const = 0;

  virtual ~TrackInfo()
  {
    MOZ_COUNT_DTOR(TrackInfo);
  }

protected:
  TrackInfo(const TrackInfo& aOther)
  {
    mId = aOther.mId;
    mKind = aOther.mKind;
    mLabel = aOther.mLabel;
    mLanguage = aOther.mLanguage;
    mEnabled = aOther.mEnabled;
    mTrackId = aOther.mTrackId;
    mMimeType = aOther.mMimeType;
    mDuration = aOther.mDuration;
    mMediaTime = aOther.mMediaTime;
    mCrypto = aOther.mCrypto;
    mType = aOther.mType;
    MOZ_COUNT_CTOR(TrackInfo);
  }

private:
  TrackType mType;
};


class VideoInfo : public TrackInfo {
public:
  VideoInfo()
    : VideoInfo(-1, -1)
  {
  }

  VideoInfo(int32_t aWidth, int32_t aHeight)
    : TrackInfo(kVideoTrack, NS_LITERAL_STRING("2"), NS_LITERAL_STRING("main"),
                EmptyString(), EmptyString(), true, 2)
    , mDisplay(nsIntSize(aWidth, aHeight))
    , mStereoMode(StereoMode::MONO)
    , mImage(nsIntSize(aWidth, aHeight))
    , mCodecSpecificConfig(new MediaByteBuffer)
    , mExtraData(new MediaByteBuffer)
  {
  }

  VideoInfo(const VideoInfo& aOther)
    : TrackInfo(aOther)
    , mDisplay(aOther.mDisplay)
    , mStereoMode(aOther.mStereoMode)
    , mImage(aOther.mImage)
    , mCodecSpecificConfig(aOther.mCodecSpecificConfig)
    , mExtraData(aOther.mExtraData)
  {
  }

  virtual bool IsValid() const override
  {
    return mDisplay.width > 0 && mDisplay.height > 0;
  }

  virtual VideoInfo* GetAsVideoInfo() override
  {
    return this;
  }

  virtual const VideoInfo* GetAsVideoInfo() const override
  {
    return this;
  }

  virtual UniquePtr<TrackInfo> Clone() const override
  {
    return MakeUnique<VideoInfo>(*this);
  }

  
  
  nsIntSize mDisplay;

  
  StereoMode mStereoMode;

  
  nsIntSize mImage;
  nsRefPtr<MediaByteBuffer> mCodecSpecificConfig;
  nsRefPtr<MediaByteBuffer> mExtraData;
};

class AudioInfo : public TrackInfo {
public:
  AudioInfo()
    : TrackInfo(kAudioTrack, NS_LITERAL_STRING("1"), NS_LITERAL_STRING("main"),
                EmptyString(), EmptyString(), true, 1)
    , mRate(0)
    , mChannels(0)
    , mBitDepth(0)
    , mProfile(0)
    , mExtendedProfile(0)
    , mCodecSpecificConfig(new MediaByteBuffer)
    , mExtraData(new MediaByteBuffer)
  {
  }

  AudioInfo(const AudioInfo& aOther)
    : TrackInfo(aOther)
    , mRate(aOther.mRate)
    , mChannels(aOther.mChannels)
    , mBitDepth(aOther.mBitDepth)
    , mProfile(aOther.mProfile)
    , mExtendedProfile(aOther.mExtendedProfile)
    , mCodecSpecificConfig(aOther.mCodecSpecificConfig)
    , mExtraData(aOther.mExtraData)
  {
  }

  virtual bool IsValid() const override
  {
    return mChannels > 0 && mRate > 0;
  }

  virtual AudioInfo* GetAsAudioInfo() override
  {
    return this;
  }

  virtual const AudioInfo* GetAsAudioInfo() const override
  {
    return this;
  }

  virtual UniquePtr<TrackInfo> Clone() const override
  {
    return MakeUnique<AudioInfo>(*this);
  }

  
  uint32_t mRate;

  
  uint32_t mChannels;

  
  uint32_t mBitDepth;

  
  int8_t mProfile;

  
  int8_t mExtendedProfile;

  nsRefPtr<MediaByteBuffer> mCodecSpecificConfig;
  nsRefPtr<MediaByteBuffer> mExtraData;

};

class EncryptionInfo {
public:
  struct InitData {
    template<typename AInitDatas>
    InitData(const nsAString& aType, AInitDatas&& aInitData)
      : mType(aType)
      , mInitData(Forward<AInitDatas>(aInitData))
    {
    }

    
    nsString mType;

    
    nsTArray<uint8_t> mInitData;
  };
  typedef nsTArray<InitData> InitDatas;

  
  bool IsEncrypted() const
  {
    return !mInitDatas.IsEmpty();
  }

  template<typename AInitDatas>
  void AddInitData(const nsAString& aType, AInitDatas&& aInitData)
  {
    mInitDatas.AppendElement(InitData(aType, Forward<AInitDatas>(aInitData)));
  }

  void AddInitData(const EncryptionInfo& aInfo)
  {
    mInitDatas.AppendElements(aInfo.mInitDatas);
  }

  
  InitDatas mInitDatas;
};

class MediaInfo {
public:
  bool HasVideo() const
  {
    return mVideo.IsValid();
  }

  bool HasAudio() const
  {
    return mAudio.IsValid();
  }

  bool IsEncrypted() const
  {
    return mCrypto.IsEncrypted();
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
