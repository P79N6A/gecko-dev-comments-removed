



#ifndef DOM_CAMERA_GONK_RECORDER_PROFILES_H
#define DOM_CAMERA_GONK_RECORDER_PROFILES_H

#include <media/MediaProfiles.h>
#include "CameraRecorderProfiles.h"
#include "ICameraControl.h"

#ifndef CHECK_SETARG_RETURN
#define CHECK_SETARG_RETURN(x, rv)      \
  do {                                  \
    if (x) {                            \
      DOM_CAMERA_LOGE(#x " failed\n");  \
      return rv;                        \
    }                                   \
  } while(0)
#endif

#ifndef CHECK_SETARG
#define CHECK_SETARG(x) CHECK_SETARG_RETURN(x, NS_ERROR_NOT_AVAILABLE)
#endif

namespace android {
class GonkRecorder;
};

namespace mozilla {




class GonkRecorderVideoProfile : public RecorderVideoProfile
{
public:
  GonkRecorderVideoProfile(uint32_t aCameraId, uint32_t aQualityIndex);
  ~GonkRecorderVideoProfile();
  android::video_encoder GetPlatformCodec() const { return mPlatformCodec; }

protected:
  android::video_encoder mPlatformCodec;
};




class GonkRecorderAudioProfile : public RecorderAudioProfile
{
public:
  GonkRecorderAudioProfile(uint32_t aCameraId, uint32_t aQualityIndex);
  ~GonkRecorderAudioProfile();
  android::audio_encoder GetPlatformCodec() const { return mPlatformCodec; }

protected:
  android::audio_encoder mPlatformCodec;
};




class GonkRecorderProfile : public RecorderProfileBase<GonkRecorderAudioProfile, GonkRecorderVideoProfile>
{
public:
  GonkRecorderProfile(uint32_t aCameraId, uint32_t aQualityIndex);

  GonkRecorderAudioProfile* GetGonkAudioProfile() { return &mAudio; }
  GonkRecorderVideoProfile* GetGonkVideoProfile() { return &mVideo; }

  android::output_format GetOutputFormat() const { return mPlatformOutputFormat; }

  
  
  
  
  
  
  nsresult ConfigureRecorder(android::GonkRecorder* aRecorder);

protected:
  virtual ~GonkRecorderProfile();

  android::output_format mPlatformOutputFormat;
};




class GonkRecorderProfileManager : public RecorderProfileManager
{
public:
  GonkRecorderProfileManager(uint32_t aCameraId);

  




  void SetSupportedResolutions(const nsTArray<ICameraControl::Size>& aSizes)
    { mSupportedSizes = aSizes; }

  



  void ClearSupportedResolutions() { mSupportedSizes.Clear(); }

  bool IsSupported(uint32_t aQualityIndex) const;

  already_AddRefed<RecorderProfile> Get(uint32_t aQualityIndex) const;
  already_AddRefed<GonkRecorderProfile> Get(const char* aProfileName) const;

protected:
  virtual ~GonkRecorderProfileManager();

  nsTArray<ICameraControl::Size> mSupportedSizes;
};

}; 

#endif 
