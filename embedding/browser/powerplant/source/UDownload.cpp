





































#include "UDownload.h"

#include "nsIExternalHelperAppService.h"
#include "nsILocalFIleMac.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIRequest.h"
#include "netCore.h"

#include "UDownloadDisplay.h"
#include "UMacUnicode.h"

#include "UNavServicesDialogs.h"





#pragma mark [CDownload]

ADownloadProgressView *CDownload::sProgressView;

CDownload::CDownload() :
    mGotFirstStateChange(false), mIsNetworkTransfer(false),
    mUserCanceled(false),
    mStatus(NS_OK)
{
}

CDownload::~CDownload()
{
}

NS_IMPL_ISUPPORTS4(CDownload, nsIDownload, nsITransfer,
                   nsIWebProgressListener, nsIWebProgressListener2)

#pragma mark -
#pragma mark [CDownload::nsIDownload]


NS_IMETHODIMP CDownload::Init(nsIURI *aSource, nsILocalFile *aTarget,
    const PRUnichar *aDisplayName, nsIMIMEInfo *aMIMEInfo, PRInt64 startTime,
    nsILocalFile* aTempFile, nsICancelable* aCancelable)
{
    try {
        mSource = aSource;
        mDestination = aTarget;
        mStartTime = startTime;
        mPercentComplete = 0;
        
        mCancelable = aCancelable;
        EnsureProgressView();
        sProgressView->AddDownloadItem(this);
    }
    catch (...) {
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}


NS_IMETHODIMP CDownload::GetSource(nsIURI * *aSource)
{
    NS_ENSURE_ARG_POINTER(aSource);
    NS_IF_ADDREF(*aSource = mSource);
    return NS_OK;
}


NS_IMETHODIMP CDownload::GetTarget(nsILocalFile * *aTarget)
{
    NS_ENSURE_ARG_POINTER(aTarget);
    NS_IF_ADDREF(*aTarget = mDestination);
    return NS_OK;
}


NS_IMETHODIMP CDownload::GetCancelable(nsIWebBrowserCancelable * *aCancelable)
{
    NS_ENSURE_ARG_POINTER(aCancelable);
    NS_IF_ADDREF(*aCancelable = mCancelable);
    return NS_OK;
}


NS_IMETHODIMP CDownload::GetPercentComplete(PRInt32 *aPercentComplete)
{
    NS_ENSURE_ARG_POINTER(aPercentComplete);
    *aPercentComplete = mPercentComplete;
    return NS_OK;
}


NS_IMETHODIMP CDownload::GetDisplayName(PRUnichar * *aDisplayName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP CDownload::GetStartTime(PRInt64 *aStartTime)
{
    NS_ENSURE_ARG_POINTER(aStartTime);
    *aStartTime = mStartTime;
    return NS_OK;
}


NS_IMETHODIMP CDownload::GetSpeed(double *aSpeed)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP CDownload::GetMIMEInfo(nsIMIMEInfo * *aMIMEInfo)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

#pragma mark -
#pragma mark [CDownload::nsIWebProgressListener]


NS_IMETHODIMP CDownload::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
    
    
    if (!mGotFirstStateChange) {
        mIsNetworkTransfer = ((aStateFlags & STATE_IS_NETWORK) != 0);
        mGotFirstStateChange = PR_TRUE;
        BroadcastMessage(msg_OnDLStart, this);
    }

    if (NS_FAILED(aStatus) && NS_SUCCEEDED(mStatus))
        mStatus = aStatus;
  
    
    if ((aStateFlags & STATE_STOP) && (!mIsNetworkTransfer || (aStateFlags & STATE_IS_NETWORK))) {
        mCancelable = nsnull;
        BroadcastMessage(msg_OnDLComplete, this);
    }
        
    return NS_OK; 
}


NS_IMETHODIMP CDownload::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
    if (mUserCanceled) {
        mCancelable->Cancel(NS_BINDING_ABORTED);
        mUserCanceled = false;
    }
    if (aMaxTotalProgress == -1)
        mPercentComplete = -1;
    else
        mPercentComplete = (PRInt32)(((float)aCurTotalProgress / (float)aMaxTotalProgress) * 100.0 + 0.5);
    
    MsgOnDLProgressChangeInfo info(this, aCurTotalProgress, aMaxTotalProgress);
    BroadcastMessage(msg_OnDLProgressChange, &info);

    return NS_OK;
}


NS_IMETHODIMP CDownload::OnProgressChange64(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt64 aCurSelfProgress, PRInt64 aMaxSelfProgress, PRInt64 aCurTotalProgress, PRInt64 aMaxTotalProgress)
{
  
  return OnProgressChange(aProgress, aRequest,
                          PRInt32(curSelfProgress), PRInt32(maxSelfProgress),
                          PRInt32(curTotalProgress), PRInt32(maxTotalProgress));
}



NS_IMETHODIMP CDownload::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    return NS_OK;
}


NS_IMETHODIMP CDownload::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
    return NS_OK;
}


NS_IMETHODIMP CDownload::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}


NS_IMETHODIMP CDownload::OnRefreshAttempted(nsIWebProgress *aWebProgress, nsIURI *aUri, PRInt32 aDelay, PRBool aSameUri, PRBool *allowRefresh)
{
    *allowRefresh = PR_TRUE;
    return NS_OK;
}

#pragma mark -
#pragma mark [CDownload Internal Methods]

void CDownload::Cancel()
{
    mUserCanceled = true;
    
    
    mStatus = NS_ERROR_ABORT;
}

void CDownload::CreateProgressView()
{
    sProgressView = new CMultiDownloadProgress;
    ThrowIfNil_(sProgressView);
}





#pragma mark -
#pragma mark [CHelperAppLauncherDialog]

CHelperAppLauncherDialog::CHelperAppLauncherDialog()
{
}

CHelperAppLauncherDialog::~CHelperAppLauncherDialog()
{
}

NS_IMPL_ISUPPORTS1(CHelperAppLauncherDialog, nsIHelperAppLauncherDialog)


NS_IMETHODIMP CHelperAppLauncherDialog::Show(nsIHelperAppLauncher *aLauncher, nsISupports *aContext, PRUint32 aReason)
{
    return aLauncher->SaveToDisk(nsnull, PR_FALSE);
}


NS_IMETHODIMP CHelperAppLauncherDialog::PromptForSaveToFile(nsIHelperAppLauncher* aLauncher, 
                                                            nsISupports *aWindowContext, 
                                                            const PRUnichar *aDefaultFile, 
                                                            const PRUnichar *aSuggestedFileExtension, 
                                                            nsILocalFile **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = nsnull;
 
    static bool sFirstTime = true;   
	UNavServicesDialogs::LFileDesignator	designator;

    if (sFirstTime) {
        
        nsCOMPtr<nsIFile> defaultDownloadDir;
        NS_GetSpecialDirectory(NS_MAC_DEFAULT_DOWNLOAD_DIR, getter_AddRefs(defaultDownloadDir));
        if (defaultDownloadDir) {
            nsCOMPtr<nsILocalFileMac> macDir(do_QueryInterface(defaultDownloadDir));
            FSSpec defaultDownloadSpec;
            if (NS_SUCCEEDED(macDir->GetFSSpec(&defaultDownloadSpec)))
                designator.SetDefaultLocation(defaultDownloadSpec, true);
        }
        sFirstTime = false;
    }
	
	Str255  defaultName;
	CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(aDefaultFile), defaultName);
    bool result = designator.AskDesignateFile(defaultName);
    
    
    
    
    
    
    
    
    
    if (LEventDispatcher::GetCurrentEventDispatcher()) { 
        EventRecord theEvent;
        while (::WaitNextEvent(updateMask | activMask, &theEvent, 0, nil))
            LEventDispatcher::GetCurrentEventDispatcher()->DispatchEvent(theEvent);
    }
        
    if (result) {
        FSSpec destSpec;
        designator.GetFileSpec(destSpec);
        nsCOMPtr<nsILocalFileMac> destFile;
        NS_NewLocalFileWithFSSpec(&destSpec, PR_TRUE, getter_AddRefs(destFile));
        if (!destFile)
            return NS_ERROR_FAILURE;
        *_retval = destFile;
        NS_ADDREF(*_retval);
        return NS_OK;
    }
    else
        return NS_ERROR_ABORT;
}

