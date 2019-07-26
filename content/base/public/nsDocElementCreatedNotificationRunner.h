




#ifndef nsDocElementCreatedNotificationRunner_h
#define nsDocElementCreatedNotificationRunner_h

#include "nsThreadUtils.h" 

#include "nsContentSink.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"

class nsDocElementCreatedNotificationRunner : public nsRunnable
{
public:
  nsDocElementCreatedNotificationRunner(nsIDocument* aDoc)
    : mDoc(aDoc)
  {
  }

  NS_IMETHOD Run()
  {
    nsContentSink::NotifyDocElementCreated(mDoc);
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> mDoc;
};

#endif 
