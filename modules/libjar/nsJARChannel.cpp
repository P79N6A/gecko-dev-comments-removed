






































#include "nsJAR.h"
#include "nsJARChannel.h"
#include "nsJARProtocolHandler.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsInt64.h"
#include "nsEscape.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIViewSourceChannel.h"
#include "nsChannelProperties.h"

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIFileURL.h"
#include "nsIJAR.h"

static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);



#define ENTRY_IS_DIRECTORY(_entry) \
  ((_entry).IsEmpty() || '/' == (_entry).Last())



#if defined(PR_LOGGING)



static PRLogModuleInfo *gJarProtocolLog = nsnull;
#endif

#define LOG(args)     PR_LOG(gJarProtocolLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gJarProtocolLog, 4)







class nsJARInputThunk : public nsIInputStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

    nsJARInputThunk(nsIFile *jarFile,
                    nsIURI* fullJarURI,
                    const nsACString &jarEntry,
                    nsIZipReaderCache *jarCache)
        : mJarCache(jarCache)
        , mJarFile(jarFile)
        , mJarEntry(jarEntry)
        , mContentLength(-1)
    {
        NS_ASSERTION(mJarFile, "no jar file");

        if (fullJarURI) {
            nsresult rv = fullJarURI->GetAsciiSpec(mJarDirSpec);
            NS_ASSERTION(NS_SUCCEEDED(rv), "this shouldn't fail");
        }
    }

    virtual ~nsJARInputThunk()
    {
        if (!mJarCache && mJarReader)
            mJarReader->Close();
    }

    void GetJarReader(nsIZipReader **result)
    {
        NS_IF_ADDREF(*result = mJarReader);
    }

    PRInt32 GetContentLength()
    {
        return mContentLength;
    }

    nsresult EnsureJarStream();

private:

    nsCOMPtr<nsIZipReaderCache> mJarCache;
    nsCOMPtr<nsIZipReader>      mJarReader;
    nsCOMPtr<nsIFile>           mJarFile;
    nsCString                   mJarDirSpec;
    nsCOMPtr<nsIInputStream>    mJarStream;
    nsCString                   mJarEntry;
    PRInt32                     mContentLength;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsJARInputThunk, nsIInputStream)

nsresult
nsJARInputThunk::EnsureJarStream()
{
    if (mJarStream)
        return NS_OK;

    nsresult rv;
    if (mJarCache)
        rv = mJarCache->GetZip(mJarFile, getter_AddRefs(mJarReader));
    else {
        
        mJarReader = do_CreateInstance(kZipReaderCID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = mJarReader->Open(mJarFile);
    }
    if (NS_FAILED(rv)) return rv;

    if (ENTRY_IS_DIRECTORY(mJarEntry)) {
        
        

        NS_ENSURE_STATE(!mJarDirSpec.IsEmpty());

        rv = mJarReader->GetInputStreamWithSpec(mJarDirSpec,
                                                mJarEntry.get(),
                                                getter_AddRefs(mJarStream));
    }
    else {
        rv = mJarReader->GetInputStream(mJarEntry.get(),
                                        getter_AddRefs(mJarStream));
    }
    if (NS_FAILED(rv)) {
        
        
        if (rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
            rv = NS_ERROR_FILE_NOT_FOUND;
        return rv;
    }

    
    mJarStream->Available((PRUint32 *) &mContentLength);

    return NS_OK;
}

NS_IMETHODIMP
nsJARInputThunk::Close()
{
    if (mJarStream)
        return mJarStream->Close();

    return NS_OK;
}

NS_IMETHODIMP
nsJARInputThunk::Available(PRUint32 *avail)
{
    nsresult rv = EnsureJarStream();
    if (NS_FAILED(rv)) return rv;

    return mJarStream->Available(avail);
}

NS_IMETHODIMP
nsJARInputThunk::Read(char *buf, PRUint32 count, PRUint32 *countRead)
{
    nsresult rv = EnsureJarStream();
    if (NS_FAILED(rv)) return rv;

    return mJarStream->Read(buf, count, countRead);
}

NS_IMETHODIMP
nsJARInputThunk::ReadSegments(nsWriteSegmentFun writer, void *closure,
                              PRUint32 count, PRUint32 *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsJARInputThunk::IsNonBlocking(PRBool *nonBlocking)
{
    *nonBlocking = PR_FALSE;
    return NS_OK;
}






nsJARChannel::nsJARChannel()
    : mContentLength(-1)
    , mLoadFlags(LOAD_NORMAL)
    , mStatus(NS_OK)
    , mIsPending(PR_FALSE)
    , mIsUnsafe(PR_TRUE)
    , mJarInput(nsnull)
{
#if defined(PR_LOGGING)
    if (!gJarProtocolLog)
        gJarProtocolLog = PR_NewLogModule("nsJarProtocol");
#endif

    
    NS_ADDREF(gJarHandler);
}

nsJARChannel::~nsJARChannel()
{
    
    NS_IF_RELEASE(mJarInput);

    
    nsJARProtocolHandler *handler = gJarHandler;
    NS_RELEASE(handler); 
}

NS_IMPL_ISUPPORTS_INHERITED6(nsJARChannel,
                             nsHashPropertyBag,
                             nsIRequest,
                             nsIChannel,
                             nsIStreamListener,
                             nsIRequestObserver,
                             nsIDownloadObserver,
                             nsIJARChannel)

nsresult 
nsJARChannel::Init(nsIURI *uri)
{
    nsresult rv;
    rv = nsHashPropertyBag::Init();
    if (NS_FAILED(rv))
        return rv;

    mJarURI = do_QueryInterface(uri, &rv);
    if (NS_FAILED(rv))
        return rv;

    mOriginalURI = mJarURI;

    
    nsCOMPtr<nsIURI> innerURI;
    rv = mJarURI->GetJARFile(getter_AddRefs(innerURI));
    if (NS_FAILED(rv))
        return rv;
    PRBool isJS;
    rv = innerURI->SchemeIs("javascript", &isJS);
    if (NS_FAILED(rv))
        return rv;
    if (isJS) {
        NS_WARNING("blocking jar:javascript:");
        return NS_ERROR_INVALID_ARG;
    }

#if defined(PR_LOGGING)
    mJarURI->GetSpec(mSpec);
#endif
    return rv;
}

nsresult
nsJARChannel::CreateJarInput(nsIZipReaderCache *jarCache)
{
    
    
    nsCOMPtr<nsIFile> clonedFile;
    nsresult rv = mJarFile->Clone(getter_AddRefs(clonedFile));
    if (NS_FAILED(rv)) return rv;

    mJarInput = new nsJARInputThunk(clonedFile, mJarURI, mJarEntry, jarCache);
    if (!mJarInput)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mJarInput);
    return NS_OK;
}

nsresult
nsJARChannel::EnsureJarInput(PRBool blocking)
{
    LOG(("nsJARChannel::EnsureJarInput [this=%x %s]\n", this, mSpec.get()));

    nsresult rv;
    nsCOMPtr<nsIURI> uri;

    rv = mJarURI->GetJARFile(getter_AddRefs(mJarBaseURI));
    if (NS_FAILED(rv)) return rv;

    rv = mJarURI->GetJAREntry(mJarEntry);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    NS_UnescapeURL(mJarEntry);

    
    {
        nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mJarBaseURI);
        if (fileURL)
            fileURL->GetFile(getter_AddRefs(mJarFile));
    }

    if (mJarFile) {
        mIsUnsafe = PR_FALSE;

        
        
        rv = CreateJarInput(gJarHandler->JarCache());
    }
    else if (blocking) {
        NS_NOTREACHED("need sync downloader");
        rv = NS_ERROR_NOT_IMPLEMENTED;
    }
    else {
        
        rv = NS_NewDownloader(getter_AddRefs(mDownloader), this);
        if (NS_SUCCEEDED(rv))
            rv = NS_OpenURI(mDownloader, nsnull, mJarBaseURI, nsnull,
                            mLoadGroup, mCallbacks,
                            mLoadFlags & ~(LOAD_DOCUMENT_URI | LOAD_CALL_CONTENT_SNIFFERS));
    }
    return rv;

}





NS_IMETHODIMP
nsJARChannel::GetName(nsACString &result)
{
    return mJarURI->GetSpec(result);
}

NS_IMETHODIMP
nsJARChannel::IsPending(PRBool *result)
{
    *result = mIsPending;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetStatus(nsresult *status)
{
    if (mPump && NS_SUCCEEDED(mStatus))
        mPump->GetStatus(status);
    else
        *status = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::Cancel(nsresult status)
{
    mStatus = status;
    if (mPump)
        return mPump->Cancel(status);

    NS_ASSERTION(!mIsPending, "need to implement cancel when downloading");
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::Suspend()
{
    if (mPump)
        return mPump->Suspend();

    NS_ASSERTION(!mIsPending, "need to implement suspend when downloading");
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::Resume()
{
    if (mPump)
        return mPump->Resume();

    NS_ASSERTION(!mIsPending, "need to implement resume when downloading");
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    NS_IF_ADDREF(*aLoadGroup = mLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}





NS_IMETHODIMP
nsJARChannel::GetOriginalURI(nsIURI **aURI)
{
    *aURI = mOriginalURI;
    NS_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetOriginalURI(nsIURI *aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);
    mOriginalURI = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetURI(nsIURI **aURI)
{
    NS_IF_ADDREF(*aURI = mJarURI);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetOwner(nsISupports **result)
{
    nsresult rv;

    if (mOwner) {
        NS_ADDREF(*result = mOwner);
        return NS_OK;
    }

    if (!mJarInput) {
        *result = nsnull;
        return NS_OK;
    }

    
    nsCOMPtr<nsIZipReader> jarReader;
    mJarInput->GetJarReader(getter_AddRefs(jarReader));
    if (!jarReader)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsIJAR> jar = do_QueryInterface(jarReader, &rv);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsIJAR not supported");
        return rv;
    }

    nsCOMPtr<nsIPrincipal> cert;
    rv = jar->GetCertificatePrincipal(mJarEntry.get(), getter_AddRefs(cert));
    if (NS_FAILED(rv)) return rv;

    if (cert) {
        nsCAutoString certFingerprint;
        rv = cert->GetFingerprint(certFingerprint);
        if (NS_FAILED(rv)) return rv;

        nsCAutoString subjectName;
        rv = cert->GetSubjectName(subjectName);
        if (NS_FAILED(rv)) return rv;

        nsCAutoString prettyName;
        rv = cert->GetPrettyName(prettyName);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsISupports> certificate;
        rv = cert->GetCertificate(getter_AddRefs(certificate));
        if (NS_FAILED(rv)) return rv;
        
        nsCOMPtr<nsIScriptSecurityManager> secMan = 
                 do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = secMan->GetCertificatePrincipal(certFingerprint, subjectName,
                                             prettyName, certificate,
                                             mJarBaseURI,
                                             getter_AddRefs(cert));
        if (NS_FAILED(rv)) return rv;

        mOwner = do_QueryInterface(cert, &rv);
        if (NS_FAILED(rv)) return rv;

        NS_ADDREF(*result = mOwner);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetOwner(nsISupports *aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
    NS_IF_ADDREF(*aCallbacks = mCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
    mCallbacks = aCallbacks;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARChannel::GetSecurityInfo(nsISupports **aSecurityInfo)
{
    NS_PRECONDITION(aSecurityInfo, "Null out param");
    NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetContentType(nsACString &result)
{
    if (mContentType.IsEmpty()) {
        
        
        
        const char *ext = nsnull, *fileName = mJarEntry.get();
        PRInt32 len = mJarEntry.Length();

        
        
        
        if (ENTRY_IS_DIRECTORY(mJarEntry)) {
            mContentType.AssignLiteral(APPLICATION_HTTP_INDEX_FORMAT);
        }
        else {
            
            for (PRInt32 i = len-1; i >= 0; i--) {
                if (fileName[i] == '.') {
                    ext = &fileName[i + 1];
                    break;
                }
            }
            if (ext) {
                nsIMIMEService *mimeServ = gJarHandler->MimeService();
                if (mimeServ)
                    mimeServ->GetTypeFromExtension(nsDependentCString(ext), mContentType);
            }
            if (mContentType.IsEmpty())
                mContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);
        }
    }
    result = mContentType;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetContentType(const nsACString &aContentType)
{
    
    

    
    NS_ParseContentType(aContentType, mContentType, mContentCharset);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetContentCharset(nsACString &aContentCharset)
{
    
    
    aContentCharset = mContentCharset;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetContentCharset(const nsACString &aContentCharset)
{
    mContentCharset = aContentCharset;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetContentLength(PRInt32 *result)
{
    
    if (mContentLength < 0 && mJarInput)
        mContentLength = mJarInput->GetContentLength();

    *result = mContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetContentLength(PRInt32 aContentLength)
{
    
    mContentLength = aContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::Open(nsIInputStream **stream)
{
    LOG(("nsJARChannel::Open [this=%x]\n", this));

    NS_ENSURE_TRUE(!mJarInput, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    mJarFile = nsnull;
    mIsUnsafe = PR_TRUE;

    nsresult rv = EnsureJarInput(PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    if (!mJarInput)
        return NS_ERROR_UNEXPECTED;

    
    
    mJarInput->EnsureJarStream();

    NS_ADDREF(*stream = mJarInput);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctx)
{
    LOG(("nsJARChannel::AsyncOpen [this=%x]\n", this));

    NS_ENSURE_ARG_POINTER(listener);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    mJarFile = nsnull;
    mIsUnsafe = PR_TRUE;

    
    NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup, mProgressSink);

    nsresult rv = EnsureJarInput(PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    
    
    mListener = listener;
    mListenerContext = ctx;
    mIsPending = PR_TRUE;
    if (mJarInput) {
        
        rv = NS_NewInputStreamPump(getter_AddRefs(mPump), mJarInput);
        if (NS_SUCCEEDED(rv))
            rv = mPump->AsyncRead(this, nsnull);

        
        
        if (NS_FAILED(rv)) {
            mIsPending = PR_FALSE;
            mListenerContext = nsnull;
            mListener = nsnull;
            return rv;
        }
    }

    if (mLoadGroup)
        mLoadGroup->AddRequest(this, nsnull);

    return NS_OK;
}




NS_IMETHODIMP
nsJARChannel::GetIsUnsafe(PRBool *isUnsafe)
{
    *isUnsafe = mIsUnsafe;
    return NS_OK;
}





NS_IMETHODIMP
nsJARChannel::OnDownloadComplete(nsIDownloader *downloader,
                                 nsIRequest    *request,
                                 nsISupports   *context,
                                 nsresult       status,
                                 nsIFile       *file)
{
    nsresult rv;

    nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
    if (channel) {
        PRUint32 loadFlags;
        channel->GetLoadFlags(&loadFlags);
        if (loadFlags & LOAD_REPLACE) {
            mLoadFlags |= LOAD_REPLACE;

            if (!mOriginalURI) {
                SetOriginalURI(mJarURI);
            }

            nsCOMPtr<nsIURI> innerURI;
            rv = channel->GetURI(getter_AddRefs(innerURI));
            if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsIJARURI> newURI;
                rv = mJarURI->CloneWithJARFile(innerURI,
                                               getter_AddRefs(newURI));
                if (NS_SUCCEEDED(rv)) {
                    mJarURI = newURI;
                }
            }
            if (NS_SUCCEEDED(status)) {
                status = rv;
            }
        }
    }

    if (NS_SUCCEEDED(status) && channel) {
        nsCAutoString header;
        
        channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

        nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
        if (httpChannel) {
            
            
            
            httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Type"),
                                           header);
            nsCAutoString contentType;
            nsCAutoString charset;
            NS_ParseContentType(header, contentType, charset);
            nsCAutoString channelContentType;
            channel->GetContentType(channelContentType);
            mIsUnsafe = !(contentType.Equals(channelContentType) &&
                          (contentType.EqualsLiteral("application/java-archive") ||
                           contentType.EqualsLiteral("application/x-jar")));
            rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Disposition"),
                                                header);
            if (NS_SUCCEEDED(rv))
                SetPropertyAsACString(NS_CHANNEL_PROP_CONTENT_DISPOSITION, header);
        } else {
            nsCOMPtr<nsIJARChannel> innerJARChannel(do_QueryInterface(channel));
            if (innerJARChannel) {
                PRBool unsafe;
                innerJARChannel->GetIsUnsafe(&unsafe);
                mIsUnsafe = unsafe;
            }
            
            rv = NS_GetContentDisposition(request, header);
            if (NS_SUCCEEDED(rv))
                SetPropertyAsACString(NS_CHANNEL_PROP_CONTENT_DISPOSITION, header);
        }
    }

    if (NS_SUCCEEDED(status) && mIsUnsafe) {
        PRBool allowUnpack = PR_FALSE;

        nsCOMPtr<nsIPrefBranch> prefs =
            do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
            prefs->GetBoolPref("network.jar.open-unsafe-types", &allowUnpack);
        }

        if (!allowUnpack) {
            status = NS_ERROR_UNSAFE_CONTENT_TYPE;
        }
    }

    if (NS_SUCCEEDED(status)) {
        
        nsCOMPtr<nsIViewSourceChannel> viewSource = do_QueryInterface(channel);
        if (viewSource) {
            status = NS_ERROR_UNSAFE_CONTENT_TYPE;
        }
    }

    if (NS_SUCCEEDED(status)) {
        mJarFile = file;
    
        rv = CreateJarInput(nsnull);
        if (NS_SUCCEEDED(rv)) {
            
            rv = NS_NewInputStreamPump(getter_AddRefs(mPump), mJarInput);
            if (NS_SUCCEEDED(rv))
                rv = mPump->AsyncRead(this, nsnull);
        }
        status = rv;
    }

    if (NS_FAILED(status)) {
        mStatus = status;
        OnStartRequest(nsnull, nsnull);
        OnStopRequest(nsnull, nsnull, status);
    }

    return NS_OK;
}





NS_IMETHODIMP
nsJARChannel::OnStartRequest(nsIRequest *req, nsISupports *ctx)
{
    LOG(("nsJARChannel::OnStartRequest [this=%x %s]\n", this, mSpec.get()));

    return mListener->OnStartRequest(this, mListenerContext);
}

NS_IMETHODIMP
nsJARChannel::OnStopRequest(nsIRequest *req, nsISupports *ctx, nsresult status)
{
    LOG(("nsJARChannel::OnStopRequest [this=%x %s status=%x]\n",
        this, mSpec.get(), status));

    if (NS_SUCCEEDED(mStatus))
        mStatus = status;

    if (mListener) {
        mListener->OnStopRequest(this, mListenerContext, status);
        mListener = 0;
        mListenerContext = 0;
    }

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    mPump = 0;
    NS_IF_RELEASE(mJarInput);
    mIsPending = PR_FALSE;
    mDownloader = 0; 

    
    mCallbacks = 0;
    mProgressSink = 0;

    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::OnDataAvailable(nsIRequest *req, nsISupports *ctx,
                               nsIInputStream *stream,
                               PRUint32 offset, PRUint32 count)
{
#if defined(PR_LOGGING)
    LOG(("nsJARChannel::OnDataAvailable [this=%x %s]\n", this, mSpec.get()));
#endif

    nsresult rv;

    rv = mListener->OnDataAvailable(this, mListenerContext, stream, offset, count);

    
    
    
    if (mProgressSink && NS_SUCCEEDED(rv) && !(mLoadFlags & LOAD_BACKGROUND))
        mProgressSink->OnProgress(this, nsnull, PRUint64(offset + count),
                                  PRUint64(mContentLength));

    return rv; 
}
