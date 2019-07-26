




#ifndef nsPluginHost_h_
#define nsPluginHost_h_

#include "nsIPluginHost.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "prlink.h"
#include "prclist.h"
#include "npapi.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPluginTag.h"
#include "nsPluginsDir.h"
#include "nsPluginDirServiceProvider.h"
#include "nsAutoPtr.h"
#include "nsWeakPtr.h"
#include "nsIPrompt.h"
#include "nsISupportsArray.h"
#include "nsWeakReference.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsTObserverArray.h"
#include "nsITimer.h"
#include "nsPluginTags.h"
#include "nsIEffectiveTLDService.h"
#include "nsIIDNService.h"
#include "nsCRT.h"

class nsNPAPIPlugin;
class nsIComponentManager;
class nsIFile;
class nsIChannel;
class nsPluginNativeWindow;
class nsObjectLoadingContent;
class nsPluginInstanceOwner;

class nsInvalidPluginTag : public nsISupports
{
public:
  nsInvalidPluginTag(const char* aFullPath, int64_t aLastModifiedTime = 0);
  virtual ~nsInvalidPluginTag();
  
  NS_DECL_ISUPPORTS
  
  nsCString   mFullPath;
  int64_t     mLastModifiedTime;
  bool        mSeen;
  
  nsRefPtr<nsInvalidPluginTag> mPrev;
  nsRefPtr<nsInvalidPluginTag> mNext;
};

class nsPluginHost : public nsIPluginHost,
                     public nsIObserver,
                     public nsITimerCallback,
                     public nsSupportsWeakReference
{
public:
  nsPluginHost();
  virtual ~nsPluginHost();

  static nsPluginHost* GetInst();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINHOST
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

  nsresult Init();
  nsresult LoadPlugins();
  nsresult UnloadPlugins();

  nsresult SetUpPluginInstance(const char *aMimeType,
                               nsIURI *aURL,
                               nsPluginInstanceOwner *aOwner);
  nsresult IsPluginEnabledForType(const char* aMimeType);
  nsresult IsPluginEnabledForExtension(const char* aExtension, const char* &aMimeType);
  bool     IsPluginPlayPreviewForType(const char *aMimeType);
  nsresult GetBlocklistStateForType(const char *aMimeType, uint32_t *state);

  nsresult GetPluginCount(uint32_t* aPluginCount);
  nsresult GetPlugins(uint32_t aPluginCount, nsIDOMPlugin** aPluginArray);

  nsresult GetURL(nsISupports* pluginInst,
                  const char* url,
                  const char* target,
                  nsNPAPIPluginStreamListener* streamListener,
                  const char* altHost,
                  const char* referrer,
                  bool forceJSEnabled);
  nsresult PostURL(nsISupports* pluginInst,
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
                   const char* postHeaders);

  nsresult FindProxyForURL(const char* url, char* *result);
  nsresult UserAgent(const char **retstring);
  nsresult ParsePostBufferToFixHeaders(const char *inPostData, uint32_t inPostDataLen,
                                       char **outPostData, uint32_t *outPostDataLen);
  nsresult CreateTempFileToPost(const char *aPostDataURL, nsIFile **aTmpFile);
  nsresult NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow);

  void AddIdleTimeTarget(nsIPluginInstanceOwner* objectFrame, bool isVisible);
  void RemoveIdleTimeTarget(nsIPluginInstanceOwner* objectFrame);

  nsresult GetPluginName(nsNPAPIPluginInstance *aPluginInstance, const char** aPluginName);
  nsresult StopPluginInstance(nsNPAPIPluginInstance* aInstance);
  nsresult HandleBadPlugin(PRLibrary* aLibrary, nsNPAPIPluginInstance *aInstance);
  nsresult GetPluginTagForInstance(nsNPAPIPluginInstance *aPluginInstance, nsIPluginTag **aPluginTag);

  nsresult
  NewPluginURLStream(const nsString& aURL, 
                     nsNPAPIPluginInstance *aInstance, 
                     nsNPAPIPluginStreamListener *aListener,
                     nsIInputStream *aPostStream = nullptr,
                     const char *aHeadersData = nullptr, 
                     uint32_t aHeadersDataLen = 0);

  nsresult
  GetURLWithHeaders(nsNPAPIPluginInstance *pluginInst, 
                    const char* url, 
                    const char* target = NULL,
                    nsNPAPIPluginStreamListener* streamListener = NULL,
                    const char* altHost = NULL,
                    const char* referrer = NULL,
                    bool forceJSEnabled = false,
                    uint32_t getHeadersLength = 0, 
                    const char* getHeaders = NULL);

  nsresult
  DoURLLoadSecurityCheck(nsNPAPIPluginInstance *aInstance,
                         const char* aURL);

  nsresult
  AddHeadersToChannel(const char *aHeadersData, uint32_t aHeadersDataLen, 
                      nsIChannel *aGenericChannel);

  static nsresult GetPluginTempDir(nsIFile **aDir);

  
  
  nsresult UpdatePluginInfo(nsPluginTag* aPluginTag);
  
  
  
  static bool IsTypeWhitelisted(const char *aType);

  
  
  static bool IsJavaMIMEType(const char *aType);

  static nsresult GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt);

  static nsresult PostPluginUnloadEvent(PRLibrary* aLibrary);

  void PluginCrashed(nsNPAPIPlugin* plugin,
                     const nsAString& pluginDumpID,
                     const nsAString& browserDumpID);

  nsNPAPIPluginInstance *FindInstance(const char *mimetype);
  nsNPAPIPluginInstance *FindOldestStoppedInstance();
  uint32_t StoppedInstanceCount();

  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > *InstanceArray();

  void DestroyRunningInstances(nsISupportsArray* aReloadDocs, nsPluginTag* aPluginTag);

  
  nsPluginTag* FindTagForLibrary(PRLibrary* aLibrary);

  
  
  nsresult InstantiateEmbeddedPluginInstance(const char *aMimeType, nsIURI* aURL,
                                             nsObjectLoadingContent *aContent,
                                             nsPluginInstanceOwner** aOwner);

  nsresult InstantiateFullPagePluginInstance(const char *aMimeType,
                                             nsIURI* aURI,
                                             nsObjectLoadingContent *aContent,
                                             nsPluginInstanceOwner **aOwner,
                                             nsIStreamListener **aStreamListener);

  
  nsPluginTag* TagForPlugin(nsNPAPIPlugin* aPlugin);

  nsresult GetPlugin(const char *aMimeType, nsNPAPIPlugin** aPlugin);

  nsresult NewEmbeddedPluginStreamListener(nsIURI* aURL, nsObjectLoadingContent *aContent,
                                           nsNPAPIPluginInstance* aInstance,
                                           nsIStreamListener **aStreamListener);

  nsresult NewFullPagePluginStreamListener(nsIURI* aURI,
                                           nsNPAPIPluginInstance *aInstance,
                                           nsIStreamListener **aStreamListener);

private:
  nsresult
  TrySetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsPluginInstanceOwner *aOwner);

  nsresult
  NewEmbeddedPluginStream(nsIURI* aURL, nsObjectLoadingContent *aContent, nsNPAPIPluginInstance* aInstance);

  nsPluginTag*
  FindPreferredPlugin(const InfallibleTArray<nsPluginTag*>& matches);

  
  
  nsPluginTag*
  FindPluginForType(const char* aMimeType, bool aCheckEnabled);

  nsPluginTag*
  FindPluginEnabledForExtension(const char* aExtension, const char* &aMimeType);

  nsresult
  FindStoppedPluginForURL(nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  FindPlugins(bool aCreatePluginList, bool * aPluginsChanged);

  
  
  enum nsRegisterType { ePluginRegister, ePluginUnregister };
  void RegisterWithCategoryManager(nsCString &aMimeType, nsRegisterType aType);

  nsresult
  ScanPluginsDirectory(nsIFile *pluginsDir,
                       bool aCreatePluginList,
                       bool *aPluginsChanged);

  nsresult
  ScanPluginsDirectoryList(nsISimpleEnumerator *dirEnum,
                           bool aCreatePluginList,
                           bool *aPluginsChanged);

  nsresult EnsurePluginLoaded(nsPluginTag* aPluginTag);

  bool IsRunningPlugin(nsPluginTag * aPluginTag);

  
  nsresult WritePluginInfo();

  
  nsresult ReadPluginInfo();

  
  
  void RemoveCachedPluginsInfo(const char *filePath,
                               nsPluginTag **result);

  
  bool IsLiveTag(nsIPluginTag* tag);
  
  
  nsPluginTag* HaveSamePlugin(const nsPluginTag * aPluginTag);
    
  
  nsPluginTag* FirstPluginWithPath(const nsCString& path);

  nsresult EnsurePrivateDirServiceProvider();

  void OnPluginInstanceDestroyed(nsPluginTag* aPluginTag);

  nsRefPtr<nsPluginTag> mPlugins;
  nsRefPtr<nsPluginTag> mCachedPlugins;
  nsRefPtr<nsInvalidPluginTag> mInvalidPlugins;
  nsTArray<nsCString> mPlayPreviewMimeTypes;
  bool mPluginsLoaded;
  bool mDontShowBadPluginMessage;

  
  bool mOverrideInternalTypes;

  
  bool mPluginsDisabled;
  
  bool mPluginsClickToPlay;

  
  
  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > mInstances;

  nsCOMPtr<nsIFile> mPluginRegFile;
#ifdef XP_WIN
  nsRefPtr<nsPluginDirServiceProvider> mPrivateDirServiceProvider;
#endif

  nsCOMPtr<nsIEffectiveTLDService> mTLDService;
  nsCOMPtr<nsIIDNService> mIDNService;

  
  nsresult NormalizeHostname(nsCString& host);
  nsresult EnumerateSiteData(const nsACString& domain,
                             const nsTArray<nsCString>& sites,
                             InfallibleTArray<nsCString>& result,
                             bool firstMatchOnly);

  nsWeakPtr mCurrentDocument; 

  static nsIFile *sPluginTempDir;

  
  
  static nsPluginHost* sInst;
};

class NS_STACK_CLASS PluginDestructionGuard : protected PRCList
{
public:
  PluginDestructionGuard(nsNPAPIPluginInstance *aInstance)
    : mInstance(aInstance)
  {
    Init();
  }

  PluginDestructionGuard(NPP npp)
    : mInstance(npp ? static_cast<nsNPAPIPluginInstance*>(npp->ndata) : nullptr)
  {
    Init();
  }

  ~PluginDestructionGuard();

  static bool DelayDestroy(nsNPAPIPluginInstance *aInstance);

protected:
  void Init()
  {
    NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");

    mDelayedDestroy = false;

    PR_INIT_CLIST(this);
    PR_INSERT_BEFORE(this, &sListHead);
  }

  nsRefPtr<nsNPAPIPluginInstance> mInstance;
  bool mDelayedDestroy;

  static PRCList sListHead;
};

#endif 
