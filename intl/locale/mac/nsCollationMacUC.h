




#ifndef nsCollationMacUC_h_
#define nsCollationMacUC_h_

#include "nsICollation.h"
#include "nsCollation.h"
#include "mozilla/Attributes.h"

#include "unicode/ucol.h"
#include <Carbon/Carbon.h>



const uint32_t kCacheSize = 128;


const uint32_t kCollationValueSizeFactor = 6;

class nsCollationMacUC final : public nsICollation {

public: 
  nsCollationMacUC();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

protected:
  ~nsCollationMacUC(); 

  nsresult ConvertLocaleICU(nsILocale* aNSLocale, char** aICULocale);
  nsresult ConvertLocale(nsILocale* aNSLocale, LocaleRef* aMacLocale);
  nsresult ConvertStrength(const int32_t aStrength,
                           UCollationStrength* aStrengthOut,
                           UColAttributeValue* aCaseLevelOut);
  nsresult StrengthToOptions(const int32_t aStrength,
                             UCCollateOptions* aOptions);
  nsresult EnsureCollator(const int32_t newStrength);
  nsresult CleanUpCollator(void);

private:
  bool mInit;
  bool mHasCollator;
  char* mLocaleICU;
  LocaleRef mLocale;
  int32_t mLastStrength;
  UCollator* mCollatorICU;
  CollatorRef mCollator;
  void *mBuffer; 
  uint32_t mBufferLen; 
  bool mUseICU;
};

#endif  
