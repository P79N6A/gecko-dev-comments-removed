





































#ifndef nsCollationMacUC_h__
#define nsCollationMacUC_h__

#include "nsICollation.h"
#include "nsCollation.h"  
#include <MacLocales.h>
#include <UnicodeUtilities.h>



const PRUint32 kCacheSize = 128;


const PRUint32 kCollationValueSizeFactor = 5;

class nsCollationMacUC : public nsICollation {

public: 
  nsCollationMacUC();
  ~nsCollationMacUC(); 

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

protected:
  nsresult ConvertLocale(nsILocale* aNSLocale, LocaleRef* aMacLocale);
  nsresult StrengthToOptions(const PRInt32 aStrength,
                             UCCollateOptions* aOptions);
  nsresult EnsureCollator(const PRInt32 newStrength);

private:
  PRPackedBool mInit;
  PRPackedBool mHasCollator;
  LocaleRef mLocale;
  PRInt32 mLastStrength;
  CollatorRef mCollator;
  void *mBuffer; 
  PRUint32 mBufferLen; 
};

#endif  
