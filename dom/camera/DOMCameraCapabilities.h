





#ifndef mozilla_dom_CameraCapabilities_h__
#define mozilla_dom_CameraCapabilities_h__

#include "nsString.h"
#include "nsAutoPtr.h"
#include "base/basictypes.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/CameraManagerBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"
#include "nsDataHashtable.h"
#include "ICameraControl.h"

struct JSContext;

namespace mozilla {
namespace dom {




class CameraRecorderVideoProfile MOZ_FINAL : public nsISupports
                                           , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CameraRecorderVideoProfile)

  explicit CameraRecorderVideoProfile(nsISupports* aParent,
    const ICameraControl::RecorderProfile::Video& aProfile);
  nsISupports* GetParentObject() const        { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint32_t BitsPerSecond() const              { return mBitrate; }
  uint32_t FramesPerSecond() const            { return mFramerate; }
  void GetCodec(nsAString& aCodec) const      { aCodec = mCodec; }

  void GetSize(dom::CameraSize& aSize) const  { aSize = mSize; }

  
  uint32_t Width() const                      { return mSize.mWidth; }
  uint32_t Height() const                     { return mSize.mHeight; }

protected:
  virtual ~CameraRecorderVideoProfile();

  nsCOMPtr<nsISupports> mParent;

  const nsString mCodec;
  uint32_t mBitrate;
  uint32_t mFramerate;
  dom::CameraSize mSize;

private:
  DISALLOW_EVIL_CONSTRUCTORS(CameraRecorderVideoProfile);
};




class CameraRecorderAudioProfile MOZ_FINAL : public nsISupports
                                           , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CameraRecorderAudioProfile)

  explicit CameraRecorderAudioProfile(nsISupports* aParent,
    const ICameraControl::RecorderProfile::Audio& aProfile);
  nsISupports* GetParentObject() const    { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint32_t BitsPerSecond() const          { return mBitrate; }
  uint32_t SamplesPerSecond() const       { return mSamplerate; }
  uint32_t Channels() const               { return mChannels; }
  void GetCodec(nsAString& aCodec) const  { aCodec = mCodec; }

protected:
  virtual ~CameraRecorderAudioProfile();

  nsCOMPtr<nsISupports> mParent;

  const nsString mCodec;
  uint32_t mBitrate;
  uint32_t mSamplerate;
  uint32_t mChannels;

private:
  DISALLOW_EVIL_CONSTRUCTORS(CameraRecorderAudioProfile);
};




class CameraRecorderProfile MOZ_FINAL : public nsISupports
                                      , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CameraRecorderProfile)

  explicit CameraRecorderProfile(nsISupports* aParent,
                                 const ICameraControl::RecorderProfile& aProfile);
  nsISupports* GetParentObject() const          { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetMimeType(nsAString& aMimeType) const  { aMimeType = mMimeType; }

  CameraRecorderVideoProfile* Video()           { return mVideo; }
  CameraRecorderAudioProfile* Audio()           { return mAudio; }

  void GetName(nsAString& aName) const          { aName = mName; }

  void
  GetContainerFormat(nsAString& aContainerFormat) const
  {
    aContainerFormat = mContainerFormat;
  }

protected:
  virtual ~CameraRecorderProfile();

  nsCOMPtr<nsISupports> mParent;

  const nsString mName;
  const nsString mContainerFormat;
  const nsString mMimeType;

  nsRefPtr<CameraRecorderVideoProfile> mVideo;
  nsRefPtr<CameraRecorderAudioProfile> mAudio;

private:
  DISALLOW_EVIL_CONSTRUCTORS(CameraRecorderProfile);
};




template<class T> class CameraClosedListenerProxy;

class CameraRecorderProfiles MOZ_FINAL : public nsISupports
                                       , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CameraRecorderProfiles)

  explicit CameraRecorderProfiles(nsISupports* aParent,
                                  ICameraControl* aCameraControl);
  nsISupports* GetParentObject() const { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  CameraRecorderProfile* NamedGetter(const nsAString& aName, bool& aFound);
  bool NameIsEnumerable(const nsAString& aName);
  void GetSupportedNames(unsigned aFlags, nsTArray<nsString>& aNames);

  virtual void OnHardwareClosed();

protected:
  virtual ~CameraRecorderProfiles();

  nsCOMPtr<nsISupports> mParent;
  nsRefPtr<ICameraControl> mCameraControl;
  nsRefPtrHashtable<nsStringHashKey, CameraRecorderProfile> mProfiles;
  nsRefPtr<CameraClosedListenerProxy<CameraRecorderProfiles>> mListener;

private:
  DISALLOW_EVIL_CONSTRUCTORS(CameraRecorderProfiles);
};




class CameraCapabilities MOZ_FINAL : public nsISupports
                                   , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CameraCapabilities)

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  explicit CameraCapabilities(nsPIDOMWindow* aWindow,
                              ICameraControl* aCameraControl);

  nsPIDOMWindow* GetParentObject() const { return mWindow; }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetPreviewSizes(nsTArray<CameraSize>& aRetVal);
  void GetPictureSizes(nsTArray<CameraSize>& aRetVal);
  void GetThumbnailSizes(nsTArray<CameraSize>& aRetVal);
  void GetVideoSizes(nsTArray<CameraSize>& aRetVal);
  void GetFileFormats(nsTArray<nsString>& aRetVal);
  void GetWhiteBalanceModes(nsTArray<nsString>& aRetVal);
  void GetSceneModes(nsTArray<nsString>& aRetVal);
  void GetEffects(nsTArray<nsString>& aRetVal);
  void GetFlashModes(nsTArray<nsString>& aRetVal);
  void GetFocusModes(nsTArray<nsString>& aRetVal);
  void GetZoomRatios(nsTArray<double>& aRetVal);
  uint32_t MaxFocusAreas();
  uint32_t MaxMeteringAreas();
  uint32_t MaxDetectedFaces();
  double MinExposureCompensation();
  double MaxExposureCompensation();
  double ExposureCompensationStep();
  void GetIsoModes(nsTArray<nsString>& aRetVal);

  CameraRecorderProfiles* RecorderProfiles();

  virtual void OnHardwareClosed();

protected:
  ~CameraCapabilities();

  nsresult TranslateToDictionary(uint32_t aKey, nsTArray<CameraSize>& aSizes);

  nsTArray<CameraSize> mPreviewSizes;
  nsTArray<CameraSize> mPictureSizes;
  nsTArray<CameraSize> mThumbnailSizes;
  nsTArray<CameraSize> mVideoSizes;

  nsTArray<nsString> mFileFormats;
  nsTArray<nsString> mWhiteBalanceModes;
  nsTArray<nsString> mSceneModes;
  nsTArray<nsString> mEffects;
  nsTArray<nsString> mFlashModes;
  nsTArray<nsString> mFocusModes;
  nsTArray<nsString> mIsoModes;

  nsTArray<double> mZoomRatios;

  uint32_t mMaxFocusAreas;
  uint32_t mMaxMeteringAreas;
  uint32_t mMaxDetectedFaces;

  double mMinExposureCompensation;
  double mMaxExposureCompensation;
  double mExposureCompensationStep;

  nsRefPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<ICameraControl> mCameraControl;
  nsRefPtr<CameraRecorderProfiles> mRecorderProfiles;
  nsRefPtr<CameraClosedListenerProxy<CameraCapabilities>> mListener;

private:
  DISALLOW_EVIL_CONSTRUCTORS(CameraCapabilities);
};

} 
} 

#endif 
