




#ifndef nsPluginTags_h_
#define nsPluginTags_h_

#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIPluginTag.h"
#include "nsNPAPIPluginInstance.h"
#include "nsITimer.h"
#include "nsIDOMMimeType.h"

class nsPluginHost;
struct PRLibrary;
struct nsPluginInfo;



class nsPluginTag : public nsIPluginTag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINTAG

  enum PluginState {
    ePluginState_Disabled = 0,
    ePluginState_Clicktoplay = 1,
    ePluginState_Enabled = 2,
  };

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
              int32_t aVariants,
              int64_t aLastModifiedTime = 0,
              bool aArgsAreUTF8 = false);
  virtual ~nsPluginTag();

  void SetHost(nsPluginHost * aHost);
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
  int64_t       mLastModifiedTime;
  nsCOMPtr<nsITimer> mUnloadTimer;
private:
  nsCString     mNiceFileName; 

  void InitMime(const char* const* aMimeTypes,
                const char* const* aMimeDescriptions,
                const char* const* aExtensions,
                uint32_t aVariantCount);
  nsresult EnsureMembersAreUTF8();
};

class DOMMimeTypeImpl : public nsIDOMMimeType {
public:
  NS_DECL_ISUPPORTS

  DOMMimeTypeImpl(nsPluginTag* aTag, uint32_t aMimeTypeIndex)
  {
    if (!aTag)
      return;
    CopyUTF8toUTF16(aTag->mMimeDescriptions[aMimeTypeIndex], mDescription);
    CopyUTF8toUTF16(aTag->mExtensions[aMimeTypeIndex], mSuffixes);
    CopyUTF8toUTF16(aTag->mMimeTypes[aMimeTypeIndex], mType);
  }

  virtual ~DOMMimeTypeImpl() {
  }

  NS_METHOD GetDescription(nsAString& aDescription)
  {
    aDescription.Assign(mDescription);
    return NS_OK;
  }

  NS_METHOD GetEnabledPlugin(nsIDOMPlugin** aEnabledPlugin)
  {
    
    *aEnabledPlugin = nullptr;
    return NS_OK;
  }

  NS_METHOD GetSuffixes(nsAString& aSuffixes)
  {
    aSuffixes.Assign(mSuffixes);
    return NS_OK;
  }

  NS_METHOD GetType(nsAString& aType)
  {
    aType.Assign(mType);
    return NS_OK;
  }
private:
  nsString mDescription;
  nsString mSuffixes;
  nsString mType;
};

#endif
