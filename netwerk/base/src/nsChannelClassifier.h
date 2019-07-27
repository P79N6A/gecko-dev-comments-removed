



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

    
    
    
    
    void Start(nsIChannel *aChannel);

private:
    
    bool mIsAllowListed;
    
    bool mSuspendedChannel;
    nsCOMPtr<nsIChannel> mChannel;

    ~nsChannelClassifier() {}
    
    void MarkEntryClassified(nsresult status);
    bool HasBeenClassified(nsIChannel *aChannel);
    
    
    
    nsresult StartInternal(nsIChannel *aChannel);
    
    nsresult ShouldEnableTrackingProtection(nsIChannel *aChannel, bool *result);

public:
    
    
    static nsresult SetBlockedTrackingContent(nsIChannel *channel);
    static nsresult NotifyTrackingProtectionDisabled(nsIChannel *aChannel);
};

#endif
