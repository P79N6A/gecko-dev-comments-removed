





































#ifndef NSUNKNOWNCONTENTTYPEHANDLER_EMB
#define NSUNKNOWNCONTENTTYPEHANDLER_EMB

#include "nsIHelperAppLauncherDialog.h"
#include "nsIExternalHelperAppService.h"
#include "nsIWebBrowserPersist.h"
#include "nsWeakReference.h"
#include "nsIWindowWatcher.h"
#include "nsIEmbeddingSiteWindow.h"
#include "EmbedDownload.h"
#include "PtMozilla.h"


static NS_DEFINE_CID( kCID, NS_IHELPERAPPLAUNCHERDIALOG_IID );

class nsUnknownContentTypeHandler : public nsIHelperAppLauncherDialog {

public:

    nsUnknownContentTypeHandler();
    virtual ~nsUnknownContentTypeHandler();



    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

private:
		PtWidget_t* GetWebBrowser(nsIDOMWindow *aWindow);

}; 

int Init_nsUnknownContentTypeHandler_Factory( );

#endif
