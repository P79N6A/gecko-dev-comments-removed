





#ifndef mozilla_dom_TVTypes_h
#define mozilla_dom_TVTypes_h

#include "nsITVService.h"

namespace mozilla {
namespace dom {

class TVTunerData MOZ_FINAL : public nsITVTunerData
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITVTUNERDATA

private:
  ~TVTunerData();

  nsString mId;
  char** mSupportedSourceTypes;
  uint32_t mCount;
};

class TVChannelData MOZ_FINAL : public nsITVChannelData
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITVCHANNELDATA

private:
  ~TVChannelData() {}

  nsString mNetworkId;
  nsString mTransportStreamId;
  nsString mServiceId;
  nsString mType;
  nsString mNumber;
  nsString mName;
  bool mIsEmergency;
  bool mIsFree;
};

class TVProgramData MOZ_FINAL : public nsITVProgramData
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITVPROGRAMDATA

private:
  ~TVProgramData();

  nsString mEventId;
  nsString mTitle;
  uint64_t mStartTime;
  uint64_t mDuration;
  nsString mDescription;
  nsString mRating;
  char** mAudioLanguages;
  uint32_t mAudioLanguageCount;
  char** mSubtitleLanguages;
  uint32_t mSubtitleLanguageCount;
};

} 
} 

#endif 
