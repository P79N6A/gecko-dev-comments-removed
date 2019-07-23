





































extern "C" {
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
}

#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIGenericFactory.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "nsIAuthPrompt.h"
#include "nsIStringBundle.h"
#include "nsIStandardURL.h"
#include "nsIURL.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsINetUtil.h"
#include "nsAutoPtr.h"
#include "nsError.h"
#include "prlog.h"
#include "prtime.h"
#include "prprf.h"
#include "plstr.h"

#define MOZ_GNOMEVFS_SCHEME              "moz-gnomevfs"
#define MOZ_GNOMEVFS_SUPPORTED_PROTOCOLS "network.gnomevfs.supported-protocols"




#ifdef PR_LOGGING
static PRLogModuleInfo *sGnomeVFSLog;
#define LOG(args) PR_LOG(sGnomeVFSLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif



static nsresult
MapGnomeVFSResult(GnomeVFSResult result)
{
  switch (result)
  {
    case GNOME_VFS_OK:                           return NS_OK;
    case GNOME_VFS_ERROR_NOT_FOUND:              return NS_ERROR_FILE_NOT_FOUND;
    case GNOME_VFS_ERROR_INTERNAL:               return NS_ERROR_UNEXPECTED;
    case GNOME_VFS_ERROR_BAD_PARAMETERS:         return NS_ERROR_INVALID_ARG;
    case GNOME_VFS_ERROR_NOT_SUPPORTED:          return NS_ERROR_NOT_AVAILABLE;
    case GNOME_VFS_ERROR_CORRUPTED_DATA:         return NS_ERROR_FILE_CORRUPTED;
    case GNOME_VFS_ERROR_TOO_BIG:                return NS_ERROR_FILE_TOO_BIG;
    case GNOME_VFS_ERROR_NO_SPACE:               return NS_ERROR_FILE_NO_DEVICE_SPACE;
    case GNOME_VFS_ERROR_READ_ONLY:
    case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:  return NS_ERROR_FILE_READ_ONLY;
    case GNOME_VFS_ERROR_INVALID_URI:
    case GNOME_VFS_ERROR_INVALID_HOST_NAME:      return NS_ERROR_MALFORMED_URI;
    case GNOME_VFS_ERROR_ACCESS_DENIED:
    case GNOME_VFS_ERROR_NOT_PERMITTED:
    case GNOME_VFS_ERROR_LOGIN_FAILED:           return NS_ERROR_FILE_ACCESS_DENIED;
    case GNOME_VFS_ERROR_EOF:                    return NS_BASE_STREAM_CLOSED;
    case GNOME_VFS_ERROR_NOT_A_DIRECTORY:        return NS_ERROR_FILE_NOT_DIRECTORY;
    case GNOME_VFS_ERROR_IN_PROGRESS:            return NS_ERROR_IN_PROGRESS;
    case GNOME_VFS_ERROR_FILE_EXISTS:            return NS_ERROR_FILE_ALREADY_EXISTS;
    case GNOME_VFS_ERROR_IS_DIRECTORY:           return NS_ERROR_FILE_IS_DIRECTORY;
    case GNOME_VFS_ERROR_NO_MEMORY:              return NS_ERROR_OUT_OF_MEMORY;
    case GNOME_VFS_ERROR_HOST_NOT_FOUND:
    case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:    return NS_ERROR_UNKNOWN_HOST;
    case GNOME_VFS_ERROR_CANCELLED:
    case GNOME_VFS_ERROR_INTERRUPTED:            return NS_ERROR_ABORT;
    case GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY:    return NS_ERROR_FILE_DIR_NOT_EMPTY;
    case GNOME_VFS_ERROR_NAME_TOO_LONG:          return NS_ERROR_FILE_NAME_TOO_LONG;
    case GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE:  return NS_ERROR_UNKNOWN_PROTOCOL;

    


















    
    default:
      return NS_ERROR_FAILURE;
  }

  return NS_ERROR_FAILURE;
}



static void
ProxiedAuthCallback(gconstpointer in,
                    gsize         in_size,
                    gpointer      out,
                    gsize         out_size,
                    gpointer      callback_data)
{
  GnomeVFSModuleCallbackAuthenticationIn *authIn =
      (GnomeVFSModuleCallbackAuthenticationIn *) in;
  GnomeVFSModuleCallbackAuthenticationOut *authOut =
      (GnomeVFSModuleCallbackAuthenticationOut *) out;

  LOG(("gnomevfs: ProxiedAuthCallback [uri=%s]\n", authIn->uri));

  
  nsIChannel *channel = (nsIChannel *) callback_data;
  if (!channel)
    return;

  nsCOMPtr<nsIAuthPrompt> prompt;
  NS_QueryNotificationCallbacks(channel, prompt);

  
  
  
  if (!prompt)
    return;

  
  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));
  if (!uri)
    return;

#ifdef DEBUG
  {
    
    
    
    
    
    
    
    
    
    nsCAutoString spec;
    uri->GetSpec(spec);
    int uriLen = strlen(authIn->uri);
    if (!StringHead(spec, uriLen).Equals(nsDependentCString(authIn->uri, uriLen)))
    {
      LOG(("gnomevfs: [spec=%s authIn->uri=%s]\n", spec.get(), authIn->uri));
      NS_ERROR("URI mismatch");
    }
  }
#endif

  nsCAutoString scheme, hostPort;
  uri->GetScheme(scheme);
  uri->GetHostPort(hostPort);

  
  
  if (scheme.IsEmpty() || hostPort.IsEmpty())
    return;

  
  
  
  nsAutoString key, realm;

  NS_ConvertUTF8toUTF16 dispHost(scheme);
  dispHost.Append(NS_LITERAL_STRING("://"));
  dispHost.Append(NS_ConvertUTF8toUTF16(hostPort));

  key = dispHost;
  if (authIn->realm)
  {
    
    
    
    realm.Append('"');
    realm.Append(NS_ConvertASCIItoUTF16(authIn->realm));
    realm.Append('"');
    key.Append(' ');
    key.Append(realm);
  }

  
  
  
  
  

  nsCOMPtr<nsIStringBundleService> bundleSvc =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  if (!bundleSvc)
    return;

  nsCOMPtr<nsIStringBundle> bundle;
  bundleSvc->CreateBundle("chrome://global/locale/prompts.properties",
                          getter_AddRefs(bundle));
  if (!bundle)
    return;

  nsString message;
  if (!realm.IsEmpty())
  {
    const PRUnichar *strings[] = { realm.get(), dispHost.get() };
    bundle->FormatStringFromName(NS_LITERAL_STRING("EnterUserPasswordForRealm").get(),
                                 strings, 2, getter_Copies(message));
  }
  else
  {
    const PRUnichar *strings[] = { dispHost.get() };
    bundle->FormatStringFromName(NS_LITERAL_STRING("EnterUserPasswordFor").get(),
                                 strings, 1, getter_Copies(message));
  }
  if (message.IsEmpty())
    return;

  
  nsresult rv;
  PRBool retval = PR_FALSE;
  PRUnichar *user = nsnull, *pass = nsnull;

  rv = prompt->PromptUsernameAndPassword(nsnull, message.get(),
                                         key.get(),
                                         nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                         &user, &pass, &retval);
  if (NS_FAILED(rv))
    return;
  if (!retval || !user || !pass)
    return;

  
  
  
  

  
  authOut->username = g_strdup(NS_LossyConvertUTF16toASCII(user).get());
  authOut->password = g_strdup(NS_LossyConvertUTF16toASCII(pass).get());

  nsMemory::Free(user);
  nsMemory::Free(pass);
}

struct nsGnomeVFSAuthCallbackEvent : public nsRunnable
{
  gconstpointer in;
  gsize         in_size;
  gpointer      out;
  gsize         out_size;
  gpointer      callback_data;

  NS_IMETHOD Run() {
    ProxiedAuthCallback(in, in_size, out, out_size, callback_data);
    return NS_OK;
  }
};

static void
AuthCallback(gconstpointer in,
             gsize         in_size,
             gpointer      out,
             gsize         out_size,
             gpointer      callback_data)
{
  
  

  nsRefPtr<nsGnomeVFSAuthCallbackEvent> ev = new nsGnomeVFSAuthCallbackEvent();
  if (!ev)
    return;  

  ev->in = in;
  ev->in_size = in_size;
  ev->out = out;
  ev->out_size = out_size;
  ev->callback_data = callback_data;

  NS_DispatchToMainThread(ev, NS_DISPATCH_SYNC);
}



static gint
FileInfoComparator(gconstpointer a, gconstpointer b)
{
  const GnomeVFSFileInfo *ia = (const GnomeVFSFileInfo *) a;
  const GnomeVFSFileInfo *ib = (const GnomeVFSFileInfo *) b;

  return strcasecmp(ia->name, ib->name);
}



class nsGnomeVFSInputStream : public nsIInputStream
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

    nsGnomeVFSInputStream(const nsCString &uriSpec)
      : mSpec(uriSpec)
      , mChannel(nsnull)
      , mHandle(nsnull)
      , mBytesRemaining(PR_UINT32_MAX)
      , mStatus(NS_OK)
      , mDirList(nsnull)
      , mDirListPtr(nsnull)
      , mDirBufCursor(0)
      , mDirOpen(PR_FALSE) {}

   ~nsGnomeVFSInputStream() { Close(); }

    void SetChannel(nsIChannel *channel)
    {
      
      
      
      
      
      
      
      
      
      
      
      

      NS_ADDREF(mChannel = channel);
    }

  private:
    GnomeVFSResult DoOpen();
    GnomeVFSResult DoRead(char *aBuf, PRUint32 aCount, PRUint32 *aCountRead);
    nsresult       SetContentTypeOfChannel(const char *contentType);

  private:
    nsCString                mSpec;
    nsIChannel              *mChannel; 
    GnomeVFSHandle          *mHandle;
    PRUint32                 mBytesRemaining;
    nsresult                 mStatus;
    GList                   *mDirList;
    GList                   *mDirListPtr;
    nsCString                mDirBuf;
    PRUint32                 mDirBufCursor;
    PRPackedBool             mDirOpen;
};

GnomeVFSResult
nsGnomeVFSInputStream::DoOpen()
{
  GnomeVFSResult rv;

  NS_ASSERTION(mHandle == nsnull, "already open");

  
  
  

  gnome_vfs_module_callback_push(GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
                                 AuthCallback, mChannel, NULL);

  
  
  
  
  
  
  
  
  
  
  
  

  GnomeVFSFileInfo info = {0};
  rv = gnome_vfs_get_file_info(mSpec.get(), &info, GNOME_VFS_FILE_INFO_DEFAULT);
  if (rv == GNOME_VFS_OK && info.type == GNOME_VFS_FILE_TYPE_DIRECTORY)
  {
    rv = gnome_vfs_directory_list_load(&mDirList, mSpec.get(),
                                       GNOME_VFS_FILE_INFO_DEFAULT);

    LOG(("gnomevfs: gnome_vfs_directory_list_load returned %d (%s) [spec=\"%s\"]\n",
        rv, gnome_vfs_result_to_string(rv), mSpec.get()));
  }
  else
  {
    rv = gnome_vfs_open(&mHandle, mSpec.get(), GNOME_VFS_OPEN_READ);

    LOG(("gnomevfs: gnome_vfs_open returned %d (%s) [spec=\"%s\"]\n",
        rv, gnome_vfs_result_to_string(rv), mSpec.get()));
  }

  gnome_vfs_module_callback_pop(GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION);

  if (rv == GNOME_VFS_OK)
  {
    if (mHandle)
    {
      
      
      
      
      

      if (info.mime_type && (strcmp(info.mime_type, APPLICATION_OCTET_STREAM) != 0))
        SetContentTypeOfChannel(info.mime_type);

      
      mBytesRemaining = (PRUint32) info.size;

      
      
      if (mBytesRemaining != PR_UINT32_MAX)
        mChannel->SetContentLength(mBytesRemaining);
    }
    else
    {
      mDirOpen = PR_TRUE;

      
      mDirList = g_list_sort(mDirList, FileInfoComparator);
      mDirListPtr = mDirList;

      
      mDirBuf.Append("300: ");
      mDirBuf.Append(mSpec);
      if (mSpec.get()[mSpec.Length() - 1] != '/')
        mDirBuf.Append('/');
      mDirBuf.Append('\n');

      
      mDirBuf.Append("200: filename content-length last-modified file-type\n");

      
      
      mDirBuf.Append("301: UTF-8\n");

      SetContentTypeOfChannel(APPLICATION_HTTP_INDEX_FORMAT);
    }
  }

  gnome_vfs_file_info_clear(&info);
  return rv;
}

GnomeVFSResult
nsGnomeVFSInputStream::DoRead(char *aBuf, PRUint32 aCount, PRUint32 *aCountRead)
{
  GnomeVFSResult rv;

  if (mHandle)
  {
    GnomeVFSFileSize bytesRead;
    rv = gnome_vfs_read(mHandle, aBuf, aCount, &bytesRead);
    if (rv == GNOME_VFS_OK)
    {
      *aCountRead = (PRUint32) bytesRead;
      mBytesRemaining -= *aCountRead;
    }
  }
  else if (mDirOpen)
  {
    rv = GNOME_VFS_OK;

    while (aCount && rv != GNOME_VFS_ERROR_EOF)
    {
      
      PRUint32 bufLen = mDirBuf.Length() - mDirBufCursor;
      if (bufLen)
      {
        PRUint32 n = PR_MIN(bufLen, aCount);
        memcpy(aBuf, mDirBuf.get() + mDirBufCursor, n);
        *aCountRead += n;
        aBuf += n;
        aCount -= n;
        mDirBufCursor += n;
      }

      if (!mDirListPtr)    
      {
        rv = GNOME_VFS_ERROR_EOF;
      }
      else if (aCount)     
      {
        GnomeVFSFileInfo *info = (GnomeVFSFileInfo *) mDirListPtr->data;

        
        if (info->name[0] == '.' &&
               (info->name[1] == '\0' ||
                   (info->name[1] == '.' && info->name[2] == '\0')))
        {
          mDirListPtr = mDirListPtr->next;
          continue;
        }

        mDirBuf.Assign("201: ");

        
        nsCString escName;
        nsCOMPtr<nsINetUtil> nu = do_GetService(NS_NETUTIL_CONTRACTID);
        if (nu) {
          nu->EscapeString(nsDependentCString(info->name),
                           nsINetUtil::ESCAPE_URL_PATH, escName);

          mDirBuf.Append(escName);
          mDirBuf.Append(' ');
        }

        
        
        mDirBuf.AppendInt(PRInt32(info->size));
        mDirBuf.Append(' ');

        
        
        
        
        PRExplodedTime tm;
        PRTime pt = ((PRTime) info->mtime) * 1000000;
        PR_ExplodeTime(pt, PR_GMTParameters, &tm);
        {
          char buf[64];
          PR_FormatTimeUSEnglish(buf, sizeof(buf),
              "%a,%%20%d%%20%b%%20%Y%%20%H:%M:%S%%20GMT ", &tm);
          mDirBuf.Append(buf);
        }

        
        switch (info->type)
        {
          case GNOME_VFS_FILE_TYPE_REGULAR:
            mDirBuf.Append("FILE ");
            break;
          case GNOME_VFS_FILE_TYPE_DIRECTORY:
            mDirBuf.Append("DIRECTORY ");
            break;
          case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
            mDirBuf.Append("SYMBOLIC-LINK ");
            break;
          default:
            break;
        }

        mDirBuf.Append('\n');

        mDirBufCursor = 0;
        mDirListPtr = mDirListPtr->next;
      }
    }
  }
  else
  {
    NS_NOTREACHED("reading from what?");
    rv = GNOME_VFS_ERROR_GENERIC;
  }

  return rv;
}


class nsGnomeVFSSetContentTypeEvent : public nsRunnable
{
  public:
    nsGnomeVFSSetContentTypeEvent(nsIChannel *channel, const char *contentType)
      : mChannel(channel), mContentType(contentType)
    {
      
      
    }

    NS_IMETHOD Run()
    {
      mChannel->SetContentType(mContentType);
      return NS_OK;
    }

  private: 
    nsIChannel *mChannel;
    nsCString   mContentType;
};

nsresult
nsGnomeVFSInputStream::SetContentTypeOfChannel(const char *contentType)
{
  
  
  
  
  

  nsresult rv;
  nsCOMPtr<nsIRunnable> ev =
      new nsGnomeVFSSetContentTypeEvent(mChannel, contentType);
  if (!ev)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    rv = NS_DispatchToMainThread(ev);
  }
  return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsGnomeVFSInputStream, nsIInputStream)

NS_IMETHODIMP
nsGnomeVFSInputStream::Close()
{
  if (mHandle)
  {
    gnome_vfs_close(mHandle);
    mHandle = nsnull;
  }

  if (mDirList)
  {
    
    g_list_foreach(mDirList, (GFunc) gnome_vfs_file_info_unref, nsnull);
    g_list_free(mDirList);
    mDirList = nsnull;
    mDirListPtr = nsnull;
  }

  if (mChannel)
  {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIThread> thread = do_GetMainThread();
    if (thread)
      rv = NS_ProxyRelease(thread, mChannel);

    NS_ASSERTION(thread && NS_SUCCEEDED(rv), "leaking channel reference");
    mChannel = nsnull;
  }

  mSpec.Truncate(); 

  
  if (NS_SUCCEEDED(mStatus))
    mStatus = NS_BASE_STREAM_CLOSED;

  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSInputStream::Available(PRUint32 *aResult)
{
  if (NS_FAILED(mStatus))
    return mStatus;

  *aResult = mBytesRemaining;
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSInputStream::Read(char *aBuf,
                            PRUint32 aCount,
                            PRUint32 *aCountRead)
{
  *aCountRead = 0;

  if (mStatus == NS_BASE_STREAM_CLOSED)
    return NS_OK;
  if (NS_FAILED(mStatus))
    return mStatus;

  GnomeVFSResult rv = GNOME_VFS_OK;

  
  if (!mHandle && !mDirOpen)
    rv = DoOpen();
  
  if (rv == GNOME_VFS_OK)
    rv = DoRead(aBuf, aCount, aCountRead);

  if (rv != GNOME_VFS_OK)
  {
    
    mStatus = MapGnomeVFSResult(rv);
    if (mStatus == NS_BASE_STREAM_CLOSED)
      return NS_OK;

    LOG(("gnomevfs: result %d [%s] mapped to 0x%x\n",
        rv, gnome_vfs_result_to_string(rv), mStatus));
  }
  return mStatus;
}

NS_IMETHODIMP
nsGnomeVFSInputStream::ReadSegments(nsWriteSegmentFun aWriter,
                                    void *aClosure,
                                    PRUint32 aCount,
                                    PRUint32 *aResult)
{
  
  
  
  NS_NOTREACHED("nsGnomeVFSInputStream::ReadSegments");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsGnomeVFSInputStream::IsNonBlocking(PRBool *aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}



class nsGnomeVFSProtocolHandler : public nsIProtocolHandler
                                , public nsIObserver
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER

    nsresult Init();

  private:
    void   InitSupportedProtocolsPref(nsIPrefBranch *prefs);
    PRBool IsSupportedProtocol(const nsCString &spec);

    nsCString mSupportedProtocols;
};

NS_IMPL_ISUPPORTS2(nsGnomeVFSProtocolHandler, nsIProtocolHandler, nsIObserver)

nsresult
nsGnomeVFSProtocolHandler::Init()
{
#ifdef PR_LOGGING
  sGnomeVFSLog = PR_NewLogModule("gnomevfs");
#endif

  if (!gnome_vfs_initialized())
  {
    if (!gnome_vfs_init())
    {
      NS_WARNING("gnome_vfs_init failed");
      return NS_ERROR_UNEXPECTED;
    }
  }

  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs)
  {
    InitSupportedProtocolsPref(prefs);
    prefs->AddObserver(MOZ_GNOMEVFS_SUPPORTED_PROTOCOLS, this, PR_FALSE);
  }

  return NS_OK;
}

void
nsGnomeVFSProtocolHandler::InitSupportedProtocolsPref(nsIPrefBranch *prefs)
{
  
  nsresult rv = prefs->GetCharPref(MOZ_GNOMEVFS_SUPPORTED_PROTOCOLS,
                                   getter_Copies(mSupportedProtocols));
  if (NS_SUCCEEDED(rv)) {
    mSupportedProtocols.StripWhitespace();
    ToLowerCase(mSupportedProtocols);
  }
  else
    mSupportedProtocols.Assign("smb:,sftp:"); 

  LOG(("gnomevfs: supported protocols \"%s\"\n", mSupportedProtocols.get()));
}

PRBool
nsGnomeVFSProtocolHandler::IsSupportedProtocol(const nsCString &aSpec)
{
  const char *specString = aSpec.get();
  const char *colon = strchr(specString, ':');
  if (!colon)
    return PR_FALSE;

  PRUint32 length = colon - specString + 1;

  
  nsCString scheme(specString, length);

  char *found = PL_strcasestr(mSupportedProtocols.get(), scheme.get());
  if (!found)
    return PR_FALSE;

  if (found[length] != ',' && found[length] != '\0')
    return PR_FALSE;

  return PR_TRUE;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::GetScheme(nsACString &aScheme)
{
  aScheme.Assign(MOZ_GNOMEVFS_SCHEME);
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::GetProtocolFlags(PRUint32 *aProtocolFlags)
{
  
  *aProtocolFlags = URI_STD | URI_DANGEROUS_TO_LOAD;
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::NewURI(const nsACString &aSpec,
                                  const char *aOriginCharset,
                                  nsIURI *aBaseURI,
                                  nsIURI **aResult)
{
  const nsCString flatSpec(aSpec);
  LOG(("gnomevfs: NewURI [spec=%s]\n", flatSpec.get()));

  if (!aBaseURI)
  {
    
    
    
    
    
    
    
    
    
    
    if (!IsSupportedProtocol(flatSpec))
      return NS_ERROR_UNKNOWN_PROTOCOL;

    
    GnomeVFSURI *uri = gnome_vfs_uri_new(flatSpec.get());
    if (!uri)
      return NS_ERROR_UNKNOWN_PROTOCOL;
  }

  
  
  
  
  
  
  
  
  
  
  nsresult rv;
  nsCOMPtr<nsIStandardURL> url =
      do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = url->Init(nsIStandardURL::URLTYPE_STANDARD, -1, flatSpec,
                 aOriginCharset, aBaseURI);
  if (NS_SUCCEEDED(rv))
    rv = CallQueryInterface(url, aResult);

  return rv;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::NewChannel(nsIURI *aURI, nsIChannel **aResult)
{
  NS_ENSURE_ARG_POINTER(aURI);
  nsresult rv;

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  if (NS_FAILED(rv))
    return rv;

  nsRefPtr<nsGnomeVFSInputStream> stream = new nsGnomeVFSInputStream(spec);
  if (!stream)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    
    
    rv = NS_NewInputStreamChannel(aResult, aURI, stream,
                                  NS_LITERAL_CSTRING(UNKNOWN_CONTENT_TYPE));
    if (NS_SUCCEEDED(rv))
      stream->SetChannel(*aResult);
  }
  return rv;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::AllowPort(PRInt32 aPort,
                                     const char *aScheme,
                                     PRBool *aResult)
{
  
  *aResult = PR_FALSE; 
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSProtocolHandler::Observe(nsISupports *aSubject,
                                   const char *aTopic,
                                   const PRUnichar *aData)
{
  if (strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    nsCOMPtr<nsIPrefBranch> prefs = do_QueryInterface(aSubject);
    InitSupportedProtocolsPref(prefs);
  }
  return NS_OK;
}



#define NS_GNOMEVFSPROTOCOLHANDLER_CID               \
{ /* 9b6dc177-a2e4-49e1-9c98-0a8384de7f6c */         \
    0x9b6dc177,                                      \
    0xa2e4,                                          \
    0x49e1,                                          \
    {0x9c, 0x98, 0x0a, 0x83, 0x84, 0xde, 0x7f, 0x6c} \
}

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGnomeVFSProtocolHandler, Init)

static const nsModuleComponentInfo components[] =
{
  { "nsGnomeVFSProtocolHandler",
    NS_GNOMEVFSPROTOCOLHANDLER_CID,
    NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX MOZ_GNOMEVFS_SCHEME,
    nsGnomeVFSProtocolHandlerConstructor
  }
};

NS_IMPL_NSGETMODULE(nsGnomeVFSModule, components)
