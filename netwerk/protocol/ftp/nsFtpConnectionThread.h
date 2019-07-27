




#ifndef __nsFtpState__h_
#define __nsFtpState__h_

#include "nsBaseContentStream.h"

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIAsyncInputStream.h"
#include "nsAutoPtr.h"
#include "nsITransport.h"
#include "mozilla/net/DNS.h"
#include "nsFtpControlConnection.h"
#include "nsIProtocolProxyCallback.h"

#ifdef MOZ_WIDGET_GONK
#include "nsINetworkManager.h"
#include "nsProxyRelease.h"
#endif


#define FTP_GENERIC_TYPE     0
#define FTP_UNIX_TYPE        1
#define FTP_VMS_TYPE         8
#define FTP_NT_TYPE          9
#define FTP_OS2_TYPE         11


typedef enum _FTP_STATE {


    FTP_INIT,
    FTP_COMMAND_CONNECT,
    FTP_READ_BUF,
    FTP_ERROR,
    FTP_COMPLETE,



    FTP_S_USER, FTP_R_USER,
    FTP_S_PASS, FTP_R_PASS,
    FTP_S_SYST, FTP_R_SYST,
    FTP_S_ACCT, FTP_R_ACCT,
    FTP_S_TYPE, FTP_R_TYPE,
    FTP_S_CWD,  FTP_R_CWD,
    FTP_S_SIZE, FTP_R_SIZE,
    FTP_S_MDTM, FTP_R_MDTM,
    FTP_S_REST, FTP_R_REST,
    FTP_S_RETR, FTP_R_RETR,
    FTP_S_STOR, FTP_R_STOR,
    FTP_S_LIST, FTP_R_LIST,
    FTP_S_PASV, FTP_R_PASV,
    FTP_S_PWD,  FTP_R_PWD,
    FTP_S_FEAT, FTP_R_FEAT,
    FTP_S_OPTS, FTP_R_OPTS
} FTP_STATE;


typedef enum _FTP_ACTION {GET, PUT} FTP_ACTION;

class nsFtpChannel;
class nsICancelable;
class nsIProxyInfo;
class nsIStreamListener;






class nsFtpState MOZ_FINAL : public nsBaseContentStream,
                             public nsIInputStreamCallback,
                             public nsITransportEventSink,
                             public nsIRequestObserver,
                             public nsFtpControlConnectionListener,
                             public nsIProtocolProxyCallback
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIPROTOCOLPROXYCALLBACK

    
    NS_IMETHOD CloseWithStatus(nsresult status) MOZ_OVERRIDE;
    NS_IMETHOD Available(uint64_t *result) MOZ_OVERRIDE;
    NS_IMETHOD ReadSegments(nsWriteSegmentFun fun, void *closure,
                            uint32_t count, uint32_t *result) MOZ_OVERRIDE;

    
    virtual void OnControlDataAvailable(const char *data, uint32_t dataLen) MOZ_OVERRIDE;
    virtual void OnControlError(nsresult status) MOZ_OVERRIDE;

    nsFtpState();
    nsresult Init(nsFtpChannel *channel);

protected:
    
    virtual void OnCallbackPending() MOZ_OVERRIDE;

private:
    virtual ~nsFtpState();

    
    
    nsresult        S_user(); FTP_STATE       R_user();
    nsresult        S_pass(); FTP_STATE       R_pass();
    nsresult        S_syst(); FTP_STATE       R_syst();
    nsresult        S_acct(); FTP_STATE       R_acct();

    nsresult        S_type(); FTP_STATE       R_type();
    nsresult        S_cwd();  FTP_STATE       R_cwd();

    nsresult        S_size(); FTP_STATE       R_size();
    nsresult        S_mdtm(); FTP_STATE       R_mdtm();
    nsresult        S_list(); FTP_STATE       R_list();

    nsresult        S_rest(); FTP_STATE       R_rest();
    nsresult        S_retr(); FTP_STATE       R_retr();
    nsresult        S_stor(); FTP_STATE       R_stor();
    nsresult        S_pasv(); FTP_STATE       R_pasv();
    nsresult        S_pwd();  FTP_STATE       R_pwd();
    nsresult        S_feat(); FTP_STATE       R_feat();
    nsresult        S_opts(); FTP_STATE       R_opts();
    
    

    
    void        MoveToNextState(FTP_STATE nextState);
    nsresult    Process();

    void KillControlConnection();
    nsresult StopProcessing();
    nsresult EstablishControlConnection();
    nsresult SendFTPCommand(const nsCSubstring& command);
    void ConvertFilespecToVMS(nsCString& fileSpec);
    void ConvertDirspecToVMS(nsCString& fileSpec);
    void ConvertDirspecFromVMS(nsCString& fileSpec);
    nsresult BuildStreamConverter(nsIStreamListener** convertStreamListener);
    nsresult SetContentType();

    





    void Connect();

    
    

        
    FTP_STATE           mState;             
    FTP_STATE           mNextState;         
    bool                mKeepRunning;       
    int32_t             mResponseCode;      
    nsCString           mResponseMsg;       

        
    nsRefPtr<nsFtpControlConnection> mControlConnection;       
    bool                            mReceivedControlData;  
    bool                            mTryingCachedControl;     
    bool                            mRETRFailed;              
    uint64_t                        mFileSize;
    nsCString                       mModTime;

        
    nsRefPtr<nsFtpChannel>          mChannel;         
    nsCOMPtr<nsIProxyInfo>          mProxyInfo;

        
    int32_t             mServerType;    

        
    nsString            mUsername;      
    nsString            mPassword;      
    FTP_ACTION          mAction;        
    bool                mAnonymous;     
    bool                mRetryPass;     
    bool                mStorReplyReceived; 
                                            
    nsresult            mInternalError; 
    bool                mReconnectAndLoginAgain;
    bool                mCacheConnection;

        
    int32_t                mPort;       
    nsString               mFilename;   
    nsCString              mPath;       
    nsCString              mPwd;        

        
    nsCOMPtr<nsITransport>        mDataTransport;
    nsCOMPtr<nsIAsyncInputStream> mDataStream;
    nsCOMPtr<nsIRequest>    mUploadRequest;
    bool                    mAddressChecked;
    bool                    mServerIsIPv6;
    bool                    mUseUTF8;

    static uint32_t         mSessionStartTime;

    mozilla::net::NetAddr   mServerAddress;

    
    nsresult                mControlStatus;
    nsCString               mControlReadCarryOverBuf;

    nsCString mSuppliedEntityID;

    nsCOMPtr<nsICancelable>  mProxyRequest;
    bool                     mDeferredCallbackPending;



    uint64_t                           mCountRecv;
#ifdef MOZ_WIDGET_GONK
    nsMainThreadPtrHandle<nsINetworkInterface> mActiveNetwork;
#endif
    nsresult                           SaveNetworkStats(bool);
    void                               CountRecvBytes(uint64_t recvBytes)
    {
        mCountRecv += recvBytes;
        SaveNetworkStats(false);
    }
};

#endif 
