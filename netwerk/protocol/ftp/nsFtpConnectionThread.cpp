





#include <ctype.h>

#include "prprf.h"
#include "mozilla/Logging.h"
#include "prtime.h"

#include "nsIOService.h"
#include "nsFTPChannel.h"
#include "nsFtpConnectionThread.h"
#include "nsFtpControlConnection.h"
#include "nsFtpProtocolHandler.h"
#include "netCore.h"
#include "nsCRT.h"
#include "nsEscape.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsStreamUtils.h"
#include "nsIURL.h"
#include "nsISocketTransport.h"
#include "nsIStreamListenerTee.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIStringBundle.h"
#include "nsAuthInformationHolder.h"
#include "nsIProtocolProxyService.h"
#include "nsICancelable.h"
#include "nsIOutputStream.h"
#include "nsIPrompt.h"
#include "nsIProtocolHandler.h"
#include "nsIProxyInfo.h"
#include "nsIRunnable.h"
#include "nsISocketTransportService.h"
#include "nsIURI.h"
#include "nsILoadInfo.h"
#include "nsNullPrincipal.h"

#ifdef MOZ_WIDGET_GONK
#include "NetStatistics.h"
#endif

extern PRLogModuleInfo* gFTPLog;
#define LOG(args)         MOZ_LOG(gFTPLog, mozilla::LogLevel::Debug, args)
#define LOG_INFO(args)  MOZ_LOG(gFTPLog, mozilla::LogLevel::Info, args)

using namespace mozilla::net;


static void
removeParamsFromPath(nsCString& path)
{
  int32_t index = path.FindChar(';');
  if (index >= 0) {
    path.SetLength(index);
  }
}

NS_IMPL_ISUPPORTS_INHERITED(nsFtpState,
                            nsBaseContentStream,
                            nsIInputStreamCallback, 
                            nsITransportEventSink,
                            nsIRequestObserver,
                            nsIProtocolProxyCallback)

nsFtpState::nsFtpState()
    : nsBaseContentStream(true)
    , mState(FTP_INIT)
    , mNextState(FTP_S_USER)
    , mKeepRunning(true)
    , mReceivedControlData(false)
    , mTryingCachedControl(false)
    , mRETRFailed(false)
    , mFileSize(kJS_MAX_SAFE_UINTEGER)
    , mServerType(FTP_GENERIC_TYPE)
    , mAction(GET)
    , mAnonymous(true)
    , mRetryPass(false)
    , mStorReplyReceived(false)
    , mInternalError(NS_OK)
    , mReconnectAndLoginAgain(false)
    , mCacheConnection(true)
    , mPort(21)
    , mAddressChecked(false)
    , mServerIsIPv6(false)
    , mUseUTF8(false)
    , mControlStatus(NS_OK)
    , mDeferredCallbackPending(false)
{
    LOG_INFO(("FTP:(%x) nsFtpState created", this));

    
    NS_ADDREF(gFtpHandler);
}

nsFtpState::~nsFtpState() 
{
    LOG_INFO(("FTP:(%x) nsFtpState destroyed", this));

    if (mProxyRequest)
        mProxyRequest->Cancel(NS_ERROR_FAILURE);

    
    nsFtpProtocolHandler *handler = gFtpHandler;
    NS_RELEASE(handler);
}


NS_IMETHODIMP
nsFtpState::OnInputStreamReady(nsIAsyncInputStream *aInStream)
{
    LOG(("FTP:(%p) data stream ready\n", this));

    
    
    if (HasPendingCallback())
        DispatchCallbackSync();

    return NS_OK;
}

void
nsFtpState::OnControlDataAvailable(const char *aData, uint32_t aDataLen)
{
    LOG(("FTP:(%p) control data available [%u]\n", this, aDataLen));
    mControlConnection->WaitData(this);  

    if (!mReceivedControlData) {
        
        OnTransportStatus(nullptr, NS_NET_STATUS_BEGIN_FTP_TRANSACTION, 0, 0);
        mReceivedControlData = true;
    }

    
    

    nsCString buffer = mControlReadCarryOverBuf;

    
    
    mControlReadCarryOverBuf.Truncate();

    buffer.Append(aData, aDataLen);

    const char* currLine = buffer.get();
    while (*currLine && mKeepRunning) {
        int32_t eolLength = strcspn(currLine, CRLF);
        int32_t currLineLength = strlen(currLine);

        
        
        
        
        
        if (eolLength == 0 && currLineLength <= 1)
            break;

        if (eolLength == currLineLength) {
            mControlReadCarryOverBuf.Assign(currLine);
            break;
        }

        
        nsAutoCString line;
        int32_t crlfLength = 0;

        if ((currLineLength > eolLength) &&
            (currLine[eolLength] == nsCRT::CR) &&
            (currLine[eolLength+1] == nsCRT::LF)) {
            crlfLength = 2; 
        } else {
            crlfLength = 1; 
        }

        line.Assign(currLine, eolLength + crlfLength);
        
        
        bool startNum = (line.Length() >= 3 &&
                           isdigit(line[0]) &&
                           isdigit(line[1]) &&
                           isdigit(line[2]));

        if (mResponseMsg.IsEmpty()) {
            
            

            NS_ASSERTION(line.Length() > 4 && startNum,
                         "Read buffer doesn't include response code");
            
            mResponseCode = atoi(PromiseFlatCString(Substring(line,0,3)).get());
        }

        mResponseMsg.Append(line);

        
        if (startNum && line[3] == ' ') {
            
            if (mState == mNextState) {
                NS_ERROR("ftp read state mixup");
                mInternalError = NS_ERROR_FAILURE;
                mState = FTP_ERROR;
            } else {
                mState = mNextState;
            }

            nsCOMPtr<nsIFTPEventSink> ftpSink;
            mChannel->GetFTPEventSink(ftpSink);
            if (ftpSink)
                ftpSink->OnFTPControlLog(true, mResponseMsg.get());
            
            nsresult rv = Process();
            mResponseMsg.Truncate();
            if (NS_FAILED(rv)) {
                CloseWithStatus(rv);
                return;
            }
        }

        currLine = currLine + eolLength + crlfLength;
    }
}

void
nsFtpState::OnControlError(nsresult status)
{
    NS_ASSERTION(NS_FAILED(status), "expecting error condition");

    LOG(("FTP:(%p) CC(%p) error [%x was-cached=%u]\n",
         this, mControlConnection.get(), status, mTryingCachedControl));

    mControlStatus = status;
    if (mReconnectAndLoginAgain && NS_SUCCEEDED(mInternalError)) {
        mReconnectAndLoginAgain = false;
        mAnonymous = false;
        mControlStatus = NS_OK;
        Connect();
    } else if (mTryingCachedControl && NS_SUCCEEDED(mInternalError)) {
        mTryingCachedControl = false;
        Connect();
    } else {
        CloseWithStatus(status);
    }
}

nsresult
nsFtpState::EstablishControlConnection()
{
    NS_ASSERTION(!mControlConnection, "we already have a control connection");
            
    nsresult rv;

    LOG(("FTP:(%x) trying cached control\n", this));
        
    
    nsFtpControlConnection *connection = nullptr;
    
    if (!mChannel->HasLoadFlag(nsIRequest::LOAD_ANONYMOUS))
        gFtpHandler->RemoveConnection(mChannel->URI(), &connection);

    if (connection) {
        mControlConnection.swap(connection);
        if (mControlConnection->IsAlive())
        {
            
            mControlConnection->WaitData(this);
            
            
            mServerType = mControlConnection->mServerType;           
            mPassword   = mControlConnection->mPassword;
            mPwd        = mControlConnection->mPwd;
            mUseUTF8    = mControlConnection->mUseUTF8;
            mTryingCachedControl = true;

            
            if (mUseUTF8)
                mChannel->SetContentCharset(NS_LITERAL_CSTRING("UTF-8"));
            
            
            mState = FTP_S_PASV;
            mResponseCode = 530;  
            mControlStatus = NS_OK;
            mReceivedControlData = false;  

            
            rv = mControlConnection->Connect(mChannel->ProxyInfo(), this);
            if (NS_SUCCEEDED(rv))
                return rv;
        }
        LOG(("FTP:(%p) cached CC(%p) is unusable\n", this,
            mControlConnection.get()));

        mControlConnection->WaitData(nullptr);
        mControlConnection = nullptr;
    }

    LOG(("FTP:(%p) creating CC\n", this));
        
    mState = FTP_READ_BUF;
    mNextState = FTP_S_USER;
    
    nsAutoCString host;
    rv = mChannel->URI()->GetAsciiHost(host);
    if (NS_FAILED(rv))
        return rv;

    mControlConnection = new nsFtpControlConnection(host, mPort);
    if (!mControlConnection)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = mControlConnection->Connect(mChannel->ProxyInfo(), this);
    if (NS_FAILED(rv)) {
        LOG(("FTP:(%p) CC(%p) failed to connect [rv=%x]\n", this,
            mControlConnection.get(), rv));
        mControlConnection = nullptr;
        return rv;
    }

    return mControlConnection->WaitData(this);
}

void 
nsFtpState::MoveToNextState(FTP_STATE nextState)
{
    if (NS_FAILED(mInternalError)) {
        mState = FTP_ERROR;
        LOG(("FTP:(%x) FAILED (%x)\n", this, mInternalError));
    } else {
        mState = FTP_READ_BUF;
        mNextState = nextState;
    }  
}

nsresult
nsFtpState::Process() 
{
    nsresult    rv = NS_OK;
    bool        processingRead = true;
    
    while (mKeepRunning && processingRead) {
        switch (mState) {
          case FTP_COMMAND_CONNECT:
            KillControlConnection();
            LOG(("FTP:(%p) establishing CC", this));
            mInternalError = EstablishControlConnection();  
            if (NS_FAILED(mInternalError)) {
                mState = FTP_ERROR;
                LOG(("FTP:(%p) FAILED\n", this));
            } else {
                LOG(("FTP:(%p) SUCCEEDED\n", this));
            }
            break;
    
          case FTP_READ_BUF:
            LOG(("FTP:(%p) Waiting for CC(%p)\n", this,
                mControlConnection.get()));
            processingRead = false;
            break;
          
          case FTP_ERROR: 
            if ((mTryingCachedControl && mResponseCode == 530 &&
                mInternalError == NS_ERROR_FTP_PASV) ||
                (mResponseCode == 425 &&
                mInternalError == NS_ERROR_FTP_PASV)) {
                
                
                
                mState = FTP_COMMAND_CONNECT;
            } else if (mResponseCode == 421 && 
                       mInternalError != NS_ERROR_FTP_LOGIN) {
                
                
                
                
                mState = FTP_COMMAND_CONNECT;
            } else if (mAnonymous && 
                       mInternalError == NS_ERROR_FTP_LOGIN) {
                
                
                mAnonymous = false;
                mState = FTP_COMMAND_CONNECT;
            } else {
                LOG(("FTP:(%x) FTP_ERROR - calling StopProcessing\n", this));
                rv = StopProcessing();
                NS_ASSERTION(NS_SUCCEEDED(rv), "StopProcessing failed.");
                processingRead = false;
            }
            break;
          
          case FTP_COMPLETE:
            LOG(("FTP:(%x) COMPLETE\n", this));
            rv = StopProcessing();
            NS_ASSERTION(NS_SUCCEEDED(rv), "StopProcessing failed.");
            processingRead = false;
            break;


          case FTP_S_USER:
            rv = S_user();
            
            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            MoveToNextState(FTP_R_USER);
            break;
            
          case FTP_R_USER:
            mState = R_user();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            break;

          case FTP_S_PASS:
            rv = S_pass();
            
            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            MoveToNextState(FTP_R_PASS);
            break;
            
          case FTP_R_PASS:
            mState = R_pass();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            break;

          case FTP_S_ACCT:
            rv = S_acct();
            
            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            MoveToNextState(FTP_R_ACCT);
            break;
            
          case FTP_R_ACCT:
            mState = R_acct();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            break;


          case FTP_S_SYST:
            rv = S_syst();
            
            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_LOGIN;
            
            MoveToNextState(FTP_R_SYST);
            break;
            
          case FTP_R_SYST:
            mState = R_syst();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FTP_LOGIN;

            break;


          case FTP_S_TYPE:
            rv = S_type();
            
            if (NS_FAILED(rv))
                mInternalError = rv;
            
            MoveToNextState(FTP_R_TYPE);
            break;
            
          case FTP_R_TYPE:
            mState = R_type();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FAILURE;
            
            break;

          case FTP_S_CWD:
            rv = S_cwd();
            
            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_CWD;
            
            MoveToNextState(FTP_R_CWD);
            break;
            
          case FTP_R_CWD:
            mState = R_cwd();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FTP_CWD;
            break;
       

          case FTP_S_LIST:
            rv = S_list();

            if (rv == NS_ERROR_NOT_RESUMABLE) {
                mInternalError = rv;
            } else if (NS_FAILED(rv)) {
                mInternalError = NS_ERROR_FTP_CWD;
            }
            
            MoveToNextState(FTP_R_LIST);
            break;

          case FTP_R_LIST:        
            mState = R_list();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FAILURE;

            break;


          case FTP_S_SIZE:
            rv = S_size();
            
            if (NS_FAILED(rv))
                mInternalError = rv;
            
            MoveToNextState(FTP_R_SIZE);
            break;
            
          case FTP_R_SIZE: 
            mState = R_size();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FAILURE;
            
            break;


          case FTP_S_REST:
            rv = S_rest();
            
            if (NS_FAILED(rv))
                mInternalError = rv;
            
            MoveToNextState(FTP_R_REST);
            break;
            
          case FTP_R_REST:
            mState = R_rest();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FAILURE;
            
            break;


          case FTP_S_MDTM:
            rv = S_mdtm();
            if (NS_FAILED(rv))
                mInternalError = rv;
            MoveToNextState(FTP_R_MDTM);
            break;

          case FTP_R_MDTM:
            mState = R_mdtm();

            
            if (FTP_ERROR == mState && NS_SUCCEEDED(mInternalError))
                mInternalError = NS_ERROR_FAILURE;
            
            break;
            

          case FTP_S_RETR:
            rv = S_retr();
            
            if (NS_FAILED(rv)) 
                mInternalError = rv;
            
            MoveToNextState(FTP_R_RETR);
            break;
            
          case FTP_R_RETR:

            mState = R_retr();
            
            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FAILURE;
            
            break;
            

          case FTP_S_STOR:
            rv = S_stor();

            if (NS_FAILED(rv))
                mInternalError = rv;
            
            MoveToNextState(FTP_R_STOR);
            break;
            
          case FTP_R_STOR:
            mState = R_stor();

            if (FTP_ERROR == mState)
                mInternalError = NS_ERROR_FAILURE;

            break;
            

          case FTP_S_PASV:
            rv = S_pasv();

            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_PASV;
            
            MoveToNextState(FTP_R_PASV);
            break;
            
          case FTP_R_PASV:
            mState = R_pasv();

            if (FTP_ERROR == mState) 
                mInternalError = NS_ERROR_FTP_PASV;

            break;
            

          case FTP_S_PWD:
            rv = S_pwd();

            if (NS_FAILED(rv))
                mInternalError = NS_ERROR_FTP_PWD;
            
            MoveToNextState(FTP_R_PWD);
            break;
            
          case FTP_R_PWD:
            mState = R_pwd();

            if (FTP_ERROR == mState) 
                mInternalError = NS_ERROR_FTP_PWD;

            break;


          case FTP_S_FEAT:
            rv = S_feat();

            if (NS_FAILED(rv))
                mInternalError = rv;

            MoveToNextState(FTP_R_FEAT);
            break;

          case FTP_R_FEAT:
            mState = R_feat();

            
            if (FTP_ERROR == mState && NS_SUCCEEDED(mInternalError))
                mInternalError = NS_ERROR_FAILURE;
            break;


          case FTP_S_OPTS:
            rv = S_opts();

            if (NS_FAILED(rv))
                mInternalError = rv;

            MoveToNextState(FTP_R_OPTS);
            break;

          case FTP_R_OPTS:
            mState = R_opts();

            
            if (FTP_ERROR == mState && NS_SUCCEEDED(mInternalError))
                mInternalError = NS_ERROR_FAILURE;
            break;

          default:
            ;
            
        }
    }

    return rv;
}




nsresult
nsFtpState::S_user() {
    
    if ((mResponseCode == 421) || (mResponseCode == 521))
        return NS_ERROR_FAILURE;

    nsresult rv;
    nsAutoCString usernameStr("USER ");

    mResponseMsg = "";

    if (mAnonymous) {
        mReconnectAndLoginAgain = true;
        usernameStr.AppendLiteral("anonymous");
    } else {
        mReconnectAndLoginAgain = false;
        if (mUsername.IsEmpty()) {

            
            if (mChannel->HasLoadFlag(nsIRequest::LOAD_ANONYMOUS))
              return NS_ERROR_FAILURE;

            nsCOMPtr<nsIAuthPrompt2> prompter;
            NS_QueryAuthPrompt2(static_cast<nsIChannel*>(mChannel),
                                getter_AddRefs(prompter));
            if (!prompter)
                return NS_ERROR_NOT_INITIALIZED;

            nsRefPtr<nsAuthInformationHolder> info =
                new nsAuthInformationHolder(nsIAuthInformation::AUTH_HOST,
                                            EmptyString(),
                                            EmptyCString());

            bool retval;
            rv = prompter->PromptAuth(mChannel, nsIAuthPrompt2::LEVEL_NONE,
                                      info, &retval);

            
            if (NS_FAILED(rv) || !retval || info->User().IsEmpty())
                return NS_ERROR_FAILURE;

            mUsername = info->User();
            mPassword = info->Password();
        }
        
        AppendUTF16toUTF8(mUsername, usernameStr);
    }
    usernameStr.Append(CRLF);

    return SendFTPCommand(usernameStr);
}

FTP_STATE
nsFtpState::R_user() {
    mReconnectAndLoginAgain = false;
    if (mResponseCode/100 == 3) {
        
        return FTP_S_PASS;
    }
    if (mResponseCode/100 == 2) {
        
        return FTP_S_SYST;
    }
    if (mResponseCode/100 == 5) {
        
        
        return FTP_ERROR;
    }
    
    return FTP_ERROR;
}


nsresult
nsFtpState::S_pass() {
    nsresult rv;
    nsAutoCString passwordStr("PASS ");

    mResponseMsg = "";

    if (mAnonymous) {
        if (!mPassword.IsEmpty()) {
            
            AppendUTF16toUTF8(mPassword, passwordStr);
        } else {
            nsXPIDLCString anonPassword;
            bool useRealEmail = false;
            nsCOMPtr<nsIPrefBranch> prefs =
                    do_GetService(NS_PREFSERVICE_CONTRACTID);
            if (prefs) {
                rv = prefs->GetBoolPref("advanced.mailftp", &useRealEmail);
                if (NS_SUCCEEDED(rv) && useRealEmail) {
                    prefs->GetCharPref("network.ftp.anonymous_password",
                                       getter_Copies(anonPassword));
                }
            }
            if (!anonPassword.IsEmpty()) {
                passwordStr.AppendASCII(anonPassword);
            } else {
                
                
                passwordStr.AppendLiteral("mozilla@example.com");
            }
        }
    } else {
        if (mPassword.IsEmpty() || mRetryPass) {
            
            
            if (mChannel->HasLoadFlag(nsIRequest::LOAD_ANONYMOUS))
                return NS_ERROR_FAILURE;

            nsCOMPtr<nsIAuthPrompt2> prompter;
            NS_QueryAuthPrompt2(static_cast<nsIChannel*>(mChannel),
                                getter_AddRefs(prompter));
            if (!prompter)
                return NS_ERROR_NOT_INITIALIZED;

            nsRefPtr<nsAuthInformationHolder> info =
                new nsAuthInformationHolder(nsIAuthInformation::AUTH_HOST |
                                            nsIAuthInformation::ONLY_PASSWORD,
                                            EmptyString(),
                                            EmptyCString());

            info->SetUserInternal(mUsername);

            bool retval;
            rv = prompter->PromptAuth(mChannel, nsIAuthPrompt2::LEVEL_NONE,
                                      info, &retval);

            
            
            if (NS_FAILED(rv) || !retval)
                return NS_ERROR_FAILURE;

            mPassword = info->Password();
        }
        
        AppendUTF16toUTF8(mPassword, passwordStr);
    }
    passwordStr.Append(CRLF);

    return SendFTPCommand(passwordStr);
}

FTP_STATE
nsFtpState::R_pass() {
    if (mResponseCode/100 == 3) {
        
        return FTP_S_ACCT;
    }
    if (mResponseCode/100 == 2) {
        
        return FTP_S_SYST;
    }
    if (mResponseCode == 503) {
        
        
        mRetryPass = false;
        return FTP_S_USER;
    }
    if (mResponseCode/100 == 5 || mResponseCode==421) {
        
        

        if (!mAnonymous)
            mRetryPass = true;

        return FTP_ERROR;
    }
    
    return FTP_ERROR;
}

nsresult
nsFtpState::S_pwd() {
    return SendFTPCommand(NS_LITERAL_CSTRING("PWD" CRLF));
}

FTP_STATE
nsFtpState::R_pwd() {
    
    
    if (mResponseCode/100 != 2)
        return FTP_S_TYPE;

    nsAutoCString respStr(mResponseMsg);
    int32_t pos = respStr.FindChar('"');
    if (pos > -1) {
        respStr.Cut(0, pos+1);
        pos = respStr.FindChar('"');
        if (pos > -1) {
            respStr.Truncate(pos);
            if (mServerType == FTP_VMS_TYPE)
                ConvertDirspecFromVMS(respStr);
            if (respStr.IsEmpty() || respStr.Last() != '/')
                respStr.Append('/');
            mPwd = respStr;
        }
    }
    return FTP_S_TYPE;
}

nsresult
nsFtpState::S_syst() {
    return SendFTPCommand(NS_LITERAL_CSTRING("SYST" CRLF));
}

FTP_STATE
nsFtpState::R_syst() {
    if (mResponseCode/100 == 2) {
        if (( mResponseMsg.Find("L8") > -1) || 
            ( mResponseMsg.Find("UNIX") > -1) || 
            ( mResponseMsg.Find("BSD") > -1) ||
            ( mResponseMsg.Find("MACOS Peter's Server") > -1) ||
            ( mResponseMsg.Find("MACOS WebSTAR FTP") > -1) ||
            ( mResponseMsg.Find("MVS") > -1) ||
            ( mResponseMsg.Find("OS/390") > -1) ||
            ( mResponseMsg.Find("OS/400") > -1)) {
            mServerType = FTP_UNIX_TYPE;
        } else if (( mResponseMsg.Find("WIN32", true) > -1) ||
                   ( mResponseMsg.Find("windows", true) > -1)) {
            mServerType = FTP_NT_TYPE;
        } else if (mResponseMsg.Find("OS/2", true) > -1) {
            mServerType = FTP_OS2_TYPE;
        } else if (mResponseMsg.Find("VMS", true) > -1) {
            mServerType = FTP_VMS_TYPE;
        } else {
            NS_ERROR("Server type list format unrecognized.");
            
            
            nsCOMPtr<nsIStringBundleService> bundleService =
                do_GetService(NS_STRINGBUNDLE_CONTRACTID);
            if (!bundleService)
                return FTP_ERROR;

            nsCOMPtr<nsIStringBundle> bundle;
            nsresult rv = bundleService->CreateBundle(NECKO_MSGS_URL,
                                                      getter_AddRefs(bundle));
            if (NS_FAILED(rv))
                return FTP_ERROR;
            
            char16_t* ucs2Response = ToNewUnicode(mResponseMsg);
            const char16_t *formatStrings[1] = { ucs2Response };
            NS_NAMED_LITERAL_STRING(name, "UnsupportedFTPServer");

            nsXPIDLString formattedString;
            rv = bundle->FormatStringFromName(name.get(), formatStrings, 1,
                                              getter_Copies(formattedString));
            free(ucs2Response);
            if (NS_FAILED(rv))
                return FTP_ERROR;

            
            nsCOMPtr<nsIPrompt> prompter;
            mChannel->GetCallback(prompter);
            if (prompter)
                prompter->Alert(nullptr, formattedString.get());
            
            
            
            mResponseMsg = "";
            return FTP_ERROR;
        }
        
        return FTP_S_FEAT;
    }

    if (mResponseCode/100 == 5) {   
        
        
        mServerType = FTP_UNIX_TYPE;

        return FTP_S_FEAT;
    }
    return FTP_ERROR;
}

nsresult
nsFtpState::S_acct() {
    return SendFTPCommand(NS_LITERAL_CSTRING("ACCT noaccount" CRLF));
}

FTP_STATE
nsFtpState::R_acct() {
    if (mResponseCode/100 == 2)
        return FTP_S_SYST;

    return FTP_ERROR;
}

nsresult
nsFtpState::S_type() {
    return SendFTPCommand(NS_LITERAL_CSTRING("TYPE I" CRLF));
}

FTP_STATE
nsFtpState::R_type() {
    if (mResponseCode/100 != 2) 
        return FTP_ERROR;
    
    return FTP_S_PASV;
}

nsresult
nsFtpState::S_cwd() {
    
    if (mPwd.IsEmpty())
        mCacheConnection = false;

    nsAutoCString cwdStr;
    if (mAction != PUT)
        cwdStr = mPath;
    if (cwdStr.IsEmpty() || cwdStr.First() != '/')
        cwdStr.Insert(mPwd,0);
    if (mServerType == FTP_VMS_TYPE)
        ConvertDirspecToVMS(cwdStr);
    cwdStr.Insert("CWD ",0);
    cwdStr.Append(CRLF);

    return SendFTPCommand(cwdStr);
}

FTP_STATE
nsFtpState::R_cwd() {
    if (mResponseCode/100 == 2) {
        if (mAction == PUT)
            return FTP_S_STOR;
        
        return FTP_S_LIST;
    }
    
    return FTP_ERROR;
}

nsresult
nsFtpState::S_size() {
    nsAutoCString sizeBuf(mPath);
    if (sizeBuf.IsEmpty() || sizeBuf.First() != '/')
        sizeBuf.Insert(mPwd,0);
    if (mServerType == FTP_VMS_TYPE)
        ConvertFilespecToVMS(sizeBuf);
    sizeBuf.Insert("SIZE ",0);
    sizeBuf.Append(CRLF);

    return SendFTPCommand(sizeBuf);
}

FTP_STATE
nsFtpState::R_size() {
    if (mResponseCode/100 == 2) {
        PR_sscanf(mResponseMsg.get() + 4, "%llu", &mFileSize);
        mChannel->SetContentLength(mFileSize);
    }

    
    return FTP_S_MDTM;
}

nsresult
nsFtpState::S_mdtm() {
    nsAutoCString mdtmBuf(mPath);
    if (mdtmBuf.IsEmpty() || mdtmBuf.First() != '/')
        mdtmBuf.Insert(mPwd,0);
    if (mServerType == FTP_VMS_TYPE)
        ConvertFilespecToVMS(mdtmBuf);
    mdtmBuf.Insert("MDTM ",0);
    mdtmBuf.Append(CRLF);

    return SendFTPCommand(mdtmBuf);
}

FTP_STATE
nsFtpState::R_mdtm() {
    if (mResponseCode == 213) {
        mResponseMsg.Cut(0,4);
        mResponseMsg.Trim(" \t\r\n");
        
        if (mResponseMsg.Length() != 14) {
            NS_ASSERTION(mResponseMsg.Length() == 14, "Unknown MDTM response");
        } else {
            mModTime = mResponseMsg;

            
            nsAutoCString timeString;
            nsresult error;
            PRExplodedTime exTime;

            mResponseMsg.Mid(timeString, 0, 4);
            exTime.tm_year  = timeString.ToInteger(&error);
            mResponseMsg.Mid(timeString, 4, 2);
            exTime.tm_month = timeString.ToInteger(&error) - 1; 
            mResponseMsg.Mid(timeString, 6, 2);
            exTime.tm_mday  = timeString.ToInteger(&error);
            mResponseMsg.Mid(timeString, 8, 2);
            exTime.tm_hour  = timeString.ToInteger(&error);
            mResponseMsg.Mid(timeString, 10, 2);
            exTime.tm_min   = timeString.ToInteger(&error);
            mResponseMsg.Mid(timeString, 12, 2);
            exTime.tm_sec   = timeString.ToInteger(&error);
            exTime.tm_usec  = 0;

            exTime.tm_params.tp_gmt_offset = 0;
            exTime.tm_params.tp_dst_offset = 0;

            PR_NormalizeTime(&exTime, PR_GMTParameters);
            exTime.tm_params = PR_LocalTimeParameters(&exTime);

            PRTime time = PR_ImplodeTime(&exTime);
            (void)mChannel->SetLastModifiedTime(time);
        }
    }

    nsCString entityID;
    entityID.Truncate();
    entityID.AppendInt(int64_t(mFileSize));
    entityID.Append('/');
    entityID.Append(mModTime);
    mChannel->SetEntityID(entityID);

    
    if (!mChannel->ResumeRequested())
        return FTP_S_RETR;

    
    if (mSuppliedEntityID.IsEmpty() || entityID.Equals(mSuppliedEntityID))
        return FTP_S_REST;

    mInternalError = NS_ERROR_ENTITY_CHANGED;
    mResponseMsg.Truncate();
    return FTP_ERROR;
}

nsresult 
nsFtpState::SetContentType()
{
    
    
    
    

    if (!mPath.IsEmpty() && mPath.Last() != '/') {
        nsCOMPtr<nsIURL> url = (do_QueryInterface(mChannel->URI()));
        nsAutoCString filePath;
        if(NS_SUCCEEDED(url->GetFilePath(filePath))) {
            filePath.Append('/');
            url->SetFilePath(filePath);
        }
    }
    return mChannel->SetContentType(
        NS_LITERAL_CSTRING(APPLICATION_HTTP_INDEX_FORMAT));
}

nsresult
nsFtpState::S_list() {
    nsresult rv = SetContentType();
    if (NS_FAILED(rv)) 
        
        
        return (nsresult)FTP_ERROR;

    rv = mChannel->PushStreamConverter("text/ftp-dir",
                                       APPLICATION_HTTP_INDEX_FORMAT);
    if (NS_FAILED(rv)) {
        
        
        mResponseMsg = "";
        return rv;
    }

    
    NS_ENSURE_TRUE(!mChannel->ResumeRequested(), NS_ERROR_NOT_RESUMABLE);

    mChannel->SetEntityID(EmptyCString());

    const char *listString;
    if (mServerType == FTP_VMS_TYPE) {
        listString = "LIST *.*;0" CRLF;
    } else {
        listString = "LIST" CRLF;
    }

    return SendFTPCommand(nsDependentCString(listString));
}

FTP_STATE
nsFtpState::R_list() {
    if (mResponseCode/100 == 1) {
        
        if (mDataStream && HasPendingCallback())
            mDataStream->AsyncWait(this, 0, 0, CallbackTarget());
        return FTP_READ_BUF;
    }

    if (mResponseCode/100 == 2) {
        
        mNextState = FTP_COMPLETE;
        return FTP_COMPLETE;
    }
    return FTP_ERROR;
}

nsresult
nsFtpState::S_retr() {
    nsAutoCString retrStr(mPath);
    if (retrStr.IsEmpty() || retrStr.First() != '/')
        retrStr.Insert(mPwd,0);
    if (mServerType == FTP_VMS_TYPE)
        ConvertFilespecToVMS(retrStr);
    retrStr.Insert("RETR ",0);
    retrStr.Append(CRLF);
    return SendFTPCommand(retrStr);
}

FTP_STATE
nsFtpState::R_retr() {
    if (mResponseCode/100 == 2) {
        
        mNextState = FTP_COMPLETE;
        return FTP_COMPLETE;
    }

    if (mResponseCode/100 == 1) {
        if (mDataStream && HasPendingCallback())
            mDataStream->AsyncWait(this, 0, 0, CallbackTarget());
        return FTP_READ_BUF;
    }
    
    
    
    if (mResponseCode == 421 || mResponseCode == 425 || mResponseCode == 426)
        return FTP_ERROR;

    if (mResponseCode/100 == 5) {
        mRETRFailed = true;
        return FTP_S_PASV;
    }

    return FTP_S_CWD;
}


nsresult
nsFtpState::S_rest() {
    
    nsAutoCString restString("REST ");
    
    restString.AppendInt(int64_t(mChannel->StartPos()), 10);
    restString.Append(CRLF);

    return SendFTPCommand(restString);
}

FTP_STATE
nsFtpState::R_rest() {
    if (mResponseCode/100 == 4) {
        
        mChannel->SetEntityID(EmptyCString());

        mInternalError = NS_ERROR_NOT_RESUMABLE;
        mResponseMsg.Truncate();

        return FTP_ERROR;
    }
   
    return FTP_S_RETR; 
}

nsresult
nsFtpState::S_stor() {
    NS_ENSURE_STATE(mChannel->UploadStream());

    NS_ASSERTION(mAction == PUT, "Wrong state to be here");
    
    nsCOMPtr<nsIURL> url = do_QueryInterface(mChannel->URI());
    NS_ASSERTION(url, "I thought you were a nsStandardURL");

    nsAutoCString storStr;
    url->GetFilePath(storStr);
    NS_ASSERTION(!storStr.IsEmpty(), "What does it mean to store a empty path");
        
    
    if (storStr.First() == '/')
        storStr.Cut(0,1);

    if (mServerType == FTP_VMS_TYPE)
        ConvertFilespecToVMS(storStr);

    NS_UnescapeURL(storStr);
    storStr.Insert("STOR ",0);
    storStr.Append(CRLF);

    return SendFTPCommand(storStr);
}

FTP_STATE
nsFtpState::R_stor() {
    if (mResponseCode/100 == 2) {
        
        mNextState = FTP_COMPLETE;
        mStorReplyReceived = true;

        
        if (!mUploadRequest && !IsClosed())
            Close();

        return FTP_COMPLETE;
    }

    if (mResponseCode/100 == 1) {
        LOG(("FTP:(%x) writing on DT\n", this));
        return FTP_READ_BUF;
    }

   mStorReplyReceived = true;
   return FTP_ERROR;
}


nsresult
nsFtpState::S_pasv() {
    if (!mAddressChecked) {
        
        mAddressChecked = true;
        mServerAddress.raw.family = AF_INET;
        mServerAddress.inet.ip = htonl(INADDR_ANY);
        mServerAddress.inet.port = htons(0);

        nsITransport *controlSocket = mControlConnection->Transport();
        if (!controlSocket)
            
            
            return (nsresult)FTP_ERROR;

        nsCOMPtr<nsISocketTransport> sTrans = do_QueryInterface(controlSocket);
        if (sTrans) {
            nsresult rv = sTrans->GetPeerAddr(&mServerAddress);
            if (NS_SUCCEEDED(rv)) {
                if (!IsIPAddrAny(&mServerAddress))
                    mServerIsIPv6 = (mServerAddress.raw.family == AF_INET6) &&
                                    !IsIPAddrV4Mapped(&mServerAddress);
                else {
                    






                    NetAddr selfAddress;
                    rv = sTrans->GetSelfAddr(&selfAddress);
                    if (NS_SUCCEEDED(rv))
                        mServerIsIPv6 = (selfAddress.raw.family == AF_INET6) &&
                                        !IsIPAddrV4Mapped(&selfAddress);
                }
            }
        }
    }

    const char *string;
    if (mServerIsIPv6) {
        string = "EPSV" CRLF;
    } else {
        string = "PASV" CRLF;
    }

    return SendFTPCommand(nsDependentCString(string));
    
}

FTP_STATE
nsFtpState::R_pasv() {
    if (mResponseCode/100 != 2)
        return FTP_ERROR;

    nsresult rv;
    int32_t port;

    nsAutoCString responseCopy(mResponseMsg);
    char *response = responseCopy.BeginWriting();

    char *ptr = response;

    

    if (mServerIsIPv6) {
        
        
        
        char delim;
        while (*ptr && *ptr != '(')
            ptr++;
        if (*ptr++ != '(')
            return FTP_ERROR;
        delim = *ptr++;
        if (!delim || *ptr++ != delim ||
                      *ptr++ != delim || 
                      *ptr < '0' || *ptr > '9')
            return FTP_ERROR;
        port = 0;
        do {
            port = port * 10 + *ptr++ - '0';
        } while (*ptr >= '0' && *ptr <= '9');
        if (*ptr++ != delim || *ptr != ')')
            return FTP_ERROR;
    } else {
        
        
        
        int32_t h0, h1, h2, h3, p0, p1;

        uint32_t fields = 0;
        
        while (*ptr && *ptr != '(')
            ++ptr;
        if (*ptr) {
            ++ptr;
            fields = PR_sscanf(ptr, 
                               "%ld,%ld,%ld,%ld,%ld,%ld",
                               &h0, &h1, &h2, &h3, &p0, &p1);
        }
        if (!*ptr || fields < 6) {
            
            ptr = response;
            while (*ptr && *ptr != ',')
                ++ptr;
            if (*ptr) {
                
                do {
                    ptr--;
                } while ((ptr >=response) && (*ptr >= '0') && (*ptr <= '9'));
                ptr++; 
                fields = PR_sscanf(ptr, 
                                   "%ld,%ld,%ld,%ld,%ld,%ld",
                                   &h0, &h1, &h2, &h3, &p0, &p1);
            }
        }

        NS_ASSERTION(fields == 6, "Can't parse PASV response");
        if (fields < 6)
            return FTP_ERROR;

        port = ((int32_t) (p0<<8)) + p1;
    }

    bool newDataConn = true;
    if (mDataTransport) {
        
        
        nsCOMPtr<nsISocketTransport> strans = do_QueryInterface(mDataTransport);
        if (strans) {
            int32_t oldPort;
            nsresult rv = strans->GetPort(&oldPort);
            if (NS_SUCCEEDED(rv)) {
                if (oldPort == port) {
                    bool isAlive;
                    if (NS_SUCCEEDED(strans->IsAlive(&isAlive)) && isAlive)
                        newDataConn = false;
                }
            }
        }

        if (newDataConn) {
            mDataTransport->Close(NS_ERROR_ABORT);
            mDataTransport = nullptr;
            mDataStream = nullptr;
        }
    }

    if (newDataConn) {
        
        nsCOMPtr<nsISocketTransportService> sts =
            do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID);
        if (!sts)
            return FTP_ERROR;
       
        nsCOMPtr<nsISocketTransport> strans;

        nsAutoCString host;
        if (!IsIPAddrAny(&mServerAddress)) {
            char buf[kIPv6CStrBufSize];
            NetAddrToString(&mServerAddress, buf, sizeof(buf));
            host.Assign(buf);
        } else {
            





            rv = mChannel->URI()->GetAsciiHost(host);
            if (NS_FAILED(rv))
                return FTP_ERROR;
        }

        rv =  sts->CreateTransport(nullptr, 0, host,
                                   port, mChannel->ProxyInfo(),
                                   getter_AddRefs(strans)); 
        if (NS_FAILED(rv))
            return FTP_ERROR;
        mDataTransport = strans;

        strans->SetQoSBits(gFtpHandler->GetDataQoSBits());
        
        LOG(("FTP:(%x) created DT (%s:%x)\n", this, host.get(), port));
        
        
        rv = mDataTransport->SetEventSink(this, NS_GetCurrentThread());
        NS_ENSURE_SUCCESS(rv, FTP_ERROR);

        if (mAction == PUT) {
            NS_ASSERTION(!mRETRFailed, "Failed before uploading");

            
            
            nsCOMPtr<nsIOutputStream> output;
            rv = mDataTransport->OpenOutputStream(nsITransport::OPEN_UNBUFFERED,
                                                  0, 0, getter_AddRefs(output));
            if (NS_FAILED(rv))
                return FTP_ERROR;

            
            
            
            nsCOMPtr<nsIEventTarget> stEventTarget =
                do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID);
            if (!stEventTarget)
                return FTP_ERROR;
            
            nsCOMPtr<nsIAsyncStreamCopier> copier;
            rv = NS_NewAsyncStreamCopier(getter_AddRefs(copier),
                                         mChannel->UploadStream(),
                                         output,
                                         stEventTarget,
                                         true,   
                                         false); 
            if (NS_FAILED(rv))
                return FTP_ERROR;
        
            rv = copier->AsyncCopy(this, nullptr);
            if (NS_FAILED(rv))
                return FTP_ERROR;

            
            mUploadRequest = copier;

            
            
            
            return FTP_S_CWD;
        }

        
        
        

        
        nsCOMPtr<nsIInputStream> input;
        rv = mDataTransport->OpenInputStream(0,
                                             nsIOService::gDefaultSegmentSize,
                                             nsIOService::gDefaultSegmentCount,
                                             getter_AddRefs(input));
        NS_ENSURE_SUCCESS(rv, FTP_ERROR);
        mDataStream = do_QueryInterface(input);
    }

    if (mRETRFailed || mPath.IsEmpty() || mPath.Last() == '/')
        return FTP_S_CWD;
    return FTP_S_SIZE;
}

nsresult
nsFtpState::S_feat() {
    return SendFTPCommand(NS_LITERAL_CSTRING("FEAT" CRLF));
}

FTP_STATE
nsFtpState::R_feat() {
    if (mResponseCode/100 == 2) {
        if (mResponseMsg.Find(NS_LITERAL_CSTRING(CRLF " UTF8" CRLF), true) > -1) {
            
            mChannel->SetContentCharset(NS_LITERAL_CSTRING("UTF-8"));
            mUseUTF8 = true;
            return FTP_S_OPTS;
        }
    }

    mUseUTF8 = false;
    return FTP_S_PWD;
}

nsresult
nsFtpState::S_opts() {
     
    return SendFTPCommand(NS_LITERAL_CSTRING("OPTS UTF8 ON" CRLF));
}

FTP_STATE
nsFtpState::R_opts() {
    
    
    return FTP_S_PWD;
}




nsresult
nsFtpState::Init(nsFtpChannel *channel)
{
    
    NS_ASSERTION(channel, "FTP: needs a channel");

    mChannel = channel; 

    
    mCountRecv = 0;

#ifdef MOZ_WIDGET_GONK
    nsCOMPtr<nsINetworkInterface> activeNetwork;
    GetActiveNetworkInterface(activeNetwork);
    mActiveNetwork =
        new nsMainThreadPtrHolder<nsINetworkInterface>(activeNetwork);
#endif

    mKeepRunning = true;
    mSuppliedEntityID = channel->EntityID();

    if (channel->UploadStream())
        mAction = PUT;

    nsresult rv;
    nsCOMPtr<nsIURL> url = do_QueryInterface(mChannel->URI());

    nsAutoCString host;
    if (url) {
        rv = url->GetAsciiHost(host);
    } else {
        rv = mChannel->URI()->GetAsciiHost(host);
    }
    if (NS_FAILED(rv) || host.IsEmpty()) {
        return NS_ERROR_MALFORMED_URI;
    }

    nsAutoCString path;
    if (url) {
        rv = url->GetFilePath(path);
    } else {
        rv = mChannel->URI()->GetPath(path);
    }
    if (NS_FAILED(rv))
        return rv;

    removeParamsFromPath(path);
    
    
    if (url) {
        url->SetFilePath(path);
    } else {
        mChannel->URI()->SetPath(path);
    }
        
    
    char *fwdPtr = path.BeginWriting();
    if (!fwdPtr)
        return NS_ERROR_OUT_OF_MEMORY;
    if (*fwdPtr == '/')
        fwdPtr++;
    if (*fwdPtr != '\0') {
        
        int32_t len = NS_UnescapeURL(fwdPtr);
        mPath.Assign(fwdPtr, len);

#ifdef DEBUG
        if (mPath.FindCharInSet(CRLF) >= 0)
            NS_ERROR("NewURI() should've prevented this!!!");
#endif
    }

    
    nsAutoCString uname;
    rv = mChannel->URI()->GetUsername(uname);
    if (NS_FAILED(rv))
        return rv;

    if (!uname.IsEmpty() && !uname.EqualsLiteral("anonymous")) {
        mAnonymous = false;
        CopyUTF8toUTF16(NS_UnescapeURL(uname), mUsername);
        
        
        if (uname.FindCharInSet(CRLF) >= 0)
            return NS_ERROR_MALFORMED_URI;
    }

    nsAutoCString password;
    rv = mChannel->URI()->GetPassword(password);
    if (NS_FAILED(rv))
        return rv;

    CopyUTF8toUTF16(NS_UnescapeURL(password), mPassword);

    
    if (mPassword.FindCharInSet(CRLF) >= 0)
        return NS_ERROR_MALFORMED_URI;

    int32_t port;
    rv = mChannel->URI()->GetPort(&port);
    if (NS_FAILED(rv))
        return rv;

    if (port > 0)
        mPort = port;

    
    
    nsCOMPtr<nsIProtocolProxyService> pps =
        do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID);

    if (pps && !mChannel->ProxyInfo()) {
        pps->AsyncResolve(static_cast<nsIChannel*>(mChannel), 0, this,
                          getter_AddRefs(mProxyRequest));
    }

    return NS_OK;
}

void
nsFtpState::Connect()
{
    mState = FTP_COMMAND_CONNECT;
    mNextState = FTP_S_USER;

    nsresult rv = Process();

    
    if (NS_FAILED(rv)) {
        LOG(("FTP:Process() failed: %x\n", rv));
        mInternalError = NS_ERROR_FAILURE;
        mState = FTP_ERROR;
        CloseWithStatus(mInternalError);
    }
}

void
nsFtpState::KillControlConnection()
{
    mControlReadCarryOverBuf.Truncate(0);

    mAddressChecked = false;
    mServerIsIPv6 = false;

    
    
    
    

    if (!mControlConnection)
        return;

    
    mControlConnection->WaitData(nullptr);

    if (NS_SUCCEEDED(mInternalError) &&
        NS_SUCCEEDED(mControlStatus) &&
        mControlConnection->IsAlive() &&
        mCacheConnection) {

        LOG_INFO(("FTP:(%p) caching CC(%p)", this, mControlConnection.get()));

        
        mControlConnection->mServerType = mServerType;           
        mControlConnection->mPassword = mPassword;
        mControlConnection->mPwd = mPwd;
        mControlConnection->mUseUTF8 = mUseUTF8;
        
        nsresult rv = NS_OK;
        
        if (!mChannel->HasLoadFlag(nsIRequest::LOAD_ANONYMOUS))
            rv = gFtpHandler->InsertConnection(mChannel->URI(),
                                               mControlConnection);
        
        mControlConnection->Disconnect(rv);
    } else {
        mControlConnection->Disconnect(NS_BINDING_ABORTED);
    }     

    mControlConnection = nullptr;
}

class nsFtpAsyncAlert : public nsRunnable
{
public:
    nsFtpAsyncAlert(nsIPrompt *aPrompter, nsString aResponseMsg)
        : mPrompter(aPrompter)
        , mResponseMsg(aResponseMsg)
    {
        MOZ_COUNT_CTOR(nsFtpAsyncAlert);
    }
protected:
    virtual ~nsFtpAsyncAlert()
    {
        MOZ_COUNT_DTOR(nsFtpAsyncAlert);
    }
public:
    NS_IMETHOD Run()
    {
        if (mPrompter) {
            mPrompter->Alert(nullptr, mResponseMsg.get());
        }
        return NS_OK;
    }
private:
    nsCOMPtr<nsIPrompt> mPrompter;
    nsString mResponseMsg;
};


nsresult
nsFtpState::StopProcessing()
{
    
    if (!mKeepRunning)
        return NS_OK;
    mKeepRunning = false;

    LOG_INFO(("FTP:(%x) nsFtpState stopping", this));

#ifdef DEBUG_dougt
    printf("FTP Stopped: [response code %d] [response msg follows:]\n%s\n", mResponseCode, mResponseMsg.get());
#endif

    if (NS_FAILED(mInternalError) && !mResponseMsg.IsEmpty()) {
        
        

        
        nsCOMPtr<nsIPrompt> prompter;
        mChannel->GetCallback(prompter);
        if (prompter) {
            nsCOMPtr<nsIRunnable> alertEvent;
            if (mUseUTF8) {
                alertEvent = new nsFtpAsyncAlert(prompter,
                    NS_ConvertUTF8toUTF16(mResponseMsg));
            } else {
                alertEvent = new nsFtpAsyncAlert(prompter,
                    NS_ConvertASCIItoUTF16(mResponseMsg));
            }
            NS_DispatchToMainThread(alertEvent);
        }
    }

    nsresult broadcastErrorCode = mControlStatus;
    if (NS_SUCCEEDED(broadcastErrorCode))
        broadcastErrorCode = mInternalError;

    mInternalError = broadcastErrorCode;

    KillControlConnection();

    
    OnTransportStatus(nullptr, NS_NET_STATUS_END_FTP_TRANSACTION, 0, 0);

    if (NS_FAILED(broadcastErrorCode))
        CloseWithStatus(broadcastErrorCode);

    return NS_OK;
}

nsresult 
nsFtpState::SendFTPCommand(const nsCSubstring& command)
{
    NS_ASSERTION(mControlConnection, "null control connection");        
    
    
    nsAutoCString logcmd(command);
    if (StringBeginsWith(command, NS_LITERAL_CSTRING("PASS "))) 
        logcmd = "PASS xxxxx";
    
    LOG(("FTP:(%x) writing \"%s\"\n", this, logcmd.get()));

    nsCOMPtr<nsIFTPEventSink> ftpSink;
    mChannel->GetFTPEventSink(ftpSink);
    if (ftpSink)
        ftpSink->OnFTPControlLog(false, logcmd.get());
    
    if (mControlConnection)
        return mControlConnection->Write(command);

    return NS_ERROR_FAILURE;
}




void
nsFtpState::ConvertFilespecToVMS(nsCString& fileString)
{
    int ntok=1;
    char *t, *nextToken;
    nsAutoCString fileStringCopy;

    
    fileStringCopy = fileString;
    t = nsCRT::strtok(fileStringCopy.BeginWriting(), "/", &nextToken);
    if (t)
        while (nsCRT::strtok(nextToken, "/", &nextToken))
            ntok++; 
    LOG(("FTP:(%x) ConvertFilespecToVMS ntok: %d\n", this, ntok));
    LOG(("FTP:(%x) ConvertFilespecToVMS from: \"%s\"\n", this, fileString.get()));

    if (fileString.First() == '/') {
        
        
        
        
        
        
        if (ntok == 1) {
            if (fileString.Length() == 1) {
                
                fileString.Truncate();
                fileString.AppendLiteral("[]");
            } else {
                
                fileStringCopy = fileString;
                fileString = Substring(fileStringCopy, 1,
                                       fileStringCopy.Length()-1);
            }
        } else {
            
            fileStringCopy = fileString;
            fileString.Truncate();
            fileString.Append(nsCRT::strtok(fileStringCopy.BeginWriting(), 
                              "/", &nextToken));
            fileString.AppendLiteral(":[");
            if (ntok > 2) {
                for (int i=2; i<ntok; i++) {
                    if (i > 2) fileString.Append('.');
                    fileString.Append(nsCRT::strtok(nextToken,
                                      "/", &nextToken));
                }
            } else {
                fileString.AppendLiteral("000000");
            }
            fileString.Append(']');
            fileString.Append(nsCRT::strtok(nextToken, "/", &nextToken));
        }
    } else {
       
        
        
        
        if (ntok == 1) {
            
        } else {
            
            fileStringCopy = fileString;
            fileString.Truncate();
            fileString.AppendLiteral("[.");
            fileString.Append(nsCRT::strtok(fileStringCopy.BeginWriting(),
                              "/", &nextToken));
            if (ntok > 2) {
                for (int i=2; i<ntok; i++) {
                    fileString.Append('.');
                    fileString.Append(nsCRT::strtok(nextToken,
                                      "/", &nextToken));
                }
            }
            fileString.Append(']');
            fileString.Append(nsCRT::strtok(nextToken, "/", &nextToken));
        }
    }
    LOG(("FTP:(%x) ConvertFilespecToVMS   to: \"%s\"\n", this, fileString.get()));
}






void
nsFtpState::ConvertDirspecToVMS(nsCString& dirSpec)
{
    LOG(("FTP:(%x) ConvertDirspecToVMS from: \"%s\"\n", this, dirSpec.get()));
    if (!dirSpec.IsEmpty()) {
        if (dirSpec.Last() != '/')
            dirSpec.Append('/');
        
        dirSpec.Append('x');
        ConvertFilespecToVMS(dirSpec);
        dirSpec.Truncate(dirSpec.Length()-1);
    }
    LOG(("FTP:(%x) ConvertDirspecToVMS   to: \"%s\"\n", this, dirSpec.get()));
}


void
nsFtpState::ConvertDirspecFromVMS(nsCString& dirSpec)
{
    LOG(("FTP:(%x) ConvertDirspecFromVMS from: \"%s\"\n", this, dirSpec.get()));
    if (dirSpec.IsEmpty()) {
        dirSpec.Insert('.', 0);
    } else {
        dirSpec.Insert('/', 0);
        dirSpec.ReplaceSubstring(":[", "/");
        dirSpec.ReplaceChar('.', '/');
        dirSpec.ReplaceChar(']', '/');
    }
    LOG(("FTP:(%x) ConvertDirspecFromVMS   to: \"%s\"\n", this, dirSpec.get()));
}



NS_IMETHODIMP
nsFtpState::OnTransportStatus(nsITransport *transport, nsresult status,
                              int64_t progress, int64_t progressMax)
{
    

    
    if (mControlConnection && transport == mControlConnection->Transport()) {
        switch (status) {
        case NS_NET_STATUS_RESOLVING_HOST:
        case NS_NET_STATUS_RESOLVED_HOST:
        case NS_NET_STATUS_CONNECTING_TO:
        case NS_NET_STATUS_CONNECTED_TO:
            break;
        default:
            return NS_OK;
        }
    }

    
    
    
    mChannel->OnTransportStatus(nullptr, status, progress,
                                mFileSize - mChannel->StartPos());
    return NS_OK;
}



NS_IMETHODIMP
nsFtpState::OnStartRequest(nsIRequest *request, nsISupports *context)
{
    mStorReplyReceived = false;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpState::OnStopRequest(nsIRequest *request, nsISupports *context,
                          nsresult status)
{
    mUploadRequest = nullptr;

    
    
    if (!mStorReplyReceived)
      return NS_OK;

    
    Close();
    return NS_OK;
}



NS_IMETHODIMP
nsFtpState::Available(uint64_t *result)
{
    if (mDataStream)
        return mDataStream->Available(result);

    return nsBaseContentStream::Available(result);
}

NS_IMETHODIMP
nsFtpState::ReadSegments(nsWriteSegmentFun writer, void *closure,
                         uint32_t count, uint32_t *result)
{
    
    

    if (mDataStream) {
        nsWriteSegmentThunk thunk = { this, writer, closure };
        nsresult rv;
        rv = mDataStream->ReadSegments(NS_WriteSegmentThunk, &thunk, count,
                                       result);
        if (NS_SUCCEEDED(rv)) {
            CountRecvBytes(*result);
        }
        return rv;
    }

    return nsBaseContentStream::ReadSegments(writer, closure, count, result);
}

nsresult
nsFtpState::SaveNetworkStats(bool enforce)
{
#ifdef MOZ_WIDGET_GONK
    
    uint32_t appId;
    bool isInBrowser;
    NS_GetAppInfo(mChannel, &appId, &isInBrowser);

    
    if (!mActiveNetwork || appId == NECKO_NO_APP_ID) {
        return NS_OK;
    }

    if (mCountRecv <= 0) {
        
        return NS_OK;
    }

    
    
    
    if (!enforce && mCountRecv < NETWORK_STATS_THRESHOLD) {
        return NS_OK;
    }

    
    
    nsRefPtr<nsRunnable> event =
        new SaveNetworkStatsEvent(appId, isInBrowser, mActiveNetwork,
                                  mCountRecv, 0, false);
    NS_DispatchToMainThread(event);

    
    mCountRecv = 0;

    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsFtpState::CloseWithStatus(nsresult status)
{
    LOG(("FTP:(%p) close [%x]\n", this, status));

    
    
    if (!IsClosed() && status != NS_BASE_STREAM_CLOSED && NS_FAILED(status)) {
        if (NS_SUCCEEDED(mInternalError))
            mInternalError = status;
        StopProcessing();
    }

    if (mUploadRequest) {
        mUploadRequest->Cancel(NS_ERROR_ABORT);
        mUploadRequest = nullptr;
    }

    if (mDataTransport) {
        
        SaveNetworkStats(true);

        
        mDataTransport->Close(NS_ERROR_ABORT);
        mDataTransport = nullptr;
    }

    mDataStream = nullptr;

    return nsBaseContentStream::CloseWithStatus(status);
}

static nsresult
CreateHTTPProxiedChannel(nsIChannel *channel, nsIProxyInfo *pi, nsIChannel **newChannel)
{
    nsresult rv;
    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = ioService->GetProtocolHandler("http", getter_AddRefs(handler));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProxiedProtocolHandler> pph = do_QueryInterface(handler, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));

    nsCOMPtr<nsILoadInfo> loadInfo;
    channel->GetLoadInfo(getter_AddRefs(loadInfo));

    return pph->NewProxiedChannel2(uri, pi, 0, nullptr, loadInfo, newChannel);
}

NS_IMETHODIMP
nsFtpState::OnProxyAvailable(nsICancelable *request, nsIChannel *channel,
                             nsIProxyInfo *pi, nsresult status)
{
  mProxyRequest = nullptr;

  

  if (NS_SUCCEEDED(status)) {
    nsAutoCString type;
    if (pi && NS_SUCCEEDED(pi->GetType(type)) && type.EqualsLiteral("http")) {
        
        
        
        
        LOG(("FTP:(%p) Configured to use a HTTP proxy channel\n", this));

        nsCOMPtr<nsIChannel> newChannel;
        if (NS_SUCCEEDED(CreateHTTPProxiedChannel(channel, pi,
                                                  getter_AddRefs(newChannel))) &&
            NS_SUCCEEDED(mChannel->Redirect(newChannel,
                                            nsIChannelEventSink::REDIRECT_INTERNAL,
                                            true))) {
            LOG(("FTP:(%p) Redirected to use a HTTP proxy channel\n", this));
            return NS_OK;
        }
    }
    else if (pi) {
        
        LOG(("FTP:(%p) Configured to use a SOCKS proxy channel\n", this));
        mChannel->SetProxyInfo(pi);
    }
  }

  if (mDeferredCallbackPending) {
      mDeferredCallbackPending = false;
      OnCallbackPending();
  }
  return NS_OK;
}

void
nsFtpState::OnCallbackPending()
{
    if (mState == FTP_INIT) {
        if (mProxyRequest) {
            mDeferredCallbackPending = true;
            return;
        }
        Connect();
    } else if (mDataStream) {
        mDataStream->AsyncWait(this, 0, 0, CallbackTarget());
    }
}

