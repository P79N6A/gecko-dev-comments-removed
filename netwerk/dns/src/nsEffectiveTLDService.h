





































#include "nsIEffectiveTLDService.h"

class nsEffectiveTLDService : public nsIEffectiveTLDService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEFFECTIVETLDSERVICE

  nsEffectiveTLDService();
  nsresult Init();

private:
  static nsEffectiveTLDService* sInstance;
  ~nsEffectiveTLDService();
};
