





































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
#include "nsIPrefBranch.h"
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

#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)
#define MAC_CARBON_PLUGINS
#endif

class nsInvalidPluginTag : public nsISupports
{
public:
  nsInvalidPluginTag(const char* aFullPath, PRInt64 aLastModifiedTime = 0);
  virtual ~nsInvalidPluginTag();
  
  NS_DECL_ISUPPORTS
  
  nsCString   mFullPath;
  PRInt64     mLastModifiedTime;
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
  nsresult Destroy();
  nsresult LoadPlugins();
  nsresult InstantiatePluginForChannel(nsIChannel* aChannel,
                                       nsIPluginInstanceOwner* aOwner,
                                       nsIStreamListener** aListener);
  nsresult SetUpPluginInstance(const char *aMimeType,
                               nsIURI *aURL,
                               nsIPluginInstanceOwner *aOwner);
  nsresult IsPluginEnabledForType(const char* aMimeType,
                                  bool aHasBeenClickedToPlay = false);
  nsresult IsPluginEnabledForExtension(const char* aExtension, const char* &aMimeType,
                                       bool aHasBeenClickedToPlay = false);
  nsresult GetPluginCount(PRUint32* aPluginCount);
  nsresult GetPlugins(PRUint32 aPluginCount, nsIDOMPlugin** aPluginArray);

  nsresult GetURL(nsISupports* pluginInst,
                  const char* url,
                  const char* target,
                  nsIPluginStreamListener* streamListener,
                  const char* altHost,
                  const char* referrer,
                  bool forceJSEnabled);
  nsresult PostURL(nsISupports* pluginInst,
                   const char* url,
                   PRUint32 postDataLen,
                   const char* postData,
                   bool isFile,
                   const char* target,
                   nsIPluginStreamListener* streamListener,
                   const char* altHost,
                   const char* referrer,
                   bool forceJSEnabled,
                   PRUint32 postHeadersLength,
                   const char* postHeaders);

  nsresult FindProxyForURL(const char* url, char* *result);
  nsresult UserAgent(const char **retstring);
  nsresult ParsePostBufferToFixHeaders(const char *inPostData, PRUint32 inPostDataLen,
                                       char **outPostData, PRUint32 *outPostDataLen);
  nsresult CreateTempFileToPost(const char *aPostDataURL, nsIFile **aTmpFile);
  nsresult NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow);
  nsresult DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow);
  nsresult InstantiateDummyJavaPlugin(nsIPluginInstanceOwner *aOwner);

  void AddIdleTimeTarget(nsIPluginInstanceOwner* objectFrame, bool isVisible);
  void RemoveIdleTimeTarget(nsIPluginInstanceOwner* objectFrame);

  nsresult GetPluginName(nsNPAPIPluginInstance *aPluginInstance, const char** aPluginName);
  nsresult StopPluginInstance(nsNPAPIPluginInstance* aInstance);
  nsresult HandleBadPlugin(PRLibrary* aLibrary, nsNPAPIPluginInstance *aInstance);
  nsresult GetPluginTagForInstance(nsNPAPIPluginInstance *aPluginInstance, nsIPluginTag **aPluginTag);

  nsresult
  NewPluginURLStream(const nsString& aURL, 
                     nsNPAPIPluginInstance *aInstance, 
                     nsIPluginStreamListener *aListener,
                     nsIInputStream *aPostStream = nsnull,
                     const char *aHeadersData = nsnull, 
                     PRUint32 aHeadersDataLen = 0);

  nsresult
  GetURLWithHeaders(nsNPAPIPluginInstance *pluginInst, 
                    const char* url, 
                    const char* target = NULL,
                    nsIPluginStreamListener* streamListener = NULL,
                    const char* altHost = NULL,
                    const char* referrer = NULL,
                    bool forceJSEnabled = false,
                    PRUint32 getHeadersLength = 0, 
                    const char* getHeaders = NULL);

  nsresult
  DoURLLoadSecurityCheck(nsNPAPIPluginInstance *aInstance,
                         const char* aURL);

  nsresult
  AddHeadersToChannel(const char *aHeadersData, PRUint32 aHeadersDataLen, 
                      nsIChannel *aGenericChannel);

  static nsresult GetPluginTempDir(nsIFile **aDir);

  
  
  nsresult UpdatePluginInfo(nsPluginTag* aPluginTag);

  
  
  static bool IsJavaMIMEType(const char *aType);

  static nsresult GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt);

  static nsresult PostPluginUnloadEvent(PRLibrary* aLibrary);

  void PluginCrashed(nsNPAPIPlugin* plugin,
                     const nsAString& pluginDumpID,
                     const nsAString& browserDumpID);

  nsNPAPIPluginInstance *FindInstance(const char *mimetype);
  nsNPAPIPluginInstance *FindOldestStoppedInstance();
  PRUint32 StoppedInstanceCount();

  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > *InstanceArray();

  void DestroyRunningInstances(nsISupportsArray* aReloadDocs, nsPluginTag* aPluginTag);

  
  nsPluginTag* FindTagForLibrary(PRLibrary* aLibrary);

  
  
  nsresult InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI* aURL,
                                     nsIPluginInstanceOwner* aOwner);

  nsresult InstantiateFullPagePlugin(const char *aMimeType,
                                     nsIURI* aURI,
                                     nsIPluginInstanceOwner *aOwner,
                                     nsIStreamListener **aStreamListener);

  
  nsPluginTag* TagForPlugin(nsNPAPIPlugin* aPlugin);

  nsresult GetPlugin(const char *aMimeType, nsNPAPIPlugin** aPlugin);

private:
  nsresult
  TrySetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  NewEmbeddedPluginStreamListener(nsIURI* aURL, nsIPluginInstanceOwner *aOwner,
                                  nsNPAPIPluginInstance* aInstance,
                                  nsIStreamListener** aListener);

  nsresult
  NewEmbeddedPluginStream(nsIURI* aURL, nsIPluginInstanceOwner *aOwner, nsNPAPIPluginInstance* aInstance);

  nsresult
  NewFullPagePluginStream(nsIURI* aURI,
                          nsNPAPIPluginInstance *aInstance,
                          nsIStreamListener **aStreamListener);

  
  
  nsPluginTag*
  FindPluginForType(const char* aMimeType, bool aCheckEnabled);

  nsPluginTag*
  FindPluginEnabledForExtension(const char* aExtension, const char* &aMimeType);

  nsresult
  FindStoppedPluginForURL(nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  FindPlugins(bool aCreatePluginList, bool * aPluginsChanged);

  nsresult
  ScanPluginsDirectory(nsIFile *pluginsDir,
                       bool aCreatePluginList,
                       bool *aPluginsChanged);

  nsresult
  ScanPluginsDirectoryList(nsISimpleEnumerator *dirEnum,
                           bool aCreatePluginList,
                           bool *aPluginsChanged);

  nsresult EnsurePluginLoaded(nsPluginTag* plugin);

  bool IsRunningPlugin(nsPluginTag * plugin);

  
  nsresult WritePluginInfo();

  
  nsresult ReadPluginInfo();

  
  
  void RemoveCachedPluginsInfo(const char *filePath,
                               nsPluginTag **result);

  
  bool IsLiveTag(nsIPluginTag* tag);

  
  nsPluginTag* HaveSamePlugin(nsPluginTag * aPluginTag);

  
  
  bool IsDuplicatePlugin(nsPluginTag * aPluginTag);

  nsresult EnsurePrivateDirServiceProvider();

  void OnPluginInstanceDestroyed(nsPluginTag* aPluginTag);

  nsRefPtr<nsPluginTag> mPlugins;
  nsRefPtr<nsPluginTag> mCachedPlugins;
  nsRefPtr<nsInvalidPluginTag> mInvalidPlugins;
  bool mPluginsLoaded;
  bool mDontShowBadPluginMessage;
  bool mIsDestroyed;

  
  bool mOverrideInternalTypes;

  
  bool mPluginsDisabled;

  
  
  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > mInstances;

  nsCOMPtr<nsIFile> mPluginRegFile;
  nsCOMPtr<nsIPrefBranch> mPrefService;
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

#ifdef MAC_CARBON_PLUGINS
  nsCOMPtr<nsITimer> mVisiblePluginTimer;
  nsTObserverArray<nsIPluginInstanceOwner*> mVisibleTimerTargets;
  nsCOMPtr<nsITimer> mHiddenPluginTimer;
  nsTObserverArray<nsIPluginInstanceOwner*> mHiddenTimerTargets;
#endif
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
    : mInstance(npp ? static_cast<nsNPAPIPluginInstance*>(npp->ndata) : nsnull)
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
