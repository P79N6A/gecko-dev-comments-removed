




#ifndef nsPluginTags_h_
#define nsPluginTags_h_

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIPluginTag.h"
#include "nsITimer.h"
#include "nsString.h"

class nsPluginHost;
struct PRLibrary;
struct nsPluginInfo;
class nsNPAPIPlugin;



class nsPluginTag : public nsIPluginTag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINTAG

  
  enum PluginState {
    ePluginState_Disabled = 0,
    ePluginState_Clicktoplay = 1,
    ePluginState_Enabled = 2,
    ePluginState_MaxValue = 3,
  };

  nsPluginTag(nsPluginInfo* aPluginInfo);
  nsPluginTag(const char* aName,
              const char* aDescription,
              const char* aFileName,
              const char* aFullPath,
              const char* aVersion,
              const char* const* aMimeTypes,
              const char* const* aMimeDescriptions,
              const char* const* aExtensions,
              int32_t aVariants,
              int64_t aLastModifiedTime = 0,
              bool aArgsAreUTF8 = false);
  virtual ~nsPluginTag();

  void TryUnloadPlugin(bool inShutdown);

  
  bool IsActive();

  bool IsEnabled();
  void SetEnabled(bool enabled);
  bool IsClicktoplay();
  bool IsBlocklisted();

  PluginState GetPluginState();
  void SetPluginState(PluginState state);

  
  void ImportFlagsToPrefs(uint32_t flag);

  bool HasSameNameAndMimes(const nsPluginTag *aPluginTag) const;
  nsCString GetNiceFileName();

  nsRefPtr<nsPluginTag> mNext;
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
  int64_t       mLastModifiedTime;
  nsCOMPtr<nsITimer> mUnloadTimer;

  uint32_t      GetBlocklistState();
  void          InvalidateBlocklistState();

private:
  nsCString     mNiceFileName; 
  uint16_t      mCachedBlocklistState;
  bool          mCachedBlocklistStateValid;

  void InitMime(const char* const* aMimeTypes,
                const char* const* aMimeDescriptions,
                const char* const* aExtensions,
                uint32_t aVariantCount);
  nsresult EnsureMembersAreUTF8();
};

#endif
