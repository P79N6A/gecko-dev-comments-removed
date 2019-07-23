




































#ifndef nsPluginHostImpl_h__
#define nsPluginHostImpl_h__

#include "nsIPluginManager.h"
#include "nsIPluginManager2.h"
#include "nsIPluginHost.h"
#include "nsIObserver.h"
#include "nsPIPluginHost.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "prlink.h"

#include "nsIPlugin.h"
#include "nsIPluginTag.h"
#include "nsIPluginTagInfo2.h"
#include "nsIPluginInstancePeer2.h"

#include "nsIFileUtilities.h"
#include "nsICookieStorage.h"
#include "nsPluginsDir.h"
#include "nsVoidArray.h"  
#include "nsPluginDirServiceProvider.h"
#include "nsAutoPtr.h"
#include "nsWeakPtr.h"
#include "nsIPrompt.h"
#include "nsISupportsArray.h"
#include "nsPluginNativeWindow.h"
#include "nsIPrefBranch.h"
#include "nsWeakReference.h"


#include "nsIFactory.h"

class ns4xPlugin;
class nsIComponentManager;
class nsIFile;
class nsIChannel;
class nsIRegistry;
class nsPluginHostImpl;

#define NS_PLUGIN_FLAG_ENABLED    0x0001    // is this plugin enabled?
#define NS_PLUGIN_FLAG_OLDSCHOOL  0x0002    // is this a pre-xpcom plugin?
#define NS_PLUGIN_FLAG_FROMCACHE  0x0004    // this plugintag info was loaded from cache
#define NS_PLUGIN_FLAG_UNWANTED   0x0008    // this is an unwanted plugin






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
              const char* const* aMimeTypes,
              const char* const* aMimeDescriptions,
              const char* const* aExtensions,
              PRInt32 aVariants,
              PRInt64 aLastModifiedTime = 0,
              PRBool aCanUnload = PR_TRUE);

  ~nsPluginTag();

  void SetHost(nsPluginHostImpl * aHost);
  void TryUnloadPlugin(PRBool aForceShutdown = PR_FALSE);
  void Mark(PRUint32 mask) {
    mFlags |= mask;
    
    
    if ((mask & NS_PLUGIN_FLAG_ENABLED) && mPluginHost) {
      RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginRegister);
    }
  }
  void UnMark(PRUint32 mask) {
    mFlags &= ~mask;
    
    
    if ((mask & NS_PLUGIN_FLAG_ENABLED) && mPluginHost) {
      RegisterWithCategoryManager(PR_FALSE, nsPluginTag::ePluginUnregister);
    }
  }
  PRBool HasFlag(PRUint32 flag) { return (mFlags & flag) != 0; }
  PRUint32 Flags() { return mFlags; }
  PRBool Equals(nsPluginTag* aPluginTag);

  enum nsRegisterType {
    ePluginRegister,
    ePluginUnregister
  };
  void RegisterWithCategoryManager(PRBool aOverrideInternalTypes,
                                   nsRegisterType aType = ePluginRegister);

  nsRefPtr<nsPluginTag>   mNext;
  nsPluginHostImpl *mPluginHost;
  char          *mName;
  char          *mDescription;
  PRInt32       mVariants;
  char          **mMimeTypeArray;
  char          **mMimeDescriptionArray;
  char          **mExtensionsArray;
  PRLibrary     *mLibrary;
  nsIPlugin     *mEntryPoint;
  PRPackedBool  mCanUnloadLibrary;
  PRPackedBool  mXPConnected;
  char          *mFileName;
  char          *mFullPath;
  PRInt64       mLastModifiedTime;
private:
  PRUint32      mFlags;
};

struct nsActivePlugin
{
  nsActivePlugin*        mNext;
  char*                  mURL;
  nsIPluginInstancePeer* mPeer;
  nsRefPtr<nsPluginTag>  mPluginTag;
  nsIPluginInstance*     mInstance;
  PRTime                 mllStopTime;
  PRPackedBool           mStopped;
  PRPackedBool           mDefaultPlugin;
  PRPackedBool           mXPConnected;
  
  nsCOMPtr <nsISupportsArray>  mStreams; 

  nsActivePlugin(nsPluginTag* aPluginTag,
                 nsIPluginInstance* aInstance, 
                 const char * url,
                 PRBool aDefaultPlugin,
                 nsIPluginInstancePeer *peer);
  ~nsActivePlugin();

  void setStopped(PRBool stopped);
};

class nsActivePluginList
{
public:
  nsActivePlugin * mFirst;
  nsActivePlugin * mLast;
  PRInt32 mCount;

  nsActivePluginList();
  ~nsActivePluginList();

  void shut();
  PRBool add(nsActivePlugin * plugin);
  PRBool remove(nsActivePlugin * plugin);
  nsActivePlugin * find(nsIPluginInstance* instance);
  nsActivePlugin * find(const char * mimetype);
  nsActivePlugin * findStopped(const char * url);
  PRUint32 getStoppedCount();
  nsActivePlugin * findOldestStopped();
  void removeAllStopped();
  void stopRunning(nsISupportsArray* aReloadDocs);
  PRBool IsLastInstance(nsActivePlugin * plugin);
};

class nsPluginHostImpl : public nsIPluginManager2,
                         public nsIPluginHost,
                         public nsIFileUtilities,
                         public nsICookieStorage,
                         public nsIObserver,
                         public nsPIPluginHost,
                         public nsSupportsWeakReference
{
public:
  nsPluginHostImpl();
  virtual ~nsPluginHostImpl();

  static nsPluginHostImpl* GetInst();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  static const char *GetPluginName(nsIPluginInstance *aPluginInstance);

  

  NS_IMETHOD
  GetValue(nsPluginManagerVariable variable, void *value);

  NS_IMETHOD
  ReloadPlugins(PRBool reloadPages);

  NS_IMETHOD
  UserAgent(const char* *resultingAgentString);

  NS_IMETHOD
  GetURL(nsISupports* pluginInst, 
           const char* url, 
           const char* target = NULL,
           nsIPluginStreamListener* streamListener = NULL,
           const char* altHost = NULL,
           const char* referrer = NULL,
           PRBool forceJSEnabled = PR_FALSE);

  NS_IMETHOD
  GetURLWithHeaders(nsISupports* pluginInst, 
                    const char* url, 
                    const char* target = NULL,
                    nsIPluginStreamListener* streamListener = NULL,
                    const char* altHost = NULL,
                    const char* referrer = NULL,
                    PRBool forceJSEnabled = PR_FALSE,
                    PRUint32 getHeadersLength = 0, 
                    const char* getHeaders = NULL);
  
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

  NS_IMETHOD
  RegisterPlugin(REFNSIID aCID,
                 const char* aPluginName,
                 const char* aDescription,
                 const char** aMimeTypes,
                 const char** aMimeDescriptions,
                 const char** aFileExtensions,
                 PRInt32 aCount);

  NS_IMETHOD
  UnregisterPlugin(REFNSIID aCID);

  

  NS_DECL_NSIPLUGINHOST
  NS_DECL_NSIPLUGINMANAGER2

  NS_IMETHOD
  ProcessNextEvent(PRBool *bEventHandled);

  
  NS_DECL_NSIFACTORY
  NS_DECL_NSIFILEUTILITIES
  NS_DECL_NSICOOKIESTORAGE
  NS_DECL_NSIOBSERVER
  NS_DECL_NSPIPLUGINHOST

  

  NS_IMETHOD
  NewPluginURLStream(const nsString& aURL, 
                     nsIPluginInstance *aInstance, 
                     nsIPluginStreamListener *aListener,
                     const char *aPostData = nsnull, 
                     PRBool isFile = PR_FALSE,
                     PRUint32 aPostDataLen = 0, 
                     const char *aHeadersData = nsnull, 
                     PRUint32 aHeadersDataLen = 0);

  nsresult
  DoURLLoadSecurityCheck(nsIPluginInstance *aInstance,
                         const char* aURL);

  NS_IMETHOD
  AddHeadersToChannel(const char *aHeadersData, PRUint32 aHeadersDataLen, 
                      nsIChannel *aGenericChannel);

  NS_IMETHOD
  AddUnusedLibrary(PRLibrary * aLibrary);

  static nsresult GetPluginTempDir(nsIFile **aDir);

  
  nsresult UpdatePluginInfo();

private:
  NS_IMETHOD
  TrySetUpPluginInstance(const char *aMimeType, nsIURI *aURL, nsIPluginInstanceOwner *aOwner);

  nsresult
  LoadXPCOMPlugins(nsIComponentManager* aComponentManager);

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
                          nsIURI* aURL, PRBool aDefaultPlugin,
                          nsIPluginInstancePeer *peer);

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

  
  
  void RemoveCachedPluginsInfo(const char *filename,
                               nsPluginTag **result);

  
  nsPluginTag* HaveSamePlugin(nsPluginTag * aPluginTag);

  
  
  PRBool IsDuplicatePlugin(nsPluginTag * aPluginTag);

  
  
  PRBool IsUnwantedJavaPlugin(nsPluginTag * aPluginTag);

  
  
  PRBool IsJavaPluginTag(nsPluginTag * aPluginTag);

  
  
  PRBool IsJavaMIMEType(const char *aType);

  
  void ClearCachedPluginInfoList();
  
  nsresult EnsurePrivateDirServiceProvider();

  nsresult GetPrompt(nsIPluginInstanceOwner *aOwner, nsIPrompt **aPrompt);

  
  nsresult ScanForRealInComponentsFolder(nsIComponentManager * aCompManager);

  
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

  nsActivePluginList mActivePluginList;
  nsVoidArray mUnusedLibraries;

  nsCOMPtr<nsIFile>                    mPluginRegFile;
  nsCOMPtr<nsIPrefBranch>              mPrefService;
  nsRefPtr<nsPluginDirServiceProvider> mPrivateDirServiceProvider;

  nsWeakPtr mCurrentDocument; 

  static nsIFile *sPluginTempDir;

  
  
  static nsPluginHostImpl* sInst;
};

#endif
