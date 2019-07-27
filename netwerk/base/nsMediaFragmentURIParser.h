




#if !defined(nsMediaFragmentURIParser_h__)
#define nsMediaFragmentURIParser_h__

#include "mozilla/Maybe.h"
#include "nsStringFwd.h"
#include "nsRect.h"

class nsIURI;










namespace mozilla { namespace net {

enum ClipUnit
{
  eClipUnit_Pixel,
  eClipUnit_Percent,
};

class nsMediaFragmentURIParser
{
public:
  
  explicit nsMediaFragmentURIParser(nsIURI* aURI);

  
  explicit nsMediaFragmentURIParser(nsCString& aRef);

  
  bool HasStartTime() const { return mStart.isSome(); }

  
  
  double GetStartTime() const { return *mStart; }

  
  bool HasEndTime() const { return mEnd.isSome(); }

  
  
  double GetEndTime() const { return *mEnd; }

  
  bool HasClip() const { return mClip.isSome(); }

  
  bool HasResolution() const { return mResolution.isSome(); }

  
  nsIntSize GetResolution() const { return *mResolution; }

  
  
  
  nsIntRect GetClip() const { return *mClip; }

  
  
  ClipUnit GetClipUnit() const { return mClipUnit; }

  bool HasSampleSize() const { return mSampleSize.isSome(); }

  int GetSampleSize() const { return *mSampleSize; }

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
  bool ParseMozSampleSize(nsDependentSubstring aString);

  
  Maybe<double>    mStart;
  Maybe<double>    mEnd;
  Maybe<nsIntRect> mClip;
  ClipUnit         mClipUnit;
  Maybe<nsIntSize> mResolution;
  Maybe<int>       mSampleSize;
};

}} 

#endif
