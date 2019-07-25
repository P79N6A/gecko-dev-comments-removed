




































#ifndef ThirdPartyUtil_h__
#define ThirdPartyUtil_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozIThirdPartyUtil.h"
#include "nsIEffectiveTLDService.h"

class nsIURI;
class nsIChannel;
class nsIDOMWindow;

class ThirdPartyUtil : public mozIThirdPartyUtil
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZITHIRDPARTYUTIL

  nsresult Init();

private:
  nsresult IsThirdPartyInternal(const nsCString& aFirstDomain,
    nsIURI* aSecondURI, bool* aResult);
  static already_AddRefed<nsIURI> GetURIFromWindow(nsIDOMWindow* aWin);

  nsCOMPtr<nsIEffectiveTLDService> mTLDService;
};

#endif

