





































#ifndef EMBEDDOWNLOAD_EMB
#define EMBEDDOWNLOAD_EMB

#include "nsIWebProgressListener2.h"
#include "nsIWebBrowserPersist.h"
#include "EmbedPrivate.h"
#include "PtMozilla.h"


class EmbedDownload : public nsIWebProgressListener2
{

public:
	EmbedDownload( PtMozillaWidget_t *aMoz, int aDownloadTicket, const char * aURL );
	virtual ~EmbedDownload();

	void ReportDownload( int type, int current, int total, char *message );

	int mDownloadTicket;
	nsIHelperAppLauncher *mLauncher; 
	nsIWebBrowserPersist *mPersist; 

	NS_DECL_ISUPPORTS
	NS_DECL_NSIWEBPROGRESSLISTENER
	NS_DECL_NSIWEBPROGRESSLISTENER2

private:
	PtMozillaWidget_t *mMozillaWidget;
	char *mURL;
	PRBool mDone;
	};

EmbedDownload *FindDownload( PtMozillaWidget_t *moz, int download_ticket );

#endif
