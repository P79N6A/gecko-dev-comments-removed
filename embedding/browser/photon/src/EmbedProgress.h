





































#ifndef __EmbedProgress_h
#define __EmbedProgress_h

#include <nsIWebProgressListener.h>
#include <nsWeakReference.h>
#include "EmbedPrivate.h"

class EmbedProgress : public nsIWebProgressListener,
                      public nsSupportsWeakReference
{
 public:
  EmbedProgress();
  virtual ~EmbedProgress();

	PRBool mSkipOnState, mDownloadDocument;

  nsresult Init(EmbedPrivate *aOwner);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIWEBPROGRESSLISTENER

 private:

  static void RequestToURIString (nsIRequest *aRequest, char **aString);

  EmbedPrivate *mOwner;

};

#endif 
