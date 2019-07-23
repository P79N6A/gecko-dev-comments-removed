






































#ifndef EMBEDPROGRESS_H
#define EMBEDPROGRESS_H

#include <nsIWebProgressListener.h>
#include <nsWeakReference.h>

#include "nsStringFwd.h"

class QGeckoEmbed;

class EmbedProgress : public nsIWebProgressListener,
                      public nsSupportsWeakReference
{
public:
    EmbedProgress(QGeckoEmbed *aOwner);
    ~EmbedProgress();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIWEBPROGRESSLISTENER

private:

    static void RequestToURIString(nsIRequest *aRequest, nsString &aString);

    QGeckoEmbed *mOwner;
};

#endif
