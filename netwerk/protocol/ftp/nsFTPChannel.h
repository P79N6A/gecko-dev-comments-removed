







































#ifndef nsFTPChannel_h___
#define nsFTPChannel_h___

#include "nsBaseChannel.h"

#include "nsIIOService.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsIProtocolHandler.h"
#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsFtpConnectionThread.h"
#include "netCore.h"
#include "nsIStreamListener.h"
#include "nsIFTPChannel.h"
#include "nsIUploadChannel.h"
#include "nsIProxyInfo.h"
#include "nsIProxiedChannel.h"
#include "nsIResumableChannel.h"
#include "nsHashPropertyBag.h"
#include "nsFtpProtocolHandler.h"

class nsFtpChannel : public nsBaseChannel,
                     public nsIFTPChannel,
                     public nsIUploadChannel,
                     public nsIResumableChannel,
                     public nsIProxiedChannel
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIUPLOADCHANNEL
    NS_DECL_NSIRESUMABLECHANNEL
    NS_DECL_NSIPROXIEDCHANNEL
    
    nsFtpChannel(nsIURI *uri, nsIProxyInfo *pi)
        : mProxyInfo(pi)
        , mStartPos(0)
        , mResumeRequested(PR_FALSE)
        , mLastModifiedTime(0)
    {
        SetURI(uri);
    }

    nsIProxyInfo *ProxyInfo() {
        return mProxyInfo;
    }

    
    bool ResumeRequested() { return mResumeRequested; }

    
    PRUint64 StartPos() { return mStartPos; }

    
    const nsCString &EntityID() {
        return mEntityID;
    }
    void SetEntityID(const nsCSubstring &entityID) {
        mEntityID = entityID;
    }

    NS_IMETHODIMP GetLastModifiedTime(PRTime* lastModifiedTime) {
        *lastModifiedTime = mLastModifiedTime;
        return NS_OK;
    }

    NS_IMETHODIMP SetLastModifiedTime(PRTime lastModifiedTime) {
        mLastModifiedTime = lastModifiedTime;
        return NS_OK;
    }

    
    nsIInputStream *UploadStream() {
        return mUploadStream;
    }

    
    void GetFTPEventSink(nsCOMPtr<nsIFTPEventSink> &aResult);

protected:
    virtual ~nsFtpChannel() {}
    virtual nsresult OpenContentStream(bool async, nsIInputStream **result,
                                       nsIChannel** channel);
    virtual bool GetStatusArg(nsresult status, nsString &statusArg);
    virtual void OnCallbacksChanged();

private:
    nsCOMPtr<nsIProxyInfo>    mProxyInfo; 
    nsCOMPtr<nsIFTPEventSink> mFTPEventSink;
    nsCOMPtr<nsIInputStream>  mUploadStream;
    PRUint64                  mStartPos;
    nsCString                 mEntityID;
    bool                      mResumeRequested;
    PRTime                    mLastModifiedTime;
};

#endif 
