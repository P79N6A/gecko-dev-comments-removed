




































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
  
  
  
  
  
  
  PRBool ParseNPT(nsDependentSubstring& aString, double& aStart, double& aEnd);
  PRBool ParseNPTTime(nsDependentSubstring& aString, double& aTime);
  PRBool ParseNPTSec(nsDependentSubstring& aString, double& aSec);
  PRBool ParseNPTFraction(nsDependentSubstring& aString, double& aFraction);
  PRBool ParseNPTMMSS(nsDependentSubstring& aString, double& aTime);
  PRBool ParseNPTHHMMSS(nsDependentSubstring& aString, double& aTime);
  PRBool ParseNPTHH(nsDependentSubstring& aString, PRUint32& aHour);
  PRBool ParseNPTMM(nsDependentSubstring& aString, PRUint32& aMinute);
  PRBool ParseNPTSS(nsDependentSubstring& aString, PRUint32& aSecond);

  
  nsCAutoString mHash;

  
  
  nsTArray<Pair> mFragments;
};

#endif
