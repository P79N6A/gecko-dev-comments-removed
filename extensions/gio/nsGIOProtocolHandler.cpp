










































#include "mozilla/ModuleUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "nsIStringBundle.h"
#include "nsIStandardURL.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "mozilla/Monitor.h"
#include <gio/gio.h>

#define MOZ_GIO_SCHEME              "moz-gio"
#define MOZ_GIO_SUPPORTED_PROTOCOLS "network.gio.supported-protocols"




#ifdef PR_LOGGING
static PRLogModuleInfo *sGIOLog;
#define LOG(args) PR_LOG(sGIOLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif



static nsresult
MapGIOResult(gint code) 
{
  switch (code)
  {
     case G_IO_ERROR_NOT_FOUND:                  return NS_ERROR_FILE_NOT_FOUND; 
     case G_IO_ERROR_INVALID_ARGUMENT:           return NS_ERROR_INVALID_ARG;
     case G_IO_ERROR_NOT_SUPPORTED:              return NS_ERROR_NOT_AVAILABLE;
     case G_IO_ERROR_NO_SPACE:                   return NS_ERROR_FILE_NO_DEVICE_SPACE;
     case G_IO_ERROR_READ_ONLY:                  return NS_ERROR_FILE_READ_ONLY;
     case G_IO_ERROR_PERMISSION_DENIED:          return NS_ERROR_FILE_ACCESS_DENIED; 
     case G_IO_ERROR_CLOSED:                     return NS_BASE_STREAM_CLOSED; 
     case G_IO_ERROR_NOT_DIRECTORY:              return NS_ERROR_FILE_NOT_DIRECTORY;
     case G_IO_ERROR_PENDING:                    return NS_ERROR_IN_PROGRESS;
     case G_IO_ERROR_EXISTS:                     return NS_ERROR_FILE_ALREADY_EXISTS;
     case G_IO_ERROR_IS_DIRECTORY:               return NS_ERROR_FILE_IS_DIRECTORY;
     case G_IO_ERROR_NOT_MOUNTED:                return NS_ERROR_NOT_CONNECTED; 
     case G_IO_ERROR_HOST_NOT_FOUND:             return NS_ERROR_UNKNOWN_HOST; 
     case G_IO_ERROR_CANCELLED:                  return NS_ERROR_ABORT;
     case G_IO_ERROR_NOT_EMPTY:                  return NS_ERROR_FILE_DIR_NOT_EMPTY;
     case G_IO_ERROR_FILENAME_TOO_LONG:          return NS_ERROR_FILE_NAME_TOO_LONG;
     case G_IO_ERROR_INVALID_FILENAME:           return NS_ERROR_FILE_INVALID_PATH;
     case G_IO_ERROR_TIMED_OUT:                  return NS_ERROR_NET_TIMEOUT; 
     case G_IO_ERROR_WOULD_BLOCK:                return NS_BASE_STREAM_WOULD_BLOCK;
     case G_IO_ERROR_FAILED_HANDLED:             return NS_ERROR_ABORT; 














    
    default:
      return NS_ERROR_FAILURE;
  }

  return NS_ERROR_FAILURE;
}

static nsresult
MapGIOResult(GError *result)
{
  if (!result)
    return NS_OK;
  else 
    return MapGIOResult(result->code);
}



typedef enum {
  MOUNT_OPERATION_IN_PROGRESS, 
  MOUNT_OPERATION_SUCCESS,     
  MOUNT_OPERATION_FAILED       
} MountOperationResult;









static gint
FileInfoComparator(gconstpointer a, gconstpointer b)
{
  GFileInfo *ia = ( GFileInfo *) a;
  GFileInfo *ib = ( GFileInfo *) b;
  if (g_file_info_get_file_type(ia) == G_FILE_TYPE_DIRECTORY
      && g_file_info_get_file_type(ib) != G_FILE_TYPE_DIRECTORY)
    return -1;
  if (g_file_info_get_file_type(ib) == G_FILE_TYPE_DIRECTORY
      && g_file_info_get_file_type(ia) != G_FILE_TYPE_DIRECTORY)
    return 1;

  return strcasecmp(g_file_info_get_name(ia), g_file_info_get_name(ib));
}


static void mount_enclosing_volume_finished (GObject *source_object,
                                             GAsyncResult *res,
                                             gpointer user_data);
static void mount_operation_ask_password (GMountOperation   *mount_op,
                                          const char        *message,
                                          const char        *default_user,
                                          const char        *default_domain,
                                          GAskPasswordFlags flags,
                                          gpointer          user_data);


class nsGIOInputStream : public nsIInputStream
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

    nsGIOInputStream(const nsCString &uriSpec)
      : mSpec(uriSpec)
      , mChannel(nsnull)
      , mHandle(nsnull)
      , mStream(nsnull)
      , mBytesRemaining(PR_UINT32_MAX)
      , mStatus(NS_OK)
      , mDirList(nsnull)
      , mDirListPtr(nsnull)
      , mDirBufCursor(0)
      , mDirOpen(PR_FALSE)
      , mMonitorMountInProgress("GIOInputStream::MountFinished") { }

   ~nsGIOInputStream() { Close(); }

    void SetChannel(nsIChannel *channel)
    {
      
      
      
      
      
      
      
      
      
      
      
      

      NS_ADDREF(mChannel = channel);
    }
    void           SetMountResult(MountOperationResult result, gint error_code);
  private:
    nsresult       DoOpen();
    nsresult       DoRead(char *aBuf, PRUint32 aCount, PRUint32 *aCountRead);
    nsresult       SetContentTypeOfChannel(const char *contentType);
    nsresult       MountVolume();
    nsresult       DoOpenDirectory();
    nsresult       DoOpenFile(GFileInfo *info);        
    nsCString             mSpec;
    nsIChannel           *mChannel; 
    GFile                *mHandle;
    GFileInputStream     *mStream;
    PRUint64              mBytesRemaining;
    nsresult              mStatus;
    GList                *mDirList;
    GList                *mDirListPtr;
    nsCString             mDirBuf;
    PRUint32              mDirBufCursor;
    PRPackedBool          mDirOpen;
    MountOperationResult  mMountRes;
    mozilla::Monitor      mMonitorMountInProgress;
    gint                  mMountErrorCode;
};





 
void
nsGIOInputStream::SetMountResult(MountOperationResult result, gint error_code)
{
  mozilla::MonitorAutoEnter mon(mMonitorMountInProgress);
  mMountRes = result;
  mMountErrorCode = error_code;
  mon.Notify();
}





nsresult
nsGIOInputStream::MountVolume() {
  GMountOperation* mount_op = g_mount_operation_new();
  g_signal_connect (mount_op, "ask-password",
                    G_CALLBACK (mount_operation_ask_password), mChannel);
  mMountRes = MOUNT_OPERATION_IN_PROGRESS;
  


  g_file_mount_enclosing_volume(mHandle,
                                G_MOUNT_MOUNT_NONE,
                                mount_op,
                                NULL,
                                mount_enclosing_volume_finished,
                                this);
  mozilla::MonitorAutoEnter mon(mMonitorMountInProgress);
    
  while (mMountRes == MOUNT_OPERATION_IN_PROGRESS)
    mon.Wait();
  
  g_object_unref(mount_op);

  if (mMountRes == MOUNT_OPERATION_FAILED) {
    return MapGIOResult(mMountErrorCode);
  } else {
    return NS_OK;
  }
}






nsresult
nsGIOInputStream::DoOpenDirectory()
{
  GError *error = NULL;

  GFileEnumerator *f_enum = g_file_enumerate_children(mHandle,
                                                      "standard::*,time::*",
                                                      G_FILE_QUERY_INFO_NONE,
                                                      NULL,
                                                      &error);
  if (!f_enum) {
    nsresult rv = MapGIOResult(error);
    g_warning("Cannot read from directory: %s", error->message);
    g_error_free(error);
    return rv;
  }
  
  GFileInfo *info = g_file_enumerator_next_file(f_enum, NULL, &error);
  while (info) {
    mDirList = g_list_append(mDirList, info);
    info = g_file_enumerator_next_file(f_enum, NULL, &error);
  }
  g_object_unref(f_enum);
  if (error) {
    g_warning("Error reading directory content: %s", error->message);
    nsresult rv = MapGIOResult(error);
    g_error_free(error);
    return rv;
  }
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
  return NS_OK;
}






nsresult
nsGIOInputStream::DoOpenFile(GFileInfo *info)
{
  GError *error = NULL;

  mStream = g_file_read(mHandle, NULL, &error);
  if (!mStream) {
    nsresult rv = MapGIOResult(error);
    g_warning("Cannot read from file: %s", error->message);
    g_error_free(error);
    return rv;
  }

  const char * content_type = g_file_info_get_content_type(info);
  if (content_type) {
    char *mime_type = g_content_type_get_mime_type(content_type);
    if (mime_type) {
      if (strcmp(mime_type, APPLICATION_OCTET_STREAM) != 0) {
        SetContentTypeOfChannel(mime_type);
      }
      g_free(mime_type);
    }
  } else {
    g_warning("Missing content type.");
  }

  mBytesRemaining = g_file_info_get_size(info);
  
  
  mChannel->SetContentLength(mBytesRemaining);

  return NS_OK;
}






nsresult
nsGIOInputStream::DoOpen()
{
  nsresult rv;
  GError *error = NULL;

  NS_ASSERTION(mHandle == nsnull, "already open");

  mHandle = g_file_new_for_uri( mSpec.get() );

  GFileInfo *info = g_file_query_info(mHandle,
                                      "standard::*",
                                      G_FILE_QUERY_INFO_NONE,
                                      NULL,
                                      &error);

  if (error) {
    if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_NOT_MOUNTED) {
      
      g_error_free(error);
      if (NS_IsMainThread()) 
        return NS_ERROR_NOT_CONNECTED;
      error = NULL;
      rv = MountVolume();
      if (rv != NS_OK) {
        return rv;
      }
      
      info = g_file_query_info(mHandle,
                               "standard::*",
                               G_FILE_QUERY_INFO_NONE,
                               NULL,
                               &error);
      
      if (!info) {
        g_warning("Unable to get file info: %s", error->message);
        rv = MapGIOResult(error);
        g_error_free(error);
        return rv;
      }
    } else {
      g_warning("Unable to get file info: %s", error->message);
      rv = MapGIOResult(error);
      g_error_free(error);
      return rv;
    }
  }
  
  GFileType f_type = g_file_info_get_file_type(info);
  if (f_type == G_FILE_TYPE_DIRECTORY) {
    
    rv = DoOpenDirectory();
  } else if (f_type != G_FILE_TYPE_UNKNOWN) {
    
    rv = DoOpenFile(info);
  } else {
    g_warning("Unable to get file type.");
    rv = NS_ERROR_FILE_NOT_FOUND;
  }
  if (info)
    g_object_unref(info);
  return rv;
}









nsresult
nsGIOInputStream::DoRead(char *aBuf, PRUint32 aCount, PRUint32 *aCountRead)
{
  nsresult rv = NS_ERROR_NOT_AVAILABLE;
  if (mStream) {
    
    GError *error = NULL;    
    PRUint32 bytes_read = g_input_stream_read(G_INPUT_STREAM(mStream),
                                              aBuf,
                                              aCount,
                                              NULL,
                                              &error);
    if (error) {
      rv = MapGIOResult(error);
      *aCountRead = 0;
      g_warning("Cannot read from file: %s", error->message);
      g_error_free(error);
      return rv;
    }
    *aCountRead = bytes_read;
    mBytesRemaining -= *aCountRead;
    return NS_OK;
  }
  else if (mDirOpen) {
    
    while (aCount && rv != NS_BASE_STREAM_CLOSED)
    {
      
      PRUint32 bufLen = mDirBuf.Length() - mDirBufCursor;
      if (bufLen)
      {
        PRUint32 n = NS_MIN(bufLen, aCount);
        memcpy(aBuf, mDirBuf.get() + mDirBufCursor, n);
        *aCountRead += n;
        aBuf += n;
        aCount -= n;
        mDirBufCursor += n;
      }

      if (!mDirListPtr)    
      {
        rv = NS_BASE_STREAM_CLOSED;
      }
      else if (aCount)     
      {
        GFileInfo *info = (GFileInfo *) mDirListPtr->data;

        
        const char * fname = g_file_info_get_name(info);
        if (fname && fname[0] == '.' && 
            (fname[1] == '\0' || (fname[1] == '.' && fname[2] == '\0')))
        {
          mDirListPtr = mDirListPtr->next;
          continue;
        }

        mDirBuf.Assign("201: ");

        
        nsCString escName;
        nsCOMPtr<nsINetUtil> nu = do_GetService(NS_NETUTIL_CONTRACTID);
        if (nu && fname) {
          nu->EscapeString(nsDependentCString(fname),
                           nsINetUtil::ESCAPE_URL_PATH, escName);

          mDirBuf.Append(escName);
          mDirBuf.Append(' ');
        }

        
        
        mDirBuf.AppendInt(PRInt32(g_file_info_get_size(info)));
        mDirBuf.Append(' ');

        
        
        
        
        GTimeVal gtime;
        g_file_info_get_modification_time(info, &gtime);

        PRExplodedTime tm;
        PRTime pt = ((PRTime) gtime.tv_sec) * 1000000;
        PR_ExplodeTime(pt, PR_GMTParameters, &tm);
        {
          char buf[64];
          PR_FormatTimeUSEnglish(buf, sizeof(buf),
              "%a,%%20%d%%20%b%%20%Y%%20%H:%M:%S%%20GMT ", &tm);
          mDirBuf.Append(buf);
        }

        
        switch (g_file_info_get_file_type(info))
        {
          case G_FILE_TYPE_REGULAR:
            mDirBuf.Append("FILE ");
            break;
          case G_FILE_TYPE_DIRECTORY:
            mDirBuf.Append("DIRECTORY ");
            break;
          case G_FILE_TYPE_SYMBOLIC_LINK:
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
  return rv;
}




class nsGIOSetContentTypeEvent : public nsRunnable
{
  public:
    nsGIOSetContentTypeEvent(nsIChannel *channel, const char *contentType)
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
nsGIOInputStream::SetContentTypeOfChannel(const char *contentType)
{
  
  
  
  
  

  nsresult rv;
  nsCOMPtr<nsIRunnable> ev =
      new nsGIOSetContentTypeEvent(mChannel, contentType);
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

NS_IMPL_THREADSAFE_ISUPPORTS1(nsGIOInputStream, nsIInputStream)




NS_IMETHODIMP
nsGIOInputStream::Close()
{
  if (mStream)
  {
    g_object_unref(mStream);
    mStream = nsnull;
  }

  if (mHandle)
  {
    g_object_unref(mHandle);
    mHandle = nsnull;
  }

  if (mDirList)
  {
    
    g_list_foreach(mDirList, (GFunc) g_object_unref, nsnull);
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
nsGIOInputStream::Available(PRUint32 *aResult)
{
  if (NS_FAILED(mStatus))
    return mStatus;

  

  if (mBytesRemaining > PR_UINT32_MAX)
    *aResult = PR_UINT32_MAX;
  else
    *aResult = mBytesRemaining;

  return NS_OK;
}







NS_IMETHODIMP
nsGIOInputStream::Read(char     *aBuf,
                       PRUint32  aCount,
                       PRUint32 *aCountRead)
{
  *aCountRead = 0;
  
  if (!mStream && !mDirOpen && mStatus == NS_OK) {
    mStatus = DoOpen();
    if (NS_FAILED(mStatus)) {
      return mStatus;
    }
  }

  mStatus = DoRead(aBuf, aCount, aCountRead);
  
  if (mStatus == NS_BASE_STREAM_CLOSED)
    return NS_OK;

  
  return mStatus;
}

NS_IMETHODIMP
nsGIOInputStream::ReadSegments(nsWriteSegmentFun aWriter,
                               void             *aClosure,
                               PRUint32          aCount,
                               PRUint32         *aResult)
{
  
  
  
  NS_NOTREACHED("nsGIOInputStream::ReadSegments");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsGIOInputStream::IsNonBlocking(PRBool *aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}











static void
mount_enclosing_volume_finished (GObject *source_object,
                                 GAsyncResult *res,
                                 gpointer user_data)
{
  GError *error = NULL;

  nsGIOInputStream* istream = static_cast<nsGIOInputStream*>(user_data);
  
  g_file_mount_enclosing_volume_finish(G_FILE (source_object), res, &error);
  
  if (error) {
    g_warning("Mount failed: %s %d", error->message, error->code);
    istream->SetMountResult(MOUNT_OPERATION_FAILED, error->code);
    g_error_free(error);
  } else {
    istream->SetMountResult(MOUNT_OPERATION_SUCCESS, 0);
  }
}











static void
mount_operation_ask_password (GMountOperation   *mount_op,
                              const char        *message,
                              const char        *default_user,
                              const char        *default_domain,
                              GAskPasswordFlags flags,
                              gpointer          user_data)
{
  nsIChannel *channel = (nsIChannel *) user_data;
  if (!channel) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  
  if (flags & G_ASK_PASSWORD_NEED_DOMAIN) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }

  nsCOMPtr<nsIAuthPrompt> prompt;
  NS_QueryNotificationCallbacks(channel, prompt);

  
  
  
  if (!prompt) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  
  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));
  if (!uri) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }

  nsCAutoString scheme, hostPort;
  uri->GetScheme(scheme);
  uri->GetHostPort(hostPort);

  
  
  if (scheme.IsEmpty() || hostPort.IsEmpty()) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  
  
  
  nsAutoString key, realm;

  NS_ConvertUTF8toUTF16 dispHost(scheme);
  dispHost.Append(NS_LITERAL_STRING("://"));
  dispHost.Append(NS_ConvertUTF8toUTF16(hostPort));

  key = dispHost;
  if (*default_domain != '\0')
  {
    
    
    
    realm.Append('"');
    realm.Append(NS_ConvertASCIItoUTF16(default_domain));
    realm.Append('"');
    key.Append(' ');
    key.Append(realm);
  }
  
  
  
  
  
  nsCOMPtr<nsIStringBundleService> bundleSvc =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  if (!bundleSvc) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  nsCOMPtr<nsIStringBundle> bundle;
  bundleSvc->CreateBundle("chrome://global/locale/commonDialogs.properties",
                          getter_AddRefs(bundle));
  if (!bundle) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  nsAutoString nsmessage;

  if (flags & G_ASK_PASSWORD_NEED_PASSWORD) {
    if (flags & G_ASK_PASSWORD_NEED_USERNAME) {
      if (!realm.IsEmpty()) {
        const PRUnichar *strings[] = { realm.get(), dispHost.get() };
        bundle->FormatStringFromName(NS_LITERAL_STRING("EnterLoginForRealm").get(),
                                     strings, 2, getter_Copies(nsmessage));
      } else {
        const PRUnichar *strings[] = { dispHost.get() };
        bundle->FormatStringFromName(NS_LITERAL_STRING("EnterUserPasswordFor").get(),
                                     strings, 1, getter_Copies(nsmessage));
      }
    } else {
      NS_ConvertUTF8toUTF16 userName(default_user);
      const PRUnichar *strings[] = { userName.get(), dispHost.get() };
      bundle->FormatStringFromName(NS_LITERAL_STRING("EnterPasswordFor").get(),
                                   strings, 2, getter_Copies(nsmessage));
    }
  } else {
    g_warning("Unknown mount operation request (flags: %x)", flags);
  }

  if (nsmessage.IsEmpty()) {
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  
  nsresult rv;
  PRBool retval = PR_FALSE;
  PRUnichar *user = nsnull, *pass = nsnull;
  if (default_user) {
    
    user = ToNewUnicode(NS_ConvertUTF8toUTF16(default_user));
  }
  if (flags & G_ASK_PASSWORD_NEED_USERNAME) {
    rv = prompt->PromptUsernameAndPassword(nsnull, nsmessage.get(),
                                           key.get(),
                                           nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                           &user, &pass, &retval);
  } else {
    rv = prompt->PromptPassword(nsnull, nsmessage.get(),
                                key.get(),
                                nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                &pass, &retval);
  }
  if (NS_FAILED(rv) || !retval) {  
    g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_ABORTED);
    return;
  }
  
  g_mount_operation_set_username(mount_op, NS_ConvertUTF16toUTF8(user).get());
  g_mount_operation_set_password(mount_op, NS_ConvertUTF16toUTF8(pass).get());
  nsMemory::Free(user);
  nsMemory::Free(pass);
  g_mount_operation_reply(mount_op, G_MOUNT_OPERATION_HANDLED);
}



class nsGIOProtocolHandler : public nsIProtocolHandler
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

NS_IMPL_ISUPPORTS2(nsGIOProtocolHandler, nsIProtocolHandler, nsIObserver)

nsresult
nsGIOProtocolHandler::Init()
{
#ifdef PR_LOGGING
  sGIOLog = PR_NewLogModule("gio");
#endif

  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs)
  {
    InitSupportedProtocolsPref(prefs);
    prefs->AddObserver(MOZ_GIO_SUPPORTED_PROTOCOLS, this, PR_FALSE);
  }

  return NS_OK;
}

void
nsGIOProtocolHandler::InitSupportedProtocolsPref(nsIPrefBranch *prefs)
{
  
  
  
  
  
  nsresult rv = prefs->GetCharPref(MOZ_GIO_SUPPORTED_PROTOCOLS,
                                   getter_Copies(mSupportedProtocols));
  if (NS_SUCCEEDED(rv)) {
    mSupportedProtocols.StripWhitespace();
    ToLowerCase(mSupportedProtocols);
  }
  else
    mSupportedProtocols.Assign("smb:,sftp:"); 

  LOG(("gio: supported protocols \"%s\"\n", mSupportedProtocols.get()));
}

PRBool
nsGIOProtocolHandler::IsSupportedProtocol(const nsCString &aSpec)
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
nsGIOProtocolHandler::GetScheme(nsACString &aScheme)
{
  aScheme.Assign(MOZ_GIO_SCHEME);
  return NS_OK;
}

NS_IMETHODIMP
nsGIOProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsGIOProtocolHandler::GetProtocolFlags(PRUint32 *aProtocolFlags)
{
  
  *aProtocolFlags = URI_STD | URI_DANGEROUS_TO_LOAD;
  return NS_OK;
}

NS_IMETHODIMP
nsGIOProtocolHandler::NewURI(const nsACString &aSpec,
                             const char *aOriginCharset,
                             nsIURI *aBaseURI,
                             nsIURI **aResult)
{
  const nsCString flatSpec(aSpec);
  LOG(("gio: NewURI [spec=%s]\n", flatSpec.get()));

  if (!aBaseURI)
  {
    
    if (!IsSupportedProtocol(flatSpec))
      return NS_ERROR_UNKNOWN_PROTOCOL;

    PRInt32 colon_location = flatSpec.FindChar(':');
    if (colon_location <= 0)
      return NS_ERROR_UNKNOWN_PROTOCOL;

    
    PRBool uri_scheme_supported = PR_FALSE;

    GVfs *gvfs = g_vfs_get_default();

    if (!gvfs) {
      g_warning("Cannot get GVfs object.");
      return NS_ERROR_UNKNOWN_PROTOCOL;
    }

    const gchar* const * uri_schemes = g_vfs_get_supported_uri_schemes(gvfs);

    while (*uri_schemes != NULL) {
      
      
      if (StringHead(flatSpec, colon_location).Equals(*uri_schemes)) {
        uri_scheme_supported = PR_TRUE;
        break;
      }
      uri_schemes++;
    }

    if (!uri_scheme_supported) {
      return NS_ERROR_UNKNOWN_PROTOCOL;
    }
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
nsGIOProtocolHandler::NewChannel(nsIURI *aURI, nsIChannel **aResult)
{
  NS_ENSURE_ARG_POINTER(aURI);
  nsresult rv;

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  if (NS_FAILED(rv))
    return rv;

  nsRefPtr<nsGIOInputStream> stream = new nsGIOInputStream(spec);
  if (!stream)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    
    
    rv = NS_NewInputStreamChannel(aResult,
                                  aURI,
                                  stream,
                                  NS_LITERAL_CSTRING(UNKNOWN_CONTENT_TYPE));
    if (NS_SUCCEEDED(rv))
      stream->SetChannel(*aResult);
  }
  return rv;
}

NS_IMETHODIMP
nsGIOProtocolHandler::AllowPort(PRInt32 aPort,
                                const char *aScheme,
                                PRBool *aResult)
{
  
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsGIOProtocolHandler::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  if (strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    nsCOMPtr<nsIPrefBranch> prefs = do_QueryInterface(aSubject);
    InitSupportedProtocolsPref(prefs);
  }
  return NS_OK;
}



#define NS_GIOPROTOCOLHANDLER_CID                    \
{ /* ee706783-3af8-4d19-9e84-e2ebfe213480 */         \
    0xee706783,                                      \
    0x3af8,                                          \
    0x4d19,                                          \
    {0x9e, 0x84, 0xe2, 0xeb, 0xfe, 0x21, 0x34, 0x80} \
}

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGIOProtocolHandler, Init)
NS_DEFINE_NAMED_CID(NS_GIOPROTOCOLHANDLER_CID);

static const mozilla::Module::CIDEntry kVFSCIDs[] = {
  { &kNS_GIOPROTOCOLHANDLER_CID, false, NULL, nsGIOProtocolHandlerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kVFSContracts[] = {
  { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX MOZ_GIO_SCHEME, &kNS_GIOPROTOCOLHANDLER_CID },
  { NULL }
};

static const mozilla::Module kVFSModule = {
  mozilla::Module::kVersion,
  kVFSCIDs,
  kVFSContracts
};

NSMODULE_DEFN(nsGIOModule) = &kVFSModule;
