




#ifndef nsPluginHost_h_
#define nsPluginHost_h_

#include "nsIPluginHost.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "prlink.h"
#include "prclist.h"
#include "npapi.h"
#include "nsIPluginTag.h"
#include "nsPluginsDir.h"
#include "nsPluginDirServiceProvider.h"
#include "nsAutoPtr.h"
#include "nsWeakPtr.h"
#include "nsIPrompt.h"
#include "nsWeakReference.h"
#include "MainThreadUtils.h"
#include "nsTArray.h"
#include "nsTObserverArray.h"
#include "nsITimer.h"
#include "nsPluginTags.h"
#include "nsPluginPlayPreviewInfo.h"
#include "nsIEffectiveTLDService.h"
#include "nsIIDNService.h"
#include "nsCRT.h"

#ifdef XP_WIN
#include "nsIWindowsRegKey.h"
#endif

namespace mozilla {
namespace plugins {
class PluginAsyncSurrogate;
class PluginTag;
} 
} 

class nsNPAPIPlugin;
class nsIFile;
class nsIChannel;
class nsPluginNativeWindow;
class nsObjectLoadingContent;
class nsPluginInstanceOwner;
class nsPluginUnloadRunnable;
class nsNPAPIPluginInstance;
class nsNPAPIPluginStreamListener;
class nsIPluginInstanceOwner;
class nsIInputStream;
class nsIStreamListener;

class nsInvalidPluginTag : public nsISupports
{
  virtual ~nsInvalidPluginTag();
public:
  explicit nsInvalidPluginTag(const char* aFullPath, int64_t aLastModifiedTime = 0);

  NS_DECL_ISUPPORTS

  nsCString   mFullPath;
  int64_t     mLastModifiedTime;
  bool        mSeen;

  nsRefPtr<nsInvalidPluginTag> mPrev;
  nsRefPtr<nsInvalidPluginTag> mNext;
};

class nsPluginHost final : public nsIPluginHost,
                           public nsIObserver,
                           public nsITimerCallback,
                           public nsSupportsWeakReference
{
  friend class nsPluginTag;
  virtual ~nsPluginHost();

public:
  nsPluginHost();

  static already_AddRefed<nsPluginHost> GetInst();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINHOST
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

  nsresult LoadPlugins();
  nsresult UnloadPlugins();

  nsresult SetUpPluginInstance(const nsACString &aMimeType,
                               nsIURI *aURL,
                               nsPluginInstanceOwner *aOwner);

  
  enum PluginFilter {
    eExcludeNone     = nsIPluginHost::EXCLUDE_NONE,
    eExcludeDisabled = nsIPluginHost::EXCLUDE_DISABLED
  };
  
  bool HavePluginForType(const nsACString & aMimeType,
                         PluginFilter aFilter = eExcludeDisabled);

  
  bool HavePluginForExtension(const nsACString & aExtension,
                               nsACString & aMimeType,
                              PluginFilter aFilter = eExcludeDisabled);

  void GetPlugins(nsTArray<nsRefPtr<nsPluginTag> >& aPluginArray);
  void FindPluginsForContent(uint32_t aPluginEpoch,
                             nsTArray<mozilla::plugins::PluginTag>* aPlugins,
                             uint32_t* aNewPluginEpoch);

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
  nsresult ParsePostBufferToFixHeaders(const char *inPostData,
                                       uint32_t inPostDataLen,
                                       char **outPostData,
                                       uint32_t *outPostDataLen);
  nsresult CreateTempFileToPost(const char *aPostDataURL, nsIFile **aTmpFile);
  nsresult NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow);

  void AddIdleTimeTarget(nsIPluginInstanceOwner* objectFrame, bool isVisible);
  void RemoveIdleTimeTarget(nsIPluginInstanceOwner* objectFrame);

  nsresult GetPluginName(nsNPAPIPluginInstance *aPluginInstance,
                         const char** aPluginName);
  nsresult StopPluginInstance(nsNPAPIPluginInstance* aInstance);
  nsresult GetPluginTagForInstance(nsNPAPIPluginInstance *aPluginInstance,
                                   nsIPluginTag **aPluginTag);

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
                    const char* target = nullptr,
                    nsNPAPIPluginStreamListener* streamListener = nullptr,
                    const char* altHost = nullptr,
                    const char* referrer = nullptr,
                    bool forceJSEnabled = false,
                    uint32_t getHeadersLength = 0,
                    const char* getHeaders = nullptr);

  nsresult
  DoURLLoadSecurityCheck(nsNPAPIPluginInstance *aInstance,
                         const char* aURL);

  nsresult
  AddHeadersToChannel(const char *aHeadersData, uint32_t aHeadersDataLen,
                      nsIChannel *aGenericChannel);

  static nsresult GetPluginTempDir(nsIFile **aDir);

  
  
  static bool IsTypeWhitelisted(const char *aType);

  
  enum SpecialType { eSpecialType_None,
                     
                     eSpecialType_Flash,
                     
                     
                     eSpecialType_Java,
                     
                     eSpecialType_Silverlight,
                     
                     eSpecialType_PDF,
                     
                     eSpecialType_RealPlayer };
  static SpecialType GetSpecialType(const nsACString & aMIMEType);

  static nsresult PostPluginUnloadEvent(PRLibrary* aLibrary);

  void PluginCrashed(nsNPAPIPlugin* plugin,
                     const nsAString& pluginDumpID,
                     const nsAString& browserDumpID);

  nsNPAPIPluginInstance *FindInstance(const char *mimetype);
  nsNPAPIPluginInstance *FindOldestStoppedInstance();
  uint32_t StoppedInstanceCount();

  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > *InstanceArray();

  
  nsPluginTag* FindTagForLibrary(PRLibrary* aLibrary);

  
  
  nsresult InstantiatePluginInstance(const nsACString& aMimeType, nsIURI* aURL,
                                     nsObjectLoadingContent *aContent,
                                     nsPluginInstanceOwner** aOwner);

  
  nsPluginTag* TagForPlugin(nsNPAPIPlugin* aPlugin);

  nsPluginTag* PluginWithId(uint32_t aId);

  nsresult GetPlugin(const nsACString &aMimeType, nsNPAPIPlugin** aPlugin);
  nsresult GetPluginForContentProcess(uint32_t aPluginId, nsNPAPIPlugin** aPlugin);
  void NotifyContentModuleDestroyed(uint32_t aPluginId);

  nsresult NewPluginStreamListener(nsIURI* aURL,
                                   nsNPAPIPluginInstance* aInstance,
                                   nsIStreamListener **aStreamListener);

  void CreateWidget(nsPluginInstanceOwner* aOwner);

private:
  friend class nsPluginUnloadRunnable;

  void DestroyRunningInstances(nsPluginTag* aPluginTag);

  
  
  void UpdatePluginInfo(nsPluginTag* aPluginTag);

  nsresult TrySetUpPluginInstance(const nsACString &aMimeType, nsIURI *aURL,
                                  nsPluginInstanceOwner *aOwner);

  
  nsPluginTag*
  FindPreferredPlugin(const InfallibleTArray<nsPluginTag*>& matches);

  
  
  nsPluginTag* FindNativePluginForType(const nsACString & aMimeType,
                                       bool aCheckEnabled);

  nsPluginTag* FindNativePluginForExtension(const nsACString & aExtension,
                                             nsACString & aMimeType,
                                            bool aCheckEnabled);

  nsresult
  FindStoppedPluginForURL(nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  nsresult FindPluginsInContent(bool aCreatePluginList, bool * aPluginsChanged);

  nsresult
  FindPlugins(bool aCreatePluginList, bool * aPluginsChanged);

  
  
  
  enum nsRegisterType { ePluginRegister, ePluginUnregister };
  void RegisterWithCategoryManager(nsCString &aMimeType, nsRegisterType aType);

  void AddPluginTag(nsPluginTag* aPluginTag);

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

  
  void IncrementChromeEpoch();

  
  uint32_t ChromeEpoch();

  
  
  uint32_t ChromeEpochForContent();
  void SetChromeEpochForContent(uint32_t aEpoch);

  nsRefPtr<nsPluginTag> mPlugins;
  nsRefPtr<nsPluginTag> mCachedPlugins;
  nsRefPtr<nsInvalidPluginTag> mInvalidPlugins;
  nsTArray< nsRefPtr<nsPluginPlayPreviewInfo> > mPlayPreviewMimeTypes;
  bool mPluginsLoaded;

  
  bool mOverrideInternalTypes;

  
  bool mPluginsDisabled;
  
  bool mPluginsClickToPlay;

  
  
  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > mInstances;

  nsCOMPtr<nsIFile> mPluginRegFile;
#ifdef XP_WIN
  nsRefPtr<nsPluginDirServiceProvider> mPrivateDirServiceProvider;

  
  
  nsCOMPtr<nsIWindowsRegKey> mRegKeyHKLM;
  nsCOMPtr<nsIWindowsRegKey> mRegKeyHKCU;
#endif

  nsCOMPtr<nsIEffectiveTLDService> mTLDService;
  nsCOMPtr<nsIIDNService> mIDNService;

  
  nsresult NormalizeHostname(nsCString& host);
  nsresult EnumerateSiteData(const nsACString& domain,
                             const InfallibleTArray<nsCString>& sites,
                             InfallibleTArray<nsCString>& result,
                             bool firstMatchOnly);

  nsWeakPtr mCurrentDocument; 

  
  
  
  
  uint32_t mPluginEpoch;

  static nsIFile *sPluginTempDir;

  
  
  static nsPluginHost* sInst;
};

class PluginDestructionGuard : protected PRCList
{
public:
  explicit PluginDestructionGuard(nsNPAPIPluginInstance *aInstance);
  explicit PluginDestructionGuard(mozilla::plugins::PluginAsyncSurrogate *aSurrogate);
  explicit PluginDestructionGuard(NPP npp);

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

  void InitAsync()
  {
    NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");

    mDelayedDestroy = false;

    PR_INIT_CLIST(this);
    
    
    PR_INSERT_AFTER(this, &sListHead);
  }

  nsRefPtr<nsNPAPIPluginInstance> mInstance;
  bool mDelayedDestroy;

  static PRCList sListHead;
};

#endif 
