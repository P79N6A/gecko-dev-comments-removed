





































#ifndef nsChannelClassifier_h__
#define nsChannelClassifier_h__

#include "nsIURIClassifier.h"
#include "nsIRunnable.h"
#include "nsCOMPtr.h"

class nsIChannel;


class nsChannelClassifier : public nsIURIClassifierCallback
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
};

#endif
