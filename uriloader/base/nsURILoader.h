




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
#include "mozilla/Attributes.h"

#include "mozilla/Logging.h"

class nsDocumentOpenInfo;

class nsURILoader final : public nsIURILoader
{
public:
  NS_DECL_NSIURILOADER
  NS_DECL_ISUPPORTS

  nsURILoader();

protected:
  ~nsURILoader();

  



  nsresult OpenChannel(nsIChannel* channel,
                                   uint32_t aFlags,
                                   nsIInterfaceRequestor* aWindowContext,
                                   bool aChannelOpen,
                                   nsIStreamListener** aListener);

  




  nsCOMArray<nsIWeakReference> m_listeners;

  


  static PRLogModuleInfo* mLog;
  
  friend class nsDocumentOpenInfo;
};

#endif 
