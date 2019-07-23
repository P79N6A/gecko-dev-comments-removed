






































#ifndef _NS_XPINSTALLMANAGER_H
#define _NS_XPINSTALLMANAGER_H

#include "nsInstall.h"

#include "nscore.h"
#include "nsISupports.h"
#include "nsString.h"

#include "nsIURL.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"
#include "nsIXPInstallManager.h"
#include "nsIXPIDialogService.h"
#include "nsXPITriggerInfo.h"
#include "nsIXPIProgressDialog.h"
#include "nsIChromeRegistry.h"
#include "nsIDOMWindowInternal.h"
#include "nsIObserver.h"
#include "nsIBadCertListener2.h"
#include "nsISSLErrorListener.h"
#include "nsIChannelEventSink.h"
#include "nsIZipReader.h"
#include "nsIXPIInstallInfo.h"
#include "nsILoadGroup.h"

#include "nsCOMPtr.h"

#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIDialogParamBlock.h"

#include "nsPICertNotification.h"

#include "nsWeakReference.h"

#define NS_XPIDIALOGSERVICE_CONTRACTID "@mozilla.org/embedui/xpinstall-dialog-service;1"
#define NS_XPINSTALLMANAGERCOMPONENT_CONTRACTID "@mozilla.org/xpinstall/install-manager;1"
#define XPI_PROGRESS_TOPIC "xpinstall-progress"

class nsXPInstallManager : public nsIXPIDialogService,
                           public nsIXPInstallManager,
                           public nsIObserver,
                           public nsIStreamListener,
                           public nsIProgressEventSink,
                           public nsIInterfaceRequestor,
                           public nsPICertNotification,
                           public nsIBadCertListener2,
                           public nsISSLErrorListener,
                           public nsIChannelEventSink,
                           public nsSupportsWeakReference
{
    public:
        nsXPInstallManager();
        virtual ~nsXPInstallManager();

        NS_DECL_ISUPPORTS
        NS_DECL_NSIXPIDIALOGSERVICE
        NS_DECL_NSIXPINSTALLMANAGER
        NS_DECL_NSIOBSERVER
        NS_DECL_NSISTREAMLISTENER
        NS_DECL_NSIPROGRESSEVENTSINK
        NS_DECL_NSIREQUESTOBSERVER
        NS_DECL_NSIINTERFACEREQUESTOR
        NS_DECL_NSPICERTNOTIFICATION
        NS_DECL_NSIBADCERTLISTENER2
        NS_DECL_NSISSLERRORLISTENER
        NS_DECL_NSICHANNELEVENTSINK

        NS_IMETHOD InitManager(nsIDOMWindowInternal* aParentWindow, nsXPITriggerInfo* aTrigger, PRUint32 aChromeType );

    private:
        nsresult    InitManagerInternal();
        nsresult    InstallItems();
        NS_IMETHOD  DownloadNext();
        void        Shutdown(PRInt32 status = nsInstall::USER_CANCELLED);
        NS_IMETHOD  GetDestinationFile(nsString& url, nsILocalFile* *file);
        NS_IMETHOD  LoadParams(PRUint32 aCount, const PRUnichar** aPackageList, nsIDialogParamBlock** aParams);
#ifdef ENABLE_SKIN_SIMPLE_INSTALLATION_UI
        PRBool      ConfirmChromeInstall(nsIDOMWindowInternal* aParentWindow, const PRUnichar** aPackage);
#endif
        PRBool      VerifyHash(nsXPITriggerItem* aItem);
        PRInt32     GetIndexFromURL(const PRUnichar* aUrl);
        nsresult    CheckCert(nsIChannel* aChannel);

        nsXPITriggerInfo*   mTriggers;
        nsXPITriggerItem*   mItem;
        PRUint32            mNextItem;
        PRUint32            mChromeType;
        PRInt32             mContentLength;
        PRInt32             mOutstandingCertLoads;
        PRBool              mDialogOpen;
        PRBool              mCancelled;
        PRBool              mNeedsShutdown;
        PRBool              mFromChrome;

        nsCOMPtr<nsIXPIProgressDialog>  mDlg;

        nsCOMPtr<nsIDOMWindowInternal>  mParentWindow;
        nsCOMPtr<nsILoadGroup>          mLoadGroup;
};

#endif
