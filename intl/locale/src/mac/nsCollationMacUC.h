




#ifndef nsCollationMacUC_h_
#define nsCollationMacUC_h_

#include "nsICollation.h"
#include "nsCollation.h"
#include "mozilla/Attributes.h"
#include <Carbon/Carbon.h>



const uint32_t kCacheSize = 128;


const uint32_t kCollationValueSizeFactor = 6;

class nsCollationMacUC MOZ_FINAL : public nsICollation {

public: 
  nsCollationMacUC();
  ~nsCollationMacUC(); 

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

protected:
  nsresult ConvertLocale(nsILocale* aNSLocale, LocaleRef* aMacLocale);
  nsresult StrengthToOptions(const int32_t aStrength,
                             UCCollateOptions* aOptions);
  nsresult EnsureCollator(const int32_t newStrength);

private:
  bool mInit;
  bool mHasCollator;
  LocaleRef mLocale;
  int32_t mLastStrength;
  CollatorRef mCollator;
  void *mBuffer; 
  uint32_t mBufferLen; 
};

#endif  
