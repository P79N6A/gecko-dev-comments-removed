





































#ifndef nsPluginTags_h_
#define nsPluginTags_h_

#include "nscore.h"
#include "prtypes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIPluginTag.h"
#include "nsIPlugin.h"
#include "nsNPAPIPluginInstance.h"
#include "nsISupportsArray.h"

class nsPluginHost;
struct PRLibrary;
struct nsPluginInfo;



#define NS_PLUGIN_FLAG_ENABLED      0x0001    // is this plugin enabled?

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
  
  void SetHost(nsPluginHost * aHost);
  void TryUnloadPlugin();
  void Mark(PRUint32 mask);
  void UnMark(PRUint32 mask);
  PRBool HasFlag(PRUint32 flag);
  PRUint32 Flags();
  PRBool Equals(nsPluginTag* aPluginTag);
  PRBool IsEnabled();
  
  nsRefPtr<nsPluginTag> mNext;
  nsPluginHost *mPluginHost;
  nsCString     mName; 
  nsCString     mDescription; 
  PRInt32       mVariants;
  char          **mMimeTypeArray;
  nsTArray<nsCString> mMimeDescriptionArray; 
  char          **mExtensionsArray;
  PRLibrary     *mLibrary;
  nsCOMPtr<nsIPlugin> mEntryPoint;
  PRPackedBool  mCanUnloadLibrary;
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
  char*                  mURL;
  nsRefPtr<nsPluginTag>  mPluginTag;
  nsNPAPIPluginInstance* mInstance; 
  
  nsCOMArray<nsIPluginStreamInfo> mStreams; 
  
  nsPluginInstanceTag(nsPluginTag* aPluginTag,
                      nsIPluginInstance* aInstance, 
                      const char * url);
  ~nsPluginInstanceTag();
};

#endif 
