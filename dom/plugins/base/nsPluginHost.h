





































#ifndef nsPluginHost_h_
#define nsPluginHost_h_

#include "nsIPluginHost.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "prlink.h"
#include "prclist.h"
#include "npapi.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPlugin.h"
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

class nsNPAPIPlugin;
class nsIComponentManager;
class nsIFile;
class nsIChannel;

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
                    PRBool forceJSEnabled = PR_FALSE,
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

  
  
  static PRBool IsJavaMIMEType(const char *aType);

  static nsresult GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt);

  static nsresult PostPluginUnloadEvent(PRLibrary* aLibrary);

  void PluginCrashed(nsNPAPIPlugin* plugin,
                     const nsAString& pluginDumpID,
                     const nsAString& browserDumpID);

  nsNPAPIPluginInstance *FindInstance(const char *mimetype);
  nsNPAPIPluginInstance *FindStoppedInstance(const char * url);
  nsNPAPIPluginInstance *FindOldestStoppedInstance();
  PRUint32 StoppedInstanceCount();

  nsTArray< nsRefPtr<nsNPAPIPluginInstance> > *InstanceArray();

  void DestroyRunningInstances(nsISupportsArray* aReloadDocs, nsPluginTag* aPluginTag);

  
  nsPluginTag* FindTagForLibrary(PRLibrary* aLibrary);

  
  
  nsresult InstantiateEmbeddedPlugin(const char *aMimeType, nsIURI* aURL,
                                     nsIPluginInstanceOwner* aOwner,
                                     PRBool aAllowOpeningStreams);

  nsresult InstantiateFullPagePlugin(const char *aMimeType,
                                     nsIURI* aURI,
                                     nsIPluginInstanceOwner *aOwner,
                                     nsIStreamListener **aStreamListener);

  
  nsPluginTag* TagForPlugin(nsNPAPIPlugin* aPlugin);

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
  FindPluginForType(const char* aMimeType, PRBool aCheckEnabled);

  nsPluginTag*
  FindPluginEnabledForExtension(const char* aExtension, const char* &aMimeType);

  nsresult
  FindStoppedPluginForURL(nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  FindPlugins(PRBool aCreatePluginList, PRBool * aPluginsChanged);

  nsresult
  ScanPluginsDirectory(nsIFile *pluginsDir,
                       PRBool aCreatePluginList,
                       PRBool *aPluginsChanged);

  nsresult
  ScanPluginsDirectoryList(nsISimpleEnumerator *dirEnum,
                           PRBool aCreatePluginList,
                           PRBool *aPluginsChanged);

  nsresult EnsurePluginLoaded(nsPluginTag* plugin);

  PRBool IsRunningPlugin(nsPluginTag * plugin);

  
  nsresult WritePluginInfo();

  
  nsresult ReadPluginInfo();

  
  
  void RemoveCachedPluginsInfo(const char *filePath,
                               nsPluginTag **result);

  
  PRBool IsLiveTag(nsIPluginTag* tag);

  
  nsPluginTag* HaveSamePlugin(nsPluginTag * aPluginTag);

  
  
  PRBool IsDuplicatePlugin(nsPluginTag * aPluginTag);

  nsresult EnsurePrivateDirServiceProvider();

  void OnPluginInstanceDestroyed(nsPluginTag* aPluginTag);

  nsRefPtr<nsPluginTag> mPlugins;
  nsRefPtr<nsPluginTag> mCachedPlugins;
  nsRefPtr<nsInvalidPluginTag> mInvalidPlugins;
  PRPackedBool mPluginsLoaded;
  PRPackedBool mDontShowBadPluginMessage;
  PRPackedBool mIsDestroyed;

  
  PRPackedBool mOverrideInternalTypes;

  
  PRPackedBool mPluginsDisabled;

  
  
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
  PluginDestructionGuard(nsIPluginInstance *aInstance)
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

  static PRBool DelayDestroy(nsIPluginInstance *aInstance);

protected:
  void Init()
  {
    NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread");

    mDelayedDestroy = PR_FALSE;

    PR_INIT_CLIST(this);
    PR_INSERT_BEFORE(this, &sListHead);
  }

  nsCOMPtr<nsIPluginInstance> mInstance;
  PRBool mDelayedDestroy;

  static PRCList sListHead;
};

#endif 
