






































#ifndef __EmbedContentListener_h
#define __EmbedContentListener_h

#include "nsIURIContentListener.h"
#include "nsWeakReference.h"

class EmbedPrivate;

class EmbedContentListener : public nsIURIContentListener,
                             public nsSupportsWeakReference
{
 public:

  EmbedContentListener();
  virtual ~EmbedContentListener();

  nsresult Init (EmbedPrivate *aOwner);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIURICONTENTLISTENER

 private:

  EmbedPrivate *mOwner;

};

#endif 
