



#ifndef nsChannelClassifier_h__
#define nsChannelClassifier_h__

#include "nsIURIClassifier.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsIChannel;
class nsIHttpChannelInternal;

class nsChannelClassifier MOZ_FINAL : public nsIURIClassifierCallback
{
public:
    nsChannelClassifier();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIURICLASSIFIERCALLBACK

    
    
    
    
    void Start(nsIChannel *aChannel, bool aContinueBeginConnect);
    
    nsresult ShouldEnableTrackingProtection(nsIChannel *aChannel, bool *result);

private:
    
    bool mIsAllowListed;
    
    bool mSuspendedChannel;
    nsCOMPtr<nsIChannel> mChannel;
    nsCOMPtr<nsIHttpChannelInternal> mChannelInternal;

    ~nsChannelClassifier() {}
    
    void MarkEntryClassified(nsresult status);
    bool HasBeenClassified(nsIChannel *aChannel);
    
    
    
    nsresult StartInternal();

public:
    
    
    static nsresult SetBlockedTrackingContent(nsIChannel *channel);
    static nsresult NotifyTrackingProtectionDisabled(nsIChannel *aChannel);
};

#endif
