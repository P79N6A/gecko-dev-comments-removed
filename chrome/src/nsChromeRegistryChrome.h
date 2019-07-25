





































#ifndef nsChromeRegistryChrome_h
#define nsChromeRegistryChrome_h

#include "nsChromeRegistry.h"

namespace mozilla {
namespace dom {
class PContentParent;
}
}

class nsIPrefBranch;

class nsChromeRegistryChrome : public nsChromeRegistry
{
 public:
  nsChromeRegistryChrome();
  ~nsChromeRegistryChrome();

  NS_OVERRIDE nsresult Init();

  NS_OVERRIDE NS_IMETHOD CheckForNewChrome();
  NS_OVERRIDE NS_IMETHOD CheckForOSAccessibility();
  NS_OVERRIDE NS_IMETHOD GetLocalesForPackage(const nsACString& aPackage,
                                              nsIUTF8StringEnumerator* *aResult);
  NS_OVERRIDE NS_IMETHOD IsLocaleRTL(const nsACString& package,
                                     PRBool *aResult);
  NS_OVERRIDE NS_IMETHOD GetSelectedLocale(const nsACString& aPackage,
                                           nsACString& aLocale);
  NS_OVERRIDE NS_IMETHOD Observe(nsISupports *aSubject, const char *aTopic,
                                 const PRUnichar *someData);

#ifdef MOZ_XUL
  NS_OVERRIDE NS_IMETHOD GetXULOverlays(nsIURI *aURI,
                                        nsISimpleEnumerator **_retval);
  NS_OVERRIDE NS_IMETHOD GetStyleOverlays(nsIURI *aURI,
                                          nsISimpleEnumerator **_retval);
#endif
  
  void SendRegisteredChrome(mozilla::dom::PContentParent* aChild);

 private:
  static PLDHashOperator CollectPackages(PLDHashTable *table,
                                         PLDHashEntryHdr *entry,
                                         PRUint32 number, void *arg);

  nsresult SelectLocaleFromPref(nsIPrefBranch* prefs);
  NS_OVERRIDE nsresult UpdateSelectedLocale();
  NS_OVERRIDE nsIURI* GetBaseURIFromPackage(const nsCString& aPackage,
                                             const nsCString& aProvider,
                                             const nsCString& aPath);
  NS_OVERRIDE nsresult GetFlagsFromPackage(const nsCString& aPackage,
                                           PRUint32* aFlags);

  static const PLDHashTableOps kTableOps;
  static PLDHashNumber HashKey(PLDHashTable *table, const void *key);
  static PRBool        MatchKey(PLDHashTable *table, const PLDHashEntryHdr *entry,
                                const void *key);
  static void          ClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry);
  static PRBool        InitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                                 const void *key);

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
    ~nsProviderArray()
    { Clear(); }

    
    
    enum MatchType {
      EXACT = 0,
      LOCALE = 1, 
      ANY = 2
    };

    nsIURI* GetBase(const nsACString& aPreferred, MatchType aType);
    const nsACString& GetSelected(const nsACString& aPreferred, MatchType aType);
    void    SetBase(const nsACString& aProvider, nsIURI* base);
    void    EnumerateToArray(nsTArray<nsCString> *a);
    void    Clear();

   private:
    ProviderEntry* GetProvider(const nsACString& aPreferred, MatchType aType);

    nsVoidArray mArray;
  };

  struct PackageEntry : public PLDHashEntryHdr
  {
    PackageEntry(const nsACString& package)
    : package(package), flags(0) { }
    ~PackageEntry() { }

    nsCString        package;
    nsCOMPtr<nsIURI> baseURI;
    PRUint32         flags;
    nsProviderArray  locales;
    nsProviderArray  skins;
  };

  class OverlayListEntry : public nsURIHashKey
  {
   public:
    typedef nsURIHashKey::KeyType        KeyType;
    typedef nsURIHashKey::KeyTypePointer KeyTypePointer;

    OverlayListEntry(KeyTypePointer aKey) : nsURIHashKey(aKey) { }
    OverlayListEntry(OverlayListEntry& toCopy) : nsURIHashKey(toCopy),
                                                 mArray(toCopy.mArray) { }
    ~OverlayListEntry() { }

    void AddURI(nsIURI* aURI);

    nsCOMArray<nsIURI> mArray;
  };

  class OverlayListHash
  {
   public:
    OverlayListHash() { }
    ~OverlayListHash() { }

    PRBool Init() { return mTable.Init(); }
    void Add(nsIURI* aBase, nsIURI* aOverlay);
    void Clear() { mTable.Clear(); }
    const nsCOMArray<nsIURI>* GetArray(nsIURI* aBase);

   private:
    nsTHashtable<OverlayListEntry> mTable;
  };

  
  
  OverlayListHash mOverlayHash;
  OverlayListHash mStyleHash;

  PRBool mProfileLoaded;
  
  nsCString mSelectedLocale;
  nsCString mSelectedSkin;

  
  PLDHashTable mPackagesHash;

  virtual void ManifestContent(ManifestProcessingContext& cx, int lineno,
                               char *const * argv, bool platform,
                               bool contentaccessible);
  virtual void ManifestLocale(ManifestProcessingContext& cx, int lineno,
                              char *const * argv, bool platform,
                              bool contentaccessible);
  virtual void ManifestSkin(ManifestProcessingContext& cx, int lineno,
                            char *const * argv, bool platform,
                            bool contentaccessible);
  virtual void ManifestOverlay(ManifestProcessingContext& cx, int lineno,
                               char *const * argv, bool platform,
                               bool contentaccessible);
  virtual void ManifestStyle(ManifestProcessingContext& cx, int lineno,
                             char *const * argv, bool platform,
                             bool contentaccessible);
  virtual void ManifestOverride(ManifestProcessingContext& cx, int lineno,
                                char *const * argv, bool platform,
                                bool contentaccessible);
  virtual void ManifestResource(ManifestProcessingContext& cx, int lineno,
                                char *const * argv, bool platform,
                                bool contentaccessible);
};

#endif 
