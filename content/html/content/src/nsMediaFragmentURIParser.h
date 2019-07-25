




































#if !defined(nsMediaFragmentURIParser_h__)
#define nsMediaFragmentURIParser_h__

#include "nsString.h"
#include "nsTArray.h"









class nsMediaFragmentURIParser
{
  struct Pair
  {
    Pair(const nsAString& aName, const nsAString& aValue) :
      mName(aName), mValue(aValue) { }

    nsString mName;
    nsString mValue;
  };

public:
  
  
  nsMediaFragmentURIParser(const nsCString& aSpec);

  
  
  void Parse();

  
  
  
  double GetStartTime();

  
  
  
  double GetEndTime();

private:
  
  
  
  
  
  
  bool ParseNPT(nsDependentSubstring& aString, double& aStart, double& aEnd);
  bool ParseNPTTime(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTSec(nsDependentSubstring& aString, double& aSec);
  bool ParseNPTFraction(nsDependentSubstring& aString, double& aFraction);
  bool ParseNPTMMSS(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTHHMMSS(nsDependentSubstring& aString, double& aTime);
  bool ParseNPTHH(nsDependentSubstring& aString, PRUint32& aHour);
  bool ParseNPTMM(nsDependentSubstring& aString, PRUint32& aMinute);
  bool ParseNPTSS(nsDependentSubstring& aString, PRUint32& aSecond);

  
  nsCAutoString mHash;

  
  
  nsTArray<Pair> mFragments;
};

#endif
