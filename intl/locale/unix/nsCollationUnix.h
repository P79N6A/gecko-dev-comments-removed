





#ifndef nsCollationUnix_h__
#define nsCollationUnix_h__


#include "nsICollation.h"
#include "nsCollation.h"  
#include "plstr.h"
#include "mozilla/Attributes.h"
#include "nsString.h"



class nsCollationUnix MOZ_FINAL : public nsICollation {

protected:
  nsCollation   *mCollation;
  nsCString     mLocale;
  nsCString     mSavedLocale;
  bool          mUseCodePointOrder;

  void DoSetLocale();
  void DoRestoreLocale();

  ~nsCollationUnix(); 

public:
  nsCollationUnix();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICOLLATION

};

#endif
