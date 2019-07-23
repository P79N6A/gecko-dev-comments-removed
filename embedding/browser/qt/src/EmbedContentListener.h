






































#ifndef EMBEDCONTENTLISTENER_H
#define EMBEDCONTENTLISTENER_H

#include <nsIURIContentListener.h>
#include <nsWeakReference.h>

class QGeckoEmbed;

class EmbedContentListener : public nsIURIContentListener,
                             public nsSupportsWeakReference
{
public:

    EmbedContentListener(QGeckoEmbed *aOwner);
    ~EmbedContentListener();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIURICONTENTLISTENER

private:
    QGeckoEmbed *mOwner;
};

#endif
