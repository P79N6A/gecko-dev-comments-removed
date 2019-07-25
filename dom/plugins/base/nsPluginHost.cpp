









































#include "nscore.h"
#include "nsPluginHost.h"

#include <stdio.h>
#include "prio.h"
#include "prmem.h"
#include "nsIComponentManager.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPluginStreamListener.h"
#include "nsIHTTPHeaderListener.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIObserverService.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIUploadChannel.h"
#include "nsIByteRangeRequest.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIURL.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIProtocolProxyService.h"
#include "nsIStreamConverterService.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsISeekableStream.h"
#include "nsNetUtil.h"
#include "nsIProgressEventSink.h"
#include "nsIDocument.h"
#include "nsICachingChannel.h"
#include "nsHashtable.h"
#include "nsIProxyInfo.h"
#include "nsPluginLogging.h"
#include "nsIPrefBranch2.h"
#include "nsIScriptChannel.h"
#include "nsPrintfCString.h"
#include "nsIBlocklistService.h"
#include "nsVersionComparator.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIObjectLoadingContent.h"
#include "nsIWritablePropertyBag2.h"
#include "nsPluginStreamListenerPeer.h"

#include "nsEnumeratorUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsISupportsPrimitives.h"

#include "nsIXULRuntime.h"


#include "nsIStringBundle.h"
#include "nsIWindowWatcher.h"
#include "nsPIDOMWindow.h"

#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIPrincipal.h"

#include "nsNetCID.h"
#include "nsIDOMPlugin.h"
#include "nsIDOMMimeType.h"
#include "nsMimeTypes.h"
#include "prprf.h"
#include "nsThreadUtils.h"
#include "nsIInputStreamTee.h"
#include "nsIInterfaceInfoManager.h"
#include "xptinfo.h"

#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsILocalFile.h"
#include "nsIFileChannel.h"

#include "nsPluginSafety.h"

#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"

#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsPluginDirServiceProvider.h"
#include "nsPluginError.h"

#include "nsUnicharUtils.h"
#include "nsPluginManifestLineReader.h"

#include "nsIWeakReferenceUtils.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIPresShell.h"
#include "nsIWebNavigation.h"
#include "nsISupportsArray.h"
#include "nsIDocShell.h"
#include "nsPluginNativeWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "mozilla/TimeStamp.h"

#if defined(XP_WIN)
#include "nsIWindowMediator.h"
#include "nsIBaseWindow.h"
#include "windows.h"
#include "winbase.h"
#endif

using namespace mozilla;
using mozilla::TimeStamp;



#define NS_ITERATIVE_UNREF_LIST(type_, list_, mNext_)                \
  {                                                                  \
    while (list_) {                                                  \
      type_ temp = list_->mNext_;                                    \
      list_->mNext_ = nsnull;                                        \
      list_ = temp;                                                  \
    }                                                                \
  }



#define kPluginTmpDirName NS_LITERAL_CSTRING("plugtmp")


















static const char *kPluginRegistryVersion = "0.15";

static const char *kMinimumRegistryVersion = "0.9";

static NS_DEFINE_IID(kIPluginTagInfoIID, NS_IPLUGINTAGINFO_IID);
static const char kDirectoryServiceContractID[] = "@mozilla.org/file/directory_service;1";


static const char kPluginsRootKey[] = "software/plugins";
static const char kPluginsNameKey[] = "name";
static const char kPluginsDescKey[] = "description";
static const char kPluginsFilenameKey[] = "filename";
static const char kPluginsFullpathKey[] = "fullpath";
static const char kPluginsModTimeKey[] = "lastModTimeStamp";
static const char kPluginsCanUnload[] = "canUnload";
static const char kPluginsVersionKey[] = "version";
static const char kPluginsMimeTypeKey[] = "mimetype";
static const char kPluginsMimeDescKey[] = "description";
static const char kPluginsMimeExtKey[] = "extension";

#define kPluginRegistryFilename NS_LITERAL_CSTRING("pluginreg.dat")

#ifdef PLUGIN_LOGGING
PRLogModuleInfo* nsPluginLogging::gNPNLog = nsnull;
PRLogModuleInfo* nsPluginLogging::gNPPLog = nsnull;
PRLogModuleInfo* nsPluginLogging::gPluginLog = nsnull;
#endif

#define BRAND_PROPERTIES_URL "chrome://branding/locale/brand.properties"
#define PLUGIN_PROPERTIES_URL "chrome://global/locale/downloadProgress.properties"


#define NS_PREF_MAX_NUM_CACHED_PLUGINS "browser.plugins.max_num_cached_plugins"
#define DEFAULT_NUMBER_OF_STOPPED_PLUGINS 10

#ifdef CALL_SAFETY_ON

PRBool gSkipPluginSafeCalls = PR_TRUE;
#endif

nsIFile *nsPluginHost::sPluginTempDir;
nsPluginHost *nsPluginHost::sInst;

NS_IMPL_ISUPPORTS0(nsInvalidPluginTag)

nsInvalidPluginTag::nsInvalidPluginTag(const char* aFullPath, PRInt64 aLastModifiedTime)
: mFullPath(aFullPath),
  mLastModifiedTime(aLastModifiedTime),
  mSeen(false)
{
  
}

nsInvalidPluginTag::~nsInvalidPluginTag()
{
  
}


static
PRBool ReadSectionHeader(nsPluginManifestLineReader& reader, const char *token)
{
  do {
    if (*reader.LinePtr() == '[') {
      char* p = reader.LinePtr() + (reader.LineLength() - 1);
      if (*p != ']')
        break;
      *p = 0;

      char* values[1];
      if (1 != reader.ParseLine(values, 1))
        break;
      
      if (PL_strcmp(values[0]+1, token)) {
        break; 
      }
      return PR_TRUE;
    }
  } while (reader.NextLine());
  return PR_FALSE;
}




class nsPluginDocReframeEvent: public nsRunnable {
public:
  nsPluginDocReframeEvent(nsISupportsArray* aDocs) { mDocs = aDocs; }

  NS_DECL_NSIRUNNABLE

  nsCOMPtr<nsISupportsArray> mDocs;
};

NS_IMETHODIMP nsPluginDocReframeEvent::Run() {
  NS_ENSURE_TRUE(mDocs, NS_ERROR_FAILURE);

  PRUint32 c;
  mDocs->Count(&c);

  
  
  for (PRUint32 i = 0; i < c; i++) {
    nsCOMPtr<nsIDocument> doc (do_QueryElementAt(mDocs, i));
    if (doc) {
      nsIPresShell *shell = doc->GetShell();

      
      if (shell) {
        








        shell->ReconstructFrames(); 
      } else {  

        NS_NOTREACHED("all plugins should have a pres shell!");

      }
    }
  }

  return mDocs->Clear();
}

static PRBool UnloadPluginsASAP()
{
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
    PRBool unloadPluginsASAP = PR_FALSE;
    rv = pref->GetBoolPref("plugins.unloadASAP", &unloadPluginsASAP);
    if (NS_SUCCEEDED(rv)) {
      return unloadPluginsASAP;
    }
  }

  return PR_FALSE;
}


class nsPluginUnloadEvent : public nsRunnable {
public:
  nsPluginUnloadEvent(PRLibrary* aLibrary)
    : mLibrary(aLibrary)
  {}
 
  NS_DECL_NSIRUNNABLE
 
  PRLibrary* mLibrary;
};

NS_IMETHODIMP nsPluginUnloadEvent::Run()
{
  if (mLibrary) {
    
    NS_TRY_SAFE_CALL_VOID(PR_UnloadLibrary(mLibrary), nsnull);
  } else {
    NS_WARNING("missing library from nsPluginUnloadEvent");
  }
  return NS_OK;
}


nsresult nsPluginHost::PostPluginUnloadEvent(PRLibrary* aLibrary)
{
  nsCOMPtr<nsIRunnable> ev = new nsPluginUnloadEvent(aLibrary);
  if (ev && NS_SUCCEEDED(NS_DispatchToCurrentThread(ev)))
    return NS_OK;

  
  NS_TRY_SAFE_CALL_VOID(PR_UnloadLibrary(aLibrary), nsnull);

  return NS_ERROR_FAILURE;
}

nsPluginHost::nsPluginHost()
  
  
{
  
  
  mPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (mPrefService) {
    PRBool tmp;
    nsresult rv = mPrefService->GetBoolPref("plugin.override_internal_types",
                                            &tmp);
    if (NS_SUCCEEDED(rv)) {
      mOverrideInternalTypes = tmp;
    }

    rv = mPrefService->GetBoolPref("plugin.disable", &tmp);
    if (NS_SUCCEEDED(rv)) {
      mPluginsDisabled = tmp;
    }
  }

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    obsService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_FALSE);
  }

#ifdef PLUGIN_LOGGING
  nsPluginLogging::gNPNLog = PR_NewLogModule(NPN_LOG_NAME);
  nsPluginLogging::gNPPLog = PR_NewLogModule(NPP_LOG_NAME);
  nsPluginLogging::gPluginLog = PR_NewLogModule(PLUGIN_LOG_NAME);

  PR_LOG(nsPluginLogging::gNPNLog, PLUGIN_LOG_ALWAYS,("NPN Logging Active!\n"));
  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_ALWAYS,("General Plugin Logging Active! (nsPluginHost::ctor)\n"));
  PR_LOG(nsPluginLogging::gNPPLog, PLUGIN_LOG_ALWAYS,("NPP Logging Active!\n"));

  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHost::ctor\n"));
  PR_LogFlush();
#endif

#ifdef MAC_CARBON_PLUGINS
  mVisiblePluginTimer = do_CreateInstance("@mozilla.org/timer;1");
  mHiddenPluginTimer = do_CreateInstance("@mozilla.org/timer;1");
#endif
}

nsPluginHost::~nsPluginHost()
{
  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHost::dtor\n"));

  Destroy();
  sInst = nsnull;
}

NS_IMPL_ISUPPORTS4(nsPluginHost,
                   nsIPluginHost,
                   nsIObserver,
                   nsITimerCallback,
                   nsISupportsWeakReference)

nsPluginHost*
nsPluginHost::GetInst()
{
  if (!sInst) {
    sInst = new nsPluginHost();
    if (!sInst)
      return nsnull;
    NS_ADDREF(sInst);
  }

  NS_ADDREF(sInst);
  return sInst;
}

PRBool nsPluginHost::IsRunningPlugin(nsPluginTag * plugin)
{
  if (!plugin || !plugin->mEntryPoint) {
    return PR_FALSE;
  }

  for (PRUint32 i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance *instance = mInstances[i].get();
    if (instance &&
        instance->GetPlugin() == plugin->mEntryPoint &&
        instance->IsRunning()) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsresult nsPluginHost::ReloadPlugins(PRBool reloadPages)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::ReloadPlugins Begin reloadPages=%d, active_instance_count=%d\n",
  reloadPages, mInstances.Length()));

  nsresult rv = NS_OK;

  
  
  if (!mPluginsLoaded)
    return LoadPlugins();

  
  
  

  
  
  
  
  PRBool pluginschanged = PR_TRUE;
  FindPlugins(PR_FALSE, &pluginschanged);

  
  if (!pluginschanged)
    return NS_ERROR_PLUGINS_PLUGINSNOTCHANGED;

  nsCOMPtr<nsISupportsArray> instsToReload;
  if (reloadPages) {
    NS_NewISupportsArray(getter_AddRefs(instsToReload));

    
    
    DestroyRunningInstances(instsToReload, nsnull);
  }

  
  nsRefPtr<nsPluginTag> prev;
  nsRefPtr<nsPluginTag> next;

  for (nsRefPtr<nsPluginTag> p = mPlugins; p != nsnull;) {
    next = p->mNext;

    
    if (!IsRunningPlugin(p)) {
      if (p == mPlugins)
        mPlugins = next;
      else
        prev->mNext = next;

      p->mNext = nsnull;

      
      p->TryUnloadPlugin();

      p = next;
      continue;
    }

    prev = p;
    p = next;
  }

  
  mPluginsLoaded = PR_FALSE;

  
  rv = LoadPlugins();

  
  
  
  PRUint32 c;
  if (reloadPages &&
      instsToReload &&
      NS_SUCCEEDED(instsToReload->Count(&c)) &&
      c > 0) {
    nsCOMPtr<nsIRunnable> ev = new nsPluginDocReframeEvent(instsToReload);
    if (ev)
      NS_DispatchToCurrentThread(ev);
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::ReloadPlugins End active_instance_count=%d\n",
  mInstances.Length()));

  return rv;
}

#define NS_RETURN_UASTRING_SIZE 128

nsresult nsPluginHost::UserAgent(const char **retstring)
{
  static char resultString[NS_RETURN_UASTRING_SIZE];
  nsresult res;

  nsCOMPtr<nsIHttpProtocolHandler> http = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &res);
  if (NS_FAILED(res))
    return res;

  nsCAutoString uaString;
  res = http->GetUserAgent(uaString);

  if (NS_SUCCEEDED(res)) {
    if (NS_RETURN_UASTRING_SIZE > uaString.Length()) {
      PL_strcpy(resultString, uaString.get());
    } else {
      
      PL_strncpy(resultString, uaString.get(), NS_RETURN_UASTRING_SIZE);
      for (int i = NS_RETURN_UASTRING_SIZE - 1; i >= 0; i--) {
        if (i == 0) {
          resultString[NS_RETURN_UASTRING_SIZE - 1] = '\0';
        }
        else if (resultString[i] == ' ') {
          resultString[i] = '\0';
          break;
        }
      }
    }
    *retstring = resultString;
  }
  else {
    *retstring = nsnull;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHost::UserAgent return=%s\n", *retstring));

  return res;
}

nsresult nsPluginHost::GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt)
{
  nsresult rv;
  nsCOMPtr<nsIPrompt> prompt;
  nsCOMPtr<nsIWindowWatcher> wwatch = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);

  if (wwatch) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    if (aOwner) {
      nsCOMPtr<nsIDocument> document;
      aOwner->GetDocument(getter_AddRefs(document));
      if (document) {
        domWindow = document->GetWindow();
      }
    }

    if (!domWindow) {
      wwatch->GetWindowByName(NS_LITERAL_STRING("_content").get(), nsnull, getter_AddRefs(domWindow));
    }
    rv = wwatch->GetNewPrompter(domWindow, getter_AddRefs(prompt));
  }

  NS_IF_ADDREF(*aPrompt = prompt);
  return rv;
}

NS_IMETHODIMP nsPluginHost::GetURL(nsISupports* pluginInst,
                                   const char* url,
                                   const char* target,
                                   nsIPluginStreamListener* streamListener,
                                   const char* altHost,
                                   const char* referrer,
                                   PRBool forceJSEnabled)
{
  return GetURLWithHeaders(static_cast<nsNPAPIPluginInstance*>(pluginInst),
                           url, target, streamListener, altHost, referrer,
                           forceJSEnabled, nsnull, nsnull);
}

nsresult nsPluginHost::GetURLWithHeaders(nsNPAPIPluginInstance* pluginInst,
                                         const char* url,
                                         const char* target,
                                         nsIPluginStreamListener* streamListener,
                                         const char* altHost,
                                         const char* referrer,
                                         PRBool forceJSEnabled,
                                         PRUint32 getHeadersLength,
                                         const char* getHeaders)
{
  nsAutoString string;
  string.AssignWithConversion(url);

  
  
  if (!target && !streamListener)
    return NS_ERROR_ILLEGAL_VALUE;

  nsresult rv = DoURLLoadSecurityCheck(pluginInst, url);
  if (NS_FAILED(rv))
    return rv;

  if (target) {
    nsCOMPtr<nsIPluginInstanceOwner> owner;
    rv = pluginInst->GetOwner(getter_AddRefs(owner));
    if (owner) {
      if ((0 == PL_strcmp(target, "newwindow")) ||
          (0 == PL_strcmp(target, "_new")))
        target = "_blank";
      else if (0 == PL_strcmp(target, "_current"))
        target = "_self";

      rv = owner->GetURL(url, target, nsnull, nsnull, 0);
    }
  }

  if (streamListener)
    rv = NewPluginURLStream(string, pluginInst, streamListener, nsnull,
                            getHeaders, getHeadersLength);

  return rv;
}

NS_IMETHODIMP nsPluginHost::PostURL(nsISupports* pluginInst,
                                    const char* url,
                                    PRUint32 postDataLen,
                                    const char* postData,
                                    PRBool isFile,
                                    const char* target,
                                    nsIPluginStreamListener* streamListener,
                                    const char* altHost,
                                    const char* referrer,
                                    PRBool forceJSEnabled,
                                    PRUint32 postHeadersLength,
                                    const char* postHeaders)
{
  nsAutoString string;
  nsresult rv;

  string.AssignWithConversion(url);

  
  
  
  if (!target && !streamListener)
    return NS_ERROR_ILLEGAL_VALUE;

  nsNPAPIPluginInstance* instance = static_cast<nsNPAPIPluginInstance*>(pluginInst);

  rv = DoURLLoadSecurityCheck(instance, url);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIInputStream> postStream;
  if (isFile) {
    nsCOMPtr<nsIFile> file;
    rv = CreateTempFileToPost(postData, getter_AddRefs(file));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIInputStream> fileStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(fileStream),
                                    file,
                                    PR_RDONLY,
                                    0600,
                                    nsIFileInputStream::DELETE_ON_CLOSE |
                                    nsIFileInputStream::CLOSE_ON_EOF);
    if (NS_FAILED(rv))
      return rv;

    rv = NS_NewBufferedInputStream(getter_AddRefs(postStream), fileStream, 8192);
    if (NS_FAILED(rv))
      return rv;
  } else {
    char *dataToPost;
    PRUint32 newDataToPostLen;
    ParsePostBufferToFixHeaders(postData, postDataLen, &dataToPost, &newDataToPostLen);
    if (!dataToPost)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIStringInputStream> sis = do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
    if (!sis) {
      NS_Free(dataToPost);
      return rv;
    }

    
    
    postDataLen = newDataToPostLen;
    sis->AdoptData(dataToPost, postDataLen);
    postStream = sis;
  }

  if (target) {
    nsCOMPtr<nsIPluginInstanceOwner> owner;
    rv = instance->GetOwner(getter_AddRefs(owner));
    if (owner) {
      if ((0 == PL_strcmp(target, "newwindow")) ||
          (0 == PL_strcmp(target, "_new"))) {
        target = "_blank";
      } else if (0 == PL_strcmp(target, "_current")) {
        target = "_self";
      }
      rv = owner->GetURL(url, target, postStream,
                         (void*)postHeaders, postHeadersLength);
    }
  }

  
  
  if (streamListener)
    rv = NewPluginURLStream(string, instance, streamListener,
                            postStream, postHeaders, postHeadersLength);

  return rv;
}











NS_IMETHODIMP nsPluginHost::FindProxyForURL(const char* url, char* *result)
{
  if (!url || !result) {
    return NS_ERROR_INVALID_ARG;
  }
  nsresult res;

  nsCOMPtr<nsIURI> uriIn;
  nsCOMPtr<nsIProtocolProxyService> proxyService;
  nsCOMPtr<nsIIOService> ioService;

  proxyService = do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res) || !proxyService)
    return res;

  ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res) || !ioService)
    return res;

  
  res = ioService->NewURI(nsDependentCString(url), nsnull, nsnull, getter_AddRefs(uriIn));
  if (NS_FAILED(res))
    return res;

  nsCOMPtr<nsIProxyInfo> pi;

  res = proxyService->Resolve(uriIn, 0, getter_AddRefs(pi));
  if (NS_FAILED(res))
    return res;

  nsCAutoString host, type;
  PRInt32 port = -1;

  
  if (pi) {
    pi->GetType(type);
    pi->GetHost(host);
    pi->GetPort(&port);
  }

  if (!pi || host.IsEmpty() || port <= 0 || host.EqualsLiteral("direct")) {
    *result = PL_strdup("DIRECT");
  } else if (type.EqualsLiteral("http")) {
    *result = PR_smprintf("PROXY %s:%d", host.get(), port);
  } else if (type.EqualsLiteral("socks4")) {
    *result = PR_smprintf("SOCKS %s:%d", host.get(), port);
  } else if (type.EqualsLiteral("socks")) {
    
    
    
    
    
    *result = PR_smprintf("SOCKS %s:%d", host.get(), port);
  } else {
    NS_ASSERTION(PR_FALSE, "Unknown proxy type!");
    *result = PL_strdup("DIRECT");
  }

  if (nsnull == *result)
    res = NS_ERROR_OUT_OF_MEMORY;

  return res;
}

NS_IMETHODIMP nsPluginHost::Init()
{
  return NS_OK;
}

NS_IMETHODIMP nsPluginHost::Destroy()
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHost::Destroy Called\n"));

  if (mIsDestroyed)
    return NS_OK;

  mIsDestroyed = PR_TRUE;

  
  
  DestroyRunningInstances(nsnull, nsnull);

  nsPluginTag *pluginTag;
  for (pluginTag = mPlugins; pluginTag; pluginTag = pluginTag->mNext) {
    pluginTag->TryUnloadPlugin();
  }

  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);

  
  if (sPluginTempDir) {
    sPluginTempDir->Remove(PR_TRUE);
    NS_RELEASE(sPluginTempDir);
  }

#ifdef XP_WIN
  if (mPrivateDirServiceProvider) {
    nsCOMPtr<nsIDirectoryService> dirService =
      do_GetService(kDirectoryServiceContractID);
    if (dirService)
      dirService->UnregisterProvider(mPrivateDirServiceProvider);
    mPrivateDirServiceProvider = nsnull;
  }
#endif 

  mPrefService = nsnull; 

  return NS_OK;
}

void nsPluginHost::OnPluginInstanceDestroyed(nsPluginTag* aPluginTag)
{
  PRBool hasInstance = PR_FALSE;
  for (PRUint32 i = 0; i < mInstances.Length(); i++) {
    if (TagForPlugin(mInstances[i]->GetPlugin()) == aPluginTag) {
      hasInstance = PR_TRUE;
      break;
    }
  }

  if (!hasInstance && UnloadPluginsASAP()) {
    aPluginTag->TryUnloadPlugin();
  }
}

nsresult
nsPluginHost::GetPluginTempDir(nsIFile **aDir)
{
  if (!sPluginTempDir) {
    nsCOMPtr<nsIFile> tmpDir;
    nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                         getter_AddRefs(tmpDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = tmpDir->AppendNative(kPluginTmpDirName);

    
    rv = tmpDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
    NS_ENSURE_SUCCESS(rv, rv);

    tmpDir.swap(sPluginTempDir);
  }

  return sPluginTempDir->Clone(aDir);
}

NS_IMETHODIMP nsPluginHost::InstantiatePluginForChannel(nsIChannel* aChannel,
                                                        nsIPluginInstanceOwner* aOwner,
                                                        nsIStreamListener** aListener)
{
  NS_PRECONDITION(aChannel && aOwner,
                  "Invalid arguments to InstantiatePluginForChannel");
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return rv;

#ifdef PLUGIN_LOGGING
  if (PR_LOG_TEST(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL)) {
    nsCAutoString urlSpec;
    uri->GetAsciiSpec(urlSpec);

    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
           ("nsPluginHost::InstantiatePluginForChannel Begin owner=%p, url=%s\n",
           aOwner, urlSpec.get()));

    PR_LogFlush();
  }
#endif

  
  
  

  return NewEmbeddedPluginStreamListener(uri, aOwner, nsnull, aListener);
}

nsresult
nsPluginHost::InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI* aURL,
                                        nsIPluginInstanceOwner* aOwner,
                                        PRBool aAllowOpeningStreams)
{
  NS_ENSURE_ARG_POINTER(aOwner);

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if (aURL)
    aURL->GetAsciiSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::InstantiateEmbeddedPlugin Begin mime=%s, owner=%p, url=%s\n",
        aMimeType, aOwner, urlSpec.get()));

  PR_LogFlush();
#endif

  nsresult rv;
  nsCOMPtr<nsIPluginTagInfo> pti;
  nsPluginTagType tagType;

  rv = aOwner->QueryInterface(kIPluginTagInfoIID, getter_AddRefs(pti));

  if (rv != NS_OK)
    return rv;

  rv = pti->GetTagType(&tagType);

  if ((rv != NS_OK) || !((tagType == nsPluginTagType_Embed)
                        || (tagType == nsPluginTagType_Applet)
                        || (tagType == nsPluginTagType_Object))) {
    return rv;
  }

  
  
  
  
  
  
  
  if (aURL && aAllowOpeningStreams) {
    nsCOMPtr<nsIScriptSecurityManager> secMan =
                    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return rv; 

    nsCOMPtr<nsIDocument> doc;
    aOwner->GetDocument(getter_AddRefs(doc));
    if (!doc)
      return NS_ERROR_NULL_POINTER;

    rv = secMan->CheckLoadURIWithPrincipal(doc->NodePrincipal(), aURL, 0);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIDOMElement> elem;
    pti->GetDOMElement(getter_AddRefs(elem));

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT; 
    nsresult rv =
      NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT,
                                aURL,
                                doc->NodePrincipal(),
                                elem,
                                nsDependentCString(aMimeType ? aMimeType : ""),
                                nsnull, 
                                &shouldLoad);
    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad))
      return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
  }

  PRBool isJava = PR_FALSE;
  nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
  if (pluginTag) {
    isJava = pluginTag->mIsJavaPlugin;
  }

  
  
  
  
  
  PRBool bCanHandleInternally = PR_FALSE;
  nsCAutoString scheme;
  if (aURL && aAllowOpeningStreams && NS_SUCCEEDED(aURL->GetScheme(scheme))) {
      nsCAutoString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
      contractID += scheme;
      ToLowerCase(contractID);
      nsCOMPtr<nsIProtocolHandler> handler = do_GetService(contractID.get());
      if (handler)
        bCanHandleInternally = PR_TRUE;
  }

  if (FindStoppedPluginForURL(aURL, aOwner) == NS_OK) {
    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginHost::InstantiateEmbeddedPlugin FoundStopped mime=%s\n", aMimeType));

    nsCOMPtr<nsIPluginInstance> instanceCOMPtr;
    aOwner->GetInstance(getter_AddRefs(instanceCOMPtr));
    nsNPAPIPluginInstance *instance = static_cast<nsNPAPIPluginInstance*>(instanceCOMPtr.get());
    if (!isJava && bCanHandleInternally)
      rv = NewEmbeddedPluginStream(aURL, aOwner, instance);

    return NS_OK;
  }

  
  
  if (!aMimeType)
    return bCanHandleInternally ? NewEmbeddedPluginStream(aURL, aOwner, nsnull) : NS_ERROR_FAILURE;

  rv = SetUpPluginInstance(aMimeType, aURL, aOwner);

  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPluginInstance> instanceCOMPtr;
  rv = aOwner->GetInstance(getter_AddRefs(instanceCOMPtr));
  
  
  if (rv == NS_ERROR_FAILURE)
    return rv;

  
  nsNPAPIPluginInstance *instance = static_cast<nsNPAPIPluginInstance*>(instanceCOMPtr.get());

  if (instance) {
    instance->Start();
    aOwner->CreateWidget();

    
    aOwner->SetWindow();

    
    
    PRBool havedata = PR_FALSE;

    nsCOMPtr<nsIPluginTagInfo> pti(do_QueryInterface(aOwner, &rv));

    if (pti) {
      const char *value;
      havedata = NS_SUCCEEDED(pti->GetAttribute("SRC", &value));
      
    }

    if (havedata && !isJava && bCanHandleInternally)
      rv = NewEmbeddedPluginStream(aURL, aOwner, instance);
  }

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec2;
  if (aURL != nsnull) aURL->GetAsciiSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::InstantiateEmbeddedPlugin Finished mime=%s, rv=%d, owner=%p, url=%s\n",
        aMimeType, rv, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

  return rv;
}

nsresult nsPluginHost::InstantiateFullPagePlugin(const char *aMimeType,
                                                 nsIURI* aURI,
                                                 nsIPluginInstanceOwner *aOwner,
                                                 nsIStreamListener **aStreamListener)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  aURI->GetSpec(urlSpec);
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::InstantiateFullPagePlugin Begin mime=%s, owner=%p, url=%s\n",
  aMimeType, aOwner, urlSpec.get()));
#endif

  if (FindStoppedPluginForURL(aURI, aOwner) == NS_OK) {
    PLUGIN_LOG(PLUGIN_LOG_NOISY,
    ("nsPluginHost::InstantiateFullPagePlugin FoundStopped mime=%s\n",aMimeType));


    nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
    if (!pluginTag || !pluginTag->mIsJavaPlugin) {
      nsCOMPtr<nsIPluginInstance> instanceCOMPtr;
      aOwner->GetInstance(getter_AddRefs(instanceCOMPtr));
      NewFullPagePluginStream(aURI, static_cast<nsNPAPIPluginInstance*>(instanceCOMPtr.get()), aStreamListener);
    }
    return NS_OK;
  }

  nsresult rv = SetUpPluginInstance(aMimeType, aURI, aOwner);

  if (NS_OK == rv) {
    nsCOMPtr<nsIPluginInstance> instanceCOMPtr;
    aOwner->GetInstance(getter_AddRefs(instanceCOMPtr));
    nsNPAPIPluginInstance *instance = static_cast<nsNPAPIPluginInstance*>(instanceCOMPtr.get());

    NPWindow* win = nsnull;
    aOwner->GetWindow(win);

    if (win && instance) {
      instance->Start();
      aOwner->CreateWidget();

      
      aOwner->SetWindow();

      rv = NewFullPagePluginStream(aURI, instance, aStreamListener);

      
      aOwner->SetWindow();
    }
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::InstantiateFullPagePlugin End mime=%s, rv=%d, owner=%p, url=%s\n",
  aMimeType, rv, aOwner, urlSpec.get()));

  return rv;
}

nsPluginTag*
nsPluginHost::FindTagForLibrary(PRLibrary* aLibrary)
{
  nsPluginTag* pluginTag;
  for (pluginTag = mPlugins; pluginTag; pluginTag = pluginTag->mNext) {
    if (pluginTag->mLibrary == aLibrary) {
      return pluginTag;
    }
  }
  return nsnull;
}

nsPluginTag*
nsPluginHost::TagForPlugin(nsNPAPIPlugin* aPlugin)
{
  nsPluginTag* pluginTag;
  for (pluginTag = mPlugins; pluginTag; pluginTag = pluginTag->mNext) {
    if (pluginTag->mEntryPoint == aPlugin) {
      return pluginTag;
    }
  }
  
  NS_ERROR("TagForPlugin has failed");
  return nsnull;
}

nsresult nsPluginHost::FindStoppedPluginForURL(nsIURI* aURL,
                                               nsIPluginInstanceOwner *aOwner)
{
  nsCAutoString url;
  if (!aURL)
    return NS_ERROR_FAILURE;

  aURL->GetAsciiSpec(url);

  nsNPAPIPluginInstance *instance = FindStoppedInstance(url.get());
  if (instance && !instance->IsRunning()) {
    aOwner->SetInstance(instance);
    instance->SetOwner(aOwner);

    instance->Start();
    aOwner->CreateWidget();

    
    aOwner->SetWindow();

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsPluginHost::SetUpPluginInstance(const char *aMimeType,
                                                nsIURI *aURL,
                                                nsIPluginInstanceOwner *aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);

  nsresult rv = NS_OK;

  rv = TrySetUpPluginInstance(aMimeType, aURL, aOwner);

  
  
  if (NS_FAILED(rv)) {
    
    
    
    
    nsCOMPtr<nsIDocument> document;
    aOwner->GetDocument(getter_AddRefs(document));

    nsCOMPtr<nsIDocument> currentdocument = do_QueryReferent(mCurrentDocument);
    if (document == currentdocument)
      return rv;

    mCurrentDocument = do_GetWeakReference(document);

    
    
    if (NS_ERROR_PLUGINS_PLUGINSNOTCHANGED == ReloadPlugins(PR_FALSE))
      return rv;

    
    aOwner->SetInstance(nsnull); 
    rv = TrySetUpPluginInstance(aMimeType, aURL, aOwner);
  }

  return rv;
}

nsresult
nsPluginHost::TrySetUpPluginInstance(const char *aMimeType,
                                     nsIURI *aURL,
                                     nsIPluginInstanceOwner *aOwner)
{
#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec;
  if (aURL != nsnull) aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::TrySetupPluginInstance Begin mime=%s, owner=%p, url=%s\n",
        aMimeType, aOwner, urlSpec.get()));

  PR_LogFlush();
#endif

  nsresult rv = NS_ERROR_FAILURE;
  
  const char* mimetype = nsnull;

  
  
  nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
  if (!pluginTag) {
    nsCOMPtr<nsIURL> url = do_QueryInterface(aURL);
    if (!url) return NS_ERROR_FAILURE;

    nsCAutoString fileExtension;
    url->GetFileExtension(fileExtension);

    
    
    if (fileExtension.IsEmpty() ||
        !(pluginTag = FindPluginEnabledForExtension(fileExtension.get(),
                                                    mimetype))) {
      return NS_ERROR_FAILURE;
    }
  }
  else {
    mimetype = aMimeType;
  }

  NS_ASSERTION(pluginTag, "Must have plugin tag here!");

  nsRefPtr<nsNPAPIPlugin> plugin;
  GetPlugin(mimetype, getter_AddRefs(plugin));

  nsCOMPtr<nsIPluginInstance> instance;
  if (plugin) {
#if defined(XP_WIN)
    static BOOL firstJavaPlugin = FALSE;
    BOOL restoreOrigDir = FALSE;
    WCHAR origDir[_MAX_PATH];
    if (pluginTag->mIsJavaPlugin && !firstJavaPlugin) {
      DWORD dw = GetCurrentDirectoryW(_MAX_PATH, origDir);
      NS_ASSERTION(dw <= _MAX_PATH, "Failed to obtain the current directory, which may lead to incorrect class loading");
      nsCOMPtr<nsIFile> binDirectory;
      rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                  getter_AddRefs(binDirectory));

      if (NS_SUCCEEDED(rv)) {
        nsAutoString path;
        binDirectory->GetPath(path);
        restoreOrigDir = SetCurrentDirectoryW(path.get());
      }
    }
#endif

    rv = plugin->CreatePluginInstance(getter_AddRefs(instance));

#if defined(XP_WIN)
    if (!firstJavaPlugin && restoreOrigDir) {
      BOOL bCheck = SetCurrentDirectoryW(origDir);
      NS_ASSERTION(bCheck, "Error restoring directory");
      firstJavaPlugin = TRUE;
    }
#endif
  }

  if (NS_FAILED(rv))
    return rv;

  
  aOwner->SetInstance(instance);

  
  
  
  rv = instance->Initialize(aOwner, mimetype);
  if (NS_FAILED(rv)) {
    aOwner->SetInstance(nsnull);
    return rv;
  }

  mInstances.AppendElement(static_cast<nsNPAPIPluginInstance*>(instance.get()));

#ifdef PLUGIN_LOGGING
  nsCAutoString urlSpec2;
  if (aURL)
    aURL->GetSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
        ("nsPluginHost::TrySetupPluginInstance Finished mime=%s, rv=%d, owner=%p, url=%s\n",
        aMimeType, rv, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

  return rv;
}

NS_IMETHODIMP
nsPluginHost::IsPluginEnabledForType(const char* aMimeType)
{
  nsPluginTag *plugin = FindPluginForType(aMimeType, PR_TRUE);
  if (plugin)
    return NS_OK;

  
  
  plugin = FindPluginForType(aMimeType, PR_FALSE);
  if (!plugin)
    return NS_ERROR_FAILURE;

  if (!plugin->IsEnabled()) {
    if (plugin->HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED))
      return NS_ERROR_PLUGIN_BLOCKLISTED;
    else
      return NS_ERROR_PLUGIN_DISABLED;
  }

  return NS_OK;
}


static int CompareExtensions(const char *aExtensionList, const char *aExtension)
{
  if (!aExtensionList || !aExtension)
    return -1;

  const char *pExt = aExtensionList;
  const char *pComma = strchr(pExt, ',');
  if (!pComma)
    return PL_strcasecmp(pExt, aExtension);

  int extlen = strlen(aExtension);
  while (pComma) {
    int length = pComma - pExt;
    if (length == extlen && 0 == PL_strncasecmp(aExtension, pExt, length))
      return 0;
    pComma++;
    pExt = pComma;
    pComma = strchr(pExt, ',');
  }

  
  return PL_strcasecmp(pExt, aExtension);
}

NS_IMETHODIMP
nsPluginHost::IsPluginEnabledForExtension(const char* aExtension,
                                          const char* &aMimeType)
{
  nsPluginTag *plugin = FindPluginEnabledForExtension(aExtension, aMimeType);
  return plugin ? NS_OK : NS_ERROR_FAILURE;
}

class DOMMimeTypeImpl : public nsIDOMMimeType {
public:
  NS_DECL_ISUPPORTS

  DOMMimeTypeImpl(nsPluginTag* aTag, PRUint32 aMimeTypeIndex)
  {
    if (!aTag)
      return;
    CopyUTF8toUTF16(aTag->mMimeDescriptions[aMimeTypeIndex], mDescription);
    CopyUTF8toUTF16(aTag->mExtensions[aMimeTypeIndex], mSuffixes);
    CopyUTF8toUTF16(aTag->mMimeTypes[aMimeTypeIndex], mType);
  }

  virtual ~DOMMimeTypeImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    aDescription.Assign(mDescription);
    return NS_OK;
  }

  NS_METHOD GetEnabledPlugin(nsIDOMPlugin** aEnabledPlugin)
  {
    
    *aEnabledPlugin = nsnull;
    return NS_OK;
  }

  NS_METHOD GetSuffixes(nsAString& aSuffixes)
  {
    aSuffixes.Assign(mSuffixes);
    return NS_OK;
  }

  NS_METHOD GetType(nsAString& aType)
  {
    aType.Assign(mType);
    return NS_OK;
  }

private:
  nsString mDescription;
  nsString mSuffixes;
  nsString mType;
};

NS_IMPL_ISUPPORTS1(DOMMimeTypeImpl, nsIDOMMimeType)

class DOMPluginImpl : public nsIDOMPlugin {
public:
  NS_DECL_ISUPPORTS

  DOMPluginImpl(nsPluginTag* aPluginTag) : mPluginTag(aPluginTag)
  {
  }

  virtual ~DOMPluginImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    CopyUTF8toUTF16(mPluginTag.mDescription, aDescription);
    return NS_OK;
  }

  NS_METHOD GetFilename(nsAString& aFilename)
  {
    PRBool bShowPath;
    nsCOMPtr<nsIPrefBranch> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefService &&
        NS_SUCCEEDED(prefService->GetBoolPref("plugin.expose_full_path", &bShowPath)) &&
        bShowPath) {
      CopyUTF8toUTF16(mPluginTag.mFullPath, aFilename);
    } else {
      CopyUTF8toUTF16(mPluginTag.mFileName, aFilename);
    }

    return NS_OK;
  }

  NS_METHOD GetVersion(nsAString& aVersion)
  {
    CopyUTF8toUTF16(mPluginTag.mVersion, aVersion);
    return NS_OK;
  }

  NS_METHOD GetName(nsAString& aName)
  {
    CopyUTF8toUTF16(mPluginTag.mName, aName);
    return NS_OK;
  }

  NS_METHOD GetLength(PRUint32* aLength)
  {
    *aLength = mPluginTag.mMimeTypes.Length();
    return NS_OK;
  }

  NS_METHOD Item(PRUint32 aIndex, nsIDOMMimeType** aReturn)
  {
    nsIDOMMimeType* mimeType = new DOMMimeTypeImpl(&mPluginTag, aIndex);
    NS_IF_ADDREF(mimeType);
    *aReturn = mimeType;
    return NS_OK;
  }

  NS_METHOD NamedItem(const nsAString& aName, nsIDOMMimeType** aReturn)
  {
    for (int i = mPluginTag.mMimeTypes.Length() - 1; i >= 0; --i) {
      if (aName.Equals(NS_ConvertUTF8toUTF16(mPluginTag.mMimeTypes[i])))
        return Item(i, aReturn);
    }
    return NS_OK;
  }

private:
  nsPluginTag mPluginTag;
};

NS_IMPL_ISUPPORTS1(DOMPluginImpl, nsIDOMPlugin)

NS_IMETHODIMP
nsPluginHost::GetPluginCount(PRUint32* aPluginCount)
{
  LoadPlugins();

  PRUint32 count = 0;

  nsPluginTag* plugin = mPlugins;
  while (plugin != nsnull) {
    if (plugin->IsEnabled()) {
      ++count;
    }
    plugin = plugin->mNext;
  }

  *aPluginCount = count;

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin** aPluginArray)
{
  LoadPlugins();

  nsPluginTag* plugin = mPlugins;
  for (PRUint32 i = 0; i < aPluginCount && plugin; plugin = plugin->mNext) {
    if (plugin->IsEnabled()) {
      nsIDOMPlugin* domPlugin = new DOMPluginImpl(plugin);
      NS_IF_ADDREF(domPlugin);
      aPluginArray[i++] = domPlugin;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPluginTags(PRUint32* aPluginCount, nsIPluginTag*** aResults)
{
  LoadPlugins();

  PRUint32 count = 0;
  nsRefPtr<nsPluginTag> plugin = mPlugins;
  while (plugin != nsnull) {
    count++;
    plugin = plugin->mNext;
  }

  *aResults = static_cast<nsIPluginTag**>
                         (nsMemory::Alloc(count * sizeof(**aResults)));
  if (!*aResults)
    return NS_ERROR_OUT_OF_MEMORY;

  *aPluginCount = count;

  plugin = mPlugins;
  PRUint32 i;
  for (i = 0; i < count; i++) {
    (*aResults)[i] = plugin;
    NS_ADDREF((*aResults)[i]);
    plugin = plugin->mNext;
  }

  return NS_OK;
}

nsPluginTag*
nsPluginHost::FindPluginForType(const char* aMimeType,
                                PRBool aCheckEnabled)
{
  if (!aMimeType) {
    return nsnull;
  }

  LoadPlugins();

  nsPluginTag *plugin = mPlugins;
  while (plugin) {
    if (!aCheckEnabled || plugin->IsEnabled()) {
      PRInt32 mimeCount = plugin->mMimeTypes.Length();
      for (PRInt32 i = 0; i < mimeCount; i++) {
        if (0 == PL_strcasecmp(plugin->mMimeTypes[i].get(), aMimeType)) {
          return plugin;
        }
      }
    }
    plugin = plugin->mNext;
  }

  return nsnull;
}

nsPluginTag*
nsPluginHost::FindPluginEnabledForExtension(const char* aExtension,
                                            const char*& aMimeType)
{
  if (!aExtension) {
    return nsnull;
  }

  LoadPlugins();

  nsPluginTag *plugin = mPlugins;
  while (plugin) {
    if (plugin->IsEnabled()) {
      PRInt32 variants = plugin->mExtensions.Length();
      for (PRInt32 i = 0; i < variants; i++) {
        
        if (0 == CompareExtensions(plugin->mExtensions[i].get(), aExtension)) {
          aMimeType = plugin->mMimeTypes[i].get();
          return plugin;
        }
      }
    }
    plugin = plugin->mNext;
  }

  return nsnull;
}

static nsresult CreateNPAPIPlugin(nsPluginTag *aPluginTag,
                                  nsNPAPIPlugin **aOutNPAPIPlugin)
{
  
  if (!nsNPAPIPlugin::RunPluginOOP(aPluginTag)) {
    if (aPluginTag->mFullPath.IsEmpty())
      return NS_ERROR_FAILURE;
    nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    file->InitWithPath(NS_ConvertUTF8toUTF16(aPluginTag->mFullPath));
    nsPluginFile pluginFile(file);
    PRLibrary* pluginLibrary = NULL;

    if (NS_FAILED(pluginFile.LoadPlugin(&pluginLibrary)) || !pluginLibrary)
      return NS_ERROR_FAILURE;

    aPluginTag->mLibrary = pluginLibrary;
  }

  nsresult rv;
  rv = nsNPAPIPlugin::CreatePlugin(aPluginTag, aOutNPAPIPlugin);

  return rv;
}

nsresult nsPluginHost::EnsurePluginLoaded(nsPluginTag* plugin)
{
  nsRefPtr<nsNPAPIPlugin> entrypoint = plugin->mEntryPoint;
  if (!entrypoint) {
    nsresult rv = CreateNPAPIPlugin(plugin, getter_AddRefs(entrypoint));
    if (NS_FAILED(rv)) {
      return rv;
    }
    plugin->mEntryPoint = entrypoint;
  }
  return NS_OK;
}

nsresult nsPluginHost::GetPlugin(const char *aMimeType, nsNPAPIPlugin** aPlugin)
{
  nsresult rv = NS_ERROR_FAILURE;
  *aPlugin = NULL;

  if (!aMimeType)
    return NS_ERROR_ILLEGAL_VALUE;

  
  LoadPlugins();

  nsPluginTag* pluginTag = FindPluginForType(aMimeType, PR_TRUE);
  if (pluginTag) {
    rv = NS_OK;
    PLUGIN_LOG(PLUGIN_LOG_BASIC,
    ("nsPluginHost::GetPlugin Begin mime=%s, plugin=%s\n",
    aMimeType, pluginTag->mFileName.get()));

#ifdef NS_DEBUG
    if (aMimeType && !pluginTag->mFileName.IsEmpty())
      printf("For %s found plugin %s\n", aMimeType, pluginTag->mFileName.get());
#endif

    rv = EnsurePluginLoaded(pluginTag);
    if (NS_FAILED(rv)) {
      return rv;
    }

    NS_ADDREF(*aPlugin = pluginTag->mEntryPoint);
    return NS_OK;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::GetPlugin End mime=%s, rv=%d, plugin=%p name=%s\n",
  aMimeType, rv, *aPlugin,
  (pluginTag ? pluginTag->mFileName.get() : "(not found)")));

  return rv;
}


nsresult
nsPluginHost::NormalizeHostname(nsCString& host)
{
  if (IsASCII(host)) {
    ToLowerCase(host);
    return NS_OK;
  }

  if (!mIDNService) {
    nsresult rv;
    mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return mIDNService->ConvertUTF8toACE(host, host);
}





nsresult
nsPluginHost::EnumerateSiteData(const nsACString& domain,
                                const nsTArray<nsCString>& sites,
                                InfallibleTArray<nsCString>& result,
                                bool firstMatchOnly)
{
  NS_ASSERTION(!domain.IsVoid(), "null domain string");

  nsresult rv;
  if (!mTLDService) {
    mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCString baseDomain;
  rv = mTLDService->GetBaseDomainFromHost(domain, 0, baseDomain);
  bool isIP = rv == NS_ERROR_HOST_IS_IP_ADDRESS;
  if (isIP || rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS) {
    
    
    baseDomain = domain;
    rv = NormalizeHostname(baseDomain);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (NS_FAILED(rv)) {
    return rv;
  }

  
  for (PRUint32 i = 0; i < sites.Length(); ++i) {
    const nsCString& site = sites[i];

    
    bool siteIsIP =
      site.Length() >= 2 && site.First() == '[' && site.Last() == ']';
    if (siteIsIP != isIP)
      continue;

    nsCString siteBaseDomain;
    if (siteIsIP) {
      
      siteBaseDomain = Substring(site, 1, site.Length() - 2);
    } else {
      
      rv = mTLDService->GetBaseDomainFromHost(site, 0, siteBaseDomain);
      if (rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS) {
        
        
        siteBaseDomain = site;
        rv = NormalizeHostname(siteBaseDomain);
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (NS_FAILED(rv)) {
        return rv;
      }
    }

    
    if (baseDomain != siteBaseDomain) {
      continue;
    }

    
    result.AppendElement(site);

    
    if (firstMatchOnly) {
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::ClearSiteData(nsIPluginTag* plugin, const nsACString& domain,
                            PRUint64 flags, PRInt64 maxAge)
{
  
  NS_ENSURE_ARG(maxAge >= 0 || maxAge == -1);

  
  if (!IsLiveTag(plugin)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsPluginTag* tag = static_cast<nsPluginTag*>(plugin);

  
  
  
  if (!tag->mIsFlashPlugin && !tag->mEntryPoint) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv = EnsurePluginLoaded(tag);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PluginLibrary* library = tag->mEntryPoint->GetLibrary();

  
  if (domain.IsVoid()) {
    return library->NPP_ClearSiteData(NULL, flags, maxAge);
  }

  
  InfallibleTArray<nsCString> sites;
  rv = library->NPP_GetSitesWithData(sites);
  NS_ENSURE_SUCCESS(rv, rv);

  
  InfallibleTArray<nsCString> matches;
  rv = EnumerateSiteData(domain, sites, matches, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (PRUint32 i = 0; i < matches.Length(); ++i) {
    const nsCString& match = matches[i];
    rv = library->NPP_ClearSiteData(match.get(), flags, maxAge);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::SiteHasData(nsIPluginTag* plugin, const nsACString& domain,
                          PRBool* result)
{
  
  if (!IsLiveTag(plugin)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsPluginTag* tag = static_cast<nsPluginTag*>(plugin);

  
  
  
  if (!tag->mIsFlashPlugin && !tag->mEntryPoint) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv = EnsurePluginLoaded(tag);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PluginLibrary* library = tag->mEntryPoint->GetLibrary();

  
  InfallibleTArray<nsCString> sites;
  rv = library->NPP_GetSitesWithData(sites);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (sites.IsEmpty()) {
    *result = false;
    return NS_OK;
  }

  
  
  if (domain.IsVoid()) {
    *result = true;
    return NS_OK;
  }

  
  InfallibleTArray<nsCString> matches;
  rv = EnumerateSiteData(domain, sites, matches, true);
  NS_ENSURE_SUCCESS(rv, rv);

  *result = !matches.IsEmpty();
  return NS_OK;
}

PRBool nsPluginHost::IsJavaMIMEType(const char* aType)
{
  return aType &&
    ((0 == PL_strncasecmp(aType, "application/x-java-vm",
                          sizeof("application/x-java-vm") - 1)) ||
     (0 == PL_strncasecmp(aType, "application/x-java-applet",
                          sizeof("application/x-java-applet") - 1)) ||
     (0 == PL_strncasecmp(aType, "application/x-java-bean",
                          sizeof("application/x-java-bean") - 1)));
}


PRBool
nsPluginHost::IsLiveTag(nsIPluginTag* aPluginTag)
{
  nsPluginTag* tag;
  for (tag = mPlugins; tag; tag = tag->mNext) {
    if (tag == aPluginTag) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

nsPluginTag * nsPluginHost::HaveSamePlugin(nsPluginTag * aPluginTag)
{
  for (nsPluginTag* tag = mPlugins; tag; tag = tag->mNext) {
    if (tag->Equals(aPluginTag))
      return tag;
  }
  return nsnull;
}

PRBool nsPluginHost::IsDuplicatePlugin(nsPluginTag * aPluginTag)
{
  nsPluginTag * tag = HaveSamePlugin(aPluginTag);
  if (tag) {
    

    
    
    
    if (!tag->mFileName.Equals(aPluginTag->mFileName))
      return PR_TRUE;

    
    
    if (!tag->mFullPath.Equals(aPluginTag->mFullPath))
      return PR_TRUE;
  }

  
  return PR_FALSE;
}


struct pluginFileinDirectory
{
  nsString mFilePath;
  PRInt64  mModTime;

  pluginFileinDirectory()
  {
    mModTime = LL_ZERO;
  }
};




NS_SPECIALIZE_TEMPLATE
class nsDefaultComparator<pluginFileinDirectory, pluginFileinDirectory>
{
  public:
  PRBool Equals(const pluginFileinDirectory& aA,
                const pluginFileinDirectory& aB) const {
    if (aA.mModTime == aB.mModTime &&
        Compare(aA.mFilePath, aB.mFilePath,
                nsCaseInsensitiveStringComparator()) == 0)
      return PR_TRUE;
    else
      return PR_FALSE;
  }
  PRBool LessThan(const pluginFileinDirectory& aA,
                  const pluginFileinDirectory& aB) const {
    if (aA.mModTime < aB.mModTime)
      return PR_TRUE;
    else if(aA.mModTime == aB.mModTime)
      return Compare(aA.mFilePath, aB.mFilePath,
                     nsCaseInsensitiveStringComparator()) < 0;
    else
      return PR_FALSE;
  }
};

typedef NS_NPAPIPLUGIN_CALLBACK(char *, NP_GETMIMEDESCRIPTION)(void);

nsresult nsPluginHost::ScanPluginsDirectory(nsIFile *pluginsDir,
                                            PRBool aCreatePluginList,
                                            PRBool *aPluginsChanged)
{
  NS_ENSURE_ARG_POINTER(aPluginsChanged);
  nsresult rv;

  *aPluginsChanged = PR_FALSE;

#ifdef PLUGIN_LOGGING
  nsCAutoString dirPath;
  pluginsDir->GetNativePath(dirPath);
  PLUGIN_LOG(PLUGIN_LOG_BASIC,
  ("nsPluginHost::ScanPluginsDirectory dir=%s\n", dirPath.get()));
#endif

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = pluginsDir->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv))
    return rv;

  
  nsAutoTArray<pluginFileinDirectory, 8> pluginFilesArray;

  PRBool hasMore;
  while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv))
      continue;
    nsCOMPtr<nsILocalFile> dirEntry(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv))
      continue;

    
    
    dirEntry->Normalize();

    nsAutoString filePath;
    rv = dirEntry->GetPath(filePath);
    if (NS_FAILED(rv))
      continue;

    if (nsPluginsDir::IsPluginFile(dirEntry)) {
      pluginFileinDirectory * item = pluginFilesArray.AppendElement();
      if (!item)
        return NS_ERROR_OUT_OF_MEMORY;

      
      PRInt64 fileModTime = LL_ZERO;
      dirEntry->GetLastModifiedTime(&fileModTime);

      item->mModTime = fileModTime;
      item->mFilePath = filePath;
    }
  } 

  
  
  pluginFilesArray.Sort();

  PRBool warnOutdated = PR_FALSE;

  
  for (PRUint32 i = 0; i < pluginFilesArray.Length(); i++) {
    pluginFileinDirectory &pfd = pluginFilesArray[i];
    nsCOMPtr <nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    nsCOMPtr <nsILocalFile> localfile = do_QueryInterface(file);
    localfile->InitWithPath(pfd.mFilePath);
    PRInt64 fileModTime = pfd.mModTime;

    
    NS_ConvertUTF16toUTF8 filePath(pfd.mFilePath);
    nsRefPtr<nsPluginTag> pluginTag;
    RemoveCachedPluginsInfo(filePath.get(),
                            getter_AddRefs(pluginTag));

    PRBool enabled = PR_TRUE;
    PRBool seenBefore = PR_FALSE;
    if (pluginTag) {
      seenBefore = PR_TRUE;
      
      if (LL_NE(fileModTime, pluginTag->mLastModifiedTime)) {
        
        enabled = (pluginTag->Flags() & NS_PLUGIN_FLAG_ENABLED) != 0;
        pluginTag = nsnull;

        
        *aPluginsChanged = PR_TRUE;
      }
      else {
        
        
        if (IsDuplicatePlugin(pluginTag)) {
          if (!pluginTag->HasFlag(NS_PLUGIN_FLAG_UNWANTED)) {
            
            *aPluginsChanged = PR_TRUE;
          }
          pluginTag->Mark(NS_PLUGIN_FLAG_UNWANTED);
          pluginTag->mNext = mCachedPlugins;
          mCachedPlugins = pluginTag;
        } else if (pluginTag->HasFlag(NS_PLUGIN_FLAG_UNWANTED)) {
          pluginTag->UnMark(NS_PLUGIN_FLAG_UNWANTED);
          
          *aPluginsChanged = PR_TRUE;
        }
      }

      
      
      if (!aCreatePluginList) {
        if (*aPluginsChanged) {
          return NS_OK;
        }
        continue;
      }
    }
    
    bool isKnownInvalidPlugin = false;
    for (nsRefPtr<nsInvalidPluginTag> invalidPlugins = mInvalidPlugins;
         invalidPlugins; invalidPlugins = invalidPlugins->mNext) {
      
      if (invalidPlugins->mFullPath.Equals(filePath.get()) &&
          invalidPlugins->mLastModifiedTime == fileModTime) {
        if (aCreatePluginList) {
          invalidPlugins->mSeen = true;
        }
        isKnownInvalidPlugin = true;
        break;
      }
    }
    if (isKnownInvalidPlugin) {
      continue;
    }

    
    if (!pluginTag) {
      nsPluginFile pluginFile(file);

      
      PRLibrary *library = nsnull;
      nsPluginInfo info;
      memset(&info, 0, sizeof(info));
      nsresult res = pluginFile.GetPluginInfo(info, &library);
      
      if (NS_FAILED(res) || !info.fMimeTypeArray) {
        nsRefPtr<nsInvalidPluginTag> invalidTag = new nsInvalidPluginTag(filePath.get(),
                                                                         fileModTime);
        pluginFile.FreePluginInfo(info);
        
        if (aCreatePluginList) {
          invalidTag->mSeen = true;
        }
        invalidTag->mNext = mInvalidPlugins;
        if (mInvalidPlugins) {
          mInvalidPlugins->mPrev = invalidTag;
        }
        mInvalidPlugins = invalidTag;
        
        
        *aPluginsChanged = PR_TRUE;
        continue;
      }

      pluginTag = new nsPluginTag(&info);
      pluginFile.FreePluginInfo(info);
      if (!pluginTag)
        return NS_ERROR_OUT_OF_MEMORY;

      pluginTag->mLibrary = library;
      pluginTag->mLastModifiedTime = fileModTime;

      nsCOMPtr<nsIBlocklistService> blocklist = do_GetService("@mozilla.org/extensions/blocklist;1");
      if (blocklist) {
        PRUint32 state;
        rv = blocklist->GetPluginBlocklistState(pluginTag, EmptyString(),
                                                EmptyString(), &state);

        if (NS_SUCCEEDED(rv)) {
          
          
          if (state == nsIBlocklistService::STATE_BLOCKED)
            pluginTag->Mark(NS_PLUGIN_FLAG_BLOCKLISTED);
          else if (state == nsIBlocklistService::STATE_SOFTBLOCKED && !seenBefore)
            enabled = PR_FALSE;
          else if (state == nsIBlocklistService::STATE_OUTDATED && !seenBefore)
            warnOutdated = PR_TRUE;
        }
      }

      if (!enabled)
        pluginTag->UnMark(NS_PLUGIN_FLAG_ENABLED);

      
      
      
      NS_ASSERTION(!pluginTag->HasFlag(NS_PLUGIN_FLAG_UNWANTED),
                   "Brand-new tags should not be unwanted");
      if (IsDuplicatePlugin(pluginTag)) {
        pluginTag->Mark(NS_PLUGIN_FLAG_UNWANTED);
        pluginTag->mNext = mCachedPlugins;
        mCachedPlugins = pluginTag;
      }

      
      
      
      if (UnloadPluginsASAP()) {
        pluginTag->TryUnloadPlugin();
      }
    }

    
    
    PRBool bAddIt = PR_TRUE;
    
    if (HaveSamePlugin(pluginTag)) {
      
      
      
      bAddIt = PR_FALSE;
    }

    
    if (bAddIt) {
      if (!seenBefore) {
        
        *aPluginsChanged = PR_TRUE;
      }

      
      
      if (!aCreatePluginList) {
        return NS_OK;
      }

      pluginTag->SetHost(this);
      pluginTag->mNext = mPlugins;
      mPlugins = pluginTag;

      if (pluginTag->IsEnabled())
        pluginTag->RegisterWithCategoryManager(mOverrideInternalTypes);
    }
  }
  
  if (warnOutdated)
    mPrefService->SetBoolPref("plugins.update.notifyUser", PR_TRUE);

  return NS_OK;
}

nsresult nsPluginHost::ScanPluginsDirectoryList(nsISimpleEnumerator *dirEnum,
                                                PRBool aCreatePluginList,
                                                PRBool *aPluginsChanged)
{
    PRBool hasMore;
    while (NS_SUCCEEDED(dirEnum->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> supports;
      nsresult rv = dirEnum->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv))
        continue;
      nsCOMPtr<nsIFile> nextDir(do_QueryInterface(supports, &rv));
      if (NS_FAILED(rv))
        continue;

      
      PRBool pluginschanged = PR_FALSE;
      ScanPluginsDirectory(nextDir, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      
      if (!aCreatePluginList && *aPluginsChanged)
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP nsPluginHost::LoadPlugins()
{
  
  
  if (mPluginsLoaded)
    return NS_OK;

  if (mPluginsDisabled)
    return NS_OK;

  PRBool pluginschanged;
  nsresult rv = FindPlugins(PR_TRUE, &pluginschanged);
  if (NS_FAILED(rv))
    return rv;

  
  if (pluginschanged) {
    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService)
      obsService->NotifyObservers(nsnull, "plugins-list-updated", nsnull);
  }

  return NS_OK;
}

#include "nsITimelineService.h"




nsresult nsPluginHost::FindPlugins(PRBool aCreatePluginList, PRBool * aPluginsChanged)
{
  
  if (aCreatePluginList) {
    NS_TIMELINE_START_TIMER("LoadPlugins");
  }

#ifdef CALL_SAFETY_ON
  
  NS_INIT_PLUGIN_SAFE_CALLS;
#endif

  NS_ENSURE_ARG_POINTER(aPluginsChanged);

  *aPluginsChanged = PR_FALSE;
  nsresult rv;

  
  
  if (ReadPluginInfo() == NS_ERROR_NOT_AVAILABLE)
    return NS_OK;

#ifdef XP_WIN
  
  rv = EnsurePrivateDirServiceProvider();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to register dir service provider.");
#endif 

  nsCOMPtr<nsIProperties> dirService(do_GetService(kDirectoryServiceContractID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> dirList;

  
  
  
  PRBool pluginschanged = PR_FALSE;

  
  rv = dirService->Get(NS_APP_PLUGINS_DIR_LIST, NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(dirList));
  if (NS_SUCCEEDED(rv)) {
    ScanPluginsDirectoryList(dirList, aCreatePluginList, &pluginschanged);

    if (pluginschanged)
      *aPluginsChanged = PR_TRUE;

    
    
    if (!aCreatePluginList && *aPluginsChanged) {
      NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
      NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
      return NS_OK;
    }
  }

  mPluginsLoaded = PR_TRUE; 
                            

#ifdef XP_WIN
  PRBool bScanPLIDs = PR_FALSE;

  if (mPrefService)
    mPrefService->GetBoolPref("plugin.scan.plid.all", &bScanPLIDs);

    
  if (bScanPLIDs && mPrivateDirServiceProvider) {
    rv = mPrivateDirServiceProvider->GetPLIDDirectories(getter_AddRefs(dirList));
    if (NS_SUCCEEDED(rv)) {
      ScanPluginsDirectoryList(dirList, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      
      
      if (!aCreatePluginList && *aPluginsChanged) {
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
        return NS_OK;
      }
    }
  }


  

  
  const char* const prefs[] = {NS_WIN_JRE_SCAN_KEY,
                               NS_WIN_ACROBAT_SCAN_KEY,
                               NS_WIN_QUICKTIME_SCAN_KEY,
                               NS_WIN_WMP_SCAN_KEY};

  PRUint32 size = sizeof(prefs) / sizeof(prefs[0]);

  for (PRUint32 i = 0; i < size; i+=1) {
    nsCOMPtr<nsIFile> dirToScan;
    PRBool bExists;
    if (NS_SUCCEEDED(dirService->Get(prefs[i], NS_GET_IID(nsIFile), getter_AddRefs(dirToScan))) &&
        dirToScan &&
        NS_SUCCEEDED(dirToScan->Exists(&bExists)) &&
        bExists) {
      
      ScanPluginsDirectory(dirToScan, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = PR_TRUE;

      
      
      if (!aCreatePluginList && *aPluginsChanged) {
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
        return NS_OK;
      }
    }
  }
#endif

  
  
  
  if (!*aPluginsChanged) {
    
    
    PRUint32 cachecount = 0;
    for (nsPluginTag * cachetag = mCachedPlugins; cachetag; cachetag = cachetag->mNext) {
      if (!cachetag->HasFlag(NS_PLUGIN_FLAG_UNWANTED))
        cachecount++;
    }
    
    
    
    if (cachecount > 0)
      *aPluginsChanged = PR_TRUE;
  }
  
  
  nsRefPtr<nsInvalidPluginTag> invalidPlugins = mInvalidPlugins;
  while (invalidPlugins) {
    if (!invalidPlugins->mSeen) {
      nsRefPtr<nsInvalidPluginTag> invalidPlugin = invalidPlugins;
      
      if (invalidPlugin->mPrev) {
        invalidPlugin->mPrev->mNext = invalidPlugin->mNext;
      }
      else {
        mInvalidPlugins = invalidPlugin->mNext;
      }
      if (invalidPlugin->mNext) {
        invalidPlugin->mNext->mPrev = invalidPlugin->mPrev; 
      }
      
      invalidPlugins = invalidPlugin->mNext;
      
      invalidPlugin->mPrev = NULL;
      invalidPlugin->mNext = NULL;
    }
    else {
      invalidPlugins->mSeen = false;
      invalidPlugins = invalidPlugins->mNext;
    }
  }

  
  if (!aCreatePluginList) {
    NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
    NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
    return NS_OK;
  }

  
  
  if (*aPluginsChanged)
    WritePluginInfo();

  
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);

  
  nsRefPtr<nsPluginTag> next;
  nsRefPtr<nsPluginTag> prev;
  for (nsRefPtr<nsPluginTag> cur = mPlugins; cur; cur = next) {
    next = cur->mNext;
    cur->mNext = prev;
    prev = cur;
  }

  mPlugins = prev;

  NS_TIMELINE_STOP_TIMER("LoadPlugins");
  NS_TIMELINE_MARK_TIMER("LoadPlugins");

  return NS_OK;
}

nsresult
nsPluginHost::UpdatePluginInfo(nsPluginTag* aPluginTag)
{
  ReadPluginInfo();
  WritePluginInfo();
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);

  if (!aPluginTag || aPluginTag->IsEnabled())
    return NS_OK;

  nsCOMPtr<nsISupportsArray> instsToReload;
  NS_NewISupportsArray(getter_AddRefs(instsToReload));
  DestroyRunningInstances(instsToReload, aPluginTag);
  
  PRUint32 c;
  if (instsToReload && NS_SUCCEEDED(instsToReload->Count(&c)) && c > 0) {
    nsCOMPtr<nsIRunnable> ev = new nsPluginDocReframeEvent(instsToReload);
    if (ev)
      NS_DispatchToCurrentThread(ev);
  }

  return NS_OK;
}

nsresult
nsPluginHost::WritePluginInfo()
{

  nsresult rv = NS_OK;
  nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID,&rv));
  if (NS_FAILED(rv))
    return rv;

  directoryService->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                        getter_AddRefs(mPluginRegFile));

  if (!mPluginRegFile)
    return NS_ERROR_FAILURE;

  PRFileDesc* fd = nsnull;

  nsCOMPtr<nsIFile> pluginReg;

  rv = mPluginRegFile->Clone(getter_AddRefs(pluginReg));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString filename(kPluginRegistryFilename);
  filename.Append(".tmp");
  rv = pluginReg->AppendNative(filename);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(pluginReg, &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = localFile->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0600, &fd);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService("@mozilla.org/xre/runtime;1");
  if (!runtime) {
    return NS_ERROR_FAILURE;
  }
    
  nsCAutoString arch;
  rv = runtime->GetXPCOMABI(arch);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PR_fprintf(fd, "Generated File. Do not edit.\n");

  PR_fprintf(fd, "\n[HEADER]\nVersion%c%s%c%c\nArch%c%s%c%c\n",
             PLUGIN_REGISTRY_FIELD_DELIMITER,
             kPluginRegistryVersion,
             PLUGIN_REGISTRY_FIELD_DELIMITER,
             PLUGIN_REGISTRY_END_OF_LINE_MARKER,
             PLUGIN_REGISTRY_FIELD_DELIMITER,
             arch.get(),
             PLUGIN_REGISTRY_FIELD_DELIMITER,
             PLUGIN_REGISTRY_END_OF_LINE_MARKER);

  
  PR_fprintf(fd, "\n[PLUGINS]\n");

  nsPluginTag *taglist[] = {mPlugins, mCachedPlugins};
  for (int i=0; i<(int)(sizeof(taglist)/sizeof(nsPluginTag *)); i++) {
    for (nsPluginTag *tag = taglist[i]; tag; tag=tag->mNext) {
      
      if ((taglist[i] == mCachedPlugins) && !tag->HasFlag(NS_PLUGIN_FLAG_UNWANTED))
        continue;
      
      
      
      PR_fprintf(fd, "%s%c%c\n%s%c%c\n%s%c%c\n",
        (tag->mFileName.get()),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        (tag->mFullPath.get()),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        (tag->mVersion.get()),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER);

      
      PR_fprintf(fd, "%lld%c%d%c%lu%c%c\n",
        tag->mLastModifiedTime,
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        tag->mCanUnloadLibrary,
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        tag->Flags(),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER);

      
      PR_fprintf(fd, "%s%c%c\n%s%c%c\n%d\n",
        (tag->mDescription.get()),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        (tag->mName.get()),
        PLUGIN_REGISTRY_FIELD_DELIMITER,
        PLUGIN_REGISTRY_END_OF_LINE_MARKER,
        tag->mMimeTypes.Length() + (tag->mIsNPRuntimeEnabledJavaPlugin ? 1 : 0));

      
      for (PRUint32 i = 0; i < tag->mMimeTypes.Length(); i++) {
        PR_fprintf(fd, "%d%c%s%c%s%c%s%c%c\n",
          i,PLUGIN_REGISTRY_FIELD_DELIMITER,
          (tag->mMimeTypes[i].get()),
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          (tag->mMimeDescriptions[i].get()),
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          (tag->mExtensions[i].get()),
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          PLUGIN_REGISTRY_END_OF_LINE_MARKER);
      }

      if (tag->mIsNPRuntimeEnabledJavaPlugin) {
        PR_fprintf(fd, "%d%c%s%c%s%c%s%c%c\n",
          tag->mMimeTypes.Length(), PLUGIN_REGISTRY_FIELD_DELIMITER,
          "application/x-java-vm-npruntime",
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          "",
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          "",
          PLUGIN_REGISTRY_FIELD_DELIMITER,
          PLUGIN_REGISTRY_END_OF_LINE_MARKER);
      }
    }
  }
  
  PR_fprintf(fd, "\n[INVALID]\n");
  
  nsRefPtr<nsInvalidPluginTag> invalidPlugins = mInvalidPlugins;
  while (invalidPlugins) {
    
    PR_fprintf(fd, "%s%c%c\n",
      (!invalidPlugins->mFullPath.IsEmpty() ? invalidPlugins->mFullPath.get() : ""),
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      PLUGIN_REGISTRY_END_OF_LINE_MARKER);

    
    PR_fprintf(fd, "%lld%c%c\n",
      invalidPlugins->mLastModifiedTime,
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      PLUGIN_REGISTRY_END_OF_LINE_MARKER);
    
    invalidPlugins = invalidPlugins->mNext;
  }

  PR_Close(fd);
  nsCOMPtr<nsIFile> parent;
  rv = localFile->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = localFile->MoveToNative(parent, kPluginRegistryFilename);
  return rv;
}

#define PLUGIN_REG_MIMETYPES_ARRAY_SIZE 12
nsresult
nsPluginHost::ReadPluginInfo()
{
  nsresult rv;

  nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID,&rv));
  if (NS_FAILED(rv))
    return rv;

  directoryService->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                        getter_AddRefs(mPluginRegFile));

  if (!mPluginRegFile) {
    
    
    directoryService->Get(NS_APP_PROFILE_DIR_STARTUP, NS_GET_IID(nsIFile),
                          getter_AddRefs(mPluginRegFile));
    if (!mPluginRegFile)
      return NS_ERROR_FAILURE;
    else
      return NS_ERROR_NOT_AVAILABLE;
  }

  PRFileDesc* fd = nsnull;

  nsCOMPtr<nsIFile> pluginReg;

  rv = mPluginRegFile->Clone(getter_AddRefs(pluginReg));
  if (NS_FAILED(rv))
    return rv;

  rv = pluginReg->AppendNative(kPluginRegistryFilename);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(pluginReg, &rv);
  if (NS_FAILED(rv))
    return rv;

  PRInt64 fileSize;
  rv = localFile->GetFileSize(&fileSize);
  if (NS_FAILED(rv))
    return rv;

  PRInt32 flen = PRInt64(fileSize);
  if (flen == 0) {
    NS_WARNING("Plugins Registry Empty!");
    return NS_OK; 
  }

  nsPluginManifestLineReader reader;
  char* registry = reader.Init(flen);
  if (!registry)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = localFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd);
  if (NS_FAILED(rv))
    return rv;

  
  rv = NS_ERROR_FAILURE;

  PRInt32 bread = PR_Read(fd, registry, flen);
  PR_Close(fd);

  if (flen > bread)
    return rv;

  if (!ReadSectionHeader(reader, "HEADER"))
    return rv;;

  if (!reader.NextLine())
    return rv;

  char* values[6];

  
  if (2 != reader.ParseLine(values, 2))
    return rv;

  
  if (PL_strcmp(values[0], "Version"))
    return rv;

  
  PRInt32 vdiff = NS_CompareVersions(values[1], kPluginRegistryVersion);
  
  if (vdiff > 0)
    return rv;
  
  if (NS_CompareVersions(values[1], kMinimumRegistryVersion) < 0)
    return rv;

  
  PRBool regHasVersion = NS_CompareVersions(values[1], "0.10") >= 0;

  
  if (NS_CompareVersions(values[1], "0.13") >= 0) {
    char* archValues[6];
    
    if (!reader.NextLine()) {
      return rv;
    }
    
    
    if (2 != reader.ParseLine(archValues, 2)) {
      return rv;
    }
      
    
    if (PL_strcmp(archValues[0], "Arch")) {
      return rv;
    }
      
    nsCOMPtr<nsIXULRuntime> runtime = do_GetService("@mozilla.org/xre/runtime;1");
    if (!runtime) {
      return rv;
    }
      
    nsCAutoString arch;
    if (NS_FAILED(runtime->GetXPCOMABI(arch))) {
      return rv;
    }
      
    
    if (PL_strcmp(archValues[1], arch.get())) {
      return rv;
    }
  }
  
  
  bool hasInvalidPlugins = (NS_CompareVersions(values[1], "0.13") >= 0);

  if (!ReadSectionHeader(reader, "PLUGINS"))
    return rv;

#if defined(XP_MACOSX)
  PRBool hasFullPathInFileNameField = PR_FALSE;
#else
  PRBool hasFullPathInFileNameField = (NS_CompareVersions(values[1], "0.11") < 0);
#endif

  while (reader.NextLine()) {
    const char *filename;
    const char *fullpath;
    nsCAutoString derivedFileName;
    
    if (hasInvalidPlugins && *reader.LinePtr() == '[') {
      break;
    }
    
    if (hasFullPathInFileNameField) {
      fullpath = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
      
      if (fullpath) {
        nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
        file->InitWithNativePath(nsDependentCString(fullpath));
        file->GetNativeLeafName(derivedFileName);
        filename = derivedFileName.get();
      } else {
        filename = NULL;
      }

      
      if (!reader.NextLine())
        return rv;
    } else {
      filename = reader.LinePtr();
      if (!reader.NextLine())
        return rv;

      fullpath = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
    }

    const char *version;
    if (regHasVersion) {
      version = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
    } else {
      version = "0";
    }

    
    if (reader.ParseLine(values, 3) != 3)
      return rv;

    
    PRInt64 lastmod = (vdiff == 0) ? nsCRT::atoll(values[0]) : -1;
    PRBool canunload = atoi(values[1]);
    PRUint32 tagflag = atoi(values[2]);
    if (!reader.NextLine())
      return rv;

    const char *description = reader.LinePtr();
    if (!reader.NextLine())
      return rv;

    const char *name = reader.LinePtr();
    if (!reader.NextLine())
      return rv;

    int mimetypecount = atoi(reader.LinePtr());

    char *stackalloced[PLUGIN_REG_MIMETYPES_ARRAY_SIZE * 3];
    char **mimetypes;
    char **mimedescriptions;
    char **extensions;
    char **heapalloced = 0;
    if (mimetypecount > PLUGIN_REG_MIMETYPES_ARRAY_SIZE - 1) {
      heapalloced = new char *[mimetypecount * 3];
      mimetypes = heapalloced;
    } else {
      mimetypes = stackalloced;
    }
    mimedescriptions = mimetypes + mimetypecount;
    extensions = mimedescriptions + mimetypecount;

    int mtr = 0; 
    for (; mtr < mimetypecount; mtr++) {
      if (!reader.NextLine())
        break;

      
      if (4 != reader.ParseLine(values, 4))
        break;
      int line = atoi(values[0]);
      if (line != mtr)
        break;
      mimetypes[mtr] = values[1];
      mimedescriptions[mtr] = values[2];
      extensions[mtr] = values[3];
    }

    if (mtr != mimetypecount) {
      if (heapalloced) {
        delete [] heapalloced;
      }
      return rv;
    }

    nsRefPtr<nsPluginTag> tag = new nsPluginTag(name,
      description,
      filename,
      fullpath,
      version,
      (const char* const*)mimetypes,
      (const char* const*)mimedescriptions,
      (const char* const*)extensions,
      mimetypecount, lastmod, canunload, PR_TRUE);
    if (heapalloced)
      delete [] heapalloced;

    if (!tag)
      continue;

    
    tag->Mark(tagflag | NS_PLUGIN_FLAG_FROMCACHE);
    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
      ("LoadCachedPluginsInfo : Loading Cached plugininfo for %s\n", tag->mFileName.get()));
    tag->mNext = mCachedPlugins;
    mCachedPlugins = tag;
  }
  
  if (hasInvalidPlugins) {
    if (!ReadSectionHeader(reader, "INVALID")) {
      return rv;
    }

    while (reader.NextLine()) {
      const char *fullpath = reader.LinePtr();
      if (!reader.NextLine()) {
        return rv;
      }
      
      const char *lastModifiedTimeStamp = reader.LinePtr();
      PRInt64 lastmod = (vdiff == 0) ? nsCRT::atoll(lastModifiedTimeStamp) : -1;
      
      nsRefPtr<nsInvalidPluginTag> invalidTag = new nsInvalidPluginTag(fullpath, lastmod);
      
      invalidTag->mNext = mInvalidPlugins;
      if (mInvalidPlugins) {
        mInvalidPlugins->mPrev = invalidTag;
      }
      mInvalidPlugins = invalidTag;
    }
  }
  return NS_OK;
}

void
nsPluginHost::RemoveCachedPluginsInfo(const char *filePath, nsPluginTag **result)
{
  nsRefPtr<nsPluginTag> prev;
  nsRefPtr<nsPluginTag> tag = mCachedPlugins;
  while (tag)
  {
    if (tag->mFullPath.Equals(filePath)) {
      
      if (prev)
        prev->mNext = tag->mNext;
      else
        mCachedPlugins = tag->mNext;
      tag->mNext = nsnull;
      *result = tag;
      NS_ADDREF(*result);
      break;
    }
    prev = tag;
    tag = tag->mNext;
  }
}

#ifdef XP_WIN
nsresult
nsPluginHost::EnsurePrivateDirServiceProvider()
{
  if (!mPrivateDirServiceProvider) {
    nsresult rv;
    mPrivateDirServiceProvider = new nsPluginDirServiceProvider();
    if (!mPrivateDirServiceProvider)
      return NS_ERROR_OUT_OF_MEMORY;
    nsCOMPtr<nsIDirectoryService> dirService(do_GetService(kDirectoryServiceContractID, &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = dirService->RegisterProvider(mPrivateDirServiceProvider);
    if (NS_FAILED(rv))
      return rv;
  }
  return NS_OK;
}
#endif 

nsresult nsPluginHost::NewPluginURLStream(const nsString& aURL,
                                          nsNPAPIPluginInstance *aInstance,
                                          nsIPluginStreamListener* aListener,
                                          nsIInputStream *aPostStream,
                                          const char *aHeadersData,
                                          PRUint32 aHeadersDataLen)
{
  nsCOMPtr<nsIURI> url;
  nsAutoString absUrl;
  nsresult rv;

  if (aURL.Length() <= 0)
    return NS_OK;

  
  
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  aInstance->GetOwner(getter_AddRefs(owner));
  if (owner) {
    rv = owner->GetDocument(getter_AddRefs(doc));
    if (NS_SUCCEEDED(rv) && doc) {
      
      rv = NS_MakeAbsoluteURI(absUrl, aURL, doc->GetDocBaseURI());
    }
  }

  if (absUrl.IsEmpty())
    absUrl.Assign(aURL);

  rv = NS_NewURI(getter_AddRefs(url), absUrl);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIPluginTagInfo> pti = do_QueryInterface(owner);
  nsCOMPtr<nsIDOMElement> element;
  if (pti)
    pti->GetDOMElement(getter_AddRefs(element));

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,
                                 url,
                                 (doc ? doc->NodePrincipal() : nsnull),
                                 element,
                                 EmptyCString(), 
                                 nsnull,         
                                 &shouldLoad);
  if (NS_FAILED(rv))
    return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  nsRefPtr<nsPluginStreamListenerPeer> listenerPeer = new nsPluginStreamListenerPeer();
  if (!listenerPeer)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = listenerPeer->Initialize(url, aInstance, aListener);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), url, nsnull,
    nsnull, 


    listenerPeer);
  if (NS_FAILED(rv))
    return rv;

  if (doc) {
    
    channel->SetOwner(doc->NodePrincipal());

    
    
    nsCOMPtr<nsIScriptChannel> scriptChannel(do_QueryInterface(channel));
    if (scriptChannel) {
      scriptChannel->SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
      
      scriptChannel->SetExecuteAsync(PR_FALSE);
    }
  }

  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    if (aPostStream) {
      
      
      
      nsCOMPtr<nsISeekableStream>
      postDataSeekable(do_QueryInterface(aPostStream));
      if (postDataSeekable)
        postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

      nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
      NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

      uploadChannel->SetUploadStream(aPostStream, EmptyCString(), -1);
    }

    if (aHeadersData)
      rv = AddHeadersToChannel(aHeadersData, aHeadersDataLen, httpChannel);
  }
  rv = channel->AsyncOpen(listenerPeer, nsnull);
  if (NS_SUCCEEDED(rv))
    listenerPeer->TrackRequest(channel);
  return rv;
}


nsresult
nsPluginHost::DoURLLoadSecurityCheck(nsNPAPIPluginInstance *aInstance,
                                     const char* aURL)
{
  if (!aURL || *aURL == '\0')
    return NS_OK;

  
  nsCOMPtr<nsIPluginInstanceOwner> owner;
  aInstance->GetOwner(getter_AddRefs(owner));
  if (!owner)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));
  if (!doc)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIURI> targetURL;
  NS_NewURI(getter_AddRefs(targetURL), aURL, doc->GetDocBaseURI());
  if (!targetURL)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> secMan(
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;

  return secMan->CheckLoadURIWithPrincipal(doc->NodePrincipal(), targetURL,
                                           nsIScriptSecurityManager::STANDARD);

}

nsresult
nsPluginHost::AddHeadersToChannel(const char *aHeadersData,
                                  PRUint32 aHeadersDataLen,
                                  nsIChannel *aGenericChannel)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIHttpChannel> aChannel = do_QueryInterface(aGenericChannel);
  if (!aChannel) {
    return NS_ERROR_NULL_POINTER;
  }

  
  nsCAutoString headersString;
  nsCAutoString oneHeader;
  nsCAutoString headerName;
  nsCAutoString headerValue;
  PRInt32 crlf = 0;
  PRInt32 colon = 0;

  
  headersString = aHeadersData;

  
  
  while (PR_TRUE) {
    crlf = headersString.Find("\r\n", PR_TRUE);
    if (-1 == crlf) {
      rv = NS_OK;
      return rv;
    }
    headersString.Mid(oneHeader, 0, crlf);
    headersString.Cut(0, crlf + 2);
    oneHeader.StripWhitespace();
    colon = oneHeader.Find(":");
    if (-1 == colon) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
    oneHeader.Left(headerName, colon);
    colon++;
    oneHeader.Mid(headerValue, colon, oneHeader.Length() - colon);

    

    rv = aChannel->SetRequestHeader(headerName, headerValue, PR_TRUE);
    if (NS_FAILED(rv)) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsPluginHost::StopPluginInstance(nsIPluginInstance* aInstance)
{
  if (PluginDestructionGuard::DelayDestroy(aInstance)) {
    return NS_OK;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::StopPluginInstance called instance=%p\n",aInstance));

  nsNPAPIPluginInstance* instance = static_cast<nsNPAPIPluginInstance*>(aInstance);
  if (instance->HasStartedDestroying())
    return NS_OK;

  aInstance->Stop();

  
  PRBool doCache = PR_TRUE;
  aInstance->ShouldCache(&doCache);
  if (doCache) {
    
    PRUint32 cachedPluginLimit;
    nsresult rv = NS_ERROR_FAILURE;
    if (mPrefService)
      rv = mPrefService->GetIntPref(NS_PREF_MAX_NUM_CACHED_PLUGINS, (int*)&cachedPluginLimit);
    if (NS_FAILED(rv))
      cachedPluginLimit = DEFAULT_NUMBER_OF_STOPPED_PLUGINS;
    
    if (StoppedInstanceCount() >= cachedPluginLimit) {
      nsNPAPIPluginInstance *oldestInstance = FindOldestStoppedInstance();
      if (oldestInstance) {
        nsPluginTag* pluginTag = TagForPlugin(oldestInstance->GetPlugin());
        oldestInstance->Destroy();
        mInstances.RemoveElement(oldestInstance);
        OnPluginInstanceDestroyed(pluginTag);
      }
    }
  } else {
    nsPluginTag* pluginTag = TagForPlugin(instance->GetPlugin());
    instance->Destroy();
    mInstances.RemoveElement(instance);
    OnPluginInstanceDestroyed(pluginTag);
  }

  return NS_OK;
}

nsresult nsPluginHost::NewEmbeddedPluginStreamListener(nsIURI* aURL,
                                                       nsIPluginInstanceOwner *aOwner,
                                                       nsNPAPIPluginInstance* aInstance,
                                                       nsIStreamListener** aListener)
{
  if (!aURL)
    return NS_OK;

  nsRefPtr<nsPluginStreamListenerPeer> listener = new nsPluginStreamListenerPeer();
  if (!listener)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;

  
  
  
  
  if (aInstance)
    rv = listener->InitializeEmbedded(aURL, aInstance);
  else if (aOwner != nsnull)
    rv = listener->InitializeEmbedded(aURL, nsnull, aOwner);
  else
    rv = NS_ERROR_ILLEGAL_VALUE;
  if (NS_SUCCEEDED(rv))
    NS_ADDREF(*aListener = listener);

  return rv;
}


nsresult nsPluginHost::NewEmbeddedPluginStream(nsIURI* aURL,
                                               nsIPluginInstanceOwner *aOwner,
                                               nsNPAPIPluginInstance* aInstance)
{
  nsCOMPtr<nsIStreamListener> listener;
  nsresult rv = NewEmbeddedPluginStreamListener(aURL, aOwner, aInstance,
                                                getter_AddRefs(listener));
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDocument> doc;
    nsCOMPtr<nsILoadGroup> loadGroup;
    if (aOwner) {
      rv = aOwner->GetDocument(getter_AddRefs(doc));
      if (NS_SUCCEEDED(rv) && doc) {
        loadGroup = doc->GetDocumentLoadGroup();
      }
    }
    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewChannel(getter_AddRefs(channel), aURL, nsnull, loadGroup, nsnull);
    if (NS_SUCCEEDED(rv)) {
      
      
      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
      if (httpChannel && doc)
        httpChannel->SetReferrer(doc->GetDocumentURI());

      rv = channel->AsyncOpen(listener, nsnull);
      if (NS_SUCCEEDED(rv))
        return NS_OK;
    }
  }

  return rv;
}


nsresult nsPluginHost::NewFullPagePluginStream(nsIURI* aURI,
                                               nsNPAPIPluginInstance *aInstance,
                                               nsIStreamListener **aStreamListener)
{
  NS_ASSERTION(aStreamListener, "Stream listener out param cannot be null");

  nsRefPtr<nsPluginStreamListenerPeer> listener = new nsPluginStreamListenerPeer();
  if (!listener)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = listener->InitializeFullPage(aURI, aInstance);
  if (NS_FAILED(rv)) {
    return rv;
  }

  listener.forget(aStreamListener);

  return NS_OK;
}

NS_IMETHODIMP nsPluginHost::Observe(nsISupports *aSubject,
                                    const char *aTopic,
                                    const PRUnichar *someData)
{
  if (!nsCRT::strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, aTopic)) {
    OnShutdown();
    Destroy();
    sInst->Release();
  }
  if (!nsCRT::strcmp(NS_PRIVATE_BROWSING_SWITCH_TOPIC, aTopic)) {
    
    for (PRUint32 i = 0; i < mInstances.Length(); i++) {
      mInstances[i]->PrivateModeStateChanged();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::HandleBadPlugin(PRLibrary* aLibrary, nsIPluginInstance *aInstance)
{
  
  
  

  NS_ERROR("Plugin performed illegal operation");
  NS_ENSURE_ARG_POINTER(aInstance);

  if (mDontShowBadPluginMessage)
    return NS_OK;

  nsCOMPtr<nsIPluginInstanceOwner> owner;
  aInstance->GetOwner(getter_AddRefs(owner));

  nsCOMPtr<nsIPrompt> prompt;
  GetPrompt(owner, getter_AddRefs(prompt));
  if (!prompt)
    return NS_OK;

  nsCOMPtr<nsIStringBundleService> strings =
    mozilla::services::GetStringBundleService();
  if (!strings)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = strings->CreateBundle(BRAND_PROPERTIES_URL, getter_AddRefs(bundle));
  if (NS_FAILED(rv))
    return rv;

  nsXPIDLString brandName;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                 getter_Copies(brandName));
  if (NS_FAILED(rv))
    return rv;

  rv = strings->CreateBundle(PLUGIN_PROPERTIES_URL, getter_AddRefs(bundle));
  if (NS_FAILED(rv))
    return rv;

  nsXPIDLString title, message, checkboxMessage;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginTitle").get(),
                                 getter_Copies(title));
  if (NS_FAILED(rv))
    return rv;

  const PRUnichar *formatStrings[] = { brandName.get() };
  if (NS_FAILED(rv = bundle->FormatStringFromName(NS_LITERAL_STRING("BadPluginMessage").get(),
                               formatStrings, 1, getter_Copies(message))))
    return rv;

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BadPluginCheckboxMessage").get(),
                                 getter_Copies(checkboxMessage));
  if (NS_FAILED(rv))
    return rv;

  nsNPAPIPluginInstance *instance = static_cast<nsNPAPIPluginInstance*>(aInstance);

  nsNPAPIPlugin *plugin = instance->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  nsPluginTag *pluginTag = TagForPlugin(plugin);

  
  nsCString pluginname;
  if (pluginTag) {
    if (!pluginTag->mName.IsEmpty()) {
      pluginname = pluginTag->mName;
    } else {
      pluginname = pluginTag->mFileName;
    }
  } else {
    pluginname.AppendLiteral("???");
  }

  NS_ConvertUTF8toUTF16 msg(pluginname);
  msg.AppendLiteral("\n\n");
  msg.Append(message);

  PRInt32 buttonPressed;
  PRBool checkboxState = PR_FALSE;
  rv = prompt->ConfirmEx(title, msg.get(),
                       nsIPrompt::BUTTON_TITLE_OK * nsIPrompt::BUTTON_POS_0,
                       nsnull, nsnull, nsnull,
                       checkboxMessage, &checkboxState, &buttonPressed);


  if (NS_SUCCEEDED(rv) && checkboxState)
    mDontShowBadPluginMessage = PR_TRUE;

  return rv;
}

NS_IMETHODIMP
nsPluginHost::ParsePostBufferToFixHeaders(const char *inPostData, PRUint32 inPostDataLen,
                                          char **outPostData, PRUint32 *outPostDataLen)
{
  if (!inPostData || !outPostData || !outPostDataLen)
    return NS_ERROR_NULL_POINTER;

  *outPostData = 0;
  *outPostDataLen = 0;

  const char CR = '\r';
  const char LF = '\n';
  const char CRLFCRLF[] = {CR,LF,CR,LF,'\0'}; 
  const char ContentLenHeader[] = "Content-length";

  nsAutoTArray<const char*, 8> singleLF;
  const char *pSCntlh = 0;
  const char *pSod = 0;   
  const char *pEoh = 0;   
  const char *pEod = inPostData + inPostDataLen; 
  if (*inPostData == LF) {
    
    
    
    pSod = inPostData + 1;
  } else {
    const char *s = inPostData; 
    while (s < pEod) {
      if (!pSCntlh &&
          (*s == 'C' || *s == 'c') &&
          (s + sizeof(ContentLenHeader) - 1 < pEod) &&
          (!PL_strncasecmp(s, ContentLenHeader, sizeof(ContentLenHeader) - 1)))
      {
        
        const char *p = pSCntlh = s;
        p += sizeof(ContentLenHeader) - 1;
        
        for (; p < pEod; p++) {
          if (*p == CR || *p == LF) {
            
            
            
            if (*(p-1) >= '0' && *(p-1) <= '9') {
              s = p;
            }
            break; 
          }
        }
        if (pSCntlh == s) { 
          pSCntlh = 0; 
          break; 
        }
      }

      if (*s == CR) {
        if (pSCntlh && 
            ((s + sizeof(CRLFCRLF)-1) <= pEod) &&
            !memcmp(s, CRLFCRLF, sizeof(CRLFCRLF)-1))
        {
          s += sizeof(CRLFCRLF)-1;
          pEoh = pSod = s; 
          break;
        }
      } else if (*s == LF) {
        if (*(s-1) != CR) {
          singleLF.AppendElement(s);
        }
        if (pSCntlh && (s+1 < pEod) && (*(s+1) == LF)) {
          s++;
          singleLF.AppendElement(s);
          s++;
          pEoh = pSod = s; 
          break;
        }
      }
      s++;
    }
  }

  
  if (!pSod) { 
    pSod = inPostData;
  }

  PRUint32 newBufferLen = 0;
  PRUint32 dataLen = pEod - pSod;
  PRUint32 headersLen = pEoh ? pSod - inPostData : 0;

  char *p; 
  if (headersLen) { 
    
    

    newBufferLen = dataLen + headersLen;
    
    
    int cntSingleLF = singleLF.Length();
    newBufferLen += cntSingleLF;

    if (!(*outPostData = p = (char*)nsMemory::Alloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;

    
    const char *s = inPostData;
    if (cntSingleLF) {
      for (int i=0; i<cntSingleLF; i++) {
        const char *plf = singleLF.ElementAt(i); 
        int n = plf - s; 
        if (n) { 
          memcpy(p, s, n);
          p += n;
        }
        *p++ = CR;
        s = plf;
        *p++ = *s++;
      }
    }
    
    headersLen = pEoh - s;
    if (headersLen) { 
      memcpy(p, s, headersLen); 
      p += headersLen;
    }
  } else  if (dataLen) { 
    
    
    PRUint32 l = sizeof(ContentLenHeader) + sizeof(CRLFCRLF) + 32;
    newBufferLen = dataLen + l;
    if (!(*outPostData = p = (char*)nsMemory::Alloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;
    headersLen = PR_snprintf(p, l,"%s: %ld%s", ContentLenHeader, dataLen, CRLFCRLF);
    if (headersLen == l) { 
      nsMemory::Free(p);
      *outPostData = 0;
      return NS_ERROR_FAILURE;
    }
    p += headersLen;
    newBufferLen = headersLen + dataLen;
  }
  
  
  
  
  if (dataLen) {
    memcpy(p, pSod, dataLen);
  }

  *outPostDataLen = newBufferLen;

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::CreateTempFileToPost(const char *aPostDataURL, nsIFile **aTmpFile)
{
  nsresult rv;
  PRInt64 fileSize;
  nsCAutoString filename;

  
  nsCOMPtr<nsIFile> inFile;
  rv = NS_GetFileFromURLSpec(nsDependentCString(aPostDataURL),
                             getter_AddRefs(inFile));
  if (NS_FAILED(rv)) {
    nsCOMPtr<nsILocalFile> localFile;
    rv = NS_NewNativeLocalFile(nsDependentCString(aPostDataURL), PR_FALSE,
                               getter_AddRefs(localFile));
    if (NS_FAILED(rv)) return rv;
    inFile = localFile;
  }
  rv = inFile->GetFileSize(&fileSize);
  if (NS_FAILED(rv)) return rv;
  rv = inFile->GetNativePath(filename);
  if (NS_FAILED(rv)) return rv;

  if (!LL_IS_ZERO(fileSize)) {
    nsCOMPtr<nsIInputStream> inStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStream), inFile);
    if (NS_FAILED(rv)) return rv;

    
    

    nsCOMPtr<nsIFile> tempFile;
    rv = GetPluginTempDir(getter_AddRefs(tempFile));
    if (NS_FAILED(rv))
      return rv;

    nsCAutoString inFileName;
    inFile->GetNativeLeafName(inFileName);
    
    inFileName.Insert(NS_LITERAL_CSTRING("post-"), 0);
    rv = tempFile->AppendNative(inFileName);

    if (NS_FAILED(rv))
      return rv;

    
    rv = tempFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIOutputStream> outStream;
    if (NS_SUCCEEDED(rv)) {
      rv = NS_NewLocalFileOutputStream(getter_AddRefs(outStream),
        tempFile,
        (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE),
        0600); 
    }
    NS_ASSERTION(NS_SUCCEEDED(rv), "Post data file couldn't be created!");
    if (NS_FAILED(rv))
      return rv;

    char buf[1024];
    PRUint32 br, bw;
    PRBool firstRead = PR_TRUE;
    while (1) {
      
      rv = inStream->Read(buf, 1024, &br);
      if (NS_FAILED(rv) || (PRInt32)br <= 0)
        break;
      if (firstRead) {
        
        
        
        

        char *parsedBuf;
        
        
        ParsePostBufferToFixHeaders((const char *)buf, br, &parsedBuf, &bw);
        rv = outStream->Write(parsedBuf, bw, &br);
        nsMemory::Free(parsedBuf);
        if (NS_FAILED(rv) || (bw != br))
          break;

        firstRead = PR_FALSE;
        continue;
      }
      bw = br;
      rv = outStream->Write(buf, bw, &br);
      if (NS_FAILED(rv) || (bw != br))
        break;
    }

    inStream->Close();
    outStream->Close();
    if (NS_SUCCEEDED(rv))
      *aTmpFile = tempFile.forget().get();
  }
  return rv;
}

NS_IMETHODIMP
nsPluginHost::NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  return PLUG_NewPluginNativeWindow(aPluginNativeWindow);
}

NS_IMETHODIMP
nsPluginHost::DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  return PLUG_DeletePluginNativeWindow(aPluginNativeWindow);
}

NS_IMETHODIMP
nsPluginHost::InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner)
{
  
  
  nsPluginTag *plugin = FindPluginForType("application/x-java-vm", PR_FALSE);

  if (!plugin || !plugin->mIsNPRuntimeEnabledJavaPlugin) {
    
    

    return NS_OK;
  }

  nsresult rv = SetUpPluginInstance("application/x-java-vm", nsnull, aOwner);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPluginInstance> instance;
  aOwner->GetInstance(*getter_AddRefs(instance));
  if (!instance)
    return NS_OK;

  instance->DefineJavaProperties();

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPluginName(nsIPluginInstance *aPluginInstance,
                            const char** aPluginName)
{
  nsNPAPIPluginInstance *instance = static_cast<nsNPAPIPluginInstance*>(aPluginInstance);
  if (!instance)
    return NS_ERROR_FAILURE;

  nsNPAPIPlugin* plugin = instance->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  *aPluginName = TagForPlugin(plugin)->mName.get();

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPluginTagForInstance(nsIPluginInstance *aPluginInstance,
                                      nsIPluginTag **aPluginTag)
{
  NS_ENSURE_ARG_POINTER(aPluginInstance);
  NS_ENSURE_ARG_POINTER(aPluginTag);

  nsNPAPIPluginInstance *instance = static_cast<nsNPAPIPluginInstance*>(aPluginInstance);
  nsNPAPIPlugin *plugin = instance->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  *aPluginTag = TagForPlugin(plugin);

  NS_ADDREF(*aPluginTag);
  return NS_OK;
}

#ifdef MAC_CARBON_PLUGINS



#define HIDDEN_PLUGIN_DELAY 125
#define VISIBLE_PLUGIN_DELAY 20
#endif

void nsPluginHost::AddIdleTimeTarget(nsIPluginInstanceOwner* objectFrame, PRBool isVisible)
{
#ifdef MAC_CARBON_PLUGINS
  nsTObserverArray<nsIPluginInstanceOwner*> *targetArray;
  if (isVisible) {
    targetArray = &mVisibleTimerTargets;
  } else {
    targetArray = &mHiddenTimerTargets;
  }

  if (targetArray->Contains(objectFrame)) {
    return;
  }

  targetArray->AppendElement(objectFrame);
  if (targetArray->Length() == 1) {
    if (isVisible) {
      mVisiblePluginTimer->InitWithCallback(this, VISIBLE_PLUGIN_DELAY, nsITimer::TYPE_REPEATING_SLACK);
    } else {
      mHiddenPluginTimer->InitWithCallback(this, HIDDEN_PLUGIN_DELAY, nsITimer::TYPE_REPEATING_SLACK);
    }
  }
#endif
}

void nsPluginHost::RemoveIdleTimeTarget(nsIPluginInstanceOwner* objectFrame)
{
#ifdef MAC_CARBON_PLUGINS
  PRBool visibleRemoved = mVisibleTimerTargets.RemoveElement(objectFrame);
  if (visibleRemoved && mVisibleTimerTargets.IsEmpty()) {
    mVisiblePluginTimer->Cancel();
  }

  PRBool hiddenRemoved = mHiddenTimerTargets.RemoveElement(objectFrame);
  if (hiddenRemoved && mHiddenTimerTargets.IsEmpty()) {
    mHiddenPluginTimer->Cancel();
  }

  NS_ASSERTION(!(hiddenRemoved && visibleRemoved), "Plugin instance received visible and hidden idle event notifications");
#endif
}

NS_IMETHODIMP nsPluginHost::Notify(nsITimer* timer)
{
#ifdef MAC_CARBON_PLUGINS
  if (timer == mVisiblePluginTimer) {
    nsTObserverArray<nsIPluginInstanceOwner*>::ForwardIterator iter(mVisibleTimerTargets);
    while (iter.HasMore()) {
      iter.GetNext()->SendIdleEvent();
    }
    return NS_OK;
  } else if (timer == mHiddenPluginTimer) {
    nsTObserverArray<nsIPluginInstanceOwner*>::ForwardIterator iter(mHiddenTimerTargets);
    while (iter.HasMore()) {
      iter.GetNext()->SendIdleEvent();
    }
    return NS_OK;
  }
#endif
  return NS_ERROR_FAILURE;
}

#ifdef XP_WIN


static void
CheckForDisabledWindows()
{
  nsCOMPtr<nsIWindowMediator> wm(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!wm)
    return;

  nsCOMPtr<nsISimpleEnumerator> windowList;
  wm->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowList));
  if (!windowList)
    return;

  PRBool haveWindows;
  do {
    windowList->HasMoreElements(&haveWindows);
    if (!haveWindows)
      return;

    nsCOMPtr<nsISupports> supportsWindow;
    windowList->GetNext(getter_AddRefs(supportsWindow));
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(supportsWindow));
    if (baseWin) {
      PRBool aFlag;
      nsCOMPtr<nsIWidget> widget;
      baseWin->GetMainWidget(getter_AddRefs(widget));
      if (widget && !widget->GetParent() &&
          NS_SUCCEEDED(widget->IsVisible(aFlag)) && aFlag == PR_TRUE &&
          NS_SUCCEEDED(widget->IsEnabled(&aFlag)) && aFlag == PR_FALSE) {
        nsIWidget * child = widget->GetFirstChild();
        PRBool enable = PR_TRUE;
        while (child)  {
          nsWindowType aType;
          if (NS_SUCCEEDED(child->GetWindowType(aType)) &&
              aType == eWindowType_dialog) {
            enable = PR_FALSE;
            break;
          }
          child = child->GetNextSibling();
        }
        if (enable) {
          widget->Enable(PR_TRUE);
        }
      }
    }
  } while (haveWindows);
}
#endif

void
nsPluginHost::PluginCrashed(nsNPAPIPlugin* aPlugin,
                            const nsAString& pluginDumpID,
                            const nsAString& browserDumpID)
{
  nsPluginTag* crashedPluginTag = TagForPlugin(aPlugin);

  
  
  PRBool submittedCrashReport = PR_FALSE;
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  nsCOMPtr<nsIWritablePropertyBag2> propbag =
    do_CreateInstance("@mozilla.org/hash-property-bag;1");
  if (obsService && propbag) {
    propbag->SetPropertyAsAString(NS_LITERAL_STRING("pluginDumpID"),
                                  pluginDumpID);
    propbag->SetPropertyAsAString(NS_LITERAL_STRING("browserDumpID"),
                                  browserDumpID);
    propbag->SetPropertyAsBool(NS_LITERAL_STRING("submittedCrashReport"),
                               submittedCrashReport);
    obsService->NotifyObservers(propbag, "plugin-crashed", nsnull);
    
    propbag->GetPropertyAsBool(NS_LITERAL_STRING("submittedCrashReport"),
                               &submittedCrashReport);
  }

  

  for (PRUint32 i = mInstances.Length(); i > 0; i--) {
    nsNPAPIPluginInstance* instance = mInstances[i - 1];
    if (instance->GetPlugin() == aPlugin) {
      
      
      nsCOMPtr<nsIDOMElement> domElement;
      instance->GetDOMElement(getter_AddRefs(domElement));
      nsCOMPtr<nsIObjectLoadingContent> objectContent(do_QueryInterface(domElement));
      if (objectContent) {
        objectContent->PluginCrashed(crashedPluginTag, pluginDumpID, browserDumpID,
                                     submittedCrashReport);
      }

      instance->Destroy();
      mInstances.RemoveElement(instance);
      OnPluginInstanceDestroyed(crashedPluginTag);
    }
  }

  
  
  

  crashedPluginTag->mEntryPoint = nsnull;

#ifdef XP_WIN
  CheckForDisabledWindows();
#endif
}

nsNPAPIPluginInstance*
nsPluginHost::FindInstance(const char *mimetype)
{
  for (PRUint32 i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance* instance = mInstances[i];

    const char* mt;
    nsresult rv = instance->GetMIMEType(&mt);
    if (NS_FAILED(rv))
      continue;

    if (PL_strcasecmp(mt, mimetype) == 0)
      return instance;
  }

  return nsnull;
}

nsNPAPIPluginInstance*
nsPluginHost::FindStoppedInstance(const char *url)
{
  for (PRUint32 i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance *instance = mInstances[i];

    nsIURI *uri = instance->GetURI();
    if (!uri)
      continue;

    nsCAutoString spec;
    uri->GetSpec(spec);
    if (!PL_strcmp(url, spec.get()) && !instance->IsRunning())
      return instance;
  }

  return nsnull;
}

nsNPAPIPluginInstance*
nsPluginHost::FindOldestStoppedInstance()
{
  nsNPAPIPluginInstance *oldestInstance = nsnull;
  TimeStamp oldestTime = TimeStamp::Now();
  for (PRUint32 i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance *instance = mInstances[i];
    if (instance->IsRunning())
      continue;

    TimeStamp time = instance->LastStopTime();
    if (time < oldestTime) {
      oldestTime = time;
      oldestInstance = instance;
    }
  }

  return oldestInstance;
}

PRUint32
nsPluginHost::StoppedInstanceCount()
{
  PRUint32 stoppedCount = 0;
  for (PRUint32 i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance *instance = mInstances[i];
    if (!instance->IsRunning())
      stoppedCount++;
  }
  return stoppedCount;
}

nsTArray< nsRefPtr<nsNPAPIPluginInstance> >*
nsPluginHost::InstanceArray()
{
  return &mInstances;
}

void 
nsPluginHost::DestroyRunningInstances(nsISupportsArray* aReloadDocs, nsPluginTag* aPluginTag)
{
  for (PRInt32 i = mInstances.Length(); i > 0; i--) {
    nsNPAPIPluginInstance *instance = mInstances[i - 1];
    if (instance->IsRunning() && (!aPluginTag || aPluginTag == TagForPlugin(instance->GetPlugin()))) {
      instance->SetWindow(nsnull);
      instance->Stop();

      
      
      
      if (aReloadDocs) {
        nsCOMPtr<nsIPluginInstanceOwner> owner;
        instance->GetOwner(getter_AddRefs(owner));
        if (owner) {
          nsCOMPtr<nsIDocument> doc;
          owner->GetDocument(getter_AddRefs(doc));
          if (doc && aReloadDocs->IndexOf(doc) == -1)  
            aReloadDocs->AppendElement(doc);
        }
      }

      
      nsPluginTag* pluginTag = TagForPlugin(instance->GetPlugin());
      instance->SetWindow(nsnull);
      instance->Destroy();
      mInstances.RemoveElement(instance);
      OnPluginInstanceDestroyed(pluginTag);
    }
  }
}



class nsPluginDestroyRunnable : public nsRunnable,
                                public PRCList
{
public:
  nsPluginDestroyRunnable(nsIPluginInstance *aInstance)
    : mInstance(aInstance)
  {
    PR_INIT_CLIST(this);
    PR_APPEND_LINK(this, &sRunnableListHead);
  }

  virtual ~nsPluginDestroyRunnable()
  {
    PR_REMOVE_LINK(this);
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIPluginInstance> instance;

    
    
    
    instance.swap(mInstance);

    if (PluginDestructionGuard::DelayDestroy(instance)) {
      
      
      return NS_OK;
    }

    nsPluginDestroyRunnable *r =
      static_cast<nsPluginDestroyRunnable*>(PR_NEXT_LINK(&sRunnableListHead));

    while (r != &sRunnableListHead) {
      if (r != this && r->mInstance == instance) {
        
        
        return NS_OK;
      }
      r = static_cast<nsPluginDestroyRunnable*>(PR_NEXT_LINK(r));
    }

    PLUGIN_LOG(PLUGIN_LOG_NORMAL,
               ("Doing delayed destroy of instance %p\n", instance.get()));

    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    if (host)
      host->StopPluginInstance(instance);

    PLUGIN_LOG(PLUGIN_LOG_NORMAL,
               ("Done with delayed destroy of instance %p\n", instance.get()));

    return NS_OK;
  }

protected:
  nsCOMPtr<nsIPluginInstance> mInstance;

  static PRCList sRunnableListHead;
};

PRCList nsPluginDestroyRunnable::sRunnableListHead =
  PR_INIT_STATIC_CLIST(&nsPluginDestroyRunnable::sRunnableListHead);

PRCList PluginDestructionGuard::sListHead =
  PR_INIT_STATIC_CLIST(&PluginDestructionGuard::sListHead);

PluginDestructionGuard::~PluginDestructionGuard()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");

  PR_REMOVE_LINK(this);

  if (mDelayedDestroy) {
    
    
    
    nsRefPtr<nsPluginDestroyRunnable> evt =
      new nsPluginDestroyRunnable(mInstance);

    NS_DispatchToMainThread(evt);
  }
}


PRBool
PluginDestructionGuard::DelayDestroy(nsIPluginInstance *aInstance)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");
  NS_ASSERTION(aInstance, "Uh, I need an instance!");

  
  

  PluginDestructionGuard *g =
    static_cast<PluginDestructionGuard*>(PR_LIST_HEAD(&sListHead));

  while (g != &sListHead) {
    if (g->mInstance == aInstance) {
      g->mDelayedDestroy = PR_TRUE;

      return PR_TRUE;
    }
    g = static_cast<PluginDestructionGuard*>(PR_NEXT_LINK(g));    
  }

  return PR_FALSE;
}
