




#ifndef nsChromeRegistryChrome_h
#define nsChromeRegistryChrome_h

#include "nsCOMArray.h"
#include "nsChromeRegistry.h"
#include "nsTArray.h"
#include "mozilla/Move.h"
#include "nsClassHashtable.h"

namespace mozilla {
namespace dom {
class PContentParent;
}
}

class nsIPrefBranch;
struct ChromePackage;

class nsChromeRegistryChrome : public nsChromeRegistry
{
 public:
  nsChromeRegistryChrome();
  ~nsChromeRegistryChrome();

  nsresult Init() MOZ_OVERRIDE;

  NS_IMETHOD CheckForNewChrome() MOZ_OVERRIDE;
  NS_IMETHOD CheckForOSAccessibility() MOZ_OVERRIDE;
  NS_IMETHOD GetLocalesForPackage(const nsACString& aPackage,
                                  nsIUTF8StringEnumerator* *aResult) MOZ_OVERRIDE;
  NS_IMETHOD IsLocaleRTL(const nsACString& package,
                         bool *aResult) MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedLocale(const nsACString& aPackage,
                               nsACString& aLocale) MOZ_OVERRIDE;
  NS_IMETHOD Observe(nsISupports *aSubject, const char *aTopic,
                     const char16_t *someData) MOZ_OVERRIDE;

#ifdef MOZ_XUL
  NS_IMETHOD GetXULOverlays(nsIURI *aURI,
                            nsISimpleEnumerator **_retval) MOZ_OVERRIDE;
  NS_IMETHOD GetStyleOverlays(nsIURI *aURI,
                              nsISimpleEnumerator **_retval) MOZ_OVERRIDE;
#endif

  
  
  
  void SendRegisteredChrome(mozilla::dom::PContentParent* aChild);

 private:
  struct PackageEntry;
  static void ChromePackageFromPackageEntry(const nsACString& aPackageName,
                                            PackageEntry* aPackage,
                                            ChromePackage* aChromePackage,
                                            const nsCString& aSelectedLocale,
                                            const nsCString& aSelectedSkin);
  static PLDHashOperator CollectPackages(const nsACString &aKey,
                                         PackageEntry *package,
                                         void *arg);

  nsresult OverrideLocalePackage(const nsACString& aPackage,
                                 nsACString& aOverride);
  nsresult SelectLocaleFromPref(nsIPrefBranch* prefs);
  nsresult UpdateSelectedLocale() MOZ_OVERRIDE;
  nsIURI* GetBaseURIFromPackage(const nsCString& aPackage,
                                 const nsCString& aProvider,
                                 const nsCString& aPath) MOZ_OVERRIDE;
  nsresult GetFlagsFromPackage(const nsCString& aPackage,
                               uint32_t* aFlags) MOZ_OVERRIDE;

  struct ProviderEntry
  {
    ProviderEntry(const nsACString& aProvider, nsIURI* aBase) :
    provider(aProvider),
    baseURI(aBase) { }

    nsCString        provider;
    nsCOMPtr<nsIURI> baseURI;
  };

  class nsProviderArray
  {
   public:
    nsProviderArray() :
    mArray(1) { }
    ~nsProviderArray() { }

    
    
    enum MatchType {
      EXACT = 0,
      LOCALE = 1, 
      ANY = 2
    };

    nsIURI* GetBase(const nsACString& aPreferred, MatchType aType);
    const nsACString& GetSelected(const nsACString& aPreferred, MatchType aType);
    void    SetBase(const nsACString& aProvider, nsIURI* base);
    void    EnumerateToArray(nsTArray<nsCString> *a);

   private:
    ProviderEntry* GetProvider(const nsACString& aPreferred, MatchType aType);

    nsTArray<ProviderEntry> mArray;
  };

  struct PackageEntry : public PLDHashEntryHdr
  {
    PackageEntry()
    : flags(0) { }
    ~PackageEntry() { }

    nsCOMPtr<nsIURI> baseURI;
    uint32_t         flags;
    nsProviderArray  locales;
    nsProviderArray  skins;
  };

  class OverlayListEntry : public nsURIHashKey
  {
   public:
    typedef nsURIHashKey::KeyType        KeyType;
    typedef nsURIHashKey::KeyTypePointer KeyTypePointer;

    explicit OverlayListEntry(KeyTypePointer aKey) : nsURIHashKey(aKey) { }
    OverlayListEntry(OverlayListEntry&& toMove) : nsURIHashKey(mozilla::Move(toMove)),
                                                  mArray(mozilla::Move(toMove.mArray)) { }
    ~OverlayListEntry() { }

    void AddURI(nsIURI* aURI);

    nsCOMArray<nsIURI> mArray;
  };

  class OverlayListHash
  {
   public:
    OverlayListHash() { }
    ~OverlayListHash() { }

    void Add(nsIURI* aBase, nsIURI* aOverlay);
    void Clear() { mTable.Clear(); }
    const nsCOMArray<nsIURI>* GetArray(nsIURI* aBase);

   private:
    nsTHashtable<OverlayListEntry> mTable;
  };

  
  
  OverlayListHash mOverlayHash;
  OverlayListHash mStyleHash;

  bool mProfileLoaded;
  bool mDynamicRegistration;

  nsCString mSelectedLocale;
  nsCString mSelectedSkin;

  
  nsClassHashtable<nsCStringHashKey, PackageEntry> mPackagesHash;

  virtual void ManifestContent(ManifestProcessingContext& cx, int lineno,
                               char *const * argv, bool platform,
                               bool contentaccessible) MOZ_OVERRIDE;
  virtual void ManifestLocale(ManifestProcessingContext& cx, int lineno,
                              char *const * argv, bool platform,
                              bool contentaccessible) MOZ_OVERRIDE;
  virtual void ManifestSkin(ManifestProcessingContext& cx, int lineno,
                            char *const * argv, bool platform,
                            bool contentaccessible) MOZ_OVERRIDE;
  virtual void ManifestOverlay(ManifestProcessingContext& cx, int lineno,
                               char *const * argv, bool platform,
                               bool contentaccessible) MOZ_OVERRIDE;
  virtual void ManifestStyle(ManifestProcessingContext& cx, int lineno,
                             char *const * argv, bool platform,
                             bool contentaccessible) MOZ_OVERRIDE;
  virtual void ManifestOverride(ManifestProcessingContext& cx, int lineno,
                                char *const * argv, bool platform,
                                bool contentaccessible) MOZ_OVERRIDE;
  virtual void ManifestResource(ManifestProcessingContext& cx, int lineno,
                                char *const * argv, bool platform,
                                bool contentaccessible) MOZ_OVERRIDE;
};

#endif 
