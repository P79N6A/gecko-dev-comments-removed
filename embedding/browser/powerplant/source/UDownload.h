





































#ifndef UDownload_h__
#define UDownload_h__
#pragma once

#include "nsIDownload.h"
#include "nsIWebProgressListener.h"
#include "nsIHelperAppLauncherDialog.h"
#include "nsIExternalHelperAppService.h"

#include "nsIURI.h"
#include "nsILocalFile.h"
#include "nsIWebBrowserPersist.h"

class ADownloadProgressView;














class CDownload : public nsIDownload,
                  public nsIWebProgressListener,
                  public LBroadcaster
{
public:
    
    
    enum {
        msg_OnDLStart           = 57723,    
        msg_OnDLComplete,                   
        msg_OnDLProgressChange              
    };
       
    struct MsgOnDLProgressChangeInfo
    {
        MsgOnDLProgressChangeInfo(CDownload* broadcaster, PRInt32 curProgress, PRInt32 maxProgress) :
            mBroadcaster(broadcaster), mCurProgress(curProgress), mMaxProgress(maxProgress)
            { }
        
        CDownload *mBroadcaster;      
        PRInt32 mCurProgress, mMaxProgress;
    };

                            CDownload();
    virtual                 ~CDownload();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOWNLOAD
    NS_DECL_NSIWEBPROGRESSLISTENER
    
    virtual void            Cancel();
    virtual void            GetStatus(nsresult& aStatus)
                            { aStatus = mStatus; }

protected:
    void                    EnsureProgressView()
                            {
                                if (!sProgressView)
                                    CreateProgressView();
                            }
    virtual void            CreateProgressView();
    
    
protected:
    nsCOMPtr<nsIURI>        mSource;
    nsCOMPtr<nsILocalFile>  mDestination;
    PRInt64                 mStartTime;
    PRInt32                 mPercentComplete;
    
    bool                    mGotFirstStateChange, mIsNetworkTransfer;
    bool                    mUserCanceled;
    nsresult                mStatus;
    
    nsCOMPtr<nsICancelable> mCancelable;
    
    static ADownloadProgressView *sProgressView;
};












class CHelperAppLauncherDialog : public nsIHelperAppLauncherDialog
{
public:
                            CHelperAppLauncherDialog();
    virtual                 ~CHelperAppLauncherDialog();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

protected:

};









class ADownloadProgressView
{
    friend class CDownload;
    
    virtual void AddDownloadItem(CDownload *aDownloadItem) = 0;
    
    
    
    
    
};


#endif
