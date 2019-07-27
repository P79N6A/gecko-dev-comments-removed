




#ifndef nsDocElementCreatedNotificationRunner_h
#define nsDocElementCreatedNotificationRunner_h

#include "mozilla/Attributes.h"
#include "nsThreadUtils.h" 

#include "nsContentSink.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"

class nsDocElementCreatedNotificationRunner : public nsRunnable
{
public:
  explicit nsDocElementCreatedNotificationRunner(nsIDocument* aDoc)
    : mDoc(aDoc)
  {
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    nsContentSink::NotifyDocElementCreated(mDoc);
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> mDoc;
};

#endif 
