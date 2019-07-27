





#ifndef nsDocShellLoadInfo_h__
#define nsDocShellLoadInfo_h__


#include "nsCOMPtr.h"
#include "nsString.h"


#include "nsIDocShellLoadInfo.h"

class nsIInputStream;
class nsISHEntry;
class nsIURI;
class nsIDocShell;

class nsDocShellLoadInfo : public nsIDocShellLoadInfo
{
public:
  nsDocShellLoadInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOCSHELLLOADINFO

protected:
  virtual ~nsDocShellLoadInfo();

protected:
  nsCOMPtr<nsIURI> mReferrer;
  nsCOMPtr<nsISupports> mOwner;
  bool mInheritOwner;
  bool mOwnerIsExplicit;
  bool mSendReferrer;
  nsDocShellInfoReferrerPolicy mReferrerPolicy;
  nsDocShellInfoLoadType mLoadType;
  nsCOMPtr<nsISHEntry> mSHEntry;
  nsString mTarget;
  nsCOMPtr<nsIInputStream> mPostDataStream;
  nsCOMPtr<nsIInputStream> mHeadersStream;
  bool mIsSrcdocLoad;
  nsString mSrcdocData;
  nsCOMPtr<nsIDocShell> mSourceDocShell;
  nsCOMPtr<nsIURI> mBaseURI;
};

#endif 
