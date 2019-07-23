





































#ifndef HEADERSNIFFER_EMB
#define HEADERSNIFFER_EMB

#include "nsIWebProgressListener.h"
#include "nsIWebBrowserPersist.h"
#include "EmbedPrivate.h"
#include "PtMozilla.h"

class HeaderSniffer : public nsIWebProgressListener
{
public:
	HeaderSniffer( nsIWebBrowserPersist* aPersist, PtMozillaWidget_t *aMoz, nsIURI* aURL, nsIFile* aFile );
	virtual ~HeaderSniffer();
	
	NS_DECL_ISUPPORTS
	NS_DECL_NSIWEBPROGRESSLISTENER

private:
	PtMozillaWidget_t *mMozillaWidget;
	nsIWebBrowserPersist*     mPersist; 
	nsCOMPtr<nsIFile>         mTmpFile;
	nsCOMPtr<nsIURI>          mURL;
};

#endif
