





































#ifndef __nsFtpState__h_
#define __nsFtpState__h_

#include "ftpCore.h"
#include "nsFTPChannel.h"
#include "nsBaseContentStream.h"

#include "nsInt64.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsICacheListener.h"
#include "nsIURI.h"
#include "prtime.h"
#include "nsString.h"
#include "nsIFTPChannel.h"
#include "nsIProtocolHandler.h"
#include "nsCOMPtr.h"
#include "nsIAsyncInputStream.h"
#include "nsIOutputStream.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsIPrompt.h"
#include "nsITransport.h"
#include "nsIProxyInfo.h"

#include "nsFtpControlConnection.h"

#include "nsICacheEntryDescriptor.h"
#include "nsICacheListener.h"


#define FTP_GENERIC_TYPE     0
#define FTP_UNIX_TYPE        1
#define FTP_VMS_TYPE         8
#define FTP_NT_TYPE          9
#define FTP_OS2_TYPE         11


typedef enum _FTP_STATE {


    FTP_INIT,
    FTP_WAIT_CACHE,
    FTP_READ_CACHE,
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
    FTP_S_PWD,  FTP_R_PWD
} FTP_STATE;


typedef enum _FTP_ACTION {GET, PUT} FTP_ACTION;

class nsFtpChannel;






class nsFtpState : public nsBaseContentStream,
                   public nsIInputStreamCallback,
                   public nsITransportEventSink,
                   public nsICacheListener,
                   public nsIRequestObserver,
                   public nsFtpControlConnectionListener {
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSICACHELISTENER
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_IMETHOD CloseWithStatus(nsresult status);
    NS_IMETHOD Available(PRUint32 *result);
    NS_IMETHOD ReadSegments(nsWriteSegmentFun fun, void *closure,
                            PRUint32 count, PRUint32 *result);

    
    virtual void OnControlDataAvailable(const char *data, PRUint32 dataLen);
    virtual void OnControlError(nsresult status);

    nsFtpState();
    nsresult Init(nsFtpChannel *channel);

protected:
    
    virtual void OnCallbackPending();

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

    








    PRBool CheckCache();

    



    PRBool CanReadCacheEntry();

    





    PRBool ReadCacheEntry();

    




    nsresult OpenCacheDataStream();

    





    nsresult InstallCacheListener();

    
    

        
    FTP_STATE           mState;             
    FTP_STATE           mNextState;         
    PRPackedBool        mKeepRunning;       
    PRInt32             mResponseCode;      
    nsCString           mResponseMsg;       

        
    nsRefPtr<nsFtpControlConnection> mControlConnection;       
    PRPackedBool                    mReceivedControlData;  
    PRPackedBool                    mTryingCachedControl;     
    PRPackedBool                    mRETRFailed;              
    PRUint64                        mFileSize;
    nsCString                       mModTime;

        
    nsRefPtr<nsFtpChannel>          mChannel;         
    nsCOMPtr<nsIProxyInfo>          mProxyInfo;

        
    PRInt32             mServerType;    

        
    nsString            mUsername;      
    nsString            mPassword;      
    FTP_ACTION          mAction;        
    PRPackedBool        mAnonymous;     
    PRPackedBool        mRetryPass;     
    nsresult            mInternalError; 

        
    PRInt32                mPort;       
    nsString               mFilename;   
    nsCString              mPath;       
    nsCString              mPwd;        

        
    nsCOMPtr<nsITransport>        mDataTransport;
    nsCOMPtr<nsIAsyncInputStream> mDataStream;
    nsCOMPtr<nsIRequest>    mUploadRequest;
    PRPackedBool            mAddressChecked;
    PRPackedBool            mServerIsIPv6;
    
    static PRUint32         mSessionStartTime;

    char                    mServerAddress[64];

    
    nsresult                mControlStatus;
    nsCString               mControlReadCarryOverBuf;

    nsCOMPtr<nsICacheEntryDescriptor> mCacheEntry;
    
    nsCString mSuppliedEntityID;
};

#endif 
