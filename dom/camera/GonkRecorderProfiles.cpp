















#include "GonkRecorderProfiles.h"
#include "nsMimeTypes.h"
#include "CameraControlImpl.h"
#include "CameraCommon.h"

#ifdef MOZ_WIDGET_GONK
#include "GonkRecorder.h"
#endif

using namespace mozilla;
using namespace android;

namespace mozilla {

struct ProfileConfig {
  const char* name;
  int quality;
};

#define DEF_GONK_RECORDER_PROFILE(e, n) { n, e },
static const ProfileConfig ProfileList[] = {
  #include "GonkRecorderProfiles.def"
};

static const size_t ProfileListSize = MOZ_ARRAY_LENGTH(ProfileList);

struct ProfileConfigDetect {
  const char* name;
  uint32_t width;
  uint32_t height;
};

#define DEF_GONK_RECORDER_PROFILE_DETECT(n, w, h) { n, w, h },
static const ProfileConfigDetect ProfileListDetect[] = {
  #include "GonkRecorderProfiles.def"
};

static const size_t ProfileListDetectSize = MOZ_ARRAY_LENGTH(ProfileListDetect);

};

 nsClassHashtable<nsUint32HashKey, ProfileHashtable> GonkRecorderProfile::sProfiles;
 android::MediaProfiles* sMediaProfiles = nullptr;

static MediaProfiles*
GetMediaProfiles()
{
  if (!sMediaProfiles) {
    sMediaProfiles = MediaProfiles::getInstance();
  }
  MOZ_ASSERT(sMediaProfiles);
  return sMediaProfiles;
}

static bool
IsProfileSupported(uint32_t aCameraId, int aQuality)
{
  MediaProfiles* profiles = GetMediaProfiles();
  return profiles->hasCamcorderProfile(static_cast<int>(aCameraId),
                                       static_cast<camcorder_quality>(aQuality));
}

static int
GetProfileParameter(uint32_t aCameraId, int aQuality, const char* aParameter)
{
  MediaProfiles* profiles = GetMediaProfiles();
  return profiles->getCamcorderProfileParamByName(aParameter, static_cast<int>(aCameraId),
                                                  static_cast<camcorder_quality>(aQuality));
}

 bool
GonkRecorderVideo::Translate(video_encoder aCodec, nsAString& aCodecName)
{
  switch (aCodec) {
    case VIDEO_ENCODER_H263:
      aCodecName.AssignASCII("h263");
      break;

    case VIDEO_ENCODER_H264:
      aCodecName.AssignASCII("h264");
      break;

    case VIDEO_ENCODER_MPEG_4_SP:
      aCodecName.AssignASCII("mpeg4sp");
      break;

    default:
      return false;
  }

  return true;
}

int
GonkRecorderVideo::GetProfileParameter(const char* aParameter)
{
  return ::GetProfileParameter(mCameraId, mQuality, aParameter);
}

GonkRecorderVideo::GonkRecorderVideo(uint32_t aCameraId, int aQuality)
  : mCameraId(aCameraId)
  , mQuality(aQuality)
  , mIsValid(false)
{
  mPlatformEncoder = static_cast<video_encoder>(GetProfileParameter("vid.codec"));
  bool isValid = Translate(mPlatformEncoder, mCodec);

  int v = GetProfileParameter("vid.width");
  if (v >= 0) {
    mSize.width = v;
  } else {
    isValid = false;
  }
  v = GetProfileParameter("vid.height");
  if (v >= 0) {
    mSize.height = v;
  } else {
    isValid = false;
  }
  v = GetProfileParameter("vid.bps");
  if (v >= 0) {
    mBitsPerSecond = v;
  } else {
    isValid = false;
  }
  v = GetProfileParameter("vid.fps");
  if (v >= 0) {
    mFramesPerSecond = v;
  } else {
    isValid = false;
  }

  mIsValid = isValid;
}

 bool
GonkRecorderAudio::Translate(audio_encoder aCodec, nsAString& aCodecName)
{
  switch (aCodec) {
    case AUDIO_ENCODER_AMR_NB:
      aCodecName.AssignASCII("amrnb");
      break;

    case AUDIO_ENCODER_AMR_WB:
      aCodecName.AssignASCII("amrwb");
      break;

    case AUDIO_ENCODER_AAC:
      aCodecName.AssignASCII("aac");
      break;

    default:
      return false;
  }

  return true;
}

int
GonkRecorderAudio::GetProfileParameter(const char* aParameter)
{
  return ::GetProfileParameter(mCameraId, mQuality, aParameter);
}

GonkRecorderAudio::GonkRecorderAudio(uint32_t aCameraId, int aQuality)
  : mCameraId(aCameraId)
  , mQuality(aQuality)
  , mIsValid(false)
{
  mPlatformEncoder = static_cast<audio_encoder>(GetProfileParameter("aud.codec"));
  bool isValid = Translate(mPlatformEncoder, mCodec);

  int v = GetProfileParameter("aud.ch");
  if (v >= 0) {
    mChannels = v;
  } else {
    isValid = false;
  }
  v = GetProfileParameter("aud.bps");
  if (v >= 0) {
    mBitsPerSecond = v;
  } else {
    isValid = false;
  }
  v = GetProfileParameter("aud.hz");
  if (v >= 0) {
    mSamplesPerSecond = v;
  } else {
    isValid = false;
  }

  mIsValid = isValid;
}

 bool
GonkRecorderProfile::Translate(output_format aContainer, nsAString& aContainerName)
{
  switch (aContainer) {
    case OUTPUT_FORMAT_THREE_GPP:
      aContainerName.AssignASCII("3gp");
      break;

    case OUTPUT_FORMAT_MPEG_4:
      aContainerName.AssignASCII("mp4");
      break;

    default:
      return false;
  }

  return true;
}

 bool
GonkRecorderProfile::GetMimeType(output_format aContainer, nsAString& aMimeType)
{
  switch (aContainer) {
    case OUTPUT_FORMAT_THREE_GPP:
      aMimeType.AssignASCII(VIDEO_3GPP);
      break;

    case OUTPUT_FORMAT_MPEG_4:
      aMimeType.AssignASCII(VIDEO_MP4);
      break;

    default:
      return false;
  }

  return true;
}

int
GonkRecorderProfile::GetProfileParameter(const char* aParameter)
{
  return ::GetProfileParameter(mCameraId, mQuality, aParameter);
}

GonkRecorderProfile::GonkRecorderProfile(uint32_t aCameraId,
                                         int aQuality)
  : GonkRecorderProfileBase<GonkRecorderAudio, GonkRecorderVideo>(aCameraId,
                                                                  aQuality)
  , mCameraId(aCameraId)
  , mQuality(aQuality)
  , mIsValid(false)
{
  mOutputFormat = static_cast<output_format>(GetProfileParameter("file.format"));
  bool isValid = Translate(mOutputFormat, mContainer);
  isValid = GetMimeType(mOutputFormat, mMimeType) ? isValid : false;

  mIsValid = isValid && mAudio.IsValid() && mVideo.IsValid();
}

 PLDHashOperator
GonkRecorderProfile::Enumerate(const nsAString& aProfileName,
                               GonkRecorderProfile* aProfile,
                               void* aUserArg)
{
  nsTArray<nsRefPtr<ICameraControl::RecorderProfile>>* profiles =
    static_cast<nsTArray<nsRefPtr<ICameraControl::RecorderProfile>>*>(aUserArg);
  MOZ_ASSERT(profiles);
  profiles->AppendElement(aProfile);
  return PL_DHASH_NEXT;
}


already_AddRefed<GonkRecorderProfile>
GonkRecorderProfile::CreateProfile(uint32_t aCameraId, int aQuality)
{
  if (!IsProfileSupported(aCameraId, aQuality)) {
    DOM_CAMERA_LOGI("Profile %d not supported by platform\n", aQuality);
    return nullptr;
  }

  nsRefPtr<GonkRecorderProfile> profile = new GonkRecorderProfile(aCameraId, aQuality);
  if (!profile->IsValid()) {
    DOM_CAMERA_LOGE("Profile %d is not valid\n", aQuality);
    return nullptr;
  }

  return profile.forget();
}


ProfileHashtable*
GonkRecorderProfile::GetProfileHashtable(uint32_t aCameraId)
{
  ProfileHashtable* profiles = sProfiles.Get(aCameraId);
  if (!profiles) {
    profiles = new ProfileHashtable();
    sProfiles.Put(aCameraId, profiles);

    

    int highestKnownQuality = CAMCORDER_QUALITY_LIST_START - 1;
    for (size_t i = 0; i < ProfileListSize; ++i) {
      const ProfileConfig& p = ProfileList[i];
      if (p.quality > highestKnownQuality) {
        highestKnownQuality = p.quality;
      }

      nsRefPtr<GonkRecorderProfile> profile = CreateProfile(aCameraId, p.quality);
      if (!profile) {
        continue;
      }

      DOM_CAMERA_LOGI("Profile %d '%s' supported by platform\n", p.quality, p.name);
      profile->mName.AssignASCII(p.name);
      profiles->Put(profile->GetName(), profile);
    }

    












    for (int q = highestKnownQuality + 1; q <= CAMCORDER_QUALITY_LIST_END; ++q) {
      nsRefPtr<GonkRecorderProfile> profile = CreateProfile(aCameraId, q);
      if (!profile) {
        continue;
      }

      const ICameraControl::Size& s = profile->GetVideo().GetSize();
      size_t match;
      for (match = 0; match < ProfileListDetectSize; ++match) {
        const ProfileConfigDetect& p = ProfileListDetect[match];
        if (s.width == p.width && s.height == p.height) {
          DOM_CAMERA_LOGI("Profile %d '%s' supported by platform\n", q, p.name);
          profile->mName.AssignASCII(p.name);
          profiles->Put(profile->GetName(), profile);
          break;
        }
      }

      if (match == ProfileListDetectSize) {
        DOM_CAMERA_LOGW("Profile %d size %u x %u is not recognized\n",
                        q, s.width, s.height);
      }
    }
  }
  return profiles;
}

 nsresult
GonkRecorderProfile::GetAll(uint32_t aCameraId,
                            nsTArray<nsRefPtr<ICameraControl::RecorderProfile>>& aProfiles)
{
  ProfileHashtable* profiles = GetProfileHashtable(aCameraId);
  if (!profiles) {
    return NS_ERROR_FAILURE;
  }

  aProfiles.Clear();
  profiles->EnumerateRead(Enumerate, static_cast<void*>(&aProfiles));

  return NS_OK;
}

#ifdef MOZ_WIDGET_GONK
nsresult
GonkRecorderProfile::ConfigureRecorder(GonkRecorder& aRecorder)
{
  static const size_t SIZE = 256;
  char buffer[SIZE];

  
  CHECK_SETARG(aRecorder.setAudioSource(AUDIO_SOURCE_CAMCORDER));
  CHECK_SETARG(aRecorder.setVideoSource(VIDEO_SOURCE_CAMERA));
  CHECK_SETARG(aRecorder.setOutputFormat(mOutputFormat));
  CHECK_SETARG(aRecorder.setVideoFrameRate(mVideo.GetFramesPerSecond()));
  CHECK_SETARG(aRecorder.setVideoSize(mVideo.GetSize().width, mVideo.GetSize().height));
  CHECK_SETARG(aRecorder.setVideoEncoder(mVideo.GetPlatformEncoder()));
  CHECK_SETARG(aRecorder.setAudioEncoder(mAudio.GetPlatformEncoder()));

  snprintf(buffer, SIZE, "video-param-encoding-bitrate=%d", mVideo.GetBitsPerSecond());
  CHECK_SETARG(aRecorder.setParameters(String8(buffer)));

  snprintf(buffer, SIZE, "audio-param-encoding-bitrate=%d", mAudio.GetBitsPerSecond());
  CHECK_SETARG(aRecorder.setParameters(String8(buffer)));

  snprintf(buffer, SIZE, "audio-param-number-of-channels=%d", mAudio.GetChannels());
  CHECK_SETARG(aRecorder.setParameters(String8(buffer)));

  snprintf(buffer, SIZE, "audio-param-sampling-rate=%d", mAudio.GetSamplesPerSecond());
  CHECK_SETARG(aRecorder.setParameters(String8(buffer)));

  return NS_OK;
}

 nsresult
GonkRecorderProfile::ConfigureRecorder(android::GonkRecorder& aRecorder,
                                       uint32_t aCameraId,
                                       const nsAString& aProfileName)
{
  ProfileHashtable* profiles = GetProfileHashtable(aCameraId);
  if (!profiles) {
    return NS_ERROR_FAILURE;
  }

  GonkRecorderProfile* profile;
  if (!profiles->Get(aProfileName, &profile)) {
    return NS_ERROR_INVALID_ARG;
  }

  return profile->ConfigureRecorder(aRecorder);
}
#endif
