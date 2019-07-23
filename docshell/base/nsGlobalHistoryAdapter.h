




































#include "nsIGlobalHistory2.h"
#include "nsIGlobalHistory.h"
#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"








#define NS_GLOBALHISTORYADAPTER_CID \
 { 0x2e9b69dd, 0x9087, 0x438c, { 0x8b, 0x5d, 0xf7, 0x7b, 0x55, 0x3a, 0xbe, 0xfb } }

class nsGlobalHistoryAdapter : public nsIGlobalHistory2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGLOBALHISTORY2

  static NS_METHOD Create(nsISupports *aOuter,
                          REFNSIID aIID,
                          void **aResult);

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_GLOBALHISTORYADAPTER_CID)

private:
  nsGlobalHistoryAdapter();
  ~nsGlobalHistoryAdapter();

  nsresult Init();
  nsCOMPtr<nsIGlobalHistory> mHistory;
};
