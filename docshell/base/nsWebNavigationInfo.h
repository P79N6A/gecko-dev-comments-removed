




#ifndef nsWebNavigationInfo_h__
#define nsWebNavigationInfo_h__

#include "nsIWebNavigationInfo.h"
#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "mozilla/Attributes.h"

class nsCString;


#define NS_WEBNAVIGATION_INFO_CID \
 { 0xf30bc0a2, 0x958b, 0x4287,{0xbf, 0x62, 0xce, 0x38, 0xba, 0x0c, 0x81, 0x1e}}

class nsWebNavigationInfo MOZ_FINAL : public nsIWebNavigationInfo
{
public:
  nsWebNavigationInfo() {}

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWEBNAVIGATIONINFO

  nsresult Init();

private:
  ~nsWebNavigationInfo() {}

  
  
  nsresult IsTypeSupportedInternal(const nsCString& aType,
                                   uint32_t* aIsSupported);

  nsCOMPtr<nsICategoryManager> mCategoryManager;
};

#endif  
