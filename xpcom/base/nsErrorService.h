





#ifndef nsErrorService_h__
#define nsErrorService_h__

#include "mozilla/Attributes.h"

#include "nsIErrorService.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

class nsErrorService MOZ_FINAL : public nsIErrorService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIERRORSERVICE

  nsErrorService()
  {
  }

  static nsresult
  Create(nsISupports* aOuter, const nsIID& aIID, void** aInstancePtr);

private:
  ~nsErrorService()
  {
  }

  nsClassHashtable<nsUint32HashKey, nsCString> mErrorStringBundleURLMap;
};

#endif 
