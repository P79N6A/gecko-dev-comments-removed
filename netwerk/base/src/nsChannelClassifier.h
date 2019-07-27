



#ifndef nsChannelClassifier_h__
#define nsChannelClassifier_h__

#include "nsIURIClassifier.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsIChannel;


class nsChannelClassifier MOZ_FINAL : public nsIURIClassifierCallback
{
public:
    nsChannelClassifier();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIURICLASSIFIERCALLBACK

    nsresult Start(nsIChannel *aChannel);

private:
    nsCOMPtr<nsIChannel> mSuspendedChannel;

    ~nsChannelClassifier() {}
    void MarkEntryClassified(nsresult status);
    bool HasBeenClassified(nsIChannel *aChannel);
    
    nsresult ShouldEnableTrackingProtection(nsIChannel *aChannel, bool *result);
};

#endif
