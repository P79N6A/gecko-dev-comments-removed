





#include "nsJAR.h"
#include "nsJARChannel.h"
#include "nsJARProtocolHandler.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIViewSourceChannel.h"
#include "nsChannelProperties.h"
#include "nsContentUtils.h"

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIFileURL.h"

#include "mozilla/Preferences.h"
#include "mozilla/net/RemoteOpenFileChild.h"
#include "nsITabChild.h"
#include "private/pprio.h"

using namespace mozilla;
using namespace mozilla::net;

static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);



#define ENTRY_IS_DIRECTORY(_entry) \
  ((_entry).IsEmpty() || '/' == (_entry).Last())





#ifdef LOG
#undef LOG
#endif

#if defined(PR_LOGGING)



static PRLogModuleInfo *gJarProtocolLog = nullptr;
#endif


#define LOG(args)     PR_LOG(gJarProtocolLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gJarProtocolLog, 4)







class nsJARInputThunk : public nsIInputStream
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

    nsJARInputThunk(nsIZipReader *zipReader,
                    nsIURI* fullJarURI,
                    const nsACString &jarEntry,
                    bool usingJarCache)
        : mUsingJarCache(usingJarCache)
        , mJarReader(zipReader)
        , mJarEntry(jarEntry)
        , mContentLength(-1)
    {
        if (fullJarURI) {
#ifdef DEBUG
            nsresult rv =
#endif
                fullJarURI->GetAsciiSpec(mJarDirSpec);
            NS_ASSERTION(NS_SUCCEEDED(rv), "this shouldn't fail");
        }
    }

    int64_t GetContentLength()
    {
        return mContentLength;
    }

    nsresult Init();

private:

    virtual ~nsJARInputThunk()
    {
        Close();
    }

    bool                        mUsingJarCache;
    nsCOMPtr<nsIZipReader>      mJarReader;
    nsCString                   mJarDirSpec;
    nsCOMPtr<nsIInputStream>    mJarStream;
    nsCString                   mJarEntry;
    int64_t                     mContentLength;
};

NS_IMPL_ISUPPORTS(nsJARInputThunk, nsIInputStream)

nsresult
nsJARInputThunk::Init()
{
    nsresult rv;
    if (ENTRY_IS_DIRECTORY(mJarEntry)) {
        
        

        NS_ENSURE_STATE(!mJarDirSpec.IsEmpty());

        rv = mJarReader->GetInputStreamWithSpec(mJarDirSpec,
                                                mJarEntry,
                                                getter_AddRefs(mJarStream));
    }
    else {
        rv = mJarReader->GetInputStream(mJarEntry,
                                        getter_AddRefs(mJarStream));
    }
    if (NS_FAILED(rv)) {
        
        
        if (rv == NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
            rv = NS_ERROR_FILE_NOT_FOUND;
        return rv;
    }

    
    uint64_t avail;
    rv = mJarStream->Available((uint64_t *) &avail);
    if (NS_FAILED(rv)) return rv;

    mContentLength = avail < INT64_MAX ? (int64_t) avail : -1;

    return NS_OK;
}

NS_IMETHODIMP
nsJARInputThunk::Close()
{
    nsresult rv = NS_OK;

    if (mJarStream)
        rv = mJarStream->Close();

    if (!mUsingJarCache && mJarReader)
        mJarReader->Close();

    mJarReader = nullptr;

    return rv;
}

NS_IMETHODIMP
nsJARInputThunk::Available(uint64_t *avail)
{
    return mJarStream->Available(avail);
}

NS_IMETHODIMP
nsJARInputThunk::Read(char *buf, uint32_t count, uint32_t *countRead)
{
    return mJarStream->Read(buf, count, countRead);
}

NS_IMETHODIMP
nsJARInputThunk::ReadSegments(nsWriteSegmentFun writer, void *closure,
                              uint32_t count, uint32_t *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsJARInputThunk::IsNonBlocking(bool *nonBlocking)
{
    *nonBlocking = false;
    return NS_OK;
}






nsJARChannel::nsJARChannel()
    : mOpened(false)
    , mAppURI(nullptr)
    , mContentLength(-1)
    , mLoadFlags(LOAD_NORMAL)
    , mStatus(NS_OK)
    , mIsPending(false)
    , mIsUnsafe(true)
    , mOpeningRemote(false)
    , mEnsureChildFd(false)
{
#if defined(PR_LOGGING)
    if (!gJarProtocolLog)
        gJarProtocolLog = PR_NewLogModule("nsJarProtocol");
#endif

    
    NS_ADDREF(gJarHandler);
}

nsJARChannel::~nsJARChannel()
{
    
    nsJARProtocolHandler *handler = gJarHandler;
    NS_RELEASE(handler); 
}

NS_IMPL_ISUPPORTS_INHERITED(nsJARChannel,
                            nsHashPropertyBag,
                            nsIRequest,
                            nsIChannel,
                            nsIStreamListener,
                            nsIRequestObserver,
                            nsIDownloadObserver,
                            nsIRemoteOpenFileListener,
                            nsIThreadRetargetableRequest,
                            nsIThreadRetargetableStreamListener,
                            nsIJARChannel)

nsresult 
nsJARChannel::Init(nsIURI *uri)
{
    nsresult rv;
    mJarURI = do_QueryInterface(uri, &rv);
    if (NS_FAILED(rv))
        return rv;

    mOriginalURI = mJarURI;

    
    nsCOMPtr<nsIURI> innerURI;
    rv = mJarURI->GetJARFile(getter_AddRefs(innerURI));
    if (NS_FAILED(rv))
        return rv;
    bool isJS;
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
nsJARChannel::CreateJarInput(nsIZipReaderCache *jarCache, nsJARInputThunk **resultInput)
{
    MOZ_ASSERT(resultInput);

    
    
    nsCOMPtr<nsIFile> clonedFile;
    nsresult rv = mJarFile->Clone(getter_AddRefs(clonedFile));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIZipReader> reader;
    if (jarCache) {
        if (mInnerJarEntry.IsEmpty())
            rv = jarCache->GetZip(clonedFile, getter_AddRefs(reader));
        else
            rv = jarCache->GetInnerZip(clonedFile, mInnerJarEntry,
                                       getter_AddRefs(reader));
    } else {
        
        nsCOMPtr<nsIZipReader> outerReader = do_CreateInstance(kZipReaderCID, &rv);
        if (NS_FAILED(rv))
            return rv;

        rv = outerReader->Open(clonedFile);
        if (NS_FAILED(rv))
            return rv;

        if (mInnerJarEntry.IsEmpty())
            reader = outerReader;
        else {
            reader = do_CreateInstance(kZipReaderCID, &rv);
            if (NS_FAILED(rv))
                return rv;

            rv = reader->OpenInner(outerReader, mInnerJarEntry);
        }
    }
    if (NS_FAILED(rv))
        return rv;

    nsRefPtr<nsJARInputThunk> input = new nsJARInputThunk(reader,
                                                          mJarURI,
                                                          mJarEntry,
                                                          jarCache != nullptr
                                                          );
    rv = input->Init();
    if (NS_FAILED(rv))
        return rv;

    
    mContentLength = input->GetContentLength();

    input.forget(resultInput);
    return NS_OK;
}

nsresult
nsJARChannel::LookupFile(bool aAllowAsync)
{
    LOG(("nsJARChannel::LookupFile [this=%x %s]\n", this, mSpec.get()));

    if (mJarFile)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsIURI> uri;

    rv = mJarURI->GetJARFile(getter_AddRefs(mJarBaseURI));
    if (NS_FAILED(rv))
        return rv;

    rv = mJarURI->GetJAREntry(mJarEntry);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    NS_UnescapeURL(mJarEntry);

    
    {
        nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mJarBaseURI);
        if (fileURL)
            fileURL->GetFile(getter_AddRefs(mJarFile));
    }
    
    
    if (!mJarFile && !gJarHandler->IsMainProcess()) {
        nsAutoCString scheme;
        rv = mJarBaseURI->GetScheme(scheme);
        if (NS_SUCCEEDED(rv) && scheme.EqualsLiteral("remoteopenfile")) {
            nsRefPtr<RemoteOpenFileChild> remoteFile = new RemoteOpenFileChild();
            rv = remoteFile->Init(mJarBaseURI, mAppURI);
            NS_ENSURE_SUCCESS(rv, rv);
            mJarFile = remoteFile;

            nsIZipReaderCache *jarCache = gJarHandler->JarCache();
            if (jarCache) {
                bool cached = false;
                rv = jarCache->IsCached(mJarFile, &cached);
                if (NS_SUCCEEDED(rv) && cached) {
                    
                    
                    
                    
                    
                    #if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
                    
                    
                    return NS_OK;
                    #else
                    if (!mEnsureChildFd) {
                        return NS_OK;
                    }
                    PRFileDesc *fd = nullptr;
                    jarCache->GetFd(mJarFile, &fd);
                    if (fd) {
                        return SetRemoteNSPRFileDesc(fd);
                    }
                    #endif
                }
            }

            if (!aAllowAsync) {
                mJarFile = nullptr;
                return NS_OK;
            }

            mOpeningRemote = true;

            #if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
            #else
            if (mEnsureChildFd && jarCache) {
                jarCache->SetMustCacheFd(remoteFile, true);
            }
            #endif

            if (gJarHandler->RemoteOpenFileInProgress(remoteFile, this)) {
                
                
                
                return NS_OK;
            }

            
            nsCOMPtr<nsITabChild> tabChild;
            NS_QueryNotificationCallbacks(this, tabChild);
            nsCOMPtr<nsILoadContext> loadContext;
            NS_QueryNotificationCallbacks(this, loadContext);
            rv = remoteFile->AsyncRemoteFileOpen(PR_RDONLY, this, tabChild,
                                                 loadContext);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }
    
    if (!mJarFile) {
        nsCOMPtr<nsIJARURI> jarURI = do_QueryInterface(mJarBaseURI);
        if (jarURI) {
            nsCOMPtr<nsIFileURL> fileURL;
            nsCOMPtr<nsIURI> innerJarURI;
            rv = jarURI->GetJARFile(getter_AddRefs(innerJarURI));
            if (NS_SUCCEEDED(rv))
                fileURL = do_QueryInterface(innerJarURI);
            if (fileURL) {
                fileURL->GetFile(getter_AddRefs(mJarFile));
                jarURI->GetJAREntry(mInnerJarEntry);
            }
        }
    }

    return rv;
}

nsresult
nsJARChannel::OpenLocalFile()
{
    MOZ_ASSERT(mIsPending);

    
    mIsUnsafe = false;

    nsRefPtr<nsJARInputThunk> input;
    nsresult rv = CreateJarInput(gJarHandler->JarCache(),
                                 getter_AddRefs(input));
    if (NS_SUCCEEDED(rv)) {
        
        rv = NS_NewInputStreamPump(getter_AddRefs(mPump), input);
        if (NS_SUCCEEDED(rv))
            rv = mPump->AsyncRead(this, nullptr);
    }

    return rv;
}

void
nsJARChannel::NotifyError(nsresult aError)
{
    MOZ_ASSERT(NS_FAILED(aError));

    mStatus = aError;

    OnStartRequest(nullptr, nullptr);
    OnStopRequest(nullptr, nullptr, aError);
}

void
nsJARChannel::FireOnProgress(uint64_t aProgress)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mProgressSink);

  mProgressSink->OnProgress(this, nullptr, aProgress,
                            uint64_t(mContentLength));
}

nsresult
nsJARChannel::SetRemoteNSPRFileDesc(PRFileDesc *fd)
{
    PROsfd osfd = dup(PR_FileDesc2NativeHandle(fd));
    if (osfd == -1) {
        return NS_ERROR_FAILURE;
    }

    RemoteOpenFileChild* remoteFile =
        static_cast<RemoteOpenFileChild*>(mJarFile.get());
    nsresult rv = remoteFile->SetNSPRFileDesc(PR_ImportFile(osfd));
    if (NS_FAILED(rv)) {
        close(osfd);
    }

    return rv;
}





NS_IMETHODIMP
nsJARChannel::GetName(nsACString &result)
{
    return mJarURI->GetSpec(result);
}

NS_IMETHODIMP
nsJARChannel::IsPending(bool *result)
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
    if (mAppURI) {
        NS_IF_ADDREF(*aURI = mAppURI);
    } else {
        NS_IF_ADDREF(*aURI = mJarURI);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetOwner(nsISupports **aOwner)
{
    
    *aOwner = mOwner;
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetOwner(nsISupports *aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetLoadInfo(nsILoadInfo **aLoadInfo)
{
  NS_IF_ADDREF(*aLoadInfo = mLoadInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetLoadInfo(nsILoadInfo* aLoadInfo)
{
  mLoadInfo = aLoadInfo;
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
    
    
    if (!mOpened) {
      result.Assign(UNKNOWN_CONTENT_TYPE);
      return NS_OK;
    }

    if (mContentType.IsEmpty()) {

        
        
        
        const char *ext = nullptr, *fileName = mJarEntry.get();
        int32_t len = mJarEntry.Length();

        
        
        
        if (ENTRY_IS_DIRECTORY(mJarEntry)) {
            mContentType.AssignLiteral(APPLICATION_HTTP_INDEX_FORMAT);
        }
        else {
            
            for (int32_t i = len-1; i >= 0; i--) {
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
nsJARChannel::GetContentDisposition(uint32_t *aContentDisposition)
{
    if (mContentDispositionHeader.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;

    *aContentDisposition = mContentDisposition;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetContentDisposition(uint32_t aContentDisposition)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsJARChannel::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsJARChannel::SetContentDispositionFilename(const nsAString &aContentDispositionFilename)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsJARChannel::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
    if (mContentDispositionHeader.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;

    aContentDispositionHeader = mContentDispositionHeader;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetContentLength(int64_t *result)
{
    *result = mContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetContentLength(int64_t aContentLength)
{
    
    mContentLength = aContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::Open(nsIInputStream **stream)
{
    LOG(("nsJARChannel::Open [this=%x]\n", this));

    NS_ENSURE_TRUE(!mOpened, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    mJarFile = nullptr;
    mIsUnsafe = true;

    nsresult rv = LookupFile(false);
    if (NS_FAILED(rv))
        return rv;

    
    if (!mJarFile) {
        NS_NOTREACHED("need sync downloader");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    nsRefPtr<nsJARInputThunk> input;
    rv = CreateJarInput(gJarHandler->JarCache(), getter_AddRefs(input));
    if (NS_FAILED(rv))
        return rv;

    input.forget(stream);
    mOpened = true;
    
    mIsUnsafe = false;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctx)
{
    LOG(("nsJARChannel::AsyncOpen [this=%x]\n", this));

    NS_ENSURE_ARG_POINTER(listener);
    NS_ENSURE_TRUE(!mOpened, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    mJarFile = nullptr;
    mIsUnsafe = true;

    
    NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup, mProgressSink);

    nsresult rv = LookupFile(true);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    mListener = listener;
    mListenerContext = ctx;
    mIsPending = true;

    if (!mJarFile) {
        
        
        rv = NS_NewDownloader(getter_AddRefs(mDownloader), this);
        if (NS_SUCCEEDED(rv)) {
            
            
            if (mLoadInfo) {
              rv = NS_OpenURIInternal(mDownloader,
                                      nullptr,   
                                      mJarBaseURI,
                                      mLoadInfo,
                                      mLoadGroup,
                                      mCallbacks,
                                      mLoadFlags & ~(LOAD_DOCUMENT_URI | LOAD_CALL_CONTENT_SNIFFERS));
            }
            else {
              rv = NS_OpenURIInternal(mDownloader,
                                      nullptr,   
                                      mJarBaseURI,
                                      nullptr, 
                                      nsContentUtils::GetSystemPrincipal(),
                                      nsILoadInfo::SEC_NORMAL,
                                      nsIContentPolicy::TYPE_OTHER,
                                      mLoadGroup,
                                      mCallbacks,
                                      mLoadFlags & ~(LOAD_DOCUMENT_URI | LOAD_CALL_CONTENT_SNIFFERS));
            }
        }
    } else if (mOpeningRemote) {
        
    } else {
        rv = OpenLocalFile();
    }

    if (NS_FAILED(rv)) {
        mIsPending = false;
        mListenerContext = nullptr;
        mListener = nullptr;
        return rv;
    }


    if (mLoadGroup)
        mLoadGroup->AddRequest(this, nullptr);

    mOpened = true;
    return NS_OK;
}




NS_IMETHODIMP
nsJARChannel::GetIsUnsafe(bool *isUnsafe)
{
    *isUnsafe = mIsUnsafe;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::SetAppURI(nsIURI *aURI) {
    NS_ENSURE_ARG_POINTER(aURI);

    nsAutoCString scheme;
    aURI->GetScheme(scheme);
    if (!scheme.EqualsLiteral("app")) {
        return NS_ERROR_INVALID_ARG;
    }

    mAppURI = aURI;
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetJarFile(nsIFile **aFile)
{
    NS_IF_ADDREF(*aFile = mJarFile);
    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::GetZipEntry(nsIZipEntry **aZipEntry)
{
    nsresult rv = LookupFile(false);
    if (NS_FAILED(rv))
        return rv;

    if (!mJarFile)
        return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsIZipReader> reader;
    rv = gJarHandler->JarCache()->GetZip(mJarFile, getter_AddRefs(reader));
    if (NS_FAILED(rv))
        return rv;

    return reader->GetEntry(mJarEntry, aZipEntry);
}

NS_IMETHODIMP
nsJARChannel::EnsureChildFd()
{
    mEnsureChildFd = true;
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
        uint32_t loadFlags;
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
        
        channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

        nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
        if (httpChannel) {
            
            
            
            nsAutoCString header;
            httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Type"),
                                           header);
            nsAutoCString contentType;
            nsAutoCString charset;
            NS_ParseContentType(header, contentType, charset);
            nsAutoCString channelContentType;
            channel->GetContentType(channelContentType);
            mIsUnsafe = !(contentType.Equals(channelContentType) &&
                          (contentType.EqualsLiteral("application/java-archive") ||
                           contentType.EqualsLiteral("application/x-jar")));
        } else {
            nsCOMPtr<nsIJARChannel> innerJARChannel(do_QueryInterface(channel));
            if (innerJARChannel) {
                bool unsafe;
                innerJARChannel->GetIsUnsafe(&unsafe);
                mIsUnsafe = unsafe;
            }
        }

        channel->GetContentDispositionHeader(mContentDispositionHeader);
        mContentDisposition = NS_GetContentDispositionFromHeader(mContentDispositionHeader, this);
    }

    if (NS_SUCCEEDED(status) && mIsUnsafe &&
        !Preferences::GetBool("network.jar.open-unsafe-types", false)) {
        status = NS_ERROR_UNSAFE_CONTENT_TYPE;
    }

    if (NS_SUCCEEDED(status)) {
        
        nsCOMPtr<nsIViewSourceChannel> viewSource = do_QueryInterface(channel);
        if (viewSource) {
            status = NS_ERROR_UNSAFE_CONTENT_TYPE;
        }
    }

    if (NS_SUCCEEDED(status)) {
        mJarFile = file;

        nsRefPtr<nsJARInputThunk> input;
        rv = CreateJarInput(nullptr, getter_AddRefs(input));
        if (NS_SUCCEEDED(rv)) {
            
            rv = NS_NewInputStreamPump(getter_AddRefs(mPump), input);
            if (NS_SUCCEEDED(rv))
                rv = mPump->AsyncRead(this, nullptr);
        }
        status = rv;
    }

    if (NS_FAILED(status)) {
        NotifyError(status);
    }

    return NS_OK;
}




nsresult
nsJARChannel::OnRemoteFileOpenComplete(nsresult aOpenStatus)
{
    nsresult rv = aOpenStatus;

    
    
    if (NS_SUCCEEDED(rv) || rv == NS_ERROR_ALREADY_OPENED) {
        #if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
        
        
        #else
        if (mEnsureChildFd) {
            
            
            mozilla::AutoFDClose fd;
            mJarFile->OpenNSPRFileDesc(PR_RDONLY, 0, &fd.rwget());
            if (!fd) {
                nsIZipReaderCache *jarCache = gJarHandler->JarCache();
                if (!jarCache) {
                    rv = NS_ERROR_FAILURE;
                }
                PRFileDesc *jar_fd = nullptr;
                jarCache->GetFd(mJarFile, &jar_fd);
                
                
                
                rv = SetRemoteNSPRFileDesc(jar_fd);
            }
        }
        #endif
        if (NS_SUCCEEDED(rv) || rv == NS_ERROR_ALREADY_OPENED) {
            rv = OpenLocalFile();
        }
    }

    if (NS_FAILED(rv)) {
        NotifyError(rv);
    }

    return NS_OK;
}





NS_IMETHODIMP
nsJARChannel::OnStartRequest(nsIRequest *req, nsISupports *ctx)
{
    LOG(("nsJARChannel::OnStartRequest [this=%x %s]\n", this, mSpec.get()));

    mRequest = req;
    nsresult rv = mListener->OnStartRequest(this, mListenerContext);
    mRequest = nullptr;

    return rv;
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
        mLoadGroup->RemoveRequest(this, nullptr, status);

    mPump = 0;
    mIsPending = false;
    mDownloader = 0; 

    
    mCallbacks = 0;
    mProgressSink = 0;

    #if defined(XP_WIN) || defined(MOZ_WIDGET_COCOA)
    #else
    if (mEnsureChildFd) {
      nsIZipReaderCache *jarCache = gJarHandler->JarCache();
      if (jarCache) {
          jarCache->SetMustCacheFd(mJarFile, false);
      }
      
      mJarFile = nullptr;
    }
    #endif

    return NS_OK;
}

NS_IMETHODIMP
nsJARChannel::OnDataAvailable(nsIRequest *req, nsISupports *ctx,
                               nsIInputStream *stream,
                               uint64_t offset, uint32_t count)
{
#if defined(PR_LOGGING)
    LOG(("nsJARChannel::OnDataAvailable [this=%x %s]\n", this, mSpec.get()));
#endif

    nsresult rv;

    rv = mListener->OnDataAvailable(this, mListenerContext, stream, offset, count);

    
    
    
    if (mProgressSink && NS_SUCCEEDED(rv)) {
        if (NS_IsMainThread()) {
            FireOnProgress(offset + count);
        } else {
            nsCOMPtr<nsIRunnable> runnable =
              NS_NewRunnableMethodWithArg<uint64_t>(this,
                                                    &nsJARChannel::FireOnProgress,
                                                    offset + count);
            NS_DispatchToMainThread(runnable);
        }
    }

    return rv; 
}

NS_IMETHODIMP
nsJARChannel::RetargetDeliveryTo(nsIEventTarget* aEventTarget)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIThreadRetargetableRequest> request = do_QueryInterface(mRequest);
  if (!request) {
    return NS_ERROR_NO_INTERFACE;
  }

  return request->RetargetDeliveryTo(aEventTarget);
}

NS_IMETHODIMP
nsJARChannel::CheckListenerChain()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIThreadRetargetableStreamListener> listener =
    do_QueryInterface(mListener);
  if (!listener) {
    return NS_ERROR_NO_INTERFACE;
  }

  return listener->CheckListenerChain();
}
