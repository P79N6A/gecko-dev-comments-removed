





#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "jsapi.h"
#include "jswrapper.h"
#include "nsCRT.h"
#include "nsError.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsJSProtocolHandler.h"
#include "nsStringStream.h"
#include "nsNetUtil.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWindowMediator.h"
#include "nsPIDOMWindow.h"
#include "nsIConsoleService.h"
#include "nsXPIDLString.h"
#include "prprf.h"
#include "nsEscape.h"
#include "nsIWebNavigation.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIXPConnect.h"
#include "nsContentUtils.h"
#include "nsNullPrincipal.h"
#include "nsJSUtils.h"
#include "nsThreadUtils.h"
#include "nsIScriptChannel.h"
#include "nsIDocument.h"
#include "nsILoadInfo.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIWritablePropertyBag2.h"
#include "nsIContentSecurityPolicy.h"
#include "nsSandboxFlags.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsILoadInfo.h"

#include "mozilla/ipc/URIUtils.h"

using mozilla::dom::AutoEntryScript;

static NS_DEFINE_CID(kJSURICID, NS_JSURI_CID);

class nsJSThunk : public nsIInputStream
{
public:
    nsJSThunk();

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_FORWARD_SAFE_NSIINPUTSTREAM(mInnerStream)

    nsresult Init(nsIURI* uri);
    nsresult EvaluateScript(nsIChannel *aChannel,
                            PopupControlState aPopupState,
                            uint32_t aExecutionPolicy,
                            nsPIDOMWindow *aOriginalInnerWindow);

protected:
    virtual ~nsJSThunk();

    nsCOMPtr<nsIInputStream>    mInnerStream;
    nsCString                   mScript;
    nsCString                   mURL;
};




NS_IMPL_ISUPPORTS(nsJSThunk, nsIInputStream)


nsJSThunk::nsJSThunk()
{
}

nsJSThunk::~nsJSThunk()
{
}

nsresult nsJSThunk::Init(nsIURI* uri)
{
    NS_ENSURE_ARG_POINTER(uri);

    
    nsresult rv = uri->GetPath(mScript);
    if (NS_FAILED(rv)) return rv;

    
    rv = uri->GetSpec(mURL);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

static bool
IsISO88591(const nsString& aString)
{
    for (nsString::const_char_iterator c = aString.BeginReading(),
                                   c_end = aString.EndReading();
         c < c_end; ++c) {
        if (*c > 255)
            return false;
    }
    return true;
}

static
nsIScriptGlobalObject* GetGlobalObject(nsIChannel* aChannel)
{
    
    nsCOMPtr<nsIDocShell> docShell;
    NS_QueryNotificationCallbacks(aChannel, docShell);
    if (!docShell) {
        NS_WARNING("Unable to get a docShell from the channel!");
        return nullptr;
    }

    
    nsIScriptGlobalObject* global = docShell->GetScriptGlobalObject();

    NS_ASSERTION(global,
                 "Unable to get an nsIScriptGlobalObject from the "
                 "docShell!");
    return global;
}

nsresult nsJSThunk::EvaluateScript(nsIChannel *aChannel,
                                   PopupControlState aPopupState,
                                   uint32_t aExecutionPolicy,
                                   nsPIDOMWindow *aOriginalInnerWindow)
{
    if (aExecutionPolicy == nsIScriptChannel::NO_EXECUTION) {
        
        return NS_ERROR_DOM_RETVAL_UNDEFINED;
    }
    
    NS_ENSURE_ARG_POINTER(aChannel);

    
    nsCOMPtr<nsISupports> owner;
    aChannel->GetOwner(getter_AddRefs(owner));
    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(owner);
    if (!principal) {
        nsCOMPtr<nsILoadInfo> loadInfo;
        aChannel->GetLoadInfo(getter_AddRefs(loadInfo));
        if (loadInfo && loadInfo->GetForceInheritPrincipal()) {
            principal = loadInfo->TriggeringPrincipal();
        } else {
            
            NS_ASSERTION(!owner, "Non-principal owner?");
            NS_WARNING("No principal to execute JS with");
            return NS_ERROR_DOM_RETVAL_UNDEFINED;
        }
    }

    nsresult rv;

    
    
    nsCOMPtr<nsIContentSecurityPolicy> csp;
    rv = principal->GetCsp(getter_AddRefs(csp));
    NS_ENSURE_SUCCESS(rv, rv);
    if (csp) {
        bool allowsInline = true;
        bool reportViolations = false;
        rv = csp->GetAllowsInlineScript(&reportViolations, &allowsInline);
        NS_ENSURE_SUCCESS(rv, rv);

        if (reportViolations) {
            
            nsCOMPtr<nsIURI> uri;
            principal->GetURI(getter_AddRefs(uri));
            nsAutoCString asciiSpec;
            uri->GetAsciiSpec(asciiSpec);
            csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_SCRIPT,
                                     NS_ConvertUTF8toUTF16(asciiSpec),
                                     NS_ConvertUTF8toUTF16(mURL),
                                     0,
                                     EmptyString(),
                                     EmptyString());
        }

        
        if (!allowsInline) {
          return NS_ERROR_DOM_RETVAL_UNDEFINED;
        }
    }

    
    nsIScriptGlobalObject* global = GetGlobalObject(aChannel);
    if (!global) {
        return NS_ERROR_FAILURE;
    }

    
    
    nsIDocument* doc = aOriginalInnerWindow->GetExtantDoc();
    if (doc && (doc->GetSandboxFlags() & SANDBOXED_SCRIPTS)) {
        return NS_ERROR_DOM_RETVAL_UNDEFINED;
    }

    
    nsAutoPopupStatePusher popupStatePusher(aPopupState);

    
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(global);
    nsPIDOMWindow *innerWin = win->GetCurrentInnerWindow();

    if (innerWin != aOriginalInnerWindow) {
        return NS_ERROR_UNEXPECTED;
    }

    nsCOMPtr<nsIScriptGlobalObject> innerGlobal = do_QueryInterface(innerWin);

    nsCOMPtr<nsIDOMWindow> domWindow(do_QueryInterface(global, &rv));
    if (NS_FAILED(rv)) {
        return NS_ERROR_FAILURE;
    }

    
    nsCOMPtr<nsIScriptContext> scriptContext = global->GetContext();
    if (!scriptContext)
        return NS_ERROR_FAILURE;

    nsAutoCString script(mScript);
    
    NS_UnescapeURL(script);


    nsCOMPtr<nsIScriptSecurityManager> securityManager;
    securityManager = do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    
    
    AutoEntryScript entryScript(innerGlobal, "javascript: URI", true,
                                scriptContext->GetNativeContext());
    JSContext* cx = entryScript.cx();
    JS::Rooted<JSObject*> globalJSObject(cx, innerGlobal->GetGlobalJSObject());
    NS_ENSURE_TRUE(globalJSObject, NS_ERROR_UNEXPECTED);

    
    
    nsIPrincipal* objectPrincipal = nsContentUtils::ObjectPrincipal(globalJSObject);

    bool subsumes;
    rv = principal->Subsumes(objectPrincipal, &subsumes);
    if (NS_FAILED(rv))
        return rv;

    if (!subsumes) {
        return NS_ERROR_DOM_RETVAL_UNDEFINED;
    }

    JS::Rooted<JS::Value> v (cx, JS::UndefinedValue());
    
    JS::CompileOptions options(cx);
    options.setFileAndLine(mURL.get(), 1)
           .setVersion(JSVERSION_DEFAULT);
    nsJSUtils::EvaluateOptions evalOptions(cx);
    evalOptions.setCoerceToString(true);
    rv = nsJSUtils::EvaluateString(cx, NS_ConvertUTF8toUTF16(script),
                                   globalJSObject, options, evalOptions, &v);

    
    
    
    
    
    
    
    ::JS_ReportPendingException(cx);

    if (NS_FAILED(rv) || !(v.isString() || v.isUndefined())) {
        return NS_ERROR_MALFORMED_URI;
    } else if (v.isUndefined()) {
        return NS_ERROR_DOM_RETVAL_UNDEFINED;
    } else {
        nsAutoJSString result;
        if (!result.init(cx, v)) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        char *bytes;
        uint32_t bytesLen;
        NS_NAMED_LITERAL_CSTRING(isoCharset, "ISO-8859-1");
        NS_NAMED_LITERAL_CSTRING(utf8Charset, "UTF-8");
        const nsCString *charset;
        if (IsISO88591(result)) {
            
            
            
            bytes = ToNewCString(result);
            bytesLen = result.Length();
            charset = &isoCharset;
        }
        else {
            bytes = ToNewUTF8String(result, &bytesLen);
            charset = &utf8Charset;
        }
        aChannel->SetContentCharset(*charset);
        if (bytes)
            rv = NS_NewByteInputStream(getter_AddRefs(mInnerStream),
                                       bytes, bytesLen,
                                       NS_ASSIGNMENT_ADOPT);
        else
            rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}



class nsJSChannel : public nsIChannel,
                    public nsIStreamListener,
                    public nsIScriptChannel,
                    public nsIPropertyBag2
{
public:
    nsJSChannel();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISCRIPTCHANNEL
    NS_FORWARD_SAFE_NSIPROPERTYBAG(mPropertyBag)
    NS_FORWARD_SAFE_NSIPROPERTYBAG2(mPropertyBag)

    nsresult Init(nsIURI *aURI);

    
    void EvaluateScript();
    
protected:
    virtual ~nsJSChannel();

    nsresult StopAll();

    void NotifyListener();

    void CleanupStrongRefs();
    
protected:
    nsCOMPtr<nsIChannel>    mStreamChannel;
    nsCOMPtr<nsIPropertyBag2> mPropertyBag;
    nsCOMPtr<nsIStreamListener> mListener;  
    nsCOMPtr<nsISupports> mContext; 
    nsCOMPtr<nsPIDOMWindow> mOriginalInnerWindow;  
                                                   
    
    
    nsCOMPtr<nsIDocument>   mDocumentOnloadBlockedOn;

    nsresult mStatus; 

    nsLoadFlags             mLoadFlags;
    nsLoadFlags             mActualLoadFlags; 

    nsRefPtr<nsJSThunk>     mIOThunk;
    PopupControlState       mPopupState;
    uint32_t                mExecutionPolicy;
    bool                    mIsAsync;
    bool                    mIsActive;
    bool                    mOpenedStreamChannel;
};

nsJSChannel::nsJSChannel() :
    mStatus(NS_OK),
    mLoadFlags(LOAD_NORMAL),
    mActualLoadFlags(LOAD_NORMAL),
    mPopupState(openOverridden),
    mExecutionPolicy(NO_EXECUTION),
    mIsAsync(true),
    mIsActive(false),
    mOpenedStreamChannel(false)
{
}

nsJSChannel::~nsJSChannel()
{
}

nsresult nsJSChannel::StopAll()
{
    nsresult rv = NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIWebNavigation> webNav;
    NS_QueryNotificationCallbacks(mStreamChannel, webNav);

    NS_ASSERTION(webNav, "Can't get nsIWebNavigation from channel!");
    if (webNav) {
        rv = webNav->Stop(nsIWebNavigation::STOP_ALL);
    }

    return rv;
}

nsresult nsJSChannel::Init(nsIURI *aURI)
{
    nsRefPtr<nsJSURI> jsURI;
    nsresult rv = aURI->QueryInterface(kJSURICID,
                                       getter_AddRefs(jsURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    mIOThunk = new nsJSThunk();
    if (!mIOThunk)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    nsCOMPtr<nsIChannel> channel;

    nsCOMPtr<nsIPrincipal> nullPrincipal = nsNullPrincipal::Create();
    NS_ENSURE_TRUE(nullPrincipal, NS_ERROR_FAILURE);

    
    
    rv = NS_NewInputStreamChannel(getter_AddRefs(channel),
                                  aURI,
                                  mIOThunk,
                                  nullPrincipal,
                                  nsILoadInfo::SEC_NORMAL,
                                  nsIContentPolicy::TYPE_OTHER,
                                  NS_LITERAL_CSTRING("text/html"));
    if (NS_FAILED(rv)) return rv;

    rv = mIOThunk->Init(aURI);
    if (NS_SUCCEEDED(rv)) {
        mStreamChannel = channel;
        mPropertyBag = do_QueryInterface(channel);
        nsCOMPtr<nsIWritablePropertyBag2> writableBag =
            do_QueryInterface(channel);
        if (writableBag && jsURI->GetBaseURI()) {
            writableBag->SetPropertyAsInterface(NS_LITERAL_STRING("baseURI"),
                                                jsURI->GetBaseURI());
        }
    }

    return rv;
}





NS_IMPL_ISUPPORTS(nsJSChannel, nsIChannel, nsIRequest, nsIRequestObserver,
                  nsIStreamListener, nsIScriptChannel, nsIPropertyBag,
                  nsIPropertyBag2)





NS_IMETHODIMP
nsJSChannel::GetName(nsACString &aResult)
{
    return mStreamChannel->GetName(aResult);
}

NS_IMETHODIMP
nsJSChannel::IsPending(bool *aResult)
{
    *aResult = mIsActive;
    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::GetStatus(nsresult *aResult)
{
    if (NS_SUCCEEDED(mStatus) && mOpenedStreamChannel) {
        return mStreamChannel->GetStatus(aResult);
    }
    
    *aResult = mStatus;
        
    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::Cancel(nsresult aStatus)
{
    mStatus = aStatus;

    if (mOpenedStreamChannel) {
        mStreamChannel->Cancel(aStatus);
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::Suspend()
{
    return mStreamChannel->Suspend();
}

NS_IMETHODIMP
nsJSChannel::Resume()
{
    return mStreamChannel->Resume();
}





NS_IMETHODIMP
nsJSChannel::GetOriginalURI(nsIURI * *aURI)
{
    return mStreamChannel->GetOriginalURI(aURI);
}

NS_IMETHODIMP
nsJSChannel::SetOriginalURI(nsIURI *aURI)
{
    return mStreamChannel->SetOriginalURI(aURI);
}

NS_IMETHODIMP
nsJSChannel::GetURI(nsIURI * *aURI)
{
    return mStreamChannel->GetURI(aURI);
}

NS_IMETHODIMP
nsJSChannel::Open(nsIInputStream **aResult)
{
    nsresult rv = mIOThunk->EvaluateScript(mStreamChannel, mPopupState,
                                           mExecutionPolicy,
                                           mOriginalInnerWindow);
    NS_ENSURE_SUCCESS(rv, rv);

    return mStreamChannel->Open(aResult);
}

NS_IMETHODIMP
nsJSChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
    NS_ENSURE_ARG(aListener);

    
    
    nsIScriptGlobalObject* global = GetGlobalObject(this);
    if (!global) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(global));
    NS_ASSERTION(win, "Our global is not a window??");

    
    
    mOriginalInnerWindow = win->EnsureInnerWindow();
    if (!mOriginalInnerWindow) {
        return NS_ERROR_NOT_AVAILABLE;
    }
    
    mListener = aListener;
    mContext = aContext;

    mIsActive = true;

    
    
    
    
    
    mActualLoadFlags = mLoadFlags;
    mLoadFlags |= LOAD_BACKGROUND;

    
    
    nsCOMPtr<nsILoadGroup> loadGroup;
    mStreamChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (loadGroup) {
        nsresult rv = loadGroup->AddRequest(this, nullptr);
        if (NS_FAILED(rv)) {
            mIsActive = false;
            CleanupStrongRefs();
            return rv;
        }
    }

    mDocumentOnloadBlockedOn = mOriginalInnerWindow->GetExtantDoc();
    if (mDocumentOnloadBlockedOn) {
        
        
        
        
        
        nsLoadFlags loadFlags;
        mStreamChannel->GetLoadFlags(&loadFlags);
        if (loadFlags & LOAD_DOCUMENT_URI) {
            mDocumentOnloadBlockedOn =
                mDocumentOnloadBlockedOn->GetParentDocument();
        }
    }
    if (mDocumentOnloadBlockedOn) {
        mDocumentOnloadBlockedOn->BlockOnload();
    }


    mPopupState = win->GetPopupControlState();

    void (nsJSChannel::*method)();
    if (mIsAsync) {
        
        method = &nsJSChannel::EvaluateScript;
    } else {
        EvaluateScript();
        if (mOpenedStreamChannel) {
            
            return NS_OK;
        }

        NS_ASSERTION(NS_FAILED(mStatus), "We should have failed _somehow_");

        
        
        
        
        if (mStatus != NS_ERROR_DOM_RETVAL_UNDEFINED &&
            mStatus != NS_BINDING_ABORTED) {
            
            
            CleanupStrongRefs();
            return mStatus;
        }

        
        
        
        method = &nsJSChannel::NotifyListener;            
    }

    nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(this, method);
    nsresult rv = NS_DispatchToCurrentThread(ev);

    if (NS_FAILED(rv)) {
        loadGroup->RemoveRequest(this, nullptr, rv);
        mIsActive = false;
        CleanupStrongRefs();
    }
    return rv;
}

void
nsJSChannel::EvaluateScript()
{
    
    
    
    

    
    
    
    
    
    if (NS_SUCCEEDED(mStatus)) {
        nsresult rv = mIOThunk->EvaluateScript(mStreamChannel, mPopupState,
                                               mExecutionPolicy,
                                               mOriginalInnerWindow);

        
        if (NS_FAILED(rv) && NS_SUCCEEDED(mStatus)) {
            mStatus = rv;
        }
    }
    
    
    nsCOMPtr<nsILoadGroup> loadGroup;
    mStreamChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (loadGroup) {
        loadGroup->RemoveRequest(this, nullptr, mStatus);
    }

    
    mLoadFlags = mActualLoadFlags;

    
    
    mIsActive = false;

    if (NS_FAILED(mStatus)) {
        if (mIsAsync) {
            NotifyListener();
        }
        return;
    }
    
    
    
    

    
    nsLoadFlags loadFlags;
    mStreamChannel->GetLoadFlags(&loadFlags);

    uint32_t disposition;
    if (NS_FAILED(mStreamChannel->GetContentDisposition(&disposition)))
        disposition = nsIChannel::DISPOSITION_INLINE;
    if (loadFlags & LOAD_DOCUMENT_URI && disposition != nsIChannel::DISPOSITION_ATTACHMENT) {
        
        
        

        nsCOMPtr<nsIDocShell> docShell;
        NS_QueryNotificationCallbacks(mStreamChannel, docShell);
        if (docShell) {
            nsCOMPtr<nsIContentViewer> cv;
            docShell->GetContentViewer(getter_AddRefs(cv));

            if (cv) {
                bool okToUnload;

                if (NS_SUCCEEDED(cv->PermitUnload(false, &okToUnload)) &&
                    !okToUnload) {
                    
                    
                    
                    mStatus = NS_ERROR_DOM_RETVAL_UNDEFINED;
                }
            }
        }

        if (NS_SUCCEEDED(mStatus)) {
            mStatus = StopAll();
        }
    }

    if (NS_FAILED(mStatus)) {
        if (mIsAsync) {
            NotifyListener();
        }
        return;
    }
    
    mStatus = mStreamChannel->AsyncOpen(this, mContext);
    if (NS_SUCCEEDED(mStatus)) {
        
        
        mOpenedStreamChannel = true;

        
        
        mIsActive = true;
        if (loadGroup) {
            mStatus = loadGroup->AddRequest(this, nullptr);

            
            
            
            
        }
        
    } else if (mIsAsync) {
        NotifyListener();
    }

    return;
}

void
nsJSChannel::NotifyListener()
{
    mListener->OnStartRequest(this, mContext);
    mListener->OnStopRequest(this, mContext, mStatus);

    CleanupStrongRefs();
}

void
nsJSChannel::CleanupStrongRefs()
{
    mListener = nullptr;
    mContext = nullptr;
    mOriginalInnerWindow = nullptr;
    if (mDocumentOnloadBlockedOn) {
        mDocumentOnloadBlockedOn->UnblockOnload(false);
        mDocumentOnloadBlockedOn = nullptr;
    }
}

NS_IMETHODIMP
nsJSChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;

    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    
    
    bool bogusLoadBackground = false;
    if (mIsActive && !(mActualLoadFlags & LOAD_BACKGROUND) &&
        (aLoadFlags & LOAD_BACKGROUND)) {
        
        
        
        bool loadGroupIsBackground = false;
        nsCOMPtr<nsILoadGroup> loadGroup;
        mStreamChannel->GetLoadGroup(getter_AddRefs(loadGroup));
        if (loadGroup) {
            nsLoadFlags loadGroupFlags;
            loadGroup->GetLoadFlags(&loadGroupFlags);
            loadGroupIsBackground = ((loadGroupFlags & LOAD_BACKGROUND) != 0);
        }
        bogusLoadBackground = !loadGroupIsBackground;
    }

    
    
    
    aLoadFlags &= ~LOAD_CLASSIFY_URI;

    
    
    
    
    

    
    
    
    mLoadFlags = aLoadFlags & ~LOAD_DOCUMENT_URI;

    if (bogusLoadBackground) {
        aLoadFlags = aLoadFlags & ~LOAD_BACKGROUND;
    }

    mActualLoadFlags = aLoadFlags;

    
    
    

    return mStreamChannel->SetLoadFlags(aLoadFlags);
}

NS_IMETHODIMP
nsJSChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    return mStreamChannel->GetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsJSChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    if (aLoadGroup) {
        bool streamPending;
        nsresult rv = mStreamChannel->IsPending(&streamPending);
        NS_ENSURE_SUCCESS(rv, rv);

        if (streamPending) {
            nsCOMPtr<nsILoadGroup> curLoadGroup;
            mStreamChannel->GetLoadGroup(getter_AddRefs(curLoadGroup));

            if (aLoadGroup != curLoadGroup) {
                
                
                
                aLoadGroup->AddRequest(mStreamChannel, nullptr);
                if (curLoadGroup) {
                    curLoadGroup->RemoveRequest(mStreamChannel, nullptr,
                                                NS_BINDING_RETARGETED);
                }
            }
        }
    }
    
    return mStreamChannel->SetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsJSChannel::GetOwner(nsISupports* *aOwner)
{
    return mStreamChannel->GetOwner(aOwner);
}

NS_IMETHODIMP
nsJSChannel::SetOwner(nsISupports* aOwner)
{
    return mStreamChannel->SetOwner(aOwner);
}

NS_IMETHODIMP
nsJSChannel::GetLoadInfo(nsILoadInfo* *aLoadInfo)
{
    return mStreamChannel->GetLoadInfo(aLoadInfo);
}

NS_IMETHODIMP
nsJSChannel::SetLoadInfo(nsILoadInfo* aLoadInfo)
{
    return mStreamChannel->SetLoadInfo(aLoadInfo);
}

NS_IMETHODIMP
nsJSChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aCallbacks)
{
    return mStreamChannel->GetNotificationCallbacks(aCallbacks);
}

NS_IMETHODIMP
nsJSChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
    return mStreamChannel->SetNotificationCallbacks(aCallbacks);
}

NS_IMETHODIMP 
nsJSChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    return mStreamChannel->GetSecurityInfo(aSecurityInfo);
}

NS_IMETHODIMP
nsJSChannel::GetContentType(nsACString &aContentType)
{
    return mStreamChannel->GetContentType(aContentType);
}

NS_IMETHODIMP
nsJSChannel::SetContentType(const nsACString &aContentType)
{
    return mStreamChannel->SetContentType(aContentType);
}

NS_IMETHODIMP
nsJSChannel::GetContentCharset(nsACString &aContentCharset)
{
    return mStreamChannel->GetContentCharset(aContentCharset);
}

NS_IMETHODIMP
nsJSChannel::SetContentCharset(const nsACString &aContentCharset)
{
    return mStreamChannel->SetContentCharset(aContentCharset);
}

NS_IMETHODIMP
nsJSChannel::GetContentDisposition(uint32_t *aContentDisposition)
{
    return mStreamChannel->GetContentDisposition(aContentDisposition);
}

NS_IMETHODIMP
nsJSChannel::SetContentDisposition(uint32_t aContentDisposition)
{
    return mStreamChannel->SetContentDisposition(aContentDisposition);
}

NS_IMETHODIMP
nsJSChannel::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
    return mStreamChannel->GetContentDispositionFilename(aContentDispositionFilename);
}

NS_IMETHODIMP
nsJSChannel::SetContentDispositionFilename(const nsAString &aContentDispositionFilename)
{
    return mStreamChannel->SetContentDispositionFilename(aContentDispositionFilename);
}

NS_IMETHODIMP
nsJSChannel::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
    return mStreamChannel->GetContentDispositionHeader(aContentDispositionHeader);
}

NS_IMETHODIMP
nsJSChannel::GetContentLength(int64_t *aContentLength)
{
    return mStreamChannel->GetContentLength(aContentLength);
}

NS_IMETHODIMP
nsJSChannel::SetContentLength(int64_t aContentLength)
{
    return mStreamChannel->SetContentLength(aContentLength);
}

NS_IMETHODIMP
nsJSChannel::OnStartRequest(nsIRequest* aRequest,
                            nsISupports* aContext)
{
    NS_ENSURE_TRUE(aRequest == mStreamChannel, NS_ERROR_UNEXPECTED);

    return mListener->OnStartRequest(this, aContext);    
}

NS_IMETHODIMP
nsJSChannel::OnDataAvailable(nsIRequest* aRequest,
                             nsISupports* aContext, 
                             nsIInputStream* aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount)
{
    NS_ENSURE_TRUE(aRequest == mStreamChannel, NS_ERROR_UNEXPECTED);

    return mListener->OnDataAvailable(this, aContext, aInputStream, aOffset,
                                      aCount);
}

NS_IMETHODIMP
nsJSChannel::OnStopRequest(nsIRequest* aRequest,
                           nsISupports* aContext,
                           nsresult aStatus)
{
    NS_ENSURE_TRUE(aRequest == mStreamChannel, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIStreamListener> listener = mListener;

    CleanupStrongRefs();

    
    if (NS_FAILED(mStatus)) {
        aStatus = mStatus;
    }
    
    nsresult rv = listener->OnStopRequest(this, aContext, aStatus);

    nsCOMPtr<nsILoadGroup> loadGroup;
    mStreamChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (loadGroup) {
        loadGroup->RemoveRequest(this, nullptr, mStatus);
    }

    mIsActive = false;

    return rv;
}

NS_IMETHODIMP
nsJSChannel::SetExecutionPolicy(uint32_t aPolicy)
{
    NS_ENSURE_ARG(aPolicy <= EXECUTE_NORMAL);
    
    mExecutionPolicy = aPolicy;
    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::GetExecutionPolicy(uint32_t* aPolicy)
{
    *aPolicy = mExecutionPolicy;
    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::SetExecuteAsync(bool aIsAsync)
{
    if (!mIsActive) {
        mIsAsync = aIsAsync;
    }
    
    NS_WARN_IF_FALSE(!mIsActive, "Calling SetExecuteAsync on active channel?");

    return NS_OK;
}

NS_IMETHODIMP
nsJSChannel::GetExecuteAsync(bool* aIsAsync)
{
    *aIsAsync = mIsAsync;
    return NS_OK;
}



nsJSProtocolHandler::nsJSProtocolHandler()
{
}

nsresult
nsJSProtocolHandler::Init()
{
    return NS_OK;
}

nsJSProtocolHandler::~nsJSProtocolHandler()
{
}

NS_IMPL_ISUPPORTS(nsJSProtocolHandler, nsIProtocolHandler)

nsresult
nsJSProtocolHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsJSProtocolHandler* ph = new nsJSProtocolHandler();
    if (!ph)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = ph->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(ph);
    return rv;
}

nsresult 
nsJSProtocolHandler::EnsureUTF8Spec(const nsAFlatCString &aSpec, const char *aCharset, 
                                    nsACString &aUTF8Spec)
{
  aUTF8Spec.Truncate();

  nsresult rv;
  
  if (!mTextToSubURI) {
    mTextToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsAutoString uStr;
  rv = mTextToSubURI->UnEscapeNonAsciiURI(nsDependentCString(aCharset), aSpec, uStr);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!IsASCII(uStr))
    NS_EscapeURL(NS_ConvertUTF16toUTF8(uStr), esc_AlwaysCopy | esc_OnlyNonASCII, aUTF8Spec);

  return NS_OK;
}




NS_IMETHODIMP
nsJSProtocolHandler::GetScheme(nsACString &result)
{
    result = "javascript";
    return NS_OK;
}

NS_IMETHODIMP
nsJSProtocolHandler::GetDefaultPort(int32_t *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsJSProtocolHandler::GetProtocolFlags(uint32_t *result)
{
    *result = URI_NORELATIVE | URI_NOAUTH | URI_INHERITS_SECURITY_CONTEXT |
        URI_LOADABLE_BY_ANYONE | URI_NON_PERSISTABLE | URI_OPENING_EXECUTES_SCRIPT;
    return NS_OK;
}

NS_IMETHODIMP
nsJSProtocolHandler::NewURI(const nsACString &aSpec,
                            const char *aCharset,
                            nsIURI *aBaseURI,
                            nsIURI **result)
{
    nsresult rv;

    
    
    

    nsCOMPtr<nsIURI> url = new nsJSURI(aBaseURI);

    if (!aCharset || !nsCRT::strcasecmp("UTF-8", aCharset))
      rv = url->SetSpec(aSpec);
    else {
      nsAutoCString utf8Spec;
      rv = EnsureUTF8Spec(PromiseFlatCString(aSpec), aCharset, utf8Spec);
      if (NS_SUCCEEDED(rv)) {
        if (utf8Spec.IsEmpty())
          rv = url->SetSpec(aSpec);
        else
          rv = url->SetSpec(utf8Spec);
      }
    }

    if (NS_FAILED(rv)) {
        return rv;
    }

    url.forget(result);
    return rv;
}

NS_IMETHODIMP
nsJSProtocolHandler::NewChannel2(nsIURI* uri,
                                 nsILoadInfo* aLoadInfo,
                                 nsIChannel** result)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(uri);

    nsRefPtr<nsJSChannel> channel = new nsJSChannel();
    if (!channel) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = channel->Init(uri);

    
    rv = channel->SetLoadInfo(aLoadInfo);
    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_SUCCEEDED(rv)) {
        channel.forget(result);
    }
    return rv;
}

NS_IMETHODIMP
nsJSProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    return NewChannel2(uri, nullptr, result);
}

NS_IMETHODIMP 
nsJSProtocolHandler::AllowPort(int32_t port, const char *scheme, bool *_retval)
{
    
    *_retval = false;
    return NS_OK;
}



static NS_DEFINE_CID(kThisSimpleURIImplementationCID,
                     NS_THIS_SIMPLEURI_IMPLEMENTATION_CID);


NS_IMPL_ADDREF_INHERITED(nsJSURI, nsSimpleURI)
NS_IMPL_RELEASE_INHERITED(nsJSURI, nsSimpleURI)

NS_INTERFACE_MAP_BEGIN(nsJSURI)
  if (aIID.Equals(kJSURICID))
      foundInterface = static_cast<nsIURI*>(this);
  else if (aIID.Equals(kThisSimpleURIImplementationCID)) {
      
      
      
      *aInstancePtr = nullptr;
      return NS_NOINTERFACE;
  }
  else
NS_INTERFACE_MAP_END_INHERITING(nsSimpleURI)



NS_IMETHODIMP
nsJSURI::Read(nsIObjectInputStream* aStream)
{
    nsresult rv = nsSimpleURI::Read(aStream);
    if (NS_FAILED(rv)) return rv;

    bool haveBase;
    rv = aStream->ReadBoolean(&haveBase);
    if (NS_FAILED(rv)) return rv;

    if (haveBase) {
        nsCOMPtr<nsISupports> supports;
        rv = aStream->ReadObject(true, getter_AddRefs(supports));
        if (NS_FAILED(rv)) return rv;
        mBaseURI = do_QueryInterface(supports);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsJSURI::Write(nsIObjectOutputStream* aStream)
{
    nsresult rv = nsSimpleURI::Write(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->WriteBoolean(mBaseURI != nullptr);
    if (NS_FAILED(rv)) return rv;

    if (mBaseURI) {
        rv = aStream->WriteObject(mBaseURI, true);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


void
nsJSURI::Serialize(mozilla::ipc::URIParams& aParams)
{
    using namespace mozilla::ipc;

    JSURIParams jsParams;
    URIParams simpleParams;

    nsSimpleURI::Serialize(simpleParams);

    jsParams.simpleParams() = simpleParams;
    if (mBaseURI) {
        SerializeURI(mBaseURI, jsParams.baseURI());
    } else {
        jsParams.baseURI() = mozilla::void_t();
    }

    aParams = jsParams;
}

bool
nsJSURI::Deserialize(const mozilla::ipc::URIParams& aParams)
{
    using namespace mozilla::ipc;

    if (aParams.type() != URIParams::TJSURIParams) {
        NS_ERROR("Received unknown parameters from the other process!");
        return false;
    }

    const JSURIParams& jsParams = aParams.get_JSURIParams();
    nsSimpleURI::Deserialize(jsParams.simpleParams());

    if (jsParams.baseURI().type() != OptionalURIParams::Tvoid_t) {
        mBaseURI = DeserializeURI(jsParams.baseURI().get_URIParams());
    } else {
        mBaseURI = nullptr;
    }
    return true;
}


 nsSimpleURI*
nsJSURI::StartClone(nsSimpleURI::RefHandlingEnum )
{
    nsCOMPtr<nsIURI> baseClone;
    if (mBaseURI) {
      
      nsresult rv = mBaseURI->Clone(getter_AddRefs(baseClone));
      if (NS_FAILED(rv)) {
        return nullptr;
      }
    }

    return new nsJSURI(baseClone);
}

 nsresult
nsJSURI::EqualsInternal(nsIURI* aOther,
                        nsSimpleURI::RefHandlingEnum aRefHandlingMode,
                        bool* aResult)
{
    NS_ENSURE_ARG_POINTER(aOther);
    NS_PRECONDITION(aResult, "null pointer for outparam");

    nsRefPtr<nsJSURI> otherJSURI;
    nsresult rv = aOther->QueryInterface(kJSURICID,
                                         getter_AddRefs(otherJSURI));
    if (NS_FAILED(rv)) {
        *aResult = false; 
        return NS_OK;
    }

    
    if (!nsSimpleURI::EqualsInternal(otherJSURI, aRefHandlingMode)) {
        *aResult = false;
        return NS_OK;
    }

    
    nsIURI* otherBaseURI = otherJSURI->GetBaseURI();

    if (mBaseURI) {
        
        return mBaseURI->Equals(otherBaseURI, aResult);
    }

    *aResult = !otherBaseURI;
    return NS_OK;
}

NS_IMETHODIMP 
nsJSURI::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kJSURICID;
    return NS_OK;
}

