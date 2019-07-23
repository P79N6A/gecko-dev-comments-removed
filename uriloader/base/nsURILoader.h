




































#ifndef nsURILoader_h__
#define nsURILoader_h__

#include "nsCURILoader.h"
#include "nsISupportsUtils.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsString.h"
#include "nsIWeakReference.h"

#ifdef MOZ_LOGGING


#endif
#include "prlog.h"

class nsDocumentOpenInfo;

class nsURILoader : public nsIURILoader
{
public:
  NS_DECL_NSIURILOADER
  NS_DECL_ISUPPORTS

  nsURILoader();
  ~nsURILoader();

protected:
  



  NS_HIDDEN_(nsresult) OpenChannel(nsIChannel* channel,
                                   PRUint32 aFlags,
                                   nsIInterfaceRequestor* aWindowContext,
                                   PRBool aChannelOpen,
                                   nsIStreamListener** aListener);

  




  nsCOMArray<nsIWeakReference> m_listeners;

#ifdef PR_LOGGING
  


  static PRLogModuleInfo* mLog;
#endif
  
  friend class nsDocumentOpenInfo;
};

#endif 
