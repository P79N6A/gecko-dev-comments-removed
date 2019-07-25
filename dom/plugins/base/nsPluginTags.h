




#ifndef nsPluginTags_h_
#define nsPluginTags_h_

#include "nscore.h"
#include "prtypes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIPluginTag.h"
#include "nsNPAPIPluginInstance.h"
#include "nsISupportsArray.h"
#include "nsITimer.h"

class nsPluginHost;
struct PRLibrary;
struct nsPluginInfo;



#define NS_PLUGIN_FLAG_ENABLED      0x0001    // is this plugin enabled?

#define NS_PLUGIN_FLAG_FROMCACHE    0x0004    // this plugintag info was loaded from cache

#define NS_PLUGIN_FLAG_BLOCKLISTED  0x0010    // this is a blocklisted plugin



class nsPluginTag : public nsIPluginTag
{
public:
  enum nsRegisterType {
    ePluginRegister,
    ePluginUnregister
  };
  
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
              bool aArgsAreUTF8 = false);
  virtual ~nsPluginTag();
  
  void SetHost(nsPluginHost * aHost);
  void TryUnloadPlugin(bool inShutdown);
  void Mark(PRUint32 mask);
  void UnMark(PRUint32 mask);
  bool HasFlag(PRUint32 flag);
  PRUint32 Flags();
  bool IsEnabled();
  void RegisterWithCategoryManager(bool aOverrideInternalTypes,
                                   nsRegisterType aType = ePluginRegister);
  
  nsRefPtr<nsPluginTag> mNext;
  nsPluginHost *mPluginHost;
  nsCString     mName; 
  nsCString     mDescription; 
  nsTArray<nsCString> mMimeTypes; 
  nsTArray<nsCString> mMimeDescriptions; 
  nsTArray<nsCString> mExtensions; 
  PRLibrary     *mLibrary;
  nsRefPtr<nsNPAPIPlugin> mPlugin;
  bool          mIsJavaPlugin;
  bool          mIsFlashPlugin;
  nsCString     mFileName; 
  nsCString     mFullPath; 
  nsCString     mVersion;  
  PRInt64       mLastModifiedTime;
  nsCOMPtr<nsITimer> mUnloadTimer;
private:
  PRUint32      mFlags;

  void InitMime(const char* const* aMimeTypes,
                const char* const* aMimeDescriptions,
                const char* const* aExtensions,
                PRUint32 aVariantCount);
  nsresult EnsureMembersAreUTF8();
};

#endif
