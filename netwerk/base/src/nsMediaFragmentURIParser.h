




#if !defined(nsMediaFragmentURIParser_h__)
#define nsMediaFragmentURIParser_h__

#include "nsIURI.h"
#include "nsString.h"
#include "nsRect.h"










namespace mozilla { namespace net {

enum ClipUnit
{
  eClipUnit_Pixel,
  eClipUnit_Percent,
};

class nsMediaFragmentURIParser
{
public:
  
  nsMediaFragmentURIParser(nsIURI* aURI);

  
  bool HasStartTime() const { return !mStart.empty(); }

  
  
  double GetStartTime() const { return mStart.ref(); }

  
  bool HasEndTime() const { return !mEnd.empty(); }

  
  
  double GetEndTime() const { return mEnd.ref(); }

  
  bool HasClip() const { return !mClip.empty(); }

  
  bool HasResolution() const { return !mResolution.empty(); }

  
  nsIntSize GetResolution() const { return mResolution.ref(); }

  
  
  
  nsIntRect GetClip() const { return mClip.ref(); }

  
  
  ClipUnit GetClipUnit() const { return mClipUnit; }

private:
  
  
  void Parse(nsACString& aRef);

  
  
  
  
  
  
  bool ParseNPT(nsDependentSubstring aString);
  bool ParseNPTTime(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTSec(nsDependentSubstring& aString, double& aSec);
  bool ParseNPTFraction(nsDependentSubstring& aString, double& aFraction);
  bool ParseNPTMMSS(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTHHMMSS(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTHH(nsDependentSubstring& aString, uint32_t& aHour);
  bool ParseNPTMM(nsDependentSubstring& aString, uint32_t& aMinute);
  bool ParseNPTSS(nsDependentSubstring& aString, uint32_t& aSecond);
  bool ParseXYWH(nsDependentSubstring aString);
  bool ParseMozResolution(nsDependentSubstring aString);

  
  Maybe<double>    mStart;
  Maybe<double>    mEnd;
  Maybe<nsIntRect> mClip;
  ClipUnit         mClipUnit;
  Maybe<nsIntSize> mResolution;
};

}} 

#endif
