




































#ifndef nsPluginHostImpl_h_
#define nsPluginHostImpl_h_

#include "nsIPluginHost.h"
#include "nsIObserver.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "prlink.h"
#include "prclist.h"
#include "npapi.h"
#include "nsNPAPIPluginInstance.h"

#include "nsIPlugin.h"
#include "nsIPluginTag.h"
#include "nsIPluginTagInfo2.h"

#include "nsPluginsDir.h"
#include "nsPluginDirServiceProvider.h"
#include "nsAutoPtr.h"
#include "nsWeakPtr.h"
#include "nsIPrompt.h"
#include "nsISupportsArray.h"
#include "nsPluginNativeWindow.h"
#include "nsIPrefBranch.h"
#include "nsWeakReference.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsIFactory.h"

class nsNPAPIPlugin;
class nsIComponentManager;
class nsIFile;
class nsIChannel;
class nsIRegistry;
class nsPluginHostImpl;

#define NS_PLUGIN_FLAG_ENABLED      0x0001    // is this plugin enabled?
#define NS_PLUGIN_FLAG_NPAPI        0x0002    // is this an NPAPI plugin?
#define NS_PLUGIN_FLAG_FROMCACHE    0x0004    // this plugintag info was loaded from cache
#define NS_PLUGIN_FLAG_UNWANTED     0x0008    // this is an unwanted plugin
#define NS_PLUGIN_FLAG_BLOCKLISTED  0x0010    // this is a blocklisted plugin



class nsPluginTag : public nsIPluginTag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINTAG

  nsPluginTag(nsPluginTag* aPluginTag);
  nsPluginTag(nsPluginInfo* aPluginInfo);

  nsPluginTag(const char* aName,
              const char* aDescription,
              const char* aFileName,
              const char* aFullPath,
              const char* aVersion,
              const char* const* aMimeTypes,
              const char* const* aMimeDescriptions,
              const char* const* aExtensions,
              PRInt32 aVariants,
              PRInt64 aLastModifiedTime = 0,
              PRBool aCanUnload = PR_TRUE,
              PRBool aArgsAreUTF8 = PR_FALSE);

  ~nsPluginTag();

  void SetHost(nsPluginHostImpl * aHost);
  void TryUnloadPlugin(PRBool aForceShutdown = PR_FALSE);
  void Mark(PRUint32 mask) {
    PRBool wasEnabled = IsEnabled();
    mFlags |= mask;
    
    if (mPluginHost && wasEnabled != IsEnabled()) {
      if (wasEnabled)
        RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginUnregister);
      else
        RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginRegister);
    }
  }
  void UnMark(PRUint32 mask) {
    PRBool wasEnabled = IsEnabled();
    mFlags &= ~mask;
    
    if (mPluginHost && wasEnabled != IsEnabled()) {
      if (wasEnabled)
        RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginUnregister);
      else
        RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginRegister);
    }
  }
  PRBool HasFlag(PRUint32 flag) { return (mFlags & flag) != 0; }
  PRUint32 Flags() { return mFlags; }
  PRBool Equals(nsPluginTag* aPluginTag);
  PRBool IsEnabled() { return HasFlag(NS_PLUGIN_FLAG_ENABLED) && !HasFlag(NS_PLUGIN_FLAG_BLOCKLISTED); }

  enum nsRegisterType {
    ePluginRegister,
    ePluginUnregister
  };
  void RegisterWithCategoryManager(PRBool aOverrideInternalTypes,
                                   nsRegisterType aType = ePluginRegister);

  nsRefPtr<nsPluginTag>   mNext;
  nsPluginHostImpl *mPluginHost;
  nsCString     mName; 
  nsCString     mDescription; 
  PRInt32       mVariants;
  char          **mMimeTypeArray;
  nsTArray<nsCString> mMimeDescriptionArray; 
  char          **mExtensionsArray;
  PRLibrary     *mLibrary;
  nsIPlugin     *mEntryPoint;
  PRPackedBool  mCanUnloadLibrary;
  PRPackedBool  mXPConnected;
  PRPackedBool  mIsJavaPlugin;
  PRPackedBool  mIsNPRuntimeEnabledJavaPlugin;
  nsCString     mFileName; 
  nsCString     mFullPath; 
  nsCString     mVersion;  
  PRInt64       mLastModifiedTime;
private:
  PRUint32      mFlags;

  nsresult EnsureMembersAreUTF8();
};

struct nsPluginInstanceTag
{
  nsPluginInstanceTag*   mNext;
  char*                  mURL;
  nsRefPtr<nsPluginTag>  mPluginTag;
  nsIPluginInstance*     mInstance;
  PRTime                 mllStopTime;
  PRPackedBool           mStopped;
  PRPackedBool           mDefaultPlugin;
  PRPackedBool           mXPConnected;
  
  nsCOMPtr <nsISupportsArray> mStreams; 

  nsPluginInstanceTag(nsPluginTag* aPluginTag,
                      nsIPluginInstance* aInstance, 
                      const char * url,
                      PRBool aDefaultPlugin);
  ~nsPluginInstanceTag();

  void setStopped(PRBool stopped);
};

class nsPluginInstanceTagList
{
public:
  nsPluginInstanceTag *mFirst;
  nsPluginInstanceTag *mLast;
  PRInt32 mCount;

  nsPluginInstanceTagList();
  ~nsPluginInstanceTagList();

  void shutdown();
  PRBool add(nsPluginInstanceTag *plugin);
  PRBool remove(nsPluginInstanceTag *plugin);
  nsPluginInstanceTag *find(nsIPluginInstance *instance);
  nsPluginInstanceTag *find(const char *mimetype);
  nsPluginInstanceTag *findStopped(const char *url);
  PRUint32 getStoppedCount();
  nsPluginInstanceTag *findOldestStopped();
  void removeAllStopped();
  void stopRunning(nsISupportsArray *aReloadDocs, nsPluginTag *aPluginTag);
  PRBool IsLastInstance(nsPluginInstanceTag *plugin);
};

class nsPluginHostImpl : public nsIPluginHost,
                         public nsIObserver,
                         public nsSupportsWeakReference
{
public:
  nsPluginHostImpl();
  virtual ~nsPluginHostImpl();

  static nsPluginHostImpl* GetInst();
  static const char *GetPluginName(nsIPluginInstance *aPluginInstance);

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINHOST
  NS_DECL_NSIFACTORY
  NS_DECL_NSIOBSERVER

  NS_IMETHOD
  GetURL(nsISupports* pluginInst, 
         const char* url, 
         const char* target = NULL,
         nsIPluginStreamListener* streamListener = NULL,
         const char* altHost = NULL,
         const char* referrer = NULL,
         PRBool forceJSEnabled = PR_FALSE);
  
  NS_IMETHOD
  PostURL(nsISupports* pluginInst,
          const char* url,
          PRUint32 postDataLen, 
          const char* postData,
          PRBool isFile = PR_FALSE,
          const char* target = NULL,
          nsIPluginStreamListener* streamListener = NULL,
          const char* altHost = NULL, 
          const char* referrer = NULL,
          PRBool forceJSEnabled = PR_FALSE,
          PRUint32 postHeadersLength = 0, 
          const char* postHeaders = NULL);

  nsresult
  NewPluginURLStream(const nsString& aURL, 
                     nsIPluginInstance *aInstance, 
                     nsIPluginStreamListener *aListener,
                     const char *aPostData = nsnull, 
                     PRBool isFile = PR_FALSE,
                     PRUint32 aPostDataLen = 0, 
                     const char *aHeadersData = nsnull, 
                     PRUint32 aHeadersDataLen = 0);

  nsresult
  GetURLWithHeaders(nsISupports* pluginInst, 
                    const char* url, 
                    const char* target = NULL,
                    nsIPluginStreamListener* streamListener = NULL,
                    const char* altHost = NULL,
                    const char* referrer = NULL,
                    PRBool forceJSEnabled = PR_FALSE,
                    PRUint32 getHeadersLength = 0, 
                    const char* getHeaders = NULL);

  nsresult
  DoURLLoadSecurityCheck(nsIPluginInstance *aInstance,
                         const char* aURL);

  nsresult
  AddHeadersToChannel(const char *aHeadersData, PRUint32 aHeadersDataLen, 
                      nsIChannel *aGenericChannel);

  nsresult
  AddUnusedLibrary(PRLibrary * aLibrary);

  static nsresult GetPluginTempDir(nsIFile **aDir);

  
  
  nsresult UpdatePluginInfo(nsPluginTag* aPluginTag);

  
  
  static PRBool IsJavaMIMEType(const char *aType);

  static nsresult GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt);

private:
  nsresult
  TrySetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  NewEmbeddedPluginStreamListener(nsIURI* aURL, nsIPluginInstanceOwner *aOwner,
                                  nsIPluginInstance* aInstance,
                                  nsIStreamListener** aListener);

  nsresult
  NewEmbeddedPluginStream(nsIURI* aURL, nsIPluginInstanceOwner *aOwner, nsIPluginInstance* aInstance);

  nsresult
  NewFullPagePluginStream(nsIStreamListener *&aStreamListener, nsIPluginInstance *aInstance);

  
  
  nsPluginTag*
  FindPluginForType(const char* aMimeType, PRBool aCheckEnabled);

  nsPluginTag*
  FindPluginEnabledForExtension(const char* aExtension, const char* &aMimeType);

  nsresult
  FindStoppedPluginForURL(nsIURI* aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  SetUpDefaultPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  AddInstanceToActiveList(nsCOMPtr<nsIPlugin> aPlugin,
                          nsIPluginInstance* aInstance,
                          nsIURI* aURL, PRBool aDefaultPlugin);

  nsresult
  FindPlugins(PRBool aCreatePluginList, PRBool * aPluginsChanged);

  nsresult
  ScanPluginsDirectory(nsIFile * pluginsDir, 
                       nsIComponentManager * compManager, 
                       PRBool aCreatePluginList,
                       PRBool * aPluginsChanged,
                       PRBool checkForUnwantedPlugins = PR_FALSE);
                       
  nsresult
  ScanPluginsDirectoryList(nsISimpleEnumerator * dirEnum,
                           nsIComponentManager * compManager, 
                           PRBool aCreatePluginList,
                           PRBool * aPluginsChanged,
                           PRBool checkForUnwantedPlugins = PR_FALSE);

  PRBool IsRunningPlugin(nsPluginTag * plugin);

  
  nsresult WritePluginInfo();

  
  nsresult ReadPluginInfo();

  
  
  void RemoveCachedPluginsInfo(const char *filePath,
                               nsPluginTag **result);

  
  nsPluginTag* HaveSamePlugin(nsPluginTag * aPluginTag);

  
  
  PRBool IsDuplicatePlugin(nsPluginTag * aPluginTag);

  nsresult EnsurePrivateDirServiceProvider();

  
  void UnloadUnusedLibraries();

  
  nsresult AddPrefObserver();
  
  char        *mPluginPath;
  nsRefPtr<nsPluginTag> mPlugins;
  nsRefPtr<nsPluginTag> mCachedPlugins;
  PRPackedBool mPluginsLoaded;
  PRPackedBool mDontShowBadPluginMessage;
  PRPackedBool mIsDestroyed;

  
  PRPackedBool mOverrideInternalTypes;

  
  PRPackedBool mAllowAlienStarHandler;

  
  PRPackedBool mDefaultPluginDisabled;

  
  PRPackedBool mJavaEnabled;

  nsPluginInstanceTagList mPluginInstanceTagList;
  nsTArray<PRLibrary*> mUnusedLibraries;

  nsCOMPtr<nsIFile> mPluginRegFile;
  nsCOMPtr<nsIPrefBranch> mPrefService;
#ifdef XP_WIN
  nsRefPtr<nsPluginDirServiceProvider> mPrivateDirServiceProvider;
#endif

  nsWeakPtr mCurrentDocument; 

  static nsIFile *sPluginTempDir;

  
  
  static nsPluginHostImpl* sInst;
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
