




































#include "nsIGlobalHistory2.h"
#include "nsIGlobalHistory.h"
#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"







#define NS_GLOBALHISTORY2ADAPTER_CID \
 { 0xa772eee4, 0x0464, 0x405d, { 0xa3, 0x29, 0xa2, 0x9d, 0xfd, 0xa3, 0x79, 0x1a } }

class nsGlobalHistory2Adapter : public nsIGlobalHistory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGLOBALHISTORY

  static NS_METHOD Create(nsISupports *aOuter,
                          REFNSIID aIID,
                          void **aResult);

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_GLOBALHISTORY2ADAPTER_CID);

private:
  nsGlobalHistory2Adapter();
  ~nsGlobalHistory2Adapter();

  nsresult Init();
  nsCOMPtr<nsIGlobalHistory2> mHistory;
};
