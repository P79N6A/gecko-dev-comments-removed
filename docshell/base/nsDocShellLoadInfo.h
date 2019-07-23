






































#ifndef nsDocShellLoadInfo_h__
#define nsDocShellLoadInfo_h__



#include "nsCOMPtr.h"
#include "nsString.h"


#include "nsIDocShellLoadInfo.h"
#include "nsIURI.h"
#include "nsIInputStream.h"
#include "nsISHEntry.h"

class nsDocShellLoadInfo : public nsIDocShellLoadInfo
{
public:
  nsDocShellLoadInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOCSHELLLOADINFO

protected:
  virtual ~nsDocShellLoadInfo();

protected:
  nsCOMPtr<nsIURI>                 mReferrer;
  nsCOMPtr<nsISupports>            mOwner;
  PRPackedBool                     mInheritOwner;
  PRPackedBool                     mSendReferrer;
  nsDocShellInfoLoadType           mLoadType;
  nsCOMPtr<nsISHEntry>             mSHEntry;
  nsString                         mTarget;
  nsCOMPtr<nsIInputStream>         mPostDataStream;
  nsCOMPtr<nsIInputStream>         mHeadersStream;
};

#endif 
