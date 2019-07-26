




#if !defined(nsMediaFragmentURIParser_h__)
#define nsMediaFragmentURIParser_h__

#include "nsIURI.h"
#include "nsString.h"
#include "nsRect.h"










enum ClipUnit
{
  eClipUnit_Pixel,
  eClipUnit_Percent,
};

class nsMediaFragmentURIParser
{
public:
  
  nsMediaFragmentURIParser(nsIURI* aURI);

  
  bool HasStartTime() const { return mHasStart; }

  
  
  double GetStartTime() const { return mStart; }

  
  bool HasEndTime() const { return mHasEnd; }

  
  
  double GetEndTime() const { return mEnd; }

  
  bool HasClip() const { return mHasClip; }

  
  
  
  nsIntRect GetClip() const { return mClip; }

  
  
  ClipUnit GetClipUnit() const { return mClipUnit; }

private:
  
  
  void Parse(nsACString& aRef);

  
  
  
  
  
  
  bool ParseNPT(nsDependentSubstring& aString);
  bool ParseNPTTime(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTSec(nsDependentSubstring& aString, double& aSec);
  bool ParseNPTFraction(nsDependentSubstring& aString, double& aFraction);
  bool ParseNPTMMSS(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTHHMMSS(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTHH(nsDependentSubstring& aString, uint32_t& aHour);
  bool ParseNPTMM(nsDependentSubstring& aString, uint32_t& aMinute);
  bool ParseNPTSS(nsDependentSubstring& aString, uint32_t& aSecond);
  bool ParseXYWH(nsDependentSubstring& aString);

  
  double    mStart;
  double    mEnd;
  nsIntRect mClip;
  ClipUnit  mClipUnit;

  
  bool mHasStart : 1;
  bool mHasEnd   : 1;
  bool mHasClip  : 1;
};

#endif
