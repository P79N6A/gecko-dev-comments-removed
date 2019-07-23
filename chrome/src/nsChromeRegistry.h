






































#include "nsIChromeRegistry.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#ifdef MOZ_XUL
#include "nsIXULOverlayProvider.h"
#endif

#include "pldhash.h"

#include "nsCOMArray.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "nsURIHashKey.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsInterfaceHashtable.h"

struct PRFileDesc;
class nsIAtom;
class nsICSSLoader;
class nsICSSStyleSheet;
class nsIDOMWindowInternal;
class nsILocalFile;
class nsIPrefBranch;
class nsIRDFDataSource;
class nsIRDFResource;
class nsIRDFService;
class nsISimpleEnumerator;
class nsIURL;



#define NS_CHROMEREGISTRY_CID \
{ 0x47049e42, 0x1d87, 0x482a, { 0x98, 0x4d, 0x56, 0xae, 0x18, 0x5e, 0x36, 0x7a } }

class nsChromeRegistry : public nsIToolkitChromeRegistry,
#ifdef MOZ_XUL
                         public nsIXULOverlayProvider,
#endif
                         public nsIObserver,
                         public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICHROMEREGISTRY
  NS_DECL_NSIXULCHROMEREGISTRY
  NS_DECL_NSITOOLKITCHROMEREGISTRY

#ifdef MOZ_XUL
  NS_DECL_NSIXULOVERLAYPROVIDER
#endif

  NS_DECL_NSIOBSERVER

  
  nsChromeRegistry() : mInitialized(PR_FALSE), mProfileLoaded(PR_FALSE) {
    mPackagesHash.ops = nsnull;
  }
  ~nsChromeRegistry();

  nsresult Init();

  static nsChromeRegistry* gChromeRegistry;

  static nsresult Canonify(nsIURL* aChromeURL);

protected:
  nsresult GetDynamicInfo(nsIURI *aChromeURL, PRBool aIsOverlay, nsISimpleEnumerator **aResult);

  nsresult LoadInstallDataSource();
  nsresult LoadProfileDataSource();

  void FlushSkinCaches();
  void FlushAllCaches();

private:
  nsresult SelectLocaleFromPref(nsIPrefBranch* prefs);

  static nsresult RefreshWindow(nsIDOMWindowInternal* aWindow,
                                nsICSSLoader* aCSSLoader);
  static nsresult GetProviderAndPath(nsIURL* aChromeURL,
                                     nsACString& aProvider, nsACString& aPath);

#ifdef MOZ_XUL
  NS_HIDDEN_(void) ProcessProvider(PRFileDesc *fd, nsIRDFService* aRDFs,
                                   nsIRDFDataSource* ds, nsIRDFResource* aRoot,
                                   PRBool aIsLocale, const nsACString& aBaseURL);
  NS_HIDDEN_(void) ProcessOverlays(PRFileDesc *fd, nsIRDFDataSource* ds,
                                   nsIRDFResource* aRoot,
                                   const nsCSubstring& aType);
#endif

  NS_HIDDEN_(nsresult) ProcessManifest(nsILocalFile* aManifest, PRBool aSkinOnly);
  NS_HIDDEN_(nsresult) ProcessManifestBuffer(char *aBuffer, PRInt32 aLength, nsILocalFile* aManifest, PRBool aSkinOnly);
  NS_HIDDEN_(nsresult) ProcessNewChromeFile(nsILocalFile *aListFile, nsIURI* aManifest);
  NS_HIDDEN_(nsresult) ProcessNewChromeBuffer(char *aBuffer, PRInt32 aLength, nsIURI* aManifest);

public:
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
    PackageEntry(const nsACString& package);
    ~PackageEntry() { }

    
    enum {
      
      
      PLATFORM_PACKAGE = 1 << 0,

      
      
      
      XPCNATIVEWRAPPERS = 1 << 1,

      
      CONTENT_ACCESSIBLE = 1 << 2
    };

    nsCString        package;
    nsCOMPtr<nsIURI> baseURI;
    PRUint32         flags;
    nsProviderArray  locales;
    nsProviderArray  skins;
  };

private:
  static PLDHashNumber HashKey(PLDHashTable *table, const void *key);
  static PRBool        MatchKey(PLDHashTable *table, const PLDHashEntryHdr *entry,
                                const void *key);
  static void          ClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry);
  static PRBool        InitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                                 const void *key);

  static const PLDHashTableOps kTableOps;

public:
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

private:
  PRBool mInitialized;
  PRBool mProfileLoaded;

  
  PLDHashTable mPackagesHash;

  
  
  OverlayListHash mOverlayHash;
  OverlayListHash mStyleHash;

  
  nsInterfaceHashtable<nsURIHashKey, nsIURI> mOverrideTable;

  nsCString mSelectedLocale;
  nsCString mSelectedSkin;
};
