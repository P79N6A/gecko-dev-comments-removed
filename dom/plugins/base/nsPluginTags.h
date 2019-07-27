




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

struct PRLibrary;
struct nsPluginInfo;
class nsNPAPIPlugin;



class nsPluginTag final : public nsIPluginTag
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

  nsPluginTag(nsPluginInfo* aPluginInfo,
              int64_t aLastModifiedTime,
              bool fromExtension);
  nsPluginTag(const char* aName,
              const char* aDescription,
              const char* aFileName,
              const char* aFullPath,
              const char* aVersion,
              const char* const* aMimeTypes,
              const char* const* aMimeDescriptions,
              const char* const* aExtensions,
              int32_t aVariants,
              int64_t aLastModifiedTime,
              bool fromExtension,
              bool aArgsAreUTF8 = false);
  nsPluginTag(uint32_t aId,
              const char* aName,
              const char* aDescription,
              const char* aFileName,
              const char* aFullPath,
              const char* aVersion,
              nsTArray<nsCString> aMimeTypes,
              nsTArray<nsCString> aMimeDescriptions,
              nsTArray<nsCString> aExtensions,
              bool aIsJavaPlugin,
              bool aIsFlashPlugin,
              int64_t aLastModifiedTime,
              bool aFromExtension);

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

  bool IsFromExtension() const;

  nsRefPtr<nsPluginTag> mNext;
  uint32_t      mId;

  
  size_t        mContentProcessRunningCount;
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

  void          InvalidateBlocklistState();

  
  
  bool          HasMimeType(const nsACString & aMimeType) const;
  
  
  
  bool          HasExtension(const nsACString & aExtension,
                              nsACString & aMatchingType) const;

private:
  virtual ~nsPluginTag();

  nsCString     mNiceFileName; 
  uint16_t      mCachedBlocklistState;
  bool          mCachedBlocklistStateValid;
  bool          mIsFromExtension;

  void InitMime(const char* const* aMimeTypes,
                const char* const* aMimeDescriptions,
                const char* const* aExtensions,
                uint32_t aVariantCount);
  nsresult EnsureMembersAreUTF8();
  void FixupVersion();

  static uint32_t sNextId;
};

#endif
