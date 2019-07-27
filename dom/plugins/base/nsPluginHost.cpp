






#include "nscore.h"
#include "nsPluginHost.h"

#include <cstdlib>
#include <stdio.h>
#include "prio.h"
#include "prmem.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsNPAPIPluginInstance.h"
#include "nsPluginInstanceOwner.h"
#include "nsObjectLoadingContent.h"
#include "nsIHTTPHeaderListener.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIObserverService.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIHttpChannel.h"
#include "nsIUploadChannel.h"
#include "nsIByteRangeRequest.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIURL.h"
#include "nsTArray.h"
#include "nsReadableUtils.h"
#include "nsProtocolProxyService.h"
#include "nsIStreamConverterService.h"
#include "nsIFile.h"
#if defined(XP_MACOSX)
#include "nsILocalFileMac.h"
#endif
#include "nsISeekableStream.h"
#include "nsNetUtil.h"
#include "nsIProgressEventSink.h"
#include "nsIDocument.h"
#include "nsPluginLogging.h"
#include "nsIScriptChannel.h"
#include "nsIBlocklistService.h"
#include "nsVersionComparator.h"
#include "nsIObjectLoadingContent.h"
#include "nsIWritablePropertyBag2.h"
#include "nsICategoryManager.h"
#include "nsPluginStreamListenerPeer.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/LoadInfo.h"
#include "mozilla/plugins/PluginAsyncSurrogate.h"
#include "mozilla/plugins/PluginBridge.h"
#include "mozilla/plugins/PluginTypes.h"
#include "mozilla/Preferences.h"

#include "nsEnumeratorUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsISupportsPrimitives.h"

#include "nsXULAppAPI.h"
#include "nsIXULRuntime.h"


#include "nsIWindowWatcher.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"

#include "nsNetCID.h"
#include "prprf.h"
#include "nsThreadUtils.h"
#include "nsIInputStreamTee.h"

#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsPluginDirServiceProvider.h"

#include "nsUnicharUtils.h"
#include "nsPluginManifestLineReader.h"

#include "nsIWeakReferenceUtils.h"
#include "nsIPresShell.h"
#include "nsPluginNativeWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#include "nsIImageLoadingContent.h"
#include "mozilla/Preferences.h"
#include "nsVersionComparator.h"
#include "nsNullPrincipal.h"

#if defined(XP_WIN)
#include "nsIWindowMediator.h"
#include "nsIBaseWindow.h"
#include "windows.h"
#include "winbase.h"
#endif

#ifdef ANDROID
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#endif

#if MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

using namespace mozilla;
using mozilla::TimeStamp;
using mozilla::plugins::PluginTag;
using mozilla::plugins::PluginAsyncSurrogate;



#define NS_ITERATIVE_UNREF_LIST(type_, list_, mNext_)                \
  {                                                                  \
    while (list_) {                                                  \
      type_ temp = list_->mNext_;                                    \
      list_->mNext_ = nullptr;                                       \
      list_ = temp;                                                  \
    }                                                                \
  }



#define kPluginTmpDirName NS_LITERAL_CSTRING("plugtmp")

static const char *kPrefWhitelist = "plugin.allowed_types";
static const char *kPrefDisableFullPage = "plugin.disable_full_page_plugin_for_types";
static const char *kPrefJavaMIME = "plugin.java.mime";



static const char *kPrefUnloadPluginTimeoutSecs = "dom.ipc.plugins.unloadTimeoutSecs";
static const uint32_t kDefaultPluginUnloadingTimeout = 30;




















static const char *kPluginRegistryVersion = "0.17";

static const char *kMinimumRegistryVersion = "0.9";

static const char kDirectoryServiceContractID[] = "@mozilla.org/file/directory_service;1";

#define kPluginRegistryFilename NS_LITERAL_CSTRING("pluginreg.dat")

#ifdef PLUGIN_LOGGING
PRLogModuleInfo* nsPluginLogging::gNPNLog = nullptr;
PRLogModuleInfo* nsPluginLogging::gNPPLog = nullptr;
PRLogModuleInfo* nsPluginLogging::gPluginLog = nullptr;
#endif


#define NS_PREF_MAX_NUM_CACHED_INSTANCES "browser.plugins.max_num_cached_plugins"


#define DEFAULT_NUMBER_OF_STOPPED_INSTANCES 50

nsIFile *nsPluginHost::sPluginTempDir;
nsPluginHost *nsPluginHost::sInst;

NS_IMPL_ISUPPORTS0(nsInvalidPluginTag)

nsInvalidPluginTag::nsInvalidPluginTag(const char* aFullPath, int64_t aLastModifiedTime)
: mFullPath(aFullPath),
  mLastModifiedTime(aLastModifiedTime),
  mSeen(false)
{}

nsInvalidPluginTag::~nsInvalidPluginTag()
{}


static bool
IsTypeInList(nsCString &aMimeType, nsCString aTypeList)
{
  nsAutoCString searchStr;
  searchStr.Assign(',');
  searchStr.Append(aTypeList);
  searchStr.Append(',');

  nsACString::const_iterator start, end;

  searchStr.BeginReading(start);
  searchStr.EndReading(end);

  nsAutoCString commaSeparated;
  commaSeparated.Assign(',');
  commaSeparated += aMimeType;
  commaSeparated.Append(',');

  return FindInReadable(commaSeparated, start, end);
}


static
bool ReadSectionHeader(nsPluginManifestLineReader& reader, const char *token)
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
      return true;
    }
  } while (reader.NextLine());
  return false;
}

static bool UnloadPluginsASAP()
{
  return (Preferences::GetUint(kPrefUnloadPluginTimeoutSecs, kDefaultPluginUnloadingTimeout) == 0);
}

nsPluginHost::nsPluginHost()
  
  
{
  
  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    IncrementChromeEpoch();
  }

  
  
  mOverrideInternalTypes =
    Preferences::GetBool("plugin.override_internal_types", false);

  mPluginsDisabled = Preferences::GetBool("plugin.disable", false);
  mPluginsClickToPlay = Preferences::GetBool("plugins.click_to_play", false);

  Preferences::AddStrongObserver(this, "plugin.disable");
  Preferences::AddStrongObserver(this, "plugins.click_to_play");

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
    obsService->AddObserver(this, "blocklist-updated", false);
#ifdef MOZ_WIDGET_ANDROID
    obsService->AddObserver(this, "application-foreground", false);
    obsService->AddObserver(this, "application-background", false);
#endif
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
}

nsPluginHost::~nsPluginHost()
{
  PLUGIN_LOG(PLUGIN_LOG_ALWAYS,("nsPluginHost::dtor\n"));

  UnloadPlugins();
  sInst = nullptr;
}

NS_IMPL_ISUPPORTS(nsPluginHost,
                  nsIPluginHost,
                  nsIObserver,
                  nsITimerCallback,
                  nsISupportsWeakReference)

already_AddRefed<nsPluginHost>
nsPluginHost::GetInst()
{
  if (!sInst) {
    sInst = new nsPluginHost();
    if (!sInst)
      return nullptr;
    NS_ADDREF(sInst);
  }

  nsRefPtr<nsPluginHost> inst = sInst;
  return inst.forget();
}

bool nsPluginHost::IsRunningPlugin(nsPluginTag * aPluginTag)
{
  if (!aPluginTag || !aPluginTag->mPlugin) {
    return false;
  }

  if (aPluginTag->mContentProcessRunningCount) {
    return true;
  }

  for (uint32_t i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance *instance = mInstances[i].get();
    if (instance &&
        instance->GetPlugin() == aPluginTag->mPlugin &&
        instance->IsRunning()) {
      return true;
    }
  }

  return false;
}

nsresult nsPluginHost::ReloadPlugins()
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::ReloadPlugins Begin\n"));

  nsresult rv = NS_OK;

  
  
  if (!mPluginsLoaded)
    return LoadPlugins();

  
  
  

  
  
  
  
  bool pluginschanged = true;
  FindPlugins(false, &pluginschanged);

  
  if (!pluginschanged)
    return NS_ERROR_PLUGINS_PLUGINSNOTCHANGED;

  
  nsRefPtr<nsPluginTag> prev;
  nsRefPtr<nsPluginTag> next;

  for (nsRefPtr<nsPluginTag> p = mPlugins; p != nullptr;) {
    next = p->mNext;

    
    if (!IsRunningPlugin(p)) {
      if (p == mPlugins)
        mPlugins = next;
      else
        prev->mNext = next;

      p->mNext = nullptr;

      
      p->TryUnloadPlugin(false);

      p = next;
      continue;
    }

    prev = p;
    p = next;
  }

  
  mPluginsLoaded = false;

  
  rv = LoadPlugins();

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::ReloadPlugins End\n"));

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

  nsAutoCString uaString;
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
    *retstring = nullptr;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHost::UserAgent return=%s\n", *retstring));

  return res;
}

nsresult nsPluginHost::GetURL(nsISupports* pluginInst,
                              const char* url,
                              const char* target,
                              nsNPAPIPluginStreamListener* streamListener,
                              const char* altHost,
                              const char* referrer,
                              bool forceJSEnabled)
{
  return GetURLWithHeaders(static_cast<nsNPAPIPluginInstance*>(pluginInst),
                           url, target, streamListener, altHost, referrer,
                           forceJSEnabled, 0, nullptr);
}

nsresult nsPluginHost::GetURLWithHeaders(nsNPAPIPluginInstance* pluginInst,
                                         const char* url,
                                         const char* target,
                                         nsNPAPIPluginStreamListener* streamListener,
                                         const char* altHost,
                                         const char* referrer,
                                         bool forceJSEnabled,
                                         uint32_t getHeadersLength,
                                         const char* getHeaders)
{
  
  
  if (!target && !streamListener)
    return NS_ERROR_ILLEGAL_VALUE;

  nsresult rv = DoURLLoadSecurityCheck(pluginInst, url);
  if (NS_FAILED(rv))
    return rv;

  if (target) {
    nsRefPtr<nsPluginInstanceOwner> owner = pluginInst->GetOwner();
    if (owner) {
      if ((0 == PL_strcmp(target, "newwindow")) ||
          (0 == PL_strcmp(target, "_new")))
        target = "_blank";
      else if (0 == PL_strcmp(target, "_current"))
        target = "_self";

      rv = owner->GetURL(url, target, nullptr, nullptr, 0);
    }
  }

  if (streamListener)
    rv = NewPluginURLStream(NS_ConvertUTF8toUTF16(url), pluginInst,
                            streamListener, nullptr,
                            getHeaders, getHeadersLength);

  return rv;
}

nsresult nsPluginHost::PostURL(nsISupports* pluginInst,
                               const char* url,
                               uint32_t postDataLen,
                               const char* postData,
                               bool isFile,
                               const char* target,
                               nsNPAPIPluginStreamListener* streamListener,
                               const char* altHost,
                               const char* referrer,
                               bool forceJSEnabled,
                               uint32_t postHeadersLength,
                               const char* postHeaders)
{
  nsresult rv;

  
  
  
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
    uint32_t newDataToPostLen;
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
    nsRefPtr<nsPluginInstanceOwner> owner = instance->GetOwner();
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
    rv = NewPluginURLStream(NS_ConvertUTF8toUTF16(url), instance,
                            streamListener,
                            postStream, postHeaders, postHeadersLength);

  return rv;
}











nsresult nsPluginHost::FindProxyForURL(const char* url, char* *result)
{
  if (!url || !result) {
    return NS_ERROR_INVALID_ARG;
  }
  nsresult res;

  nsCOMPtr<nsIProtocolProxyService> proxyService =
    do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res) || !proxyService)
    return res;

  nsRefPtr<nsProtocolProxyService> rawProxyService = do_QueryObject(proxyService);
  if (!rawProxyService) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIIOService> ioService = do_GetService(NS_IOSERVICE_CONTRACTID, &res);
  if (NS_FAILED(res) || !ioService)
    return res;

  
  nsCOMPtr<nsIChannel> tempChannel;
  res = ioService->NewChannel(nsDependentCString(url), nullptr, nullptr, getter_AddRefs(tempChannel));
  if (NS_FAILED(res))
    return res;

  nsCOMPtr<nsIProxyInfo> pi;

  
  res = rawProxyService->DeprecatedBlockingResolve(tempChannel, 0, getter_AddRefs(pi));
  if (NS_FAILED(res))
    return res;

  nsAutoCString host, type;
  int32_t port = -1;

  
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
    NS_ASSERTION(false, "Unknown proxy type!");
    *result = PL_strdup("DIRECT");
  }

  if (nullptr == *result)
    res = NS_ERROR_OUT_OF_MEMORY;

  return res;
}

nsresult nsPluginHost::UnloadPlugins()
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL, ("nsPluginHost::UnloadPlugins Called\n"));

  if (!mPluginsLoaded)
    return NS_OK;

  
  
  DestroyRunningInstances(nullptr);

  nsPluginTag *pluginTag;
  for (pluginTag = mPlugins; pluginTag; pluginTag = pluginTag->mNext) {
    pluginTag->TryUnloadPlugin(true);
  }

  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);

  
  if (sPluginTempDir) {
    sPluginTempDir->Remove(true);
    NS_RELEASE(sPluginTempDir);
  }

#ifdef XP_WIN
  if (mPrivateDirServiceProvider) {
    nsCOMPtr<nsIDirectoryService> dirService =
      do_GetService(kDirectoryServiceContractID);
    if (dirService)
      dirService->UnregisterProvider(mPrivateDirServiceProvider);
    mPrivateDirServiceProvider = nullptr;
  }
#endif 

  mPluginsLoaded = false;

  return NS_OK;
}

void nsPluginHost::OnPluginInstanceDestroyed(nsPluginTag* aPluginTag)
{
  bool hasInstance = false;
  for (uint32_t i = 0; i < mInstances.Length(); i++) {
    if (TagForPlugin(mInstances[i]->GetPlugin()) == aPluginTag) {
      hasInstance = true;
      break;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!hasInstance) {
    if (UnloadPluginsASAP()) {
      aPluginTag->TryUnloadPlugin(false);
    } else {
      if (aPluginTag->mUnloadTimer) {
        aPluginTag->mUnloadTimer->Cancel();
      } else {
        aPluginTag->mUnloadTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
      }
      uint32_t unloadTimeout = Preferences::GetUint(kPrefUnloadPluginTimeoutSecs,
                                                    kDefaultPluginUnloadingTimeout);
      aPluginTag->mUnloadTimer->InitWithCallback(this,
                                                 1000 * unloadTimeout,
                                                 nsITimer::TYPE_ONE_SHOT);
    }
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

nsresult
nsPluginHost::InstantiatePluginInstance(const nsACString& aMimeType, nsIURI* aURL,
                                        nsObjectLoadingContent *aContent,
                                        nsPluginInstanceOwner** aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);

#ifdef PLUGIN_LOGGING
  nsAutoCString urlSpec;
  if (aURL)
    aURL->GetAsciiSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::InstantiatePlugin Begin mime=%s, url=%s\n",
         PromiseFlatCString(aMimeType).get(), urlSpec.get()));

  PR_LogFlush();
#endif

  if (aMimeType.IsEmpty()) {
    NS_NOTREACHED("Attempting to spawn a plugin with no mime type");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsPluginInstanceOwner> instanceOwner = new nsPluginInstanceOwner();
  if (!instanceOwner) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIContent> ourContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(aContent));
  nsresult rv = instanceOwner->Init(ourContent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsPluginTagType tagType;
  rv = instanceOwner->GetTagType(&tagType);
  if (NS_FAILED(rv)) {
    instanceOwner->Destroy();
    return rv;
  }

  if (tagType != nsPluginTagType_Embed &&
      tagType != nsPluginTagType_Applet &&
      tagType != nsPluginTagType_Object) {
    instanceOwner->Destroy();
    return NS_ERROR_FAILURE;
  }

  rv = SetUpPluginInstance(aMimeType, aURL, instanceOwner);
  if (NS_FAILED(rv)) {
    instanceOwner->Destroy();
    return NS_ERROR_FAILURE;
  }
  const bool isAsyncInit = (rv == NS_PLUGIN_INIT_PENDING);

  nsRefPtr<nsNPAPIPluginInstance> instance;
  rv = instanceOwner->GetInstance(getter_AddRefs(instance));
  if (NS_FAILED(rv)) {
    instanceOwner->Destroy();
    return rv;
  }

  
  if (!isAsyncInit && instance) {
    CreateWidget(instanceOwner);
  }

  
  instanceOwner.forget(aOwner);

#ifdef PLUGIN_LOGGING
  nsAutoCString urlSpec2;
  if (aURL != nullptr) aURL->GetAsciiSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::InstantiatePlugin Finished mime=%s, rv=%d, url=%s\n",
         PromiseFlatCString(aMimeType).get(), rv, urlSpec2.get()));

  PR_LogFlush();
#endif

  return NS_OK;
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
  return nullptr;
}

nsPluginTag*
nsPluginHost::TagForPlugin(nsNPAPIPlugin* aPlugin)
{
  nsPluginTag* pluginTag;
  for (pluginTag = mPlugins; pluginTag; pluginTag = pluginTag->mNext) {
    if (pluginTag->mPlugin == aPlugin) {
      return pluginTag;
    }
  }
  
  NS_ERROR("TagForPlugin has failed");
  return nullptr;
}

nsresult nsPluginHost::SetUpPluginInstance(const nsACString &aMimeType,
                                           nsIURI *aURL,
                                           nsPluginInstanceOwner *aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);

  nsresult rv = TrySetUpPluginInstance(aMimeType, aURL, aOwner);
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }

  
  
  
  
  nsCOMPtr<nsIDocument> document;
  aOwner->GetDocument(getter_AddRefs(document));

  nsCOMPtr<nsIDocument> currentdocument = do_QueryReferent(mCurrentDocument);
  if (document == currentdocument) {
    return rv;
  }

  mCurrentDocument = do_GetWeakReference(document);

  
  if (ReloadPlugins() == NS_ERROR_PLUGINS_PLUGINSNOTCHANGED) {
    return rv;
  }

  return TrySetUpPluginInstance(aMimeType, aURL, aOwner);
}

nsresult
nsPluginHost::TrySetUpPluginInstance(const nsACString &aMimeType,
                                     nsIURI *aURL,
                                     nsPluginInstanceOwner *aOwner)
{
#ifdef PLUGIN_LOGGING
  nsAutoCString urlSpec;
  if (aURL != nullptr) aURL->GetSpec(urlSpec);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_NORMAL,
        ("nsPluginHost::TrySetupPluginInstance Begin mime=%s, owner=%p, url=%s\n",
         PromiseFlatCString(aMimeType).get(), aOwner, urlSpec.get()));

  PR_LogFlush();
#endif

#ifdef XP_WIN
  bool changed;
  if ((mRegKeyHKLM && NS_SUCCEEDED(mRegKeyHKLM->HasChanged(&changed)) && changed) ||
      (mRegKeyHKCU && NS_SUCCEEDED(mRegKeyHKCU->HasChanged(&changed)) && changed)) {
    ReloadPlugins();
  }
#endif

  nsRefPtr<nsNPAPIPlugin> plugin;
  GetPlugin(aMimeType, getter_AddRefs(plugin));
  if (!plugin) {
    return NS_ERROR_FAILURE;
  }

  nsPluginTag* pluginTag = FindNativePluginForType(aMimeType, true);

  NS_ASSERTION(pluginTag, "Must have plugin tag here!");

#if defined(MOZ_WIDGET_ANDROID) && defined(MOZ_CRASHREPORTER)
  if (pluginTag->mIsFlashPlugin) {
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("FlashVersion"), pluginTag->mVersion);
  }
#endif

  nsRefPtr<nsNPAPIPluginInstance> instance = new nsNPAPIPluginInstance();

  
  
  
  aOwner->SetInstance(instance.get());

  
  
  mInstances.AppendElement(instance.get());

  
  
  
  nsresult rv = instance->Initialize(plugin.get(), aOwner, aMimeType);
  if (NS_FAILED(rv)) {
    mInstances.RemoveElement(instance.get());
    aOwner->SetInstance(nullptr);
    return rv;
  }

  
  
  if (pluginTag->mUnloadTimer) {
    pluginTag->mUnloadTimer->Cancel();
  }

#ifdef PLUGIN_LOGGING
  nsAutoCString urlSpec2;
  if (aURL)
    aURL->GetSpec(urlSpec2);

  PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
        ("nsPluginHost::TrySetupPluginInstance Finished mime=%s, rv=%d, owner=%p, url=%s\n",
         PromiseFlatCString(aMimeType).get(), rv, aOwner, urlSpec2.get()));

  PR_LogFlush();
#endif

  return rv;
}

bool
nsPluginHost::HavePluginForType(const nsACString & aMimeType,
                                PluginFilter aFilter)
{
  bool checkEnabled = aFilter & eExcludeDisabled;
  return FindNativePluginForType(aMimeType, checkEnabled);
}

NS_IMETHODIMP
nsPluginHost::GetPluginTagForType(const nsACString& aMimeType,
                                  uint32_t aExcludeFlags,
                                  nsIPluginTag** aResult)
{
  bool includeDisabled = !(aExcludeFlags & eExcludeDisabled);

  nsPluginTag *tag = FindNativePluginForType(aMimeType, true);

  
  if (includeDisabled && !tag) {
    tag = FindNativePluginForType(aMimeType, false);
  }

  if (!tag) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ADDREF(*aResult = tag);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetStateForType(const nsACString &aMimeType,
                              uint32_t aExcludeFlags,
                              uint32_t* aResult)
{
  nsCOMPtr<nsIPluginTag> tag;
  nsresult rv = GetPluginTagForType(aMimeType, aExcludeFlags,
                                    getter_AddRefs(tag));
  NS_ENSURE_SUCCESS(rv, rv);

  return tag->GetEnabledState(aResult);
}

NS_IMETHODIMP
nsPluginHost::GetBlocklistStateForType(const nsACString &aMimeType,
                                       uint32_t aExcludeFlags,
                                       uint32_t *aState)
{
  nsCOMPtr<nsIPluginTag> tag;
  nsresult rv = GetPluginTagForType(aMimeType,
                                    aExcludeFlags,
                                    getter_AddRefs(tag));
  NS_ENSURE_SUCCESS(rv, rv);

  return tag->GetBlocklistState(aState);
}

NS_IMETHODIMP
nsPluginHost::GetPermissionStringForType(const nsACString &aMimeType,
                                         uint32_t aExcludeFlags,
                                         nsACString &aPermissionString)
{
  nsCOMPtr<nsIPluginTag> tag;
  nsresult rv = GetPluginTagForType(aMimeType, aExcludeFlags,
                                    getter_AddRefs(tag));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(tag, NS_ERROR_FAILURE);

  aPermissionString.Truncate();
  uint32_t blocklistState;
  rv = tag->GetBlocklistState(&blocklistState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (blocklistState == nsIBlocklistService::STATE_VULNERABLE_UPDATE_AVAILABLE ||
      blocklistState == nsIBlocklistService::STATE_VULNERABLE_NO_UPDATE) {
    aPermissionString.AssignLiteral("plugin-vulnerable:");
  }
  else {
    aPermissionString.AssignLiteral("plugin:");
  }

  nsCString niceName;
  rv = tag->GetNiceName(niceName);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(!niceName.IsEmpty(), NS_ERROR_FAILURE);

  aPermissionString.Append(niceName);

  return NS_OK;
}

bool
nsPluginHost::HavePluginForExtension(const nsACString & aExtension,
                                      nsACString & aMimeType,
                                     PluginFilter aFilter)
{
  bool checkEnabled = aFilter & eExcludeDisabled;
  return FindNativePluginForExtension(aExtension, aMimeType, checkEnabled);
}

void
nsPluginHost::GetPlugins(nsTArray<nsRefPtr<nsPluginTag> >& aPluginArray)
{
  aPluginArray.Clear();

  LoadPlugins();

  nsPluginTag* plugin = mPlugins;
  while (plugin != nullptr) {
    if (plugin->IsEnabled()) {
      aPluginArray.AppendElement(plugin);
    }
    plugin = plugin->mNext;
  }
}

NS_IMETHODIMP
nsPluginHost::GetPluginTags(uint32_t* aPluginCount, nsIPluginTag*** aResults)
{
  LoadPlugins();

  uint32_t count = 0;
  nsRefPtr<nsPluginTag> plugin = mPlugins;
  while (plugin != nullptr) {
    count++;
    plugin = plugin->mNext;
  }

  *aResults = static_cast<nsIPluginTag**>
                         (moz_xmalloc(count * sizeof(**aResults)));
  if (!*aResults)
    return NS_ERROR_OUT_OF_MEMORY;

  *aPluginCount = count;

  plugin = mPlugins;
  for (uint32_t i = 0; i < count; i++) {
    (*aResults)[i] = plugin;
    NS_ADDREF((*aResults)[i]);
    plugin = plugin->mNext;
  }

  return NS_OK;
}

nsPluginTag*
nsPluginHost::FindPreferredPlugin(const InfallibleTArray<nsPluginTag*>& matches)
{
  
  
  
  
  
  

  if (matches.IsEmpty()) {
    return nullptr;
  }

  nsPluginTag *preferredPlugin = matches[0];
  for (unsigned int i = 1; i < matches.Length(); i++) {
    if (mozilla::Version(matches[i]->mVersion.get()) > preferredPlugin->mVersion.get()) {
      preferredPlugin = matches[i];
    }
  }

  return preferredPlugin;
}

nsPluginTag*
nsPluginHost::FindNativePluginForType(const nsACString & aMimeType,
                                      bool aCheckEnabled)
{
  if (aMimeType.IsEmpty()) {
    return nullptr;
  }

  LoadPlugins();

  InfallibleTArray<nsPluginTag*> matchingPlugins;

  nsPluginTag *plugin = mPlugins;
  while (plugin) {
    if ((!aCheckEnabled || plugin->IsActive()) &&
        plugin->HasMimeType(aMimeType)) {
      matchingPlugins.AppendElement(plugin);
    }
    plugin = plugin->mNext;
  }

  return FindPreferredPlugin(matchingPlugins);
}

nsPluginTag*
nsPluginHost::FindNativePluginForExtension(const nsACString & aExtension,
                                            nsACString & aMimeType,
                                           bool aCheckEnabled)
{
  if (aExtension.IsEmpty()) {
    return nullptr;
  }

  LoadPlugins();

  InfallibleTArray<nsPluginTag*> matchingPlugins;
  nsCString matchingMime; 
  nsPluginTag *plugin = mPlugins;

  while (plugin) {
    if (!aCheckEnabled || plugin->IsActive()) {
      if (plugin->HasExtension(aExtension, matchingMime)) {
        matchingPlugins.AppendElement(plugin);
      }
    }
    plugin = plugin->mNext;
  }

  nsPluginTag *preferredPlugin = FindPreferredPlugin(matchingPlugins);
  if (!preferredPlugin) {
    return nullptr;
  }

  
  preferredPlugin->HasExtension(aExtension, aMimeType);
  return preferredPlugin;
}

static nsresult CreateNPAPIPlugin(nsPluginTag *aPluginTag,
                                  nsNPAPIPlugin **aOutNPAPIPlugin)
{
  
  if (!nsNPAPIPlugin::RunPluginOOP(aPluginTag)) {
    if (aPluginTag->mFullPath.IsEmpty())
      return NS_ERROR_FAILURE;
    nsCOMPtr<nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    file->InitWithPath(NS_ConvertUTF8toUTF16(aPluginTag->mFullPath));
    nsPluginFile pluginFile(file);
    PRLibrary* pluginLibrary = nullptr;

    if (NS_FAILED(pluginFile.LoadPlugin(&pluginLibrary)) || !pluginLibrary)
      return NS_ERROR_FAILURE;

    aPluginTag->mLibrary = pluginLibrary;
  }

  nsresult rv;
  rv = nsNPAPIPlugin::CreatePlugin(aPluginTag, aOutNPAPIPlugin);

  return rv;
}

nsresult nsPluginHost::EnsurePluginLoaded(nsPluginTag* aPluginTag)
{
  nsRefPtr<nsNPAPIPlugin> plugin = aPluginTag->mPlugin;
  if (!plugin) {
    nsresult rv = CreateNPAPIPlugin(aPluginTag, getter_AddRefs(plugin));
    if (NS_FAILED(rv)) {
      return rv;
    }
    aPluginTag->mPlugin = plugin;
  }
  return NS_OK;
}

nsresult
nsPluginHost::GetPluginForContentProcess(uint32_t aPluginId, nsNPAPIPlugin** aPlugin)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  
  LoadPlugins();

  nsPluginTag* pluginTag = PluginWithId(aPluginId);
  if (pluginTag) {
    nsresult rv = EnsurePluginLoaded(pluginTag);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    
    
    
    pluginTag->mContentProcessRunningCount++;
    NS_ADDREF(*aPlugin = pluginTag->mPlugin);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

class nsPluginUnloadRunnable : public nsRunnable
{
public:
  explicit nsPluginUnloadRunnable(uint32_t aPluginId) : mPluginId(aPluginId) {}

  NS_IMETHOD Run()
  {
    nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
    if (!host) {
      return NS_OK;
    }
    nsPluginTag* pluginTag = host->PluginWithId(mPluginId);
    if (!pluginTag) {
      return NS_OK;
    }

    MOZ_ASSERT(pluginTag->mContentProcessRunningCount > 0);
    pluginTag->mContentProcessRunningCount--;

    if (!pluginTag->mContentProcessRunningCount) {
      if (!host->IsRunningPlugin(pluginTag)) {
        pluginTag->TryUnloadPlugin(false);
      }
    }
    return NS_OK;
  }

protected:
  uint32_t mPluginId;
};

void
nsPluginHost::NotifyContentModuleDestroyed(uint32_t aPluginId)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  
  
  nsRefPtr<nsPluginUnloadRunnable> runnable =
    new nsPluginUnloadRunnable(aPluginId);
  NS_DispatchToMainThread(runnable);
}

nsresult nsPluginHost::GetPlugin(const nsACString &aMimeType,
                                 nsNPAPIPlugin** aPlugin)
{
  nsresult rv = NS_ERROR_FAILURE;
  *aPlugin = nullptr;

  
  LoadPlugins();

  nsPluginTag* pluginTag = FindNativePluginForType(aMimeType, true);
  if (pluginTag) {
    rv = NS_OK;
    PLUGIN_LOG(PLUGIN_LOG_BASIC,
    ("nsPluginHost::GetPlugin Begin mime=%s, plugin=%s\n",
     PromiseFlatCString(aMimeType).get(), pluginTag->mFileName.get()));

#ifdef DEBUG
    if (!pluginTag->mFileName.IsEmpty())
      printf("For %s found plugin %s\n",
             PromiseFlatCString(aMimeType).get(), pluginTag->mFileName.get());
#endif

    rv = EnsurePluginLoaded(pluginTag);
    if (NS_FAILED(rv)) {
      return rv;
    }

    NS_ADDREF(*aPlugin = pluginTag->mPlugin);
    return NS_OK;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::GetPlugin End mime=%s, rv=%d, plugin=%p name=%s\n",
   PromiseFlatCString(aMimeType).get(), rv, *aPlugin,
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
                                const InfallibleTArray<nsCString>& sites,
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

  
  for (uint32_t i = 0; i < sites.Length(); ++i) {
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
nsPluginHost::RegisterPlayPreviewMimeType(const nsACString& mimeType,
                                          bool ignoreCTP,
                                          const nsACString& redirectURL,
                                          const nsACString& whitelist)
{
  nsAutoCString mt(mimeType);
  nsAutoCString url(redirectURL);
  if (url.Length() == 0) {
    
    url.AssignLiteral("data:application/x-moz-playpreview;,");
    url.Append(mimeType);
  }
  nsAutoCString wl(whitelist);

  nsRefPtr<nsPluginPlayPreviewInfo> playPreview =
    new nsPluginPlayPreviewInfo(mt.get(), ignoreCTP, url.get(), wl.get());
  mPlayPreviewMimeTypes.AppendElement(playPreview);
  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::UnregisterPlayPreviewMimeType(const nsACString& mimeType)
{
  nsAutoCString mimeTypeToRemove(mimeType);
  for (uint32_t i = mPlayPreviewMimeTypes.Length(); i > 0; i--) {
    nsRefPtr<nsPluginPlayPreviewInfo> pp = mPlayPreviewMimeTypes[i - 1];
    if (PL_strcasecmp(pp.get()->mMimeType.get(), mimeTypeToRemove.get()) == 0) {
      mPlayPreviewMimeTypes.RemoveElementAt(i - 1);
      break;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::GetPlayPreviewInfo(const nsACString& mimeType,
                                 nsIPluginPlayPreviewInfo** aResult)
{
  nsAutoCString mimeTypeToFind(mimeType);
  for (uint32_t i = 0; i < mPlayPreviewMimeTypes.Length(); i++) {
    nsRefPtr<nsPluginPlayPreviewInfo> pp = mPlayPreviewMimeTypes[i];
    if (PL_strcasecmp(pp.get()->mMimeType.get(), mimeTypeToFind.get()) == 0) {
      *aResult = new nsPluginPlayPreviewInfo(pp.get());
      NS_ADDREF(*aResult);
      return NS_OK;
    }
  }
  *aResult = nullptr;
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsPluginHost::ClearSiteData(nsIPluginTag* plugin, const nsACString& domain,
                            uint64_t flags, int64_t maxAge)
{
  
  NS_ENSURE_ARG(maxAge >= 0 || maxAge == -1);

  
  if (!IsLiveTag(plugin)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsPluginTag* tag = static_cast<nsPluginTag*>(plugin);

  if (!tag->IsEnabled()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  if (!tag->mIsFlashPlugin && !tag->mPlugin) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv = EnsurePluginLoaded(tag);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PluginLibrary* library = tag->mPlugin->GetLibrary();

  
  if (domain.IsVoid()) {
    return library->NPP_ClearSiteData(nullptr, flags, maxAge);
  }

  
  InfallibleTArray<nsCString> sites;
  rv = library->NPP_GetSitesWithData(sites);
  NS_ENSURE_SUCCESS(rv, rv);

  
  InfallibleTArray<nsCString> matches;
  rv = EnumerateSiteData(domain, sites, matches, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (uint32_t i = 0; i < matches.Length(); ++i) {
    const nsCString& match = matches[i];
    rv = library->NPP_ClearSiteData(match.get(), flags, maxAge);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPluginHost::SiteHasData(nsIPluginTag* plugin, const nsACString& domain,
                          bool* result)
{
  
  if (!IsLiveTag(plugin)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsPluginTag* tag = static_cast<nsPluginTag*>(plugin);

  
  
  
  if (!tag->mIsFlashPlugin && !tag->mPlugin) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv = EnsurePluginLoaded(tag);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PluginLibrary* library = tag->mPlugin->GetLibrary();

  
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

nsPluginHost::SpecialType
nsPluginHost::GetSpecialType(const nsACString & aMIMEType)
{
  if (aMIMEType.LowerCaseEqualsASCII("application/x-shockwave-flash") ||
      aMIMEType.LowerCaseEqualsASCII("application/futuresplash")) {
    return eSpecialType_Flash;
  }

  if (aMIMEType.LowerCaseEqualsASCII("application/x-silverlight") ||
      aMIMEType.LowerCaseEqualsASCII("application/x-silverlight-2")) {
    return eSpecialType_Silverlight;
  }

  if (aMIMEType.LowerCaseEqualsASCII("audio/x-pn-realaudio-plugin")) {
    NS_WARNING("You are loading RealPlayer");
    return eSpecialType_RealPlayer;
  }

  if (aMIMEType.LowerCaseEqualsASCII("application/pdf")) {
    return eSpecialType_PDF;
  }

  
  
  const nsACString &noParam = Substring(aMIMEType, 0, aMIMEType.FindChar(';'));

  
  
  nsAdoptingCString javaMIME = Preferences::GetCString(kPrefJavaMIME);
  if ((!javaMIME.IsEmpty() && noParam.LowerCaseEqualsASCII(javaMIME)) ||
      noParam.LowerCaseEqualsASCII("application/x-java-vm") ||
      noParam.LowerCaseEqualsASCII("application/x-java-applet") ||
      noParam.LowerCaseEqualsASCII("application/x-java-bean")) {
    return eSpecialType_Java;
  }

  return eSpecialType_None;
}


bool
nsPluginHost::IsLiveTag(nsIPluginTag* aPluginTag)
{
  nsPluginTag* tag;
  for (tag = mPlugins; tag; tag = tag->mNext) {
    if (tag == aPluginTag) {
      return true;
    }
  }
  return false;
}

nsPluginTag*
nsPluginHost::HaveSamePlugin(const nsPluginTag* aPluginTag)
{
  for (nsPluginTag* tag = mPlugins; tag; tag = tag->mNext) {
    if (tag->HasSameNameAndMimes(aPluginTag)) {
        return tag;
    }
  }
  return nullptr;
}

nsPluginTag*
nsPluginHost::FirstPluginWithPath(const nsCString& path)
{
  for (nsPluginTag* tag = mPlugins; tag; tag = tag->mNext) {
    if (tag->mFullPath.Equals(path)) {
      return tag;
    }
  }
  return nullptr;
}

nsPluginTag*
nsPluginHost::PluginWithId(uint32_t aId)
{
  for (nsPluginTag* tag = mPlugins; tag; tag = tag->mNext) {
    if (tag->mId == aId) {
      return tag;
    }
  }
  return nullptr;
}

namespace {

int64_t GetPluginLastModifiedTime(const nsCOMPtr<nsIFile>& localfile)
{
  PRTime fileModTime = 0;

#if defined(XP_MACOSX)
  
  
  
  nsCOMPtr<nsILocalFileMac> localFileMac = do_QueryInterface(localfile);
  if (localFileMac) {
    localFileMac->GetBundleContentsLastModifiedTime(&fileModTime);
  } else {
    localfile->GetLastModifiedTime(&fileModTime);
  }
#else
  localfile->GetLastModifiedTime(&fileModTime);
#endif

  return fileModTime;
}

bool
GetPluginIsFromExtension(const nsCOMPtr<nsIFile>& pluginFile,
                         const nsCOMArray<nsIFile>& extensionDirs)
{
  for (uint32_t i = 0; i < extensionDirs.Length(); ++i) {
    bool contains;
    if (NS_FAILED(extensionDirs[i]->Contains(pluginFile, &contains)) || !contains) {
      continue;
    }

    return true;
  }

  return false;
}

void
GetExtensionDirectories(nsCOMArray<nsIFile>& dirs)
{
  nsCOMPtr<nsIProperties> dirService = do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
  if (!dirService) {
    return;
  }

  nsCOMPtr<nsISimpleEnumerator> list;
  nsresult rv = dirService->Get(XRE_EXTENSIONS_DIR_LIST,
                                NS_GET_IID(nsISimpleEnumerator),
                                getter_AddRefs(list));
  if (NS_FAILED(rv)) {
    return;
  }

  bool more;
  while (NS_SUCCEEDED(list->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> next;
    if (NS_FAILED(list->GetNext(getter_AddRefs(next)))) {
      break;
    }
    nsCOMPtr<nsIFile> file = do_QueryInterface(next);
    if (file) {
      file->Normalize();
      dirs.AppendElement(file);
    }
  }
}

struct CompareFilesByTime
{
  bool
  LessThan(const nsCOMPtr<nsIFile>& a, const nsCOMPtr<nsIFile>& b) const
  {
    return GetPluginLastModifiedTime(a) < GetPluginLastModifiedTime(b);
  }

  bool
  Equals(const nsCOMPtr<nsIFile>& a, const nsCOMPtr<nsIFile>& b) const
  {
    return GetPluginLastModifiedTime(a) == GetPluginLastModifiedTime(b);
  }
};

} 

void
nsPluginHost::AddPluginTag(nsPluginTag* aPluginTag)
{
  aPluginTag->mNext = mPlugins;
  mPlugins = aPluginTag;

  if (aPluginTag->IsActive()) {
    nsAdoptingCString disableFullPage =
      Preferences::GetCString(kPrefDisableFullPage);
    for (uint32_t i = 0; i < aPluginTag->mMimeTypes.Length(); i++) {
      if (!IsTypeInList(aPluginTag->mMimeTypes[i], disableFullPage)) {
        RegisterWithCategoryManager(aPluginTag->mMimeTypes[i],
                                    ePluginRegister);
      }
    }
  }
}

typedef NS_NPAPIPLUGIN_CALLBACK(char *, NP_GETMIMEDESCRIPTION)(void);

nsresult nsPluginHost::ScanPluginsDirectory(nsIFile *pluginsDir,
                                            bool aCreatePluginList,
                                            bool *aPluginsChanged)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  NS_ENSURE_ARG_POINTER(aPluginsChanged);
  nsresult rv;

  *aPluginsChanged = false;

#ifdef PLUGIN_LOGGING
  nsAutoCString dirPath;
  pluginsDir->GetNativePath(dirPath);
  PLUGIN_LOG(PLUGIN_LOG_BASIC,
  ("nsPluginHost::ScanPluginsDirectory dir=%s\n", dirPath.get()));
#endif

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = pluginsDir->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv))
    return rv;

  nsAutoTArray<nsCOMPtr<nsIFile>, 6> pluginFiles;

  bool hasMore;
  while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv))
      continue;
    nsCOMPtr<nsIFile> dirEntry(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv))
      continue;

    
    
    dirEntry->Normalize();

    if (nsPluginsDir::IsPluginFile(dirEntry)) {
      pluginFiles.AppendElement(dirEntry);
    }
  }

  pluginFiles.Sort(CompareFilesByTime());

  nsCOMArray<nsIFile> extensionDirs;
  GetExtensionDirectories(extensionDirs);

  bool warnOutdated = false;

  for (int32_t i = (pluginFiles.Length() - 1); i >= 0; i--) {
    nsCOMPtr<nsIFile>& localfile = pluginFiles[i];

    nsString utf16FilePath;
    rv = localfile->GetPath(utf16FilePath);
    if (NS_FAILED(rv))
      continue;

    const int64_t fileModTime = GetPluginLastModifiedTime(localfile);
    const bool fromExtension = GetPluginIsFromExtension(localfile, extensionDirs);

    
    NS_ConvertUTF16toUTF8 filePath(utf16FilePath);
    nsRefPtr<nsPluginTag> pluginTag;
    RemoveCachedPluginsInfo(filePath.get(), getter_AddRefs(pluginTag));

    bool seenBefore = false;

    if (pluginTag) {
      seenBefore = true;
      
      if (fileModTime != pluginTag->mLastModifiedTime) {
        
        pluginTag = nullptr;

        
        *aPluginsChanged = true;
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
      nsPluginFile pluginFile(localfile);

      
      PRLibrary *library = nullptr;
      nsPluginInfo info;
      memset(&info, 0, sizeof(info));
      nsresult res;
      
      {
        Telemetry::AutoTimer<Telemetry::PLUGIN_LOAD_METADATA> telemetry;
        res = pluginFile.GetPluginInfo(info, &library);
      }
      
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

        
        *aPluginsChanged = true;
        continue;
      }

      pluginTag = new nsPluginTag(&info, fileModTime, fromExtension);
      pluginFile.FreePluginInfo(info);
      if (!pluginTag)
        return NS_ERROR_OUT_OF_MEMORY;

      pluginTag->mLibrary = library;
      uint32_t state;
      rv = pluginTag->GetBlocklistState(&state);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      if (state == nsIBlocklistService::STATE_SOFTBLOCKED && !seenBefore) {
        pluginTag->SetEnabledState(nsIPluginTag::STATE_DISABLED);
      }
      if (state == nsIBlocklistService::STATE_OUTDATED && !seenBefore) {
        warnOutdated = true;
      }

      
      
      
      if (UnloadPluginsASAP()) {
        pluginTag->TryUnloadPlugin(false);
      }
    }

    
    if (!seenBefore) {
      
      *aPluginsChanged = true;
    }

    
    
    if (!nsNPAPIPlugin::RunPluginOOP(pluginTag)) {
      if (HaveSamePlugin(pluginTag)) {
        continue;
      }
    }

    
    if (nsPluginTag* duplicate = FirstPluginWithPath(pluginTag->mFullPath)) {
      if (pluginTag->mLastModifiedTime == duplicate->mLastModifiedTime) {
        continue;
      }
    }

    
    
    if (!aCreatePluginList) {
      return NS_OK;
    }

    AddPluginTag(pluginTag);
  }

  if (warnOutdated) {
    Preferences::SetBool("plugins.update.notifyUser", true);
  }

  return NS_OK;
}

nsresult nsPluginHost::ScanPluginsDirectoryList(nsISimpleEnumerator *dirEnum,
                                                bool aCreatePluginList,
                                                bool *aPluginsChanged)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

    bool hasMore;
    while (NS_SUCCEEDED(dirEnum->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> supports;
      nsresult rv = dirEnum->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv))
        continue;
      nsCOMPtr<nsIFile> nextDir(do_QueryInterface(supports, &rv));
      if (NS_FAILED(rv))
        continue;

      
      bool pluginschanged = false;
      ScanPluginsDirectory(nextDir, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = true;

      
      if (!aCreatePluginList && *aPluginsChanged)
        break;
    }
    return NS_OK;
}

void
nsPluginHost::IncrementChromeEpoch()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  mPluginEpoch++;
}

uint32_t
nsPluginHost::ChromeEpoch()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  return mPluginEpoch;
}

uint32_t
nsPluginHost::ChromeEpochForContent()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Content);
  return mPluginEpoch;
}

void
nsPluginHost::SetChromeEpochForContent(uint32_t aEpoch)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Content);
  mPluginEpoch = aEpoch;
}

#ifdef XP_WIN
static void
WatchRegKey(uint32_t aRoot, nsCOMPtr<nsIWindowsRegKey>& aKey)
{
  if (aKey) {
    return;
  }

  aKey = do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!aKey) {
    return;
  }
  nsresult rv = aKey->Open(aRoot,
                           NS_LITERAL_STRING("Software\\MozillaPlugins"),
                           nsIWindowsRegKey::ACCESS_READ | nsIWindowsRegKey::ACCESS_NOTIFY);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aKey = nullptr;
    return;
  }
  aKey->StartWatching(true);
}
#endif

nsresult nsPluginHost::LoadPlugins()
{
#ifdef ANDROID
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return NS_OK;
  }
#endif
  
  
  if (mPluginsLoaded)
    return NS_OK;

  if (mPluginsDisabled)
    return NS_OK;

#ifdef XP_WIN
  WatchRegKey(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE, mRegKeyHKLM);
  WatchRegKey(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER, mRegKeyHKCU);
#endif

  bool pluginschanged;
  nsresult rv = FindPlugins(true, &pluginschanged);
  if (NS_FAILED(rv))
    return rv;

  
  if (pluginschanged) {
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
      IncrementChromeEpoch();
    }

    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService)
      obsService->NotifyObservers(nullptr, "plugins-list-updated", nullptr);
  }

  return NS_OK;
}

nsresult
nsPluginHost::FindPluginsInContent(bool aCreatePluginList, bool* aPluginsChanged)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Content);

  dom::ContentChild* cp = dom::ContentChild::GetSingleton();
  nsTArray<PluginTag> plugins;
  uint32_t parentEpoch;
  if (!cp->SendFindPlugins(ChromeEpochForContent(), &plugins, &parentEpoch)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (parentEpoch != ChromeEpochForContent()) {
    SetChromeEpochForContent(parentEpoch);
    *aPluginsChanged = true;
    if (!aCreatePluginList) {
      return NS_OK;
    }

    for (size_t i = 0; i < plugins.Length(); i++) {
      PluginTag& tag = plugins[i];

      
      if (PluginWithId(tag.id())) {
        continue;
      }

      nsPluginTag *pluginTag = new nsPluginTag(tag.id(),
                                               tag.name().get(),
                                               tag.description().get(),
                                               tag.filename().get(),
                                               "", 
                                               tag.version().get(),
                                               nsTArray<nsCString>(tag.mimeTypes()),
                                               nsTArray<nsCString>(tag.mimeDescriptions()),
                                               nsTArray<nsCString>(tag.extensions()),
                                               tag.isJavaPlugin(),
                                               tag.isFlashPlugin(),
                                               tag.lastModifiedTime(),
                                               tag.isFromExtension());
      AddPluginTag(pluginTag);
    }
  }

  mPluginsLoaded = true;
  return NS_OK;
}




nsresult nsPluginHost::FindPlugins(bool aCreatePluginList, bool * aPluginsChanged)
{
  Telemetry::AutoTimer<Telemetry::FIND_PLUGINS> telemetry;

  NS_ENSURE_ARG_POINTER(aPluginsChanged);

  *aPluginsChanged = false;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return FindPluginsInContent(aCreatePluginList, aPluginsChanged);
  }

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

  
  
  
  bool pluginschanged = false;

  
  rv = dirService->Get(NS_APP_PLUGINS_DIR_LIST, NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(dirList));
  if (NS_SUCCEEDED(rv)) {
    ScanPluginsDirectoryList(dirList, aCreatePluginList, &pluginschanged);

    if (pluginschanged)
      *aPluginsChanged = true;

    
    
    if (!aCreatePluginList && *aPluginsChanged) {
      NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
      NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
      return NS_OK;
    }
  } else {
#ifdef ANDROID
    LOG("getting plugins dir failed");
#endif
  }

  mPluginsLoaded = true; 
                            

#ifdef XP_WIN
  bool bScanPLIDs = Preferences::GetBool("plugin.scan.plid.all", false);

    
  if (bScanPLIDs && mPrivateDirServiceProvider) {
    rv = mPrivateDirServiceProvider->GetPLIDDirectories(getter_AddRefs(dirList));
    if (NS_SUCCEEDED(rv)) {
      ScanPluginsDirectoryList(dirList, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = true;

      
      
      if (!aCreatePluginList && *aPluginsChanged) {
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
        return NS_OK;
      }
    }
  }


  

  
  const char* const prefs[] = {NS_WIN_ACROBAT_SCAN_KEY,
                               NS_WIN_QUICKTIME_SCAN_KEY,
                               NS_WIN_WMP_SCAN_KEY};

  uint32_t size = sizeof(prefs) / sizeof(prefs[0]);

  for (uint32_t i = 0; i < size; i+=1) {
    nsCOMPtr<nsIFile> dirToScan;
    bool bExists;
    if (NS_SUCCEEDED(dirService->Get(prefs[i], NS_GET_IID(nsIFile), getter_AddRefs(dirToScan))) &&
        dirToScan &&
        NS_SUCCEEDED(dirToScan->Exists(&bExists)) &&
        bExists) {

      ScanPluginsDirectory(dirToScan, aCreatePluginList, &pluginschanged);

      if (pluginschanged)
        *aPluginsChanged = true;

      
      
      if (!aCreatePluginList && *aPluginsChanged) {
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
        NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);
        return NS_OK;
      }
    }
  }
#endif

  
  
  
  
  if (!*aPluginsChanged && mCachedPlugins) {
    *aPluginsChanged = true;
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

      invalidPlugin->mPrev = nullptr;
      invalidPlugin->mNext = nullptr;
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

  return NS_OK;
}

bool
mozilla::plugins::FindPluginsForContent(uint32_t aPluginEpoch,
                                        nsTArray<PluginTag>* aPlugins,
                                        uint32_t* aNewPluginEpoch)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  nsRefPtr<nsPluginHost> host = nsPluginHost::GetInst();
  host->FindPluginsForContent(aPluginEpoch, aPlugins, aNewPluginEpoch);
  return true;
}

void
nsPluginHost::FindPluginsForContent(uint32_t aPluginEpoch,
                                    nsTArray<PluginTag>* aPlugins,
                                    uint32_t* aNewPluginEpoch)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  
  LoadPlugins();

  *aNewPluginEpoch = ChromeEpoch();
  if (aPluginEpoch == ChromeEpoch()) {
    return;
  }

  nsTArray<nsRefPtr<nsPluginTag>> plugins;
  GetPlugins(plugins);

  for (size_t i = 0; i < plugins.Length(); i++) {
    nsRefPtr<nsPluginTag> tag = plugins[i];

    if (!nsNPAPIPlugin::RunPluginOOP(tag)) {
      
      
      continue;
    }

    aPlugins->AppendElement(PluginTag(tag->mId,
                                      tag->mName,
                                      tag->mDescription,
                                      tag->mMimeTypes,
                                      tag->mMimeDescriptions,
                                      tag->mExtensions,
                                      tag->mIsJavaPlugin,
                                      tag->mIsFlashPlugin,
                                      tag->mFileName,
                                      tag->mVersion,
                                      tag->mLastModifiedTime,
                                      tag->IsFromExtension()));
  }
}

void
nsPluginHost::UpdatePluginInfo(nsPluginTag* aPluginTag)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  ReadPluginInfo();
  WritePluginInfo();
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsPluginTag>, mCachedPlugins, mNext);
  NS_ITERATIVE_UNREF_LIST(nsRefPtr<nsInvalidPluginTag>, mInvalidPlugins, mNext);

  if (!aPluginTag) {
    return;
  }

  
  nsAdoptingCString disableFullPage =
    Preferences::GetCString(kPrefDisableFullPage);
  for (uint32_t i = 0; i < aPluginTag->mMimeTypes.Length(); i++) {
    nsRegisterType shouldRegister;

    if (IsTypeInList(aPluginTag->mMimeTypes[i], disableFullPage)) {
      shouldRegister = ePluginUnregister;
    } else {
      nsPluginTag *plugin = FindNativePluginForType(aPluginTag->mMimeTypes[i],
                                                    true);
      shouldRegister = plugin ? ePluginRegister : ePluginUnregister;
    }

    RegisterWithCategoryManager(aPluginTag->mMimeTypes[i], shouldRegister);
  }

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService)
    obsService->NotifyObservers(nullptr, "plugin-info-updated", nullptr);
}

 bool
nsPluginHost::IsTypeWhitelisted(const char *aMimeType)
{
  nsAdoptingCString whitelist = Preferences::GetCString(kPrefWhitelist);
  if (!whitelist.Length()) {
    return true;
  }
  nsDependentCString wrap(aMimeType);
  return IsTypeInList(wrap, whitelist);
}

void
nsPluginHost::RegisterWithCategoryManager(nsCString &aMimeType,
                                          nsRegisterType aType)
{
  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
             ("nsPluginTag::RegisterWithCategoryManager type = %s, removing = %s\n",
              aMimeType.get(), aType == ePluginUnregister ? "yes" : "no"));

  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMan) {
    return;
  }

  const char *contractId =
    "@mozilla.org/content/plugin/document-loader-factory;1";

  if (aType == ePluginRegister) {
    catMan->AddCategoryEntry("Gecko-Content-Viewers",
                             aMimeType.get(),
                             contractId,
                             false, 
                             mOverrideInternalTypes,
                             nullptr);
  } else {
    
    nsXPIDLCString value;
    nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers",
                                           aMimeType.get(),
                                           getter_Copies(value));
    if (NS_SUCCEEDED(rv) && strcmp(value, contractId) == 0) {
      catMan->DeleteCategoryEntry("Gecko-Content-Viewers",
                                  aMimeType.get(),
                                  true);
    }
  }
}

nsresult
nsPluginHost::WritePluginInfo()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  nsresult rv = NS_OK;
  nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID,&rv));
  if (NS_FAILED(rv))
    return rv;

  directoryService->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                        getter_AddRefs(mPluginRegFile));

  if (!mPluginRegFile)
    return NS_ERROR_FAILURE;

  PRFileDesc* fd = nullptr;

  nsCOMPtr<nsIFile> pluginReg;

  rv = mPluginRegFile->Clone(getter_AddRefs(pluginReg));
  if (NS_FAILED(rv))
    return rv;

  nsAutoCString filename(kPluginRegistryFilename);
  filename.AppendLiteral(".tmp");
  rv = pluginReg->AppendNative(filename);
  if (NS_FAILED(rv))
    return rv;

  rv = pluginReg->OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, 0600, &fd);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService("@mozilla.org/xre/runtime;1");
  if (!runtime) {
    return NS_ERROR_FAILURE;
  }

  nsAutoCString arch;
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

  for (nsPluginTag *tag = mPlugins; tag; tag = tag->mNext) {
    
    
    
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

    
    PR_fprintf(fd, "%lld%c%d%c%lu%c%d%c%c\n",
      tag->mLastModifiedTime,
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      false, 
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      0, 
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      tag->IsFromExtension(),
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      PLUGIN_REGISTRY_END_OF_LINE_MARKER);

    
    PR_fprintf(fd, "%s%c%c\n%s%c%c\n%d\n",
      (tag->mDescription.get()),
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      PLUGIN_REGISTRY_END_OF_LINE_MARKER,
      (tag->mName.get()),
      PLUGIN_REGISTRY_FIELD_DELIMITER,
      PLUGIN_REGISTRY_END_OF_LINE_MARKER,
      tag->mMimeTypes.Length());

    
    for (uint32_t i = 0; i < tag->mMimeTypes.Length(); i++) {
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

  PRStatus prrc;
  prrc = PR_Close(fd);
  if (prrc != PR_SUCCESS) {
    
    rv = NS_ERROR_FAILURE;
    MOZ_ASSERT(false, "PR_Close() failed.");
    return rv;
  }
  nsCOMPtr<nsIFile> parent;
  rv = pluginReg->GetParent(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = pluginReg->MoveToNative(parent, kPluginRegistryFilename);
  return rv;
}

nsresult
nsPluginHost::ReadPluginInfo()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  const long PLUGIN_REG_MIMETYPES_ARRAY_SIZE = 12;
  const long PLUGIN_REG_MAX_MIMETYPES = 1000;

  
  const bool pluginStateImported =
    Preferences::GetDefaultBool("plugin.importedState", false);

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

  PRFileDesc* fd = nullptr;

  nsCOMPtr<nsIFile> pluginReg;

  rv = mPluginRegFile->Clone(getter_AddRefs(pluginReg));
  if (NS_FAILED(rv))
    return rv;

  rv = pluginReg->AppendNative(kPluginRegistryFilename);
  if (NS_FAILED(rv))
    return rv;

  int64_t fileSize;
  rv = pluginReg->GetFileSize(&fileSize);
  if (NS_FAILED(rv))
    return rv;

  if (fileSize > INT32_MAX) {
    return NS_ERROR_FAILURE;
  }
  int32_t flen = int32_t(fileSize);
  if (flen == 0) {
    NS_WARNING("Plugins Registry Empty!");
    return NS_OK; 
  }

  nsPluginManifestLineReader reader;
  char* registry = reader.Init(flen);
  if (!registry)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = pluginReg->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd);
  if (NS_FAILED(rv))
    return rv;

  
  rv = NS_ERROR_FAILURE;

  int32_t bread = PR_Read(fd, registry, flen);

  PRStatus prrc;
  prrc = PR_Close(fd);
  if (prrc != PR_SUCCESS) {
    
    
    MOZ_ASSERT(false, "PR_Close() failed.");
    return rv;
  }

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

  
  int32_t vdiff = mozilla::CompareVersions(values[1], kPluginRegistryVersion);
  mozilla::Version version(values[1]);
  
  if (vdiff > 0)
    return rv;
  
  if (version < kMinimumRegistryVersion)
    return rv;

  
  bool regHasVersion = (version >= "0.10");

  
  if (version >= "0.13") {
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

    nsAutoCString arch;
    if (NS_FAILED(runtime->GetXPCOMABI(arch))) {
      return rv;
    }

    
    if (PL_strcmp(archValues[1], arch.get())) {
      return rv;
    }
  }

  
  const bool hasInvalidPlugins = (version >= "0.13");

  
  const bool hasValidFlags = (version < "0.16");

  
  const bool hasFromExtension = (version >= "0.17");

#if defined(XP_MACOSX)
  const bool hasFullPathInFileNameField = false;
#else
  const bool hasFullPathInFileNameField = (version < "0.11");
#endif

  if (!ReadSectionHeader(reader, "PLUGINS"))
    return rv;

  while (reader.NextLine()) {
    const char *filename;
    const char *fullpath;
    nsAutoCString derivedFileName;

    if (hasInvalidPlugins && *reader.LinePtr() == '[') {
      break;
    }

    if (hasFullPathInFileNameField) {
      fullpath = reader.LinePtr();
      if (!reader.NextLine())
        return rv;
      
      if (fullpath) {
        nsCOMPtr<nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
        file->InitWithNativePath(nsDependentCString(fullpath));
        file->GetNativeLeafName(derivedFileName);
        filename = derivedFileName.get();
      } else {
        filename = nullptr;
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

    
    const int count = hasFromExtension ? 4 : 3;
    if (reader.ParseLine(values, count) != count)
      return rv;

    
    int64_t lastmod = (vdiff == 0) ? nsCRT::atoll(values[0]) : -1;
    uint32_t tagflag = atoi(values[2]);
    bool fromExtension = false;
    if (hasFromExtension) {
      fromExtension = atoi(values[3]);
    }
    if (!reader.NextLine())
      return rv;

    char *description = reader.LinePtr();
    if (!reader.NextLine())
      return rv;

#if MOZ_WIDGET_ANDROID
    
    
    if (PL_strncmp("Shockwave Flash ", description, 16) == 0 && description[16]) {
      version = &description[16];
    }
#endif

    const char *name = reader.LinePtr();
    if (!reader.NextLine())
      return rv;

    long mimetypecount = std::strtol(reader.LinePtr(), nullptr, 10);
    if (mimetypecount == LONG_MAX || mimetypecount == LONG_MIN ||
        mimetypecount >= PLUGIN_REG_MAX_MIMETYPES || mimetypecount < 0) {
      return NS_ERROR_FAILURE;
    }

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
      mimetypecount, lastmod, fromExtension, true);
    if (heapalloced)
      delete [] heapalloced;

    
    if (hasValidFlags && !pluginStateImported) {
      tag->ImportFlagsToPrefs(tagflag);
    }

    PR_LOG(nsPluginLogging::gPluginLog, PLUGIN_LOG_BASIC,
      ("LoadCachedPluginsInfo : Loading Cached plugininfo for %s\n", tag->mFileName.get()));
    tag->mNext = mCachedPlugins;
    mCachedPlugins = tag;
  }


#ifndef MOZ_WIDGET_ANDROID
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
      int64_t lastmod = (vdiff == 0) ? nsCRT::atoll(lastModifiedTimeStamp) : -1;

      nsRefPtr<nsInvalidPluginTag> invalidTag = new nsInvalidPluginTag(fullpath, lastmod);

      invalidTag->mNext = mInvalidPlugins;
      if (mInvalidPlugins) {
        mInvalidPlugins->mPrev = invalidTag;
      }
      mInvalidPlugins = invalidTag;
    }
  }
#endif

  
  Preferences::SetBool("plugin.importedState", true);

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
      tag->mNext = nullptr;
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
                                          nsNPAPIPluginStreamListener* aListener,
                                          nsIInputStream *aPostStream,
                                          const char *aHeadersData,
                                          uint32_t aHeadersDataLen)
{
  nsCOMPtr<nsIURI> url;
  nsAutoString absUrl;
  nsresult rv;

  if (aURL.Length() <= 0)
    return NS_OK;

  
  
  nsRefPtr<nsPluginInstanceOwner> owner = aInstance->GetOwner();
  if (owner) {
    rv = NS_MakeAbsoluteURI(absUrl, aURL, nsCOMPtr<nsIURI>(owner->GetBaseURI()));
  }

  if (absUrl.IsEmpty())
    absUrl.Assign(aURL);

  rv = NS_NewURI(getter_AddRefs(url), absUrl);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDOMElement> element;
  nsCOMPtr<nsIDocument> doc;
  if (owner) {
    owner->GetDOMElement(getter_AddRefs(element));
    owner->GetDocument(getter_AddRefs(doc));
  }
  nsCOMPtr<nsIPrincipal> principal = doc ? doc->NodePrincipal() : nullptr;

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,
                                 url,
                                 principal,
                                 element,
                                 EmptyCString(), 
                                 nullptr,         
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
  nsCOMPtr<nsINode> requestingNode(do_QueryInterface(element));
  if (requestingNode) {
    rv = NS_NewChannel(getter_AddRefs(channel),
                       url,
                       requestingNode,
                       nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                       nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,
                       nullptr,  
                       listenerPeer);
  }
  else {
    
    
    
    principal = nsNullPrincipal::Create();
    NS_ENSURE_TRUE(principal, NS_ERROR_FAILURE);
    rv = NS_NewChannel(getter_AddRefs(channel),
                       url,
                       principal,
                       nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                       nsIContentPolicy::TYPE_OBJECT_SUBREQUEST,
                       nullptr,  
                       listenerPeer);
  }

  if (NS_FAILED(rv))
    return rv;

  if (doc) {
    
    
    nsCOMPtr<nsIScriptChannel> scriptChannel(do_QueryInterface(channel));
    if (scriptChannel) {
      scriptChannel->SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
      
      scriptChannel->SetExecuteAsync(false);
    }
  }

  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    if (!aPostStream) {
      
      
      
      nsCOMPtr<nsIURI> referer;
      net::ReferrerPolicy referrerPolicy = net::RP_Default;

      nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(element);
      if (olc)
        olc->GetSrcURI(getter_AddRefs(referer));


      if (!referer) {
        if (!doc) {
          return NS_ERROR_FAILURE;
        }
        referer = doc->GetDocumentURI();
        referrerPolicy = doc->GetReferrerPolicy();
      }

      rv = httpChannel->SetReferrerWithPolicy(referer, referrerPolicy);
      NS_ENSURE_SUCCESS(rv,rv);
    }

    if (aPostStream) {
      
      
      
      nsCOMPtr<nsISeekableStream>
      postDataSeekable(do_QueryInterface(aPostStream));
      if (postDataSeekable)
        postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

      nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
      NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

      uploadChannel->SetUploadStream(aPostStream, EmptyCString(), -1);
    }

    if (aHeadersData) {
      rv = AddHeadersToChannel(aHeadersData, aHeadersDataLen, httpChannel);
      NS_ENSURE_SUCCESS(rv,rv);
    }
  }
  rv = channel->AsyncOpen(listenerPeer, nullptr);
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

  
  nsRefPtr<nsPluginInstanceOwner> owner = aInstance->GetOwner();
  if (!owner)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIURI> baseURI = owner->GetBaseURI();
  if (!baseURI)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIURI> targetURL;
  NS_NewURI(getter_AddRefs(targetURL), aURL, baseURI);
  if (!targetURL)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));
  if (!doc)
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
                                  uint32_t aHeadersDataLen,
                                  nsIChannel *aGenericChannel)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIHttpChannel> aChannel = do_QueryInterface(aGenericChannel);
  if (!aChannel) {
    return NS_ERROR_NULL_POINTER;
  }

  
  nsAutoCString headersString;
  nsAutoCString oneHeader;
  nsAutoCString headerName;
  nsAutoCString headerValue;
  int32_t crlf = 0;
  int32_t colon = 0;

  
  headersString = aHeadersData;

  
  
  while (true) {
    crlf = headersString.Find("\r\n", true);
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

    

    rv = aChannel->SetRequestHeader(headerName, headerValue, true);
    if (NS_FAILED(rv)) {
      rv = NS_ERROR_NULL_POINTER;
      return rv;
    }
  }
  return rv;
}

nsresult
nsPluginHost::StopPluginInstance(nsNPAPIPluginInstance* aInstance)
{
  if (PluginDestructionGuard::DelayDestroy(aInstance)) {
    return NS_OK;
  }

  PLUGIN_LOG(PLUGIN_LOG_NORMAL,
  ("nsPluginHost::StopPluginInstance called instance=%p\n",aInstance));

  if (aInstance->HasStartedDestroying()) {
    return NS_OK;
  }

  Telemetry::AutoTimer<Telemetry::PLUGIN_SHUTDOWN_MS> timer;
  aInstance->Stop();

  
  bool doCache = aInstance->ShouldCache();
  if (doCache) {
    
    uint32_t cachedInstanceLimit =
      Preferences::GetUint(NS_PREF_MAX_NUM_CACHED_INSTANCES,
                           DEFAULT_NUMBER_OF_STOPPED_INSTANCES);
    if (StoppedInstanceCount() >= cachedInstanceLimit) {
      nsNPAPIPluginInstance *oldestInstance = FindOldestStoppedInstance();
      if (oldestInstance) {
        nsPluginTag* pluginTag = TagForPlugin(oldestInstance->GetPlugin());
        oldestInstance->Destroy();
        mInstances.RemoveElement(oldestInstance);
        
        if (pluginTag) {
          OnPluginInstanceDestroyed(pluginTag);
        }
      }
    }
  } else {
    nsPluginTag* pluginTag = TagForPlugin(aInstance->GetPlugin());
    aInstance->Destroy();
    mInstances.RemoveElement(aInstance);
    
    if (pluginTag) {
      OnPluginInstanceDestroyed(pluginTag);
    }
  }

  return NS_OK;
}

nsresult nsPluginHost::NewPluginStreamListener(nsIURI* aURI,
                                               nsNPAPIPluginInstance* aInstance,
                                               nsIStreamListener **aStreamListener)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aStreamListener);

  nsRefPtr<nsPluginStreamListenerPeer> listener = new nsPluginStreamListenerPeer();
  nsresult rv = listener->Initialize(aURI, aInstance, nullptr);
  if (NS_FAILED(rv)) {
    return rv;
  }

  listener.forget(aStreamListener);

  return NS_OK;
}

void nsPluginHost::CreateWidget(nsPluginInstanceOwner* aOwner)
{
  aOwner->CreateWidget();

  
  aOwner->CallSetWindow();
}

NS_IMETHODIMP nsPluginHost::Observe(nsISupports *aSubject,
                                    const char *aTopic,
                                    const char16_t *someData)
{
  if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, aTopic)) {
    OnShutdown();
    UnloadPlugins();
    sInst->Release();
  }
  if (!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic)) {
    mPluginsDisabled = Preferences::GetBool("plugin.disable", false);
    mPluginsClickToPlay = Preferences::GetBool("plugins.click_to_play", false);
    
    if (mPluginsDisabled) {
      UnloadPlugins();
    } else {
      LoadPlugins();
    }
  }
  if (!strcmp("blocklist-updated", aTopic)) {
    nsPluginTag* plugin = mPlugins;
    while (plugin) {
      plugin->InvalidateBlocklistState();
      plugin = plugin->mNext;
    }
  }
#ifdef MOZ_WIDGET_ANDROID
  if (!strcmp("application-background", aTopic)) {
    for(uint32_t i = 0; i < mInstances.Length(); i++) {
      mInstances[i]->NotifyForeground(false);
    }
  }
  if (!strcmp("application-foreground", aTopic)) {
    for(uint32_t i = 0; i < mInstances.Length(); i++) {
      if (mInstances[i]->IsOnScreen())
        mInstances[i]->NotifyForeground(true);
    }
  }
  if (!strcmp("memory-pressure", aTopic)) {
    for(uint32_t i = 0; i < mInstances.Length(); i++) {
      mInstances[i]->MemoryPressure();
    }
  }
#endif
  return NS_OK;
}

nsresult
nsPluginHost::ParsePostBufferToFixHeaders(const char *inPostData, uint32_t inPostDataLen,
                                          char **outPostData, uint32_t *outPostDataLen)
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

  uint32_t newBufferLen = 0;
  uint32_t dataLen = pEod - pSod;
  uint32_t headersLen = pEoh ? pSod - inPostData : 0;

  char *p; 
  if (headersLen) { 
    
    

    newBufferLen = dataLen + headersLen;
    
    
    int cntSingleLF = singleLF.Length();
    newBufferLen += cntSingleLF;

    if (!(*outPostData = p = (char*)moz_xmalloc(newBufferLen)))
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
    
    
    uint32_t l = sizeof(ContentLenHeader) + sizeof(CRLFCRLF) + 32;
    newBufferLen = dataLen + l;
    if (!(*outPostData = p = (char*)moz_xmalloc(newBufferLen)))
      return NS_ERROR_OUT_OF_MEMORY;
    headersLen = PR_snprintf(p, l,"%s: %ld%s", ContentLenHeader, dataLen, CRLFCRLF);
    if (headersLen == l) { 
      free(p);
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

nsresult
nsPluginHost::CreateTempFileToPost(const char *aPostDataURL, nsIFile **aTmpFile)
{
  nsresult rv;
  int64_t fileSize;
  nsAutoCString filename;

  
  nsCOMPtr<nsIFile> inFile;
  rv = NS_GetFileFromURLSpec(nsDependentCString(aPostDataURL),
                             getter_AddRefs(inFile));
  if (NS_FAILED(rv)) {
    nsCOMPtr<nsIFile> localFile;
    rv = NS_NewNativeLocalFile(nsDependentCString(aPostDataURL), false,
                               getter_AddRefs(localFile));
    if (NS_FAILED(rv)) return rv;
    inFile = localFile;
  }
  rv = inFile->GetFileSize(&fileSize);
  if (NS_FAILED(rv)) return rv;
  rv = inFile->GetNativePath(filename);
  if (NS_FAILED(rv)) return rv;

  if (fileSize != 0) {
    nsCOMPtr<nsIInputStream> inStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStream), inFile);
    if (NS_FAILED(rv)) return rv;

    
    

    nsCOMPtr<nsIFile> tempFile;
    rv = GetPluginTempDir(getter_AddRefs(tempFile));
    if (NS_FAILED(rv))
      return rv;

    nsAutoCString inFileName;
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
    uint32_t br, bw;
    bool firstRead = true;
    while (1) {
      
      rv = inStream->Read(buf, 1024, &br);
      if (NS_FAILED(rv) || (int32_t)br <= 0)
        break;
      if (firstRead) {
        
        
        
        

        char *parsedBuf;
        
        
        ParsePostBufferToFixHeaders((const char *)buf, br, &parsedBuf, &bw);
        rv = outStream->Write(parsedBuf, bw, &br);
        free(parsedBuf);
        if (NS_FAILED(rv) || (bw != br))
          break;

        firstRead = false;
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
      tempFile.forget(aTmpFile);
  }
  return rv;
}

nsresult
nsPluginHost::NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  return PLUG_NewPluginNativeWindow(aPluginNativeWindow);
}

nsresult
nsPluginHost::GetPluginName(nsNPAPIPluginInstance *aPluginInstance,
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

nsresult
nsPluginHost::GetPluginTagForInstance(nsNPAPIPluginInstance *aPluginInstance,
                                      nsIPluginTag **aPluginTag)
{
  NS_ENSURE_ARG_POINTER(aPluginInstance);
  NS_ENSURE_ARG_POINTER(aPluginTag);

  nsNPAPIPlugin *plugin = aPluginInstance->GetPlugin();
  if (!plugin)
    return NS_ERROR_FAILURE;

  *aPluginTag = TagForPlugin(plugin);

  NS_ADDREF(*aPluginTag);
  return NS_OK;
}

NS_IMETHODIMP nsPluginHost::Notify(nsITimer* timer)
{
  nsRefPtr<nsPluginTag> pluginTag = mPlugins;
  while (pluginTag) {
    if (pluginTag->mUnloadTimer == timer) {
      if (!IsRunningPlugin(pluginTag)) {
        pluginTag->TryUnloadPlugin(false);
      }
      return NS_OK;
    }
    pluginTag = pluginTag->mNext;
  }

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
  wm->GetXULWindowEnumerator(nullptr, getter_AddRefs(windowList));
  if (!windowList)
    return;

  bool haveWindows;
  do {
    windowList->HasMoreElements(&haveWindows);
    if (!haveWindows)
      return;

    nsCOMPtr<nsISupports> supportsWindow;
    windowList->GetNext(getter_AddRefs(supportsWindow));
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(supportsWindow));
    if (baseWin) {
      nsCOMPtr<nsIWidget> widget;
      baseWin->GetMainWidget(getter_AddRefs(widget));
      if (widget && !widget->GetParent() &&
          widget->IsVisible() &&
          !widget->IsEnabled()) {
        nsIWidget* child = widget->GetFirstChild();
        bool enable = true;
        while (child)  {
          if (child->WindowType() == eWindowType_dialog) {
            enable = false;
            break;
          }
          child = child->GetNextSibling();
        }
        if (enable) {
          widget->Enable(true);
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
  MOZ_ASSERT(crashedPluginTag);

  
  
  bool submittedCrashReport = false;
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  nsCOMPtr<nsIWritablePropertyBag2> propbag =
    do_CreateInstance("@mozilla.org/hash-property-bag;1");
  if (obsService && propbag) {
    uint32_t runID = 0;
    PluginLibrary* library = aPlugin->GetLibrary();

    if (!NS_WARN_IF(!library)) {
      library->GetRunID(&runID);
    }
    propbag->SetPropertyAsUint32(NS_LITERAL_STRING("runID"), runID);

    nsCString pluginName;
    crashedPluginTag->GetName(pluginName);
    propbag->SetPropertyAsAString(NS_LITERAL_STRING("pluginName"),
                                   NS_ConvertUTF8toUTF16(pluginName));
    propbag->SetPropertyAsAString(NS_LITERAL_STRING("pluginDumpID"),
                                  pluginDumpID);
    propbag->SetPropertyAsAString(NS_LITERAL_STRING("browserDumpID"),
                                  browserDumpID);
    propbag->SetPropertyAsBool(NS_LITERAL_STRING("submittedCrashReport"),
                               submittedCrashReport);
    obsService->NotifyObservers(propbag, "plugin-crashed", nullptr);
    
    propbag->GetPropertyAsBool(NS_LITERAL_STRING("submittedCrashReport"),
                               &submittedCrashReport);
  }

  

  for (uint32_t i = mInstances.Length(); i > 0; i--) {
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

  
  
  

  crashedPluginTag->mPlugin = nullptr;
  crashedPluginTag->mContentProcessRunningCount = 0;

#ifdef XP_WIN
  CheckForDisabledWindows();
#endif
}

nsNPAPIPluginInstance*
nsPluginHost::FindInstance(const char *mimetype)
{
  for (uint32_t i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance* instance = mInstances[i];

    const char* mt;
    nsresult rv = instance->GetMIMEType(&mt);
    if (NS_FAILED(rv))
      continue;

    if (PL_strcasecmp(mt, mimetype) == 0)
      return instance;
  }

  return nullptr;
}

nsNPAPIPluginInstance*
nsPluginHost::FindOldestStoppedInstance()
{
  nsNPAPIPluginInstance *oldestInstance = nullptr;
  TimeStamp oldestTime = TimeStamp::Now();
  for (uint32_t i = 0; i < mInstances.Length(); i++) {
    nsNPAPIPluginInstance *instance = mInstances[i];
    if (instance->IsRunning())
      continue;

    TimeStamp time = instance->StopTime();
    if (time < oldestTime) {
      oldestTime = time;
      oldestInstance = instance;
    }
  }

  return oldestInstance;
}

uint32_t
nsPluginHost::StoppedInstanceCount()
{
  uint32_t stoppedCount = 0;
  for (uint32_t i = 0; i < mInstances.Length(); i++) {
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
nsPluginHost::DestroyRunningInstances(nsPluginTag* aPluginTag)
{
  for (int32_t i = mInstances.Length(); i > 0; i--) {
    nsNPAPIPluginInstance *instance = mInstances[i - 1];
    if (instance->IsRunning() && (!aPluginTag || aPluginTag == TagForPlugin(instance->GetPlugin()))) {
      instance->SetWindow(nullptr);
      instance->Stop();

      
      nsPluginTag* pluginTag = TagForPlugin(instance->GetPlugin());
      instance->SetWindow(nullptr);

      nsCOMPtr<nsIDOMElement> domElement;
      instance->GetDOMElement(getter_AddRefs(domElement));
      nsCOMPtr<nsIObjectLoadingContent> objectContent =
        do_QueryInterface(domElement);

      instance->Destroy();

      mInstances.RemoveElement(instance);
      OnPluginInstanceDestroyed(pluginTag);

      
      if (objectContent) {
        objectContent->PluginDestroyed();
      }
    }
  }
}



class nsPluginDestroyRunnable : public nsRunnable,
                                public PRCList
{
public:
  explicit nsPluginDestroyRunnable(nsNPAPIPluginInstance *aInstance)
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
    nsRefPtr<nsNPAPIPluginInstance> instance;

    
    
    
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
  nsRefPtr<nsNPAPIPluginInstance> mInstance;

  static PRCList sRunnableListHead;
};

PRCList nsPluginDestroyRunnable::sRunnableListHead =
  PR_INIT_STATIC_CLIST(&nsPluginDestroyRunnable::sRunnableListHead);

PRCList PluginDestructionGuard::sListHead =
  PR_INIT_STATIC_CLIST(&PluginDestructionGuard::sListHead);

PluginDestructionGuard::PluginDestructionGuard(nsNPAPIPluginInstance *aInstance)
  : mInstance(aInstance)
{
  Init();
}

PluginDestructionGuard::PluginDestructionGuard(PluginAsyncSurrogate *aSurrogate)
  : mInstance(static_cast<nsNPAPIPluginInstance*>(aSurrogate->GetNPP()->ndata))
{
  InitAsync();
}

PluginDestructionGuard::PluginDestructionGuard(NPP npp)
  : mInstance(npp ? static_cast<nsNPAPIPluginInstance*>(npp->ndata) : nullptr)
{
  Init();
}

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


bool
PluginDestructionGuard::DelayDestroy(nsNPAPIPluginInstance *aInstance)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");
  NS_ASSERTION(aInstance, "Uh, I need an instance!");

  
  

  PluginDestructionGuard *g =
    static_cast<PluginDestructionGuard*>(PR_LIST_HEAD(&sListHead));

  while (g != &sListHead) {
    if (g->mInstance == aInstance) {
      g->mDelayedDestroy = true;

      return true;
    }
    g = static_cast<PluginDestructionGuard*>(PR_NEXT_LINK(g));
  }

  return false;
}
