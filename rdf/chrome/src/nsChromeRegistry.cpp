








































#include <string.h>
#include "nsArrayEnumerator.h"
#include "nsCOMPtr.h"
#include "nsIChromeRegistry.h"
#include "nsChromeRegistry.h"
#include "nsChromeUIDataSource.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFObserver.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFXMLSink.h"
#include "rdf.h"
#include "nsIServiceManager.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIRDFResource.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsHashtable.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsISimpleEnumerator.h"
#include "nsNetUtil.h"
#include "nsIFileChannel.h"
#include "nsIXBLService.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMLocation.h"
#include "nsIWindowMediator.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIStyleSheet.h"
#include "nsICSSStyleSheet.h"
#include "nsIPresShell.h"
#include "nsIDocShell.h"
#include "nsISupportsArray.h"
#include "nsIDocumentObserver.h"
#include "nsIIOService.h"
#include "nsLayoutCID.h"
#include "prio.h"
#include "nsInt64.h"
#include "nsIDirectoryService.h"
#include "nsILocalFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIObserverService.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindowCollection.h"
#include "nsIAtom.h"
#include "nsStaticAtom.h"
#include "nsNetCID.h"
#include "nsIJARURI.h"
#include "nsIFileURL.h"
#include "nsIXPConnect.h"
#include "nsPresShellIterator.h"

static char kChromePrefix[] = "chrome://";
nsIAtom* nsChromeRegistry::sCPrefix; 

#define kChromeFileName           NS_LITERAL_CSTRING("chrome.rdf")
#define kInstalledChromeFileName  NS_LITERAL_CSTRING("installed-chrome.txt")

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFXMLDataSourceCID, NS_RDFXMLDATASOURCE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,      NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kCSSLoaderCID, NS_CSS_LOADER_CID);

class nsChromeRegistry;

nsIChromeRegistry* gChromeRegistry = nsnull;

#define CHROME_URI "http://www.mozilla.org/rdf/chrome#"

DEFINE_RDF_VOCAB(CHROME_URI, CHROME, selectedSkin);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, selectedLocale);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, baseURL);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, packages);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, package);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, name);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, image);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, locType);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, allowScripts);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, hasOverlays);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, hasStylesheets);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, skinVersion);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, localeVersion);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, packageVersion);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, disabled);
DEFINE_RDF_VOCAB(CHROME_URI, CHROME, xpcNativeWrappers);



nsChromeRegistry::nsChromeRegistry() : mRDFService(nsnull),
                                       mRDFContainerUtils(nsnull),
                                       mInstallInitialized(PR_FALSE),
                                       mProfileInitialized(PR_FALSE),
                                       mRuntimeProvider(PR_FALSE),
                                       mBatchInstallFlushes(PR_FALSE),
                                       mSearchedForOverride(PR_FALSE),
                                       mLegacyOverlayinfo(PR_FALSE)
{
  mDataSourceTable = nsnull;
}


static PRBool PR_CALLBACK
DatasourceEnumerator(nsHashKey *aKey, void *aData, void* closure)
{
  if (!closure || !aData)
    return PR_FALSE;

  nsIRDFCompositeDataSource* compositeDS = (nsIRDFCompositeDataSource*) closure;

  nsCOMPtr<nsISupports> supports = (nsISupports*)aData;

  nsCOMPtr<nsIRDFDataSource> dataSource = do_QueryInterface(supports);
  if (!dataSource)
    return PR_FALSE;

#ifdef DEBUG
  nsresult rv =
#endif
  compositeDS->RemoveDataSource(dataSource);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to RemoveDataSource");
  return PR_TRUE;
}


nsChromeRegistry::~nsChromeRegistry()
{
  gChromeRegistry = nsnull;
  
  if (mDataSourceTable) {
      mDataSourceTable->Enumerate(DatasourceEnumerator, mChromeDataSource);
      delete mDataSourceTable;
  }

  NS_IF_RELEASE(mRDFService);
  NS_IF_RELEASE(mRDFContainerUtils);

}

NS_IMPL_THREADSAFE_ISUPPORTS6(nsChromeRegistry,
                              nsIChromeRegistry,
                              nsIXULChromeRegistry,
                              nsIChromeRegistrySea,
                              nsIXULOverlayProvider,
                              nsIObserver,
                              nsISupportsWeakReference)




nsresult
nsChromeRegistry::Init()
{
  
  
  
  
  static const nsStaticAtom atoms[] = {
    { "c",             &sCPrefix },
    { "chrome",        nsnull },
    { "NC",            nsnull },
    { "baseURL",       nsnull},
    { "allowScripts",  nsnull },
    { "skinVersion",   nsnull },
    { "package",       nsnull },
    { "packages",      nsnull },
    { "locType",       nsnull },
    { "displayName",   nsnull },
    { "author",        nsnull },
    { "localeVersion", nsnull },
    { "localeType",    nsnull },
    { "selectedLocale", nsnull },
    { "selectedSkin",  nsnull },
    { "hasOverlays",   nsnull },
    { "xpcNativeWrappers", nsnull },
    { "previewURL", nsnull },
  };

  NS_RegisterStaticAtoms(atoms, NS_ARRAY_LENGTH(atoms));
  
  gChromeRegistry = this;
  
  nsresult rv;
  rv = CallGetService(kRDFServiceCID, &mRDFService);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(kRDFContainerUtilsCID, &mRDFContainerUtils);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_selectedSkin),
                                getter_AddRefs(mSelectedSkin));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_selectedLocale),
                                getter_AddRefs(mSelectedLocale));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_baseURL),
                                getter_AddRefs(mBaseURL));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_packages),
                                getter_AddRefs(mPackages));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_package),
                                getter_AddRefs(mPackage));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_name),
                                getter_AddRefs(mName));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_image),
                                getter_AddRefs(mImage));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_locType),
                                getter_AddRefs(mLocType));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_allowScripts),
                                getter_AddRefs(mAllowScripts));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_hasOverlays),
                                getter_AddRefs(mHasOverlays));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_hasStylesheets),
                                getter_AddRefs(mHasStylesheets));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_skinVersion),
                                getter_AddRefs(mSkinVersion));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_localeVersion),
                                getter_AddRefs(mLocaleVersion));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_packageVersion),
                                getter_AddRefs(mPackageVersion));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_disabled),
                                getter_AddRefs(mDisabled));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  rv = mRDFService->GetResource(nsDependentCString(kURICHROME_xpcNativeWrappers),
                                getter_AddRefs(mXPCNativeWrappers));
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF resource");

  nsCOMPtr<nsIObserverService> observerService =
           do_GetService("@mozilla.org/observer-service;1", &rv);
  if (observerService) {
    observerService->AddObserver(this, "profile-before-change", PR_TRUE);
    observerService->AddObserver(this, "profile-after-change", PR_TRUE);
  }

  CheckForNewChrome();
  
  FlagXPCNativeWrappers();

  return NS_OK;
}


static nsresult
SplitURL(nsIURI *aChromeURI, nsCString& aPackage, nsCString& aProvider, nsCString& aFile,
         PRBool *aModified = nsnull)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsresult rv;

  nsCAutoString str;
  rv = aChromeURI->GetSpec(str);
  if (NS_FAILED(rv)) return rv;

  
  
  if (PL_strncmp(str.get(), kChromePrefix, sizeof(kChromePrefix) - 1) != 0)
    return NS_ERROR_INVALID_ARG;

  
  aPackage = str.get() + sizeof(kChromePrefix) - 1;

  PRInt32 idx;
  idx = aPackage.FindChar('/');
  if (idx < 0)
    return NS_OK;

  
  aPackage.Right(aProvider, aPackage.Length() - (idx + 1));
  aPackage.Truncate(idx);

  idx = aProvider.FindChar('/');
  if (idx < 0) {
    
    idx = aProvider.Length();
    aProvider.Append('/');
  }

  
  aProvider.Right(aFile, aProvider.Length() - (idx + 1));
  aProvider.Truncate(idx);

  PRBool nofile = aFile.IsEmpty();
  if (nofile) {
    
    aFile = aPackage;

    if (aProvider.Equals("content")) {
      aFile += ".xul";
    }
    else if (aProvider.Equals("skin")) {
      aFile += ".css";
    }
    else if (aProvider.Equals("locale")) {
      aFile += ".dtd";
    }
    else {
      NS_ERROR("unknown provider");
      return NS_ERROR_FAILURE;
    }
  } else {
    
    
    int depth = 0;
    PRBool sawSlash = PR_TRUE;  
    for (const char* p=aFile.get(); *p; p++) {
      if (sawSlash) {
        if (p[0] == '.' && p[1] == '.'){
          depth--;    
        } else {
          static const char escape[] = "%2E%2E";
          if (PL_strncasecmp(p, escape, sizeof(escape)-1) == 0)
            depth--;   
        }
      } else if (p[0] != '/') {
        depth++;        
      }
      sawSlash = (p[0] == '/');

      if (depth < 0) {
        return NS_ERROR_FAILURE;
      }
    }
  }
  if (aModified)
    *aModified = nofile;
  return NS_OK;
}

static nsresult
GetBaseURLFile(const nsACString& aBaseURL, nsIFile** aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  *aFile = nsnull;

  nsresult rv;
  nsCOMPtr<nsIIOService> ioServ(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIURI> uri;
  rv = ioServ->NewURI(aBaseURL, nsnull, nsnull, getter_AddRefs(uri));
  if (NS_FAILED(rv)) return rv;

  
  
  nsCOMPtr<nsIJARURI> jarURI;
  while ((jarURI = do_QueryInterface(uri)) != nsnull)
    jarURI->GetJARFile(getter_AddRefs(uri));

  
  
  nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(uri));
  if (fileURL) {
    nsCOMPtr<nsIFile> file;
    fileURL->GetFile(getter_AddRefs(file));
    if (file) {
      NS_ADDREF(*aFile = file);
      return NS_OK;
    }
  }
  NS_ERROR("GetBaseURLFile() failed. Remote chrome?");
  return NS_ERROR_FAILURE;
}

nsresult
nsChromeRegistry::Canonify(nsIURI* aChromeURI)
{
  
  
  
  
  if (! aChromeURI)
      return NS_ERROR_NULL_POINTER;

  PRBool modified = PR_TRUE; 
  nsCAutoString package, provider, file;
  nsresult rv;
  rv = SplitURL(aChromeURI, package, provider, file, &modified);
  if (NS_FAILED(rv))
    return rv;

  if (!modified)
    return NS_OK;

  nsCAutoString canonical( kChromePrefix );
  canonical += package;
  canonical += "/";
  canonical += provider;
  canonical += "/";
  canonical += file;

  return aChromeURI->SetSpec(canonical);
}

NS_IMETHODIMP
nsChromeRegistry::ConvertChromeURL(nsIURI* aChromeURL, nsIURI* *aResult)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(aChromeURL);

  
  
  

  
  nsCAutoString package, provider, remaining;

  rv = SplitURL(aChromeURL, package, provider, remaining);
  if (NS_FAILED(rv)) return rv;

  
  
  if (!mProfileInitialized) {
    rv = LoadProfileDataSource();
    if (NS_FAILED(rv)) return rv;
  }
  if (!mInstallInitialized) {
    rv = LoadInstallDataSource();
    if (NS_FAILED(rv)) return rv;
  }

  nsCAutoString finalURL;

  rv = GetOverrideURL(package, provider, remaining, finalURL);
  if (NS_SUCCEEDED(rv))
    return NS_OK;
  
  rv = GetBaseURL(package, provider, finalURL);
#ifdef DEBUG
  if (NS_FAILED(rv)) {
    nsCAutoString msg("chrome: failed to get base url");
    nsCAutoString url;
    rv = aChromeURL->GetSpec(url);
    if (NS_SUCCEEDED(rv)) {
      msg += " for ";
      msg += url.get();
    }
    msg += " -- using wacky default";
    NS_WARNING(msg.get());
  }
#endif
  if (finalURL.IsEmpty()) {
    
    if (provider.Equals("skin")) {
      finalURL = "resource:/chrome/skins/classic/";
    }
    else if (provider.Equals("locale")) {
      finalURL = "resource:/chrome/locales/en-US/";
    }
    else if (package.Equals("aim")) {
      finalURL = "resource:/chrome/packages/aim/";
    }
    else if (package.Equals("messenger")) {
      finalURL = "resource:/chrome/packages/messenger/";
    }
    else if (package.Equals("global")) {
      finalURL = "resource:/chrome/packages/widget-toolkit/";
    }
    else {
      finalURL = "resource:/chrome/packages/core/";
    }
  }

  finalURL.Append(remaining);

  return NS_NewURI(aResult, finalURL);
}

nsresult
nsChromeRegistry::GetBaseURL(const nsACString& aPackage,
                             const nsACString& aProvider,
                             nsACString& aBaseURL)
{
  nsCOMPtr<nsIRDFResource> resource;

  nsCAutoString resourceStr("urn:mozilla:package:");
  resourceStr += aPackage;

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> packageResource;
  rv = GetResource(resourceStr, getter_AddRefs(packageResource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }

  
  nsCOMPtr<nsIRDFResource> arc;
  if (aProvider.EqualsLiteral("skin")) {
    arc = mSelectedSkin;
  }
  else if (aProvider.EqualsLiteral("locale")) {
    arc = mSelectedLocale;
  }
  else
    
    resource = packageResource;

  if (arc) {

    nsCOMPtr<nsIRDFNode> selectedProvider;
    if (NS_FAILED(rv = mChromeDataSource->GetTarget(packageResource, arc, PR_TRUE, getter_AddRefs(selectedProvider)))) {
      NS_ERROR("Unable to obtain the provider.");
      return rv;
    }

    resource = do_QueryInterface(selectedProvider);

    if (resource) {
      PRBool providerOK;
      rv = VerifyCompatibleProvider(packageResource, resource, arc, &providerOK);
      if (NS_FAILED(rv)) return rv;
      if (!providerOK) {
        
        
        
        
        
        if (NS_FAILED(rv = mInstallDirChromeDataSource->GetTarget(packageResource, arc, PR_TRUE, getter_AddRefs(selectedProvider)))) {
          NS_ERROR("Unable to obtain the provider.");
          return rv;
        }
        resource = do_QueryInterface(selectedProvider);
        if (resource) {
          rv = VerifyCompatibleProvider(packageResource, resource, arc, &providerOK);
          if (NS_FAILED(rv)) return rv;
          if (!providerOK) 
            selectedProvider = nsnull;
        }
      }
    }

    if (!selectedProvider) {
      
      
      FindProvider(aPackage, aProvider, arc, getter_AddRefs(selectedProvider));
      resource = do_QueryInterface(selectedProvider);
    }

    if (!selectedProvider)
      return rv;

    if (!resource)
      return NS_ERROR_FAILURE;
  }

  
  return FollowArc(mChromeDataSource, aBaseURL, resource, mBaseURL);
}

nsresult
nsChromeRegistry::GetOverrideURL(const nsACString& aPackage,
                                 const nsACString& aProvider,
                                 const nsACString& aPath,
                                 nsACString& aResult)
{
  nsresult rv = InitOverrideJAR();
  if (NS_FAILED(rv)) return rv;

  

  aResult.SetCapacity(mOverrideJARURL.Length() +
                      aPackage.Length() +
                      aProvider.Length() +
                      aPath.Length() + 2);
  
  aResult = mOverrideJARURL;
  aResult += aPackage;
  aResult += '/';
  aResult += aProvider;
  aResult += '/';

  
  
  
  if (aProvider.EqualsLiteral("skin") ||
      aProvider.EqualsLiteral("locale")) {

    
    nsIRDFResource* providerArc;
    if (aProvider.Equals("skin"))
      providerArc = mSelectedSkin;
    else
      providerArc = mSelectedLocale;
    
    nsCAutoString selectedProvider;
    rv = GetSelectedProvider(aPackage, aProvider, providerArc, selectedProvider);

    if (NS_SUCCEEDED(rv)) {
      aResult += selectedProvider;
      aResult += '/';
    }
  }
  
  aResult += aPath;

  
  PRBool ok;
  rv = mOverrideJAR->HasEntry(aResult, &ok);
  if (NS_FAILED(rv) || !ok) {
    aResult.Truncate();
  }
  return rv;
}

nsresult
nsChromeRegistry::InitOverrideJAR()
{
  
  if (mSearchedForOverride && !mOverrideJAR)
    return NS_ERROR_FAILURE;

  mSearchedForOverride = PR_TRUE;

  nsresult rv;
  
  
  
  nsCOMPtr<nsIFile> overrideFile;
  rv = GetInstallRoot(getter_AddRefs(overrideFile));
  if (NS_FAILED(rv)) return rv;

  rv = overrideFile->AppendNative(NS_LITERAL_CSTRING("custom.jar"));
  if (NS_FAILED(rv)) return rv;

  PRBool exists;
  rv = overrideFile->Exists(&exists);
  if (NS_FAILED(rv)) return rv;

  
  if (!exists)
    return NS_ERROR_FAILURE;

  
  
  
  mOverrideJARURL.Assign("jar:");
  nsCAutoString jarURL;
  rv = NS_GetURLSpecFromFile(overrideFile, jarURL);
  if (NS_FAILED(rv)) return rv;

  mOverrideJARURL.Append(jarURL);
  mOverrideJARURL.Append("!/");
  if (NS_FAILED(rv)) return rv;

  
  
  
  nsCOMPtr<nsIZipReaderCache> readerCache =
    do_CreateInstance("@mozilla.org/libjar/zip-reader-cache;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = readerCache->Init(32);
  
  rv = readerCache->GetZip(overrideFile, getter_AddRefs(mOverrideJAR));
  if (NS_FAILED(rv)) {
    mOverrideJARURL.Truncate();
    return rv;
  }
  
  return NS_OK;
}

nsresult
nsChromeRegistry::VerifyCompatibleProvider(nsIRDFResource* aPackageResource,
                                           nsIRDFResource* aProviderResource,
                                           nsIRDFResource* aArc,
                                           PRBool *aAcceptable)
{
  
  
  
  nsCOMPtr<nsIRDFResource> versionArc;
  if (aArc == mSelectedSkin)
    versionArc = mSkinVersion;
  else 
    versionArc = mLocaleVersion;

  nsCOMPtr<nsIRDFNode> packageVersionNode;
  mChromeDataSource->GetTarget(aPackageResource, versionArc, PR_TRUE,
                               getter_AddRefs(packageVersionNode));
  if (packageVersionNode) {
    
    
    mChromeDataSource->HasAssertion(aProviderResource, versionArc,
                                    packageVersionNode, PR_TRUE, aAcceptable);
    if (!*aAcceptable)
      return NS_OK;
  }
  
  
  
  nsCAutoString providerBaseURL;
  nsresult rv = FollowArc(mChromeDataSource, providerBaseURL,
                          aProviderResource, mBaseURL);
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsIFile> baseURLFile;
  rv = GetBaseURLFile(providerBaseURL, getter_AddRefs(baseURLFile));
  if (NS_FAILED(rv))
    return rv;
  rv = baseURLFile->Exists(aAcceptable);
#if DEBUG
  if (NS_FAILED(rv) || !*aAcceptable)
    printf("BaseURL %s cannot be found.\n",
           PromiseFlatCString(providerBaseURL).get());
#endif
  return rv;
}


nsresult
nsChromeRegistry::FindProvider(const nsACString& aPackage,
                               const nsACString& aProvider,
                               nsIRDFResource *aArc,
                               nsIRDFNode **aSelectedProvider)
{
  *aSelectedProvider = nsnull;

  nsCAutoString rootStr("urn:mozilla:");
  nsresult rv = NS_OK;

  rootStr += aProvider;
  rootStr += ":root";

  
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(rootStr, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the provider root resource.");
    return rv;
  }

  
  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = container->Init(mChromeDataSource, resource);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsISimpleEnumerator> arcs;
  rv = container->GetElements(getter_AddRefs(arcs));
  if (NS_FAILED(rv)) return rv;

  
  PRBool moreElements;
  rv = arcs->HasMoreElements(&moreElements);
  if (NS_FAILED(rv)) return rv;
  for ( ; moreElements; arcs->HasMoreElements(&moreElements)) {

    
    nsCOMPtr<nsISupports> supports;
    rv = arcs->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIRDFResource> kid = do_QueryInterface(supports);

    if (kid) {
      
      nsCAutoString providerName;
      rv = FollowArc(mChromeDataSource, providerName, kid, mName);
      if (NS_FAILED(rv)) return rv;

      
      nsCOMPtr<nsIRDFNode> packageNode;
      nsCOMPtr<nsIRDFResource> packageList;
      rv = mChromeDataSource->GetTarget(kid, mPackages, PR_TRUE, getter_AddRefs(packageNode));
      if (NS_SUCCEEDED(rv))
        packageList = do_QueryInterface(packageNode);
      if (!packageList)
        continue;

      
      rv = SelectPackageInProvider(packageList, aPackage, aProvider, providerName,
                                   aArc, aSelectedProvider);
      if (NS_FAILED(rv))
        continue; 

      if (*aSelectedProvider)
        return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsChromeRegistry::SelectPackageInProvider(nsIRDFResource *aPackageList,
                                          const nsACString& aPackage,
                                          const nsACString& aProvider,
                                          const nsACString& aProviderName,
                                          nsIRDFResource *aArc,
                                          nsIRDFNode **aSelectedProvider)
{
  *aSelectedProvider = nsnull;

  nsresult rv = NS_OK;

  
  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_SUCCEEDED(rv))
    rv = container->Init(mChromeDataSource, aPackageList);
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsISimpleEnumerator> arcs;
  rv = container->GetElements(getter_AddRefs(arcs));
  if (NS_FAILED(rv)) return rv;

  PRBool moreElements;
  rv = arcs->HasMoreElements(&moreElements);
  if (NS_FAILED(rv)) return rv;
  for ( ; moreElements; arcs->HasMoreElements(&moreElements)) {

    
    nsCOMPtr<nsISupports> supports;
    rv = arcs->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIRDFResource> kid = do_QueryInterface(supports);

    if (kid) {
      
      nsCOMPtr<nsIRDFNode> packageNode;
      nsCOMPtr<nsIRDFResource> package;
      rv = mChromeDataSource->GetTarget(kid, mPackage, PR_TRUE, getter_AddRefs(packageNode));
      if (NS_SUCCEEDED(rv))
        package = do_QueryInterface(packageNode);
      if (!package)
        continue;

      
      nsCAutoString packageName;
      rv = FollowArc(mChromeDataSource, packageName, package, mName);
      if (NS_FAILED(rv))
        continue;       

      if (packageName.Equals(aPackage)) {
        PRBool useProfile = !mProfileRoot.IsEmpty();
        
        if (packageName.Equals("global") || packageName.Equals("communicator"))
          useProfile = PR_FALSE; 
                                 
        rv = SelectProviderForPackage(aProvider,
                                      aProviderName,
                                      NS_ConvertASCIItoUTF16(packageName).get(),
                                      aArc, useProfile, PR_TRUE);
        if (NS_FAILED(rv))
          return NS_ERROR_FAILURE;

        *aSelectedProvider = kid;
        NS_ADDREF(*aSelectedProvider);
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

nsresult
nsChromeRegistry::GetDynamicDataSource(nsIURI *aChromeURL,
                                       PRBool aIsOverlay, PRBool aUseProfile,
                                       PRBool aCreateDS,
                                       nsIRDFDataSource **aResult)
{
  *aResult = nsnull;

  nsresult rv;

  if (!mDataSourceTable)
    return NS_OK;

  
  nsCAutoString package, provider, remaining;

  rv = SplitURL(aChromeURL, package, provider, remaining);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aCreateDS) {
    
    
    
    
    nsDependentCString dataSourceStr(kChromeFileName);
    nsCOMPtr<nsIRDFDataSource> mainDataSource;
    rv = LoadDataSource(dataSourceStr, getter_AddRefs(mainDataSource), aUseProfile, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    nsCOMPtr<nsIRDFResource> hasDynamicDataArc;
    if (aIsOverlay)
      hasDynamicDataArc = mHasOverlays;
    else
      hasDynamicDataArc = mHasStylesheets;
    
    
    nsCAutoString packageResourceStr("urn:mozilla:package:");
    packageResourceStr += package;
    nsCOMPtr<nsIRDFResource> packageResource;
    GetResource(packageResourceStr, getter_AddRefs(packageResource));
    
    
    
    nsCOMPtr<nsIRDFNode> hasDynamicDSNode;
    mainDataSource->GetTarget(packageResource, hasDynamicDataArc, PR_TRUE,
                              getter_AddRefs(hasDynamicDSNode));
    if (!hasDynamicDSNode)
      return NS_OK; 
  }

  
  nsCAutoString overlayFile; 
  if (aUseProfile && mLegacyOverlayinfo)
  {
    overlayFile.AppendLiteral("overlayinfo/");
    overlayFile += package;
    if (aIsOverlay)
      overlayFile.AppendLiteral("/content/");
    else
      overlayFile.AppendLiteral("/skin/");
  }
  if (aIsOverlay)
    overlayFile.AppendLiteral("overlays.rdf");
  else
    overlayFile.AppendLiteral("stylesheets.rdf");

  return LoadDataSource(overlayFile, aResult, aUseProfile, nsnull);
}

NS_IMETHODIMP
nsChromeRegistry::GetStyleOverlays(nsIURI *aChromeURL,
                                   nsISimpleEnumerator **aResult)
{
  return GetDynamicInfo(aChromeURL, PR_FALSE, aResult);
}

NS_IMETHODIMP
nsChromeRegistry::GetXULOverlays(nsIURI *aChromeURL, nsISimpleEnumerator **aResult)
{
  return GetDynamicInfo(aChromeURL, PR_TRUE, aResult);
}

nsresult
nsChromeRegistry::GetURIList(nsIRDFDataSource *aSource,
                             nsIRDFResource *aResource,
                             nsCOMArray<nsIURI>& aArray)
{
  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> arcs;
  nsCOMPtr<nsIRDFContainer> container =
    do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv)) goto end_GetURIList;

  rv = container->Init(aSource, aResource);
  if (NS_FAILED(rv)) {
    rv = NS_OK;
    goto end_GetURIList;
  }

  rv = container->GetElements(getter_AddRefs(arcs));
  if (NS_FAILED(rv)) goto end_GetURIList;

  {
    nsCOMPtr<nsISupports> supports;
    nsCOMPtr<nsIRDFLiteral> value;
    nsCOMPtr<nsIURI> uri;
    PRBool hasMore;

    while (NS_SUCCEEDED(rv = arcs->HasMoreElements(&hasMore)) && hasMore) {
      rv = arcs->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv)) break;

      value = do_QueryInterface(supports, &rv);
      if (NS_FAILED(rv)) continue;

      const PRUnichar* valueStr;
      rv = value->GetValueConst(&valueStr);
      if (NS_FAILED(rv)) continue;

      rv = NS_NewURI(getter_AddRefs(uri), NS_ConvertUTF16toUTF8(valueStr));
      if (NS_FAILED(rv)) continue;

      if (IsOverlayAllowed(uri)) {
        if (!aArray.AppendObject(uri)) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }
      }
    }
  }

end_GetURIList:
  return rv;
}

nsresult
nsChromeRegistry::GetDynamicInfo(nsIURI *aChromeURL, PRBool aIsOverlay,
                                 nsISimpleEnumerator **aResult)
{
  *aResult = nsnull;

  nsresult rv;

  if (!mDataSourceTable)
    return NS_OK;

  nsCOMPtr<nsIRDFDataSource> installSource;
  rv = GetDynamicDataSource(aChromeURL, aIsOverlay, PR_FALSE, PR_FALSE,
                            getter_AddRefs(installSource));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRDFDataSource> profileSource;
  if (mProfileInitialized) {
    rv = GetDynamicDataSource(aChromeURL, aIsOverlay, PR_TRUE, PR_FALSE,
                              getter_AddRefs(profileSource));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCAutoString lookup;
  rv = aChromeURL->GetSpec(lookup);
  NS_ENSURE_SUCCESS(rv, rv);
   
  
  nsCOMPtr<nsIRDFResource> chromeResource;
  rv = GetResource(lookup, getter_AddRefs(chromeResource));
  if (NS_FAILED(rv)) {
      NS_ERROR("Unable to retrieve the resource corresponding to the chrome skin or content.");
      return rv;
  }

  nsCOMArray<nsIURI> overlayURIs;

  if (installSource) {
    GetURIList(installSource, chromeResource, overlayURIs);
  }
  if (profileSource) {
    GetURIList(profileSource, chromeResource, overlayURIs);
  }

  return NS_NewArrayEnumerator(aResult, overlayURIs);
}

nsresult
nsChromeRegistry::LoadDataSource(const nsACString &aFileName,
                                 nsIRDFDataSource **aResult,
                                 PRBool aUseProfileDir,
                                 const char *aProfilePath)
{
  
  *aResult = nsnull;

  nsCAutoString key;

  
  if (aUseProfileDir) {
    
    if (aProfilePath) {
      key = aProfilePath;
      key += "chrome/";
    }
    else
      key = mProfileRoot;

    key += aFileName;
  }
  else {
    key = mInstallRoot;
    key += aFileName;
  }

  if (mDataSourceTable)
  {
    nsCStringKey skey(key);
    nsCOMPtr<nsISupports> supports =
      getter_AddRefs(static_cast<nsISupports*>(mDataSourceTable->Get(&skey)));

    if (supports)
    {
      nsCOMPtr<nsIRDFDataSource> dataSource = do_QueryInterface(supports);
      if (dataSource)
      {
        *aResult = dataSource;
        NS_ADDREF(*aResult);
        return NS_OK;
      }
      return NS_ERROR_FAILURE;
    }
  }

  nsresult rv = CallCreateInstance(kRDFXMLDataSourceCID, aResult);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(*aResult);
  if (sink)
    sink->AddNameSpace(sCPrefix, NS_ConvertASCIItoUTF16(CHROME_URI));

  nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(*aResult);
  if (! remote)
      return NS_ERROR_UNEXPECTED;

  if (!mDataSourceTable)
    mDataSourceTable = new nsSupportsHashtable;

  
  rv = remote->Init(key.get());
  if (NS_SUCCEEDED(rv))
    rv = remote->Refresh(PR_TRUE);

  nsCOMPtr<nsISupports> supports = do_QueryInterface(remote);
  nsCStringKey skey(key);
  mDataSourceTable->Put(&skey, supports.get());

  return NS_OK;
}



nsresult
nsChromeRegistry::GetResource(const nsACString& aURL,
                              nsIRDFResource** aResult)
{
  nsresult rv = NS_OK;
  if (NS_FAILED(rv = mRDFService->GetResource(aURL, aResult))) {
    NS_ERROR("Unable to retrieve a resource for this URL.");
    *aResult = nsnull;
    return rv;
  }
  return NS_OK;
}

nsresult
nsChromeRegistry::FollowArc(nsIRDFDataSource *aDataSource,
                            nsACString& aResult,
                            nsIRDFResource* aChromeResource,
                            nsIRDFResource* aProperty)
{
  if (!aDataSource)
    return NS_ERROR_FAILURE;

  nsresult rv;

  nsCOMPtr<nsIRDFNode> chromeBase;
  rv = aDataSource->GetTarget(aChromeResource, aProperty, PR_TRUE, getter_AddRefs(chromeBase));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain a base resource.");
    return rv;
  }

  if (chromeBase == nsnull)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIRDFResource> resource(do_QueryInterface(chromeBase));

  if (resource) {
    nsXPIDLCString uri;
    rv = resource->GetValue(getter_Copies(uri));
    if (NS_FAILED(rv)) return rv;
    aResult.Assign(uri);
    return NS_OK;
  }

  nsCOMPtr<nsIRDFLiteral> literal(do_QueryInterface(chromeBase));
  if (literal) {
    const PRUnichar *s;
    rv = literal->GetValueConst(&s);
    if (NS_FAILED(rv)) return rv;
    CopyUTF16toUTF8(s, aResult);
  }
  else {
    
    NS_ERROR("uh, this isn't a resource or a literal!");
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

nsresult
nsChromeRegistry::UpdateArc(nsIRDFDataSource *aDataSource, nsIRDFResource* aSource,
                            nsIRDFResource* aProperty,
                            nsIRDFNode *aTarget, PRBool aRemove)
{
  nsresult rv;
  
  nsCOMPtr<nsIRDFNode> retVal;
  rv = aDataSource->GetTarget(aSource, aProperty, PR_TRUE, getter_AddRefs(retVal));
  if (NS_FAILED(rv)) return rv;

  if (retVal) {
    if (!aRemove)
      aDataSource->Change(aSource, aProperty, retVal, aTarget);
    else
      aDataSource->Unassert(aSource, aProperty, aTarget);
  }
  else if (!aRemove)
    aDataSource->Assert(aSource, aProperty, aTarget, PR_TRUE);

  return NS_OK;
}






static void FlushSkinBindingsForWindow(nsIDOMWindowInternal* aWindow)
{
  
  nsCOMPtr<nsIDOMDocument> domDocument;
  aWindow->GetDocument(getter_AddRefs(domDocument));
  if (!domDocument)
    return;

  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);
  if (!document)
    return;

  
  document->FlushSkinBindings();
}


NS_IMETHODIMP nsChromeRegistry::RefreshSkins()
{
  nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!windowMediator)
    return NS_OK;

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
  PRBool more;
  windowEnumerator->HasMoreElements(&more);
  while (more) {
    nsCOMPtr<nsISupports> protoWindow;
    windowEnumerator->GetNext(getter_AddRefs(protoWindow));
    if (protoWindow) {
      nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(protoWindow);
      if (domWindow)
        FlushSkinBindingsForWindow(domWindow);
    }
    windowEnumerator->HasMoreElements(&more);
  }

  FlushSkinCaches();
  
  windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
  windowEnumerator->HasMoreElements(&more);
  while (more) {
    nsCOMPtr<nsISupports> protoWindow;
    windowEnumerator->GetNext(getter_AddRefs(protoWindow));
    if (protoWindow) {
      nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(protoWindow);
      if (domWindow)
        RefreshWindow(domWindow);
    }
    windowEnumerator->HasMoreElements(&more);
  }
   
  return NS_OK;
}


void
nsChromeRegistry::FlushSkinCaches()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ASSERTION(obsSvc, "Couldn't get observer service.");

  obsSvc->NotifyObservers((nsIChromeRegistry*) this,
                          NS_CHROME_FLUSH_SKINS_TOPIC, nsnull);
}

static PRBool IsChromeURI(nsIURI* aURI)
{
    PRBool isChrome=PR_FALSE;
    if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome)
        return PR_TRUE;
    return PR_FALSE;
}


nsresult nsChromeRegistry::RefreshWindow(nsIDOMWindowInternal* aWindow)
{
  
  nsCOMPtr<nsIDOMWindowCollection> frames;
  aWindow->GetFrames(getter_AddRefs(frames));
  PRUint32 length;
  frames->GetLength(&length);
  PRUint32 j;
  for (j = 0; j < length; j++) {
    nsCOMPtr<nsIDOMWindow> childWin;
    frames->Item(j, getter_AddRefs(childWin));
    nsCOMPtr<nsIDOMWindowInternal> childInt(do_QueryInterface(childWin));
    RefreshWindow(childInt);
  }

  nsresult rv;
  
  nsCOMPtr<nsIDOMDocument> domDocument;
  aWindow->GetDocument(getter_AddRefs(domDocument));
  if (!domDocument)
    return NS_OK;

  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);
  if (!document)
    return NS_OK;

  
  nsPresShellIterator iter(document);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    
    nsCOMArray<nsIStyleSheet> agentSheets;
    rv = shell->GetAgentStyleSheets(agentSheets);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMArray<nsIStyleSheet> newAgentSheets;
    for (PRInt32 l = 0; l < agentSheets.Count(); ++l) {
      nsIStyleSheet *sheet = agentSheets[l];

      nsCOMPtr<nsIURI> uri;
      rv = sheet->GetSheetURI(getter_AddRefs(uri));
      if (NS_FAILED(rv)) return rv;

      if (IsChromeURI(uri)) {
        
        nsCOMPtr<nsICSSStyleSheet> newSheet;
        rv = LoadStyleSheetWithURL(uri, PR_TRUE, getter_AddRefs(newSheet));
        if (NS_FAILED(rv)) return rv;
        if (newSheet) {
          rv = newAgentSheets.AppendObject(newSheet) ? NS_OK : NS_ERROR_FAILURE;
          if (NS_FAILED(rv)) return rv;
        }
      }
      else {  
        rv = newAgentSheets.AppendObject(sheet) ? NS_OK : NS_ERROR_FAILURE;
        if (NS_FAILED(rv)) return rv;
      }
    }

    rv = shell->SetAgentStyleSheets(newAgentSheets);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMArray<nsIStyleSheet> oldSheets;
  nsCOMArray<nsIStyleSheet> newSheets;

  PRInt32 count = document->GetNumberOfStyleSheets();

  
  PRInt32 i;
  for (i = 0; i < count; i++) {
    
    nsIStyleSheet *styleSheet = document->GetStyleSheetAt(i);
    
    if (!oldSheets.AppendObject(styleSheet)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  for (i = 0; i < count; i++) {
    nsCOMPtr<nsIStyleSheet> sheet = oldSheets[i];
    nsCOMPtr<nsIURI> uri;
    rv = sheet->GetSheetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    if (IsChromeURI(uri)) {
      
#ifdef DEBUG
      nsCOMPtr<nsICSSStyleSheet> oldCSSSheet = do_QueryInterface(sheet);
      NS_ASSERTION(oldCSSSheet, "Don't know how to reload a non-CSS sheet");
#endif
      nsCOMPtr<nsICSSStyleSheet> newSheet;
      
      
      
      LoadStyleSheetWithURL(uri, PR_FALSE, getter_AddRefs(newSheet));
      
      newSheets.AppendObject(newSheet);
    }
    else {
      
      newSheets.AppendObject(sheet);
    }
  }

  
  document->UpdateStyleSheets(oldSheets, newSheets);
  return NS_OK;
}

nsresult
nsChromeRegistry::WriteInfoToDataSource(const char *aDocURI,
                                        const PRUnichar *aOverlayURI,
                                        PRBool aIsOverlay,
                                        PRBool aUseProfile,
                                        PRBool aRemove)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  nsCAutoString str(aDocURI);
  rv = NS_NewURI(getter_AddRefs(uri), str);
  if (NS_FAILED(rv)) return rv;

  if (!aRemove) {
    
    
    
    
    
    nsCAutoString package, provider, file;
    rv = SplitURL(uri, package, provider, file);
    if (NS_FAILED(rv)) return NS_OK;
    
    
    nsDependentCString dataSourceStr(kChromeFileName);
    nsCOMPtr<nsIRDFDataSource> mainDataSource;
    rv = LoadDataSource(dataSourceStr, getter_AddRefs(mainDataSource), aUseProfile, nsnull);
    if (NS_FAILED(rv)) return rv;
    
    
    
    
    nsCOMPtr<nsIRDFResource> hasDynamicDataArc;
    if (aIsOverlay)
      hasDynamicDataArc = mHasOverlays;
    else
      hasDynamicDataArc = mHasStylesheets;
    
    
    nsCAutoString packageResourceStr("urn:mozilla:package:");
    packageResourceStr += package;
    nsCOMPtr<nsIRDFResource> packageResource;
    GetResource(packageResourceStr, getter_AddRefs(packageResource));
    
    
    nsCOMPtr<nsIRDFLiteral> trueLiteral;
    mRDFService->GetLiteral(NS_LITERAL_STRING("true").get(), getter_AddRefs(trueLiteral));
    nsChromeRegistry::UpdateArc(mainDataSource, packageResource, 
                                hasDynamicDataArc, 
                                trueLiteral, PR_FALSE);
  }

  nsCOMPtr<nsIRDFDataSource> dataSource;
  rv = GetDynamicDataSource(uri, aIsOverlay, aUseProfile, PR_TRUE, getter_AddRefs(dataSource));
  if (NS_FAILED(rv)) return rv;

  if (!dataSource)
    return NS_OK;

  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(str, getter_AddRefs(resource));

  if (NS_FAILED(rv))
    return NS_OK;

  nsCOMPtr<nsIRDFContainer> container;
  rv = mRDFContainerUtils->MakeSeq(dataSource, resource, getter_AddRefs(container));
  if (NS_FAILED(rv)) return rv;
  if (!container) {
    
    container = do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
    if (NS_FAILED(rv)) return rv;
    rv = container->Init(dataSource, resource);
    if (NS_FAILED(rv)) return rv;
  }

  nsAutoString unistr(aOverlayURI);
  nsCOMPtr<nsIRDFLiteral> literal;
  rv = mRDFService->GetLiteral(unistr.get(), getter_AddRefs(literal));
  if (NS_FAILED(rv)) return rv;

  if (aRemove) {
    rv = container->RemoveElement(literal, PR_TRUE);
    if (NS_FAILED(rv)) return rv;
  }
  else {
    PRInt32 index;
    rv = container->IndexOf(literal, &index);
    if (NS_FAILED(rv)) return rv;
    if (index == -1) {
      rv = container->AppendElement(literal);
      if (NS_FAILED(rv)) return rv;
    }
  }

  nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(dataSource, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = remote->Flush();
  }

  return rv;
}

nsresult
nsChromeRegistry::UpdateDynamicDataSource(nsIRDFDataSource *aDataSource,
                                          nsIRDFResource *aResource,
                                          PRBool aIsOverlay,
                                          PRBool aUseProfile, PRBool aRemove)
{
  nsresult rv;

  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = container->Init(aDataSource, aResource);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISimpleEnumerator> arcs;
  rv = container->GetElements(getter_AddRefs(arcs));
  if (NS_FAILED(rv)) return rv;

  PRBool moreElements;
  rv = arcs->HasMoreElements(&moreElements);
  if (NS_FAILED(rv)) return rv;

  const char *value;
  rv = aResource->GetValueConst(&value);
  if (NS_FAILED(rv)) return rv;

  while (moreElements)
  {
    nsCOMPtr<nsISupports> supports;
    rv = arcs->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(supports, &rv);

    if (NS_SUCCEEDED(rv))
    {
      const PRUnichar* valueStr;
      rv = literal->GetValueConst(&valueStr);
      if (NS_FAILED(rv)) return rv;

      rv = WriteInfoToDataSource(value, valueStr, aIsOverlay, aUseProfile, aRemove);
      if (NS_FAILED(rv)) return rv;
    }
    rv = arcs->HasMoreElements(&moreElements);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}


nsresult
nsChromeRegistry::UpdateDynamicDataSources(nsIRDFDataSource *aDataSource,
                                           PRBool aIsOverlay,
                                           PRBool aUseProfile, PRBool aRemove)
{
  nsresult rv;
  nsCOMPtr<nsIRDFResource> resource;
  nsCAutoString root;
  if (aIsOverlay)
    root.Assign("urn:mozilla:overlays");
  else root.Assign("urn:mozilla:stylesheets");

  rv = GetResource(root, getter_AddRefs(resource));

  if (!resource)
    return NS_OK;

  nsCOMPtr<nsIRDFContainer> container(do_CreateInstance("@mozilla.org/rdf/container;1"));
  if (!container)
    return NS_OK;

  if (NS_FAILED(container->Init(aDataSource, resource)))
    return NS_OK;

  nsCOMPtr<nsISimpleEnumerator> arcs;
  if (NS_FAILED(container->GetElements(getter_AddRefs(arcs))))
    return NS_OK;

  PRBool moreElements;
  rv = arcs->HasMoreElements(&moreElements);
  if (NS_FAILED(rv)) return rv;

  while (moreElements)
  {
    nsCOMPtr<nsISupports> supports;
    rv = arcs->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> resource2 = do_QueryInterface(supports, &rv);

    if (NS_SUCCEEDED(rv))
    {
      rv = UpdateDynamicDataSource(aDataSource, resource2, aIsOverlay, aUseProfile, aRemove);
      if (NS_FAILED(rv)) return rv;
    }

    rv = arcs->HasMoreElements(&moreElements);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP nsChromeRegistry::SelectSkin(const nsACString& aSkin,
                                        PRBool aUseProfile)
{
  nsresult rv = SetProvider(NS_LITERAL_CSTRING("skin"), mSelectedSkin, aSkin, aUseProfile, nsnull, PR_TRUE);
  FlushSkinCaches();
  return rv;
}

NS_IMETHODIMP nsChromeRegistry::SelectLocale(const nsACString& aLocale,
                                          PRBool aUseProfile)
{
  return SetProvider(NS_LITERAL_CSTRING("locale"), mSelectedLocale, aLocale, aUseProfile, nsnull, PR_TRUE);
}

NS_IMETHODIMP nsChromeRegistry::SelectLocaleForProfile(const nsACString& aLocale,
                                                       const PRUnichar *aProfilePath)
{
  
  return SetProvider(NS_LITERAL_CSTRING("locale"), mSelectedLocale, aLocale, PR_TRUE, NS_ConvertUTF16toUTF8(aProfilePath).get(), PR_TRUE);
}

NS_IMETHODIMP nsChromeRegistry::SelectSkinForProfile(const nsACString& aSkin,
                                                     const PRUnichar *aProfilePath)
{
  return SetProvider(NS_LITERAL_CSTRING("skin"), mSelectedSkin, aSkin, PR_TRUE, NS_ConvertUTF16toUTF8(aProfilePath).get(), PR_TRUE);
}



NS_IMETHODIMP nsChromeRegistry::SetRuntimeProvider(PRBool runtimeProvider)
{
  mRuntimeProvider = runtimeProvider;
  return NS_OK;
}



NS_IMETHODIMP
nsChromeRegistry::GetSelectedLocale(const nsACString& aPackageName,
                                    nsACString& aResult)
{
  return GetSelectedProvider(aPackageName,
                             NS_LITERAL_CSTRING("locale"), mSelectedLocale,
                             aResult);
}

NS_IMETHODIMP
nsChromeRegistry::GetSelectedSkin(const nsACString& aPackageName,
                                  nsACString& aResult)
{
  return GetSelectedProvider(aPackageName,
                             NS_LITERAL_CSTRING("skin"), mSelectedSkin,
                             aResult);
}

nsresult
nsChromeRegistry::GetSelectedProvider(const nsACString& aPackageName,
                                      const nsACString& aProvider,
                                      nsIRDFResource* aSelectionArc,
                                      nsACString& _retval)
{
  
  
  if (!mChromeDataSource) {
    return NS_ERROR_FAILURE;
  }

  nsCAutoString resourceStr("urn:mozilla:package:");
  resourceStr += aPackageName;

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(resourceStr, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }

  if (mChromeDataSource == nsnull)
    return NS_ERROR_NULL_POINTER;

  
  nsCOMPtr<nsIRDFNode> selectedProvider;
  if (NS_FAILED(rv = mChromeDataSource->GetTarget(resource, aSelectionArc, PR_TRUE, getter_AddRefs(selectedProvider)))) {
    NS_ERROR("Unable to obtain the provider.");
    return rv;
  }

  if (!selectedProvider) {
    rv = FindProvider(aPackageName, aProvider, aSelectionArc, getter_AddRefs(selectedProvider));
    if (!selectedProvider)
      return rv;
  }

  resource = do_QueryInterface(selectedProvider);
  if (!resource)
    return NS_ERROR_FAILURE;

  
  const char *uri;
  if (NS_FAILED(rv = resource->GetValueConst(&uri)))
    return rv;

  
  nsCAutoString packageStr(":");
  packageStr += aPackageName;

  nsCAutoString ustr(uri);
  PRInt32 pos = ustr.RFind(packageStr);
  nsCAutoString urn;
  ustr.Left(urn, pos);

  rv = GetResource(urn, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the provider resource.");
    return rv;
  }

  
  return FollowArc(mChromeDataSource, _retval, resource, mName);
}

NS_IMETHODIMP nsChromeRegistry::DeselectSkin(const nsACString& aSkin,
                                        PRBool aUseProfile)
{
  nsresult rv = SetProvider(NS_LITERAL_CSTRING("skin"), mSelectedSkin, aSkin, aUseProfile, nsnull, PR_FALSE);
  FlushSkinCaches();
  return rv;
}

NS_IMETHODIMP nsChromeRegistry::DeselectLocale(const nsACString& aLocale,
                                          PRBool aUseProfile)
{
  return SetProvider(NS_LITERAL_CSTRING("locale"), mSelectedLocale, aLocale, aUseProfile, nsnull, PR_FALSE);
}

nsresult
nsChromeRegistry::SetProvider(const nsACString& aProvider,
                              nsIRDFResource* aSelectionArc,
                              const nsACString& aProviderName,
                              PRBool aUseProfile, const char *aProfilePath,
                              PRBool aIsAdding)
{
  
  
  nsCAutoString resourceStr( "urn:mozilla:" );
  resourceStr += aProvider;
  resourceStr += ":";
  resourceStr += aProviderName;

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(resourceStr, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }
  NS_ASSERTION(resource, "failed to GetResource");

  
  nsCOMPtr<nsIRDFNode> packageList;
  rv = mChromeDataSource->GetTarget(resource, mPackages, PR_TRUE, getter_AddRefs(packageList));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the SEQ for the package list.");
    return rv;
  }
  

  nsCOMPtr<nsIRDFResource> packageSeq(do_QueryInterface(packageList, &rv));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv))
    return NS_OK;

  if (NS_FAILED(container->Init(mChromeDataSource, packageSeq)))
    return NS_OK;

  nsCOMPtr<nsISimpleEnumerator> arcs;
  if (NS_FAILED(container->GetElements(getter_AddRefs(arcs))))
    return NS_OK;

  
  
  PRBool more;
  rv = arcs->HasMoreElements(&more);
  if (NS_FAILED(rv)) return rv;
  while (more) {
    nsCOMPtr<nsISupports> packageSkinEntry;
    rv = arcs->GetNext(getter_AddRefs(packageSkinEntry));
    if (NS_SUCCEEDED(rv) && packageSkinEntry) {
      nsCOMPtr<nsIRDFResource> entry = do_QueryInterface(packageSkinEntry);
      if (entry) {
         
         nsCOMPtr<nsIRDFNode> packageNode;
         rv = mChromeDataSource->GetTarget(entry, mPackage, PR_TRUE, getter_AddRefs(packageNode));
         if (NS_FAILED(rv)) {
           NS_ERROR("Unable to obtain the package resource.");
           return rv;
         }

         
         nsCOMPtr<nsIRDFResource> packageResource(do_QueryInterface(packageNode));
         if (packageResource) {
           rv = SetProviderForPackage(aProvider, packageResource, entry, aSelectionArc, aUseProfile, aProfilePath, aIsAdding);
           if (NS_FAILED(rv))
             continue; 
         }
      }
    }
    rv = arcs->HasMoreElements(&more);
    if (NS_FAILED(rv)) return rv;
  }

  
  mRuntimeProvider = PR_FALSE;

  return NS_OK;
}

nsresult
nsChromeRegistry::SetProviderForPackage(const nsACString& aProvider,
                                        nsIRDFResource* aPackageResource,
                                        nsIRDFResource* aProviderPackageResource,
                                        nsIRDFResource* aSelectionArc,
                                        PRBool aUseProfile, const char *aProfilePath,
                                        PRBool aIsAdding)
{
  nsresult rv;
  
  if (aUseProfile && !mProfileInitialized) {
    rv = LoadProfileDataSource();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsCOMPtr<nsIRDFDataSource> dataSource;
  rv = LoadDataSource(kChromeFileName, getter_AddRefs(dataSource), aUseProfile, aProfilePath);
  if (NS_FAILED(rv)) return rv;

  rv = nsChromeRegistry::UpdateArc(dataSource, aPackageResource, aSelectionArc, aProviderPackageResource, !aIsAdding);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(dataSource, &rv);
  if (NS_FAILED(rv)) return rv;

  
  
  if (!mBatchInstallFlushes && !mRuntimeProvider)
    rv = remote->Flush();

  return rv;
}

NS_IMETHODIMP nsChromeRegistry::SelectSkinForPackage(const nsACString& aSkin,
                                                     const PRUnichar *aPackageName,
                                                  PRBool aUseProfile)
{
  return SelectProviderForPackage(NS_LITERAL_CSTRING("skin"), aSkin, aPackageName, mSelectedSkin, aUseProfile, PR_TRUE);
}

NS_IMETHODIMP nsChromeRegistry::SelectLocaleForPackage(const nsACString& aLocale,
                                                       const PRUnichar *aPackageName,
                                                    PRBool aUseProfile)
{
  return SelectProviderForPackage(NS_LITERAL_CSTRING("locale"), aLocale, aPackageName, mSelectedLocale, aUseProfile, PR_TRUE);
}

NS_IMETHODIMP nsChromeRegistry::DeselectSkinForPackage(const nsACString& aSkin,
                                                  const PRUnichar *aPackageName,
                                                  PRBool aUseProfile)
{
  return SelectProviderForPackage(NS_LITERAL_CSTRING("skin"), aSkin, aPackageName, mSelectedSkin, aUseProfile, PR_FALSE);
}

NS_IMETHODIMP nsChromeRegistry::DeselectLocaleForPackage(const nsACString& aLocale,
                                                    const PRUnichar *aPackageName,
                                                    PRBool aUseProfile)
{
  return SelectProviderForPackage(NS_LITERAL_CSTRING("locale"), aLocale, aPackageName, mSelectedLocale, aUseProfile, PR_FALSE);
}

NS_IMETHODIMP nsChromeRegistry::IsSkinSelectedForPackage(const nsACString& aSkin,
                                                  const PRUnichar *aPackageName,
                                                  PRBool aUseProfile, PRBool* aResult)
{
  return IsProviderSelectedForPackage(NS_LITERAL_CSTRING("skin"), aSkin, aPackageName, mSelectedSkin, aUseProfile, aResult);
}

NS_IMETHODIMP nsChromeRegistry::IsLocaleSelectedForPackage(const nsACString& aLocale,
                                                    const PRUnichar *aPackageName,
                                                    PRBool aUseProfile, PRBool* aResult)
{
  return IsProviderSelectedForPackage(NS_LITERAL_CSTRING("locale"), aLocale, aPackageName, mSelectedLocale, aUseProfile, aResult);
}

nsresult
nsChromeRegistry::SelectProviderForPackage(const nsACString& aProviderType,
                                           const nsACString& aProviderName,
                                           const PRUnichar *aPackageName,
                                           nsIRDFResource* aSelectionArc,
                                           PRBool aUseProfile, PRBool aIsAdding)
{
  nsCAutoString package( "urn:mozilla:package:" );
  AppendUTF16toUTF8(aPackageName, package);

  nsCAutoString provider( "urn:mozilla:" );
  provider += aProviderType;
  provider += ":";
  provider += aProviderName;
  provider += ":";
  AppendUTF16toUTF8(aPackageName, provider);

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> packageResource;
  rv = GetResource(package, getter_AddRefs(packageResource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }
  NS_ASSERTION(packageResource, "failed to get packageResource");

  
  nsCOMPtr<nsIRDFResource> providerResource;
  rv = GetResource(provider, getter_AddRefs(providerResource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the provider resource.");
    return rv;
  }
  NS_ASSERTION(providerResource, "failed to get providerResource");

  
  
  PRBool acceptable;
  rv = VerifyCompatibleProvider(packageResource, providerResource,
                                aSelectionArc, &acceptable);
  if (NS_FAILED(rv))
    return rv;
  if (!acceptable)
    return NS_ERROR_FAILURE;

  rv = SetProviderForPackage(aProviderType, packageResource, providerResource, aSelectionArc,
                             aUseProfile, nsnull, aIsAdding);
  
  mRuntimeProvider = PR_FALSE;

  return rv;
}

NS_IMETHODIMP nsChromeRegistry::IsSkinSelected(const nsACString& aSkin,
                                               PRBool aUseProfile, PRInt32* aResult)
{
  return IsProviderSelected(NS_LITERAL_CSTRING("skin"), aSkin, mSelectedSkin, aUseProfile, aResult);
}

NS_IMETHODIMP nsChromeRegistry::IsLocaleSelected(const nsACString& aLocale,
                                                 PRBool aUseProfile, PRInt32* aResult)
{
  return IsProviderSelected(NS_LITERAL_CSTRING("locale"), aLocale, mSelectedLocale, aUseProfile, aResult);
}

nsresult
nsChromeRegistry::IsProviderSelected(const nsACString& aProvider,
                                     const nsACString& aProviderName,
                                     nsIRDFResource* aSelectionArc,
                                     PRBool aUseProfile, PRInt32* aResult)
{
  
  
  *aResult = NONE;
  nsCAutoString resourceStr( "urn:mozilla:" );
  resourceStr += aProvider;
  resourceStr += ":";
  resourceStr += aProviderName;
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(resourceStr, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }
  NS_ASSERTION(resource, "failed to GetResource");

  
  nsCOMPtr<nsIRDFNode> packageList;
  rv = mChromeDataSource->GetTarget(resource, mPackages, PR_TRUE, getter_AddRefs(packageList));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the SEQ for the package list.");
    return rv;
  }
  

  nsCOMPtr<nsIRDFResource> packageSeq(do_QueryInterface(packageList, &rv));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIRDFContainer> container(do_CreateInstance("@mozilla.org/rdf/container;1"));
  if (NS_FAILED(container->Init(mChromeDataSource, packageSeq)))
    return NS_OK;

  nsCOMPtr<nsISimpleEnumerator> arcs;
  container->GetElements(getter_AddRefs(arcs));

  
  
  PRBool more;
  PRInt32 numSet = 0;
  PRInt32 numPackages = 0;
  rv = arcs->HasMoreElements(&more);
  if (NS_FAILED(rv)) return rv;
  while (more) {
    nsCOMPtr<nsISupports> packageSkinEntry;
    rv = arcs->GetNext(getter_AddRefs(packageSkinEntry));
    if (NS_SUCCEEDED(rv) && packageSkinEntry) {
      nsCOMPtr<nsIRDFResource> entry = do_QueryInterface(packageSkinEntry);
      if (entry) {
         
         nsCOMPtr<nsIRDFNode> packageNode;
         rv = mChromeDataSource->GetTarget(entry, mPackage, PR_TRUE, getter_AddRefs(packageNode));
         if (NS_FAILED(rv)) {
           NS_ERROR("Unable to obtain the package resource.");
           return rv;
         }

         
         nsCOMPtr<nsIRDFResource> packageResource(do_QueryInterface(packageNode));
         if (packageResource) {
           PRBool isSet = PR_FALSE;
           rv = IsProviderSetForPackage(aProvider, packageResource, entry, aSelectionArc, aUseProfile, &isSet);
           if (NS_FAILED(rv)) {
             NS_ERROR("Unable to set provider for package resource.");
             return rv;
           }
           ++numPackages;
           if (isSet)
             ++numSet;
         }
      }
    }
    rv = arcs->HasMoreElements(&more);
    if (NS_FAILED(rv)) return rv;
  }
  if (numPackages == numSet)
    *aResult = FULL;
  else if (numSet)
    *aResult = PARTIAL;
  return NS_OK;
}

nsresult
nsChromeRegistry::IsProviderSelectedForPackage(const nsACString& aProviderType,
                                               const nsACString& aProviderName,
                                               const PRUnichar *aPackageName,
                                               nsIRDFResource* aSelectionArc,
                                               PRBool aUseProfile, PRBool* aResult)
{
  *aResult = PR_FALSE;
  nsCAutoString package( "urn:mozilla:package:" );
  AppendUTF16toUTF8(aPackageName, package);

  nsCAutoString provider( "urn:mozilla:" );
  provider += aProviderType;
  provider += ":";
  provider += aProviderName;
  provider += ":";
  AppendUTF16toUTF8(aPackageName, provider);

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> packageResource;
  rv = GetResource(package, getter_AddRefs(packageResource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }
  NS_ASSERTION(packageResource, "failed to get packageResource");

  
  nsCOMPtr<nsIRDFResource> providerResource;
  rv = GetResource(provider, getter_AddRefs(providerResource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the provider resource.");
    return rv;
  }
  NS_ASSERTION(providerResource, "failed to get providerResource");

  return IsProviderSetForPackage(aProviderType, packageResource, providerResource, aSelectionArc,
                                 aUseProfile, aResult);
}

nsresult
nsChromeRegistry::IsProviderSetForPackage(const nsACString& aProvider,
                                          nsIRDFResource* aPackageResource,
                                          nsIRDFResource* aProviderPackageResource,
                                          nsIRDFResource* aSelectionArc,
                                          PRBool aUseProfile, PRBool* aResult)
{
  nsresult rv;
  
  
  
  nsCOMPtr<nsIRDFDataSource> dataSource;
  rv = LoadDataSource(kChromeFileName, getter_AddRefs(dataSource), aUseProfile, nsnull);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIRDFNode> retVal;
  dataSource->GetTarget(aPackageResource, aSelectionArc, PR_TRUE, getter_AddRefs(retVal));
  if (retVal) {
    nsCOMPtr<nsIRDFNode> node(do_QueryInterface(aProviderPackageResource));
    if (node == retVal)
      *aResult = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsChromeRegistry::InstallProvider(const nsACString& aProviderType,
                                  const nsACString& aBaseURL,
                                  PRBool aUseProfile, PRBool aAllowScripts,
                                  PRBool aRemove)
{
  
#ifdef DEBUG
  printf("*** Chrome Registration of %-7s: Checking for contents.rdf at %s\n", PromiseFlatCString(aProviderType).get(), PromiseFlatCString(aBaseURL).get());
#endif

  
  nsresult rv;
  nsCOMPtr<nsIRDFDataSource> dataSource =
      do_CreateInstance(kRDFXMLDataSourceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(dataSource, &rv);
  if (NS_FAILED(rv)) return rv;

  
  nsCAutoString key(aBaseURL);
  key += "contents.rdf";
  remote->Init(key.get());
  remote->Refresh(PR_TRUE);

  PRBool skinCount = GetProviderCount(NS_LITERAL_CSTRING("skin"), dataSource);
  PRBool localeCount = GetProviderCount(NS_LITERAL_CSTRING("locale"), dataSource);
  PRBool packageCount = GetProviderCount(NS_LITERAL_CSTRING("package"), dataSource);

  PRBool appendPackage = PR_FALSE;
  PRBool appendProvider = PR_FALSE;
  PRBool appendProviderName = PR_FALSE;

  if (skinCount == 0 && localeCount == 0 && packageCount == 0) {
    
    key = aBaseURL;
    key += "manifest.rdf";
    (void)remote->Init(key.get());      
    rv = remote->Refresh(PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    appendPackage = PR_TRUE;
    appendProvider = PR_TRUE;
    NS_WARNING("Trying old-style manifest.rdf. Please update to contents.rdf.");
  }
  else {
    if ((skinCount > 1 && aProviderType.Equals("skin")) ||
        (localeCount > 1 && aProviderType.Equals("locale")))
      appendProviderName = PR_TRUE;

    if (!appendProviderName && packageCount > 1) {
      appendPackage = PR_TRUE;
    }

    if (aProviderType.Equals("skin")) {
      if (!appendProviderName && (localeCount == 1 || packageCount != 0))
        appendProvider = PR_TRUE;
    }
    else if (aProviderType.Equals("locale")) {
      if (!appendProviderName && (skinCount == 1 || packageCount != 0))
        appendProvider = PR_TRUE;
    }
    else {
      
      if (localeCount == 1 || skinCount == 1)
        appendProvider = PR_TRUE;
    }
  }

  
  nsCOMPtr<nsIRDFDataSource> installSource;
  rv = LoadDataSource(kChromeFileName, getter_AddRefs(installSource), aUseProfile, nsnull);
  if (NS_FAILED(rv)) return rv;
  NS_ASSERTION(installSource, "failed to get installSource");

  
  if (aProviderType.Equals("package"))
    rv = UpdateDynamicDataSources(dataSource, PR_TRUE, aUseProfile, aRemove);
  else if (aProviderType.Equals("skin"))
    rv = UpdateDynamicDataSources(dataSource, PR_FALSE, aUseProfile, aRemove);
  if (NS_FAILED(rv)) return rv;

  
  nsAutoString locstr;
  if (aUseProfile)
    locstr.AssignLiteral("profile");
  else locstr.AssignLiteral("install");
  nsCOMPtr<nsIRDFLiteral> locLiteral;
  rv = mRDFService->GetLiteral(locstr.get(), getter_AddRefs(locLiteral));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIRDFLiteral> scriptLiteral;
  rv = mRDFService->GetLiteral(NS_LITERAL_STRING("false").get(),
                               getter_AddRefs(scriptLiteral));
  if (NS_FAILED(rv)) return rv;

  
  
  nsCAutoString prefix( "urn:mozilla:" );
  prefix += aProviderType;
  prefix += ":";

  
  nsCOMPtr<nsISimpleEnumerator> resources;
  rv = dataSource->GetAllResources(getter_AddRefs(resources));
  if (NS_FAILED(rv)) return rv;

  
  PRBool moreElements;
  rv = resources->HasMoreElements(&moreElements);
  if (NS_FAILED(rv)) return rv;

  while (moreElements) {
    nsCOMPtr<nsISupports> supports;
    rv = resources->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(supports);

    
    const char* value;
    rv = resource->GetValueConst(&value);
    if (NS_FAILED(rv)) return rv;
    nsCAutoString val(value);
    if (val.Find(prefix) == 0) {
      

      if (aProviderType.Equals("package") && !val.Equals("urn:mozilla:package:root")) {
        
        
        nsCAutoString baseURL(aBaseURL);

        
        const char* val2;
        rv = resource->GetValueConst(&val2);
        if (NS_FAILED(rv)) return rv;
        nsCAutoString value2(val2);
        PRInt32 index = value2.RFind(":");
        nsCAutoString packageName;
        value2.Right(packageName, value2.Length() - index - 1);

        if (appendPackage) {
          baseURL += packageName;
          baseURL += "/";
        }
        if (appendProvider) {
          baseURL += "content/";
        }

        nsCOMPtr<nsIRDFLiteral> baseLiteral;
        mRDFService->GetLiteral(NS_ConvertASCIItoUTF16(baseURL).get(), getter_AddRefs(baseLiteral));

        rv = nsChromeRegistry::UpdateArc(installSource, resource, mBaseURL, baseLiteral, aRemove);
        if (NS_FAILED(rv)) return rv;
        rv = nsChromeRegistry::UpdateArc(installSource, resource, mLocType, locLiteral, aRemove);
        if (NS_FAILED(rv)) return rv;
      }

      nsCOMPtr<nsIRDFContainer> container =
          do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
      if (NS_FAILED(rv)) return rv;
      rv = container->Init(dataSource, resource);
      if (NS_SUCCEEDED(rv)) {
        
        
        
        nsCOMPtr<nsIRDFContainer> installContainer;
        rv = mRDFContainerUtils->MakeSeq(installSource, resource, getter_AddRefs(installContainer));
        if (NS_FAILED(rv)) return rv;
        if (!installContainer) {
          
          installContainer = do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
          if (NS_FAILED(rv)) return rv;
          rv = installContainer->Init(installSource, resource);
          if (NS_FAILED(rv)) return rv;
        }

        
        nsCOMPtr<nsISimpleEnumerator> seqKids;
        rv = container->GetElements(getter_AddRefs(seqKids));
        if (NS_FAILED(rv)) return rv;
        PRBool moreKids;
        rv = seqKids->HasMoreElements(&moreKids);
        if (NS_FAILED(rv)) return rv;
        while (moreKids) {
          nsCOMPtr<nsISupports> supp;
          rv = seqKids->GetNext(getter_AddRefs(supp));
          if (NS_FAILED(rv)) return rv;
          nsCOMPtr<nsIRDFNode> kid = do_QueryInterface(supp);
          if (aRemove) {
            rv = installContainer->RemoveElement(kid, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
          }
          else {
            PRInt32 index;
            rv = installContainer->IndexOf(kid, &index);
            if (NS_FAILED(rv)) return rv;
            if (index == -1) {
              rv = installContainer->AppendElement(kid);
              if (NS_FAILED(rv)) return rv;
            }
            rv = seqKids->HasMoreElements(&moreKids);
            if (NS_FAILED(rv)) return rv;
          }
        }

        
        
        if (val.Find(":packages") != -1 && !aProviderType.EqualsLiteral("package")) {
          PRBool doAppendPackage = appendPackage;
          PRInt32 perProviderPackageCount;
          container->GetCount(&perProviderPackageCount);
          if (perProviderPackageCount > 1)
            doAppendPackage = PR_TRUE;

          
          nsCOMPtr<nsISimpleEnumerator> seqKids2;
          rv = container->GetElements(getter_AddRefs(seqKids2));
          if (NS_FAILED(rv)) return rv;
          PRBool moreKids2;
          rv = seqKids2->HasMoreElements(&moreKids2);
          if (NS_FAILED(rv)) return rv;
          while (moreKids2) {
            nsCOMPtr<nsISupports> supp;
            rv = seqKids2->GetNext(getter_AddRefs(supp));
            if (NS_FAILED(rv)) return rv;
            nsCOMPtr<nsIRDFResource> entry(do_QueryInterface(supp));
            if (entry) {
              
              nsCAutoString baseURL(aBaseURL);

              
              const char* val2;
              rv = entry->GetValueConst(&val2);
              if (NS_FAILED(rv)) return rv;
              nsCAutoString value2(val2);
              PRInt32 index = value2.RFind(":");
              nsCAutoString packageName;
              value2.Right(packageName, value2.Length() - index - 1);
              nsCAutoString remainder;
              value2.Left(remainder, index);

              nsCAutoString providerName;
              index = remainder.RFind(":");
              remainder.Right(providerName, remainder.Length() - index - 1);

              
              if (appendProviderName) {
                baseURL += providerName;
                baseURL += "/";
              }
              if (doAppendPackage) {
                baseURL += packageName;
                baseURL += "/";
              }
              if (appendProvider) {
                baseURL += aProviderType;
                baseURL += "/";
              }

              nsCOMPtr<nsIRDFLiteral> baseLiteral;
              mRDFService->GetLiteral(NS_ConvertASCIItoUTF16(baseURL).get(), getter_AddRefs(baseLiteral));

              rv = nsChromeRegistry::UpdateArc(installSource, entry, mBaseURL, baseLiteral, aRemove);
              if (NS_FAILED(rv)) return rv;
              if (aProviderType.EqualsLiteral("skin") && !aAllowScripts) {
                rv = nsChromeRegistry::UpdateArc(installSource, entry, mAllowScripts, scriptLiteral, aRemove);
                if (NS_FAILED(rv)) return rv;
              }

              
              if (index != -1) {
                

                nsCAutoString resourceName("urn:mozilla:package:");
                resourceName += packageName;
                nsCOMPtr<nsIRDFResource> packageResource;
                rv = GetResource(resourceName, getter_AddRefs(packageResource));
                if (NS_FAILED(rv)) return rv;
                if (packageResource) {
                  rv = nsChromeRegistry::UpdateArc(installSource, entry, mPackage, packageResource, aRemove);
                  if (NS_FAILED(rv)) return rv;
                }
              }
            }

            rv = seqKids2->HasMoreElements(&moreKids2);
            if (NS_FAILED(rv)) return rv;
          }
        }
      }
      else {
        
        nsCOMPtr<nsISimpleEnumerator> arcs;
        rv = dataSource->ArcLabelsOut(resource, getter_AddRefs(arcs));
        if (NS_FAILED(rv)) return rv;

        PRBool moreArcs;
        rv = arcs->HasMoreElements(&moreArcs);
        if (NS_FAILED(rv)) return rv;
        while (moreArcs) {
          nsCOMPtr<nsISupports> supp;
          rv = arcs->GetNext(getter_AddRefs(supp));
          if (NS_FAILED(rv)) return rv;
          nsCOMPtr<nsIRDFResource> arc = do_QueryInterface(supp);

          if (arc == mPackages) {
            
            
            rv = nsChromeRegistry::UpdateArc(installSource, resource, mLocType, locLiteral, aRemove);
            if (NS_FAILED(rv)) return rv;
          }

          nsCOMPtr<nsISimpleEnumerator> targets;
          rv = installSource->GetTargets(resource, arc, PR_TRUE, getter_AddRefs(targets));
          if (NS_FAILED(rv)) return rv;

          PRBool moreTargets;
          rv = targets->HasMoreElements(&moreTargets);
          if (NS_FAILED(rv)) return rv;

          while (moreTargets) {
            targets->GetNext(getter_AddRefs(supp));
            nsCOMPtr<nsIRDFNode> node(do_QueryInterface(supp));
            installSource->Unassert(resource, arc, node);

            rv = targets->HasMoreElements(&moreTargets);
            if (NS_FAILED(rv)) return rv;
          }

          if (!aRemove) {
            rv = dataSource->GetTargets(resource, arc, PR_TRUE, getter_AddRefs(targets));
            if (NS_FAILED(rv)) return rv;

            rv = targets->HasMoreElements(&moreTargets);
            if (NS_FAILED(rv)) return rv;

            while (moreTargets) {
              nsresult rv = targets->GetNext(getter_AddRefs(supp));
              if (NS_FAILED(rv)) return rv;
              nsCOMPtr<nsIRDFNode> newTarget(do_QueryInterface(supp));

              if (arc == mImage) {
                
                nsCOMPtr<nsIRDFLiteral> literal(do_QueryInterface(newTarget));
                if (literal) {
                  const PRUnichar* valueStr;
                  literal->GetValueConst(&valueStr);
                  nsAutoString imageURL(valueStr);
                  if (imageURL.FindChar(':') == -1) {
                    
                    
                    NS_ConvertUTF8toUTF16 fullURL(aBaseURL);
                    fullURL += imageURL;
                    mRDFService->GetLiteral(fullURL.get(), getter_AddRefs(literal));
                    newTarget = do_QueryInterface(literal);
                  }
                }
              }

              rv = installSource->Assert(resource, arc, newTarget, PR_TRUE);
              if (NS_FAILED(rv)) return rv;

              rv = targets->HasMoreElements(&moreTargets);
              if (NS_FAILED(rv)) return rv;
            }
          }

          rv = arcs->HasMoreElements(&moreArcs);
          if (NS_FAILED(rv)) return rv;
        }
      }
    }
    rv = resources->HasMoreElements(&moreElements);
    if (NS_FAILED(rv)) return rv;
  }

  
  nsCOMPtr<nsIRDFRemoteDataSource> remoteInstall = do_QueryInterface(installSource, &rv);
  if (NS_FAILED(rv))
    return NS_OK;

  if (!mBatchInstallFlushes) {
    rv = remoteInstall->Flush();
    if (NS_SUCCEEDED(rv) && aProviderType.Equals("package"))
      rv = FlagXPCNativeWrappers();
  }

  

  return rv;
}

NS_IMETHODIMP nsChromeRegistry::SetAllowOverlaysForPackage(const PRUnichar *aPackageName, PRBool allowOverlays)
{
  nsCAutoString package("urn:mozilla:package:");
  AppendUTF16toUTF8(aPackageName, package);

  
  nsCOMPtr<nsIRDFResource> packageResource;
  nsresult rv = GetResource(package, getter_AddRefs(packageResource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }
  NS_ASSERTION(packageResource, "failed to get packageResource");

  nsCOMPtr<nsIRDFDataSource> dataSource;
  rv = LoadDataSource(kChromeFileName, getter_AddRefs(dataSource), PR_TRUE, nsnull);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIRDFLiteral> trueLiteral;
  mRDFService->GetLiteral(NS_LITERAL_STRING("true").get(), getter_AddRefs(trueLiteral));
  nsChromeRegistry::UpdateArc(dataSource, packageResource, 
                              mDisabled, 
                              trueLiteral, allowOverlays);

  nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(dataSource, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = remote->Flush();
  return rv;
}

PRBool nsChromeRegistry::IsOverlayAllowed(nsIURI *aChromeURL)
{
  nsCAutoString package, provider, file;
  nsresult rv = SplitURL(aChromeURL, package, provider, file);
  if (NS_FAILED(rv)) return PR_FALSE;

  
  nsCAutoString rdfpackage( "urn:mozilla:package:" );
  rdfpackage.Append(package);

  
  nsCOMPtr<nsIRDFResource> packageResource;
  rv = GetResource(rdfpackage, getter_AddRefs(packageResource));
  if (NS_FAILED(rv) || !packageResource) {
    NS_ERROR("Unable to obtain the package resource.");
    return PR_FALSE;
  }

  
  nsCOMPtr<nsIRDFNode> disabledNode;
  mChromeDataSource->GetTarget(packageResource, mDisabled, PR_TRUE,
                               getter_AddRefs(disabledNode));
  return !disabledNode;
}

NS_IMETHODIMP nsChromeRegistry::InstallSkin(const char* aBaseURL, PRBool aUseProfile, PRBool aAllowScripts)
{
  return InstallProvider(NS_LITERAL_CSTRING("skin"),
                         nsDependentCString(aBaseURL),
                         aUseProfile, aAllowScripts, PR_FALSE);
}

NS_IMETHODIMP nsChromeRegistry::InstallLocale(const char* aBaseURL, PRBool aUseProfile)
{
  return InstallProvider(NS_LITERAL_CSTRING("locale"),
                         nsDependentCString(aBaseURL),
                         aUseProfile, PR_TRUE, PR_FALSE);
}

NS_IMETHODIMP nsChromeRegistry::InstallPackage(const char* aBaseURL, PRBool aUseProfile)
{
  return InstallProvider(NS_LITERAL_CSTRING("package"),
                         nsDependentCString(aBaseURL),
                         aUseProfile, PR_TRUE, PR_FALSE);
}

NS_IMETHODIMP nsChromeRegistry::UninstallSkin(const nsACString& aSkinName, PRBool aUseProfile)
{
  
  DeselectSkin(aSkinName, aUseProfile);

  
  return UninstallProvider(NS_LITERAL_CSTRING("skin"), aSkinName, aUseProfile);
}

NS_IMETHODIMP nsChromeRegistry::UninstallLocale(const nsACString& aLocaleName, PRBool aUseProfile)
{
  
  DeselectLocale(aLocaleName, aUseProfile);

  return UninstallProvider(NS_LITERAL_CSTRING("locale"), aLocaleName, aUseProfile);
}

NS_IMETHODIMP nsChromeRegistry::UninstallPackage(const nsACString& aPackageName, PRBool aUseProfile)
{
  NS_ERROR("XXX Write me!\n");
  return NS_ERROR_FAILURE;
}

nsresult
nsChromeRegistry::UninstallProvider(const nsACString& aProviderType,
                                    const nsACString& aProviderName,
                                    PRBool aUseProfile)
{
  
  
  
  
  nsresult rv = NS_OK;
  nsCAutoString prefix( "urn:mozilla:" );
  prefix += aProviderType;
  prefix += ":";

  
  nsCAutoString providerRoot(prefix);
  providerRoot += "root";

  
  nsCAutoString specificChild(prefix);
  specificChild += aProviderName;

  
  nsCOMPtr<nsIRDFDataSource> installSource;
  rv = LoadDataSource(kChromeFileName, getter_AddRefs(installSource), aUseProfile, nsnull);
  if (NS_FAILED(rv)) return rv;
  NS_ASSERTION(installSource, "failed to get installSource");

  
  nsCOMPtr<nsIRDFContainer> container(do_CreateInstance("@mozilla.org/rdf/container;1"));

  
  nsCOMPtr<nsIRDFResource> chromeResource;
  if (NS_FAILED(rv = GetResource(providerRoot, getter_AddRefs(chromeResource)))) {
    NS_ERROR("Unable to retrieve the resource corresponding to the skin/locale root.");
    return rv;
  }

  if (NS_FAILED(container->Init(installSource, chromeResource)))
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIRDFResource> childResource;
  if (NS_FAILED(rv = GetResource(specificChild, getter_AddRefs(childResource)))) {
    NS_ERROR("Unable to retrieve the resource corresponding to the skin/locale child being removed.");
    return rv;
  }

  
  container->RemoveElement(childResource, PR_TRUE);

  
  nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(installSource);
  remote->Flush();

  return NS_OK;
}

nsresult
nsChromeRegistry::GetProfileRoot(nsACString& aFileURL)
{
   nsresult rv;
   nsCOMPtr<nsIFile> userChromeDir;

   
   
   rv = NS_GetSpecialDirectory(NS_APP_USER_CHROME_DIR, getter_AddRefs(userChromeDir));
   if (NS_FAILED(rv) || !userChromeDir)
     return NS_ERROR_FAILURE;

   PRBool exists;
   rv = userChromeDir->Exists(&exists);
   if (NS_SUCCEEDED(rv) && !exists) {
     rv = userChromeDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
     if (NS_SUCCEEDED(rv)) {
       
       

       
       nsCOMPtr<nsIFile> defaultUserContentFile;
       nsCOMPtr<nsIFile> defaultUserChromeFile;
       rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_50_DIR,
                                   getter_AddRefs(defaultUserContentFile));
       if (NS_FAILED(rv))
         rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR,
                                     getter_AddRefs(defaultUserContentFile));
       if (NS_FAILED(rv))
         return(rv);
       rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_50_DIR,
                                   getter_AddRefs(defaultUserChromeFile));
       if (NS_FAILED(rv))
         rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR,
                                     getter_AddRefs(defaultUserChromeFile));
       if (NS_FAILED(rv))
         return(rv);
       defaultUserContentFile->AppendNative(NS_LITERAL_CSTRING("chrome"));
       defaultUserContentFile->AppendNative(NS_LITERAL_CSTRING("userContent-example.css"));
       defaultUserChromeFile->AppendNative(NS_LITERAL_CSTRING("chrome"));
       defaultUserChromeFile->AppendNative(NS_LITERAL_CSTRING("userChrome-example.css"));

       const nsAFlatCString& empty = EmptyCString();

       
       
       defaultUserContentFile->CopyToNative(userChromeDir, empty);
       defaultUserChromeFile->CopyToNative(userChromeDir, empty);
     }
   }
   if (NS_FAILED(rv))
     return rv;

   return NS_GetURLSpecFromFile(userChromeDir, aFileURL);
}

nsresult
nsChromeRegistry::GetInstallRoot(nsIFile** aFileURL)
{
  return NS_GetSpecialDirectory(NS_APP_CHROME_DIR, aFileURL);
}

void
nsChromeRegistry::FlushAllCaches()
{
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ASSERTION(obsSvc, "Couldn't get observer service.");

  obsSvc->NotifyObservers((nsIChromeRegistry*) this,
                          NS_CHROME_FLUSH_TOPIC, nsnull);

}  


NS_IMETHODIMP
nsChromeRegistry::ReloadChrome()
{
  FlushAllCaches();
  
  nsresult rv = NS_OK;

  
  nsCOMPtr<nsIWindowMediator> windowMediator = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsISimpleEnumerator> windowEnumerator;

    rv = windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
    if (NS_SUCCEEDED(rv)) {
      
      PRBool more;
      rv = windowEnumerator->HasMoreElements(&more);
      if (NS_FAILED(rv)) return rv;
      while (more) {
        nsCOMPtr<nsISupports> protoWindow;
        rv = windowEnumerator->GetNext(getter_AddRefs(protoWindow));
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIDOMWindowInternal> domWindow =
            do_QueryInterface(protoWindow);
          if (domWindow) {
            nsCOMPtr<nsIDOMLocation> location;
            domWindow->GetLocation(getter_AddRefs(location));
            if (location) {
              rv = location->Reload(PR_FALSE);
              if (NS_FAILED(rv)) return rv;
            }
          }
        }
        rv = windowEnumerator->HasMoreElements(&more);
        if (NS_FAILED(rv)) return rv;
      }
    }
  }
  return rv;
}

nsresult
nsChromeRegistry::GetArcs(nsIRDFDataSource* aDataSource,
                          const nsACString& aType,
                          nsISimpleEnumerator** aResult)
{
  nsresult rv;

  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv))
    return NS_OK;

  nsCAutoString lookup("chrome:");
  lookup += aType;

  
  nsCOMPtr<nsIRDFResource> chromeResource;
  if (NS_FAILED(rv = GetResource(lookup, getter_AddRefs(chromeResource)))) {
    NS_ERROR("Unable to retrieve the resource corresponding to the chrome skin or content.");
    return rv;
  }

  if (NS_FAILED(container->Init(aDataSource, chromeResource)))
    return NS_OK;

  nsCOMPtr<nsISimpleEnumerator> arcs;
  if (NS_FAILED(container->GetElements(getter_AddRefs(arcs))))
    return NS_OK;

  *aResult = arcs;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsChromeRegistry::AddToCompositeDataSource(PRBool aUseProfile)
{
  nsresult rv = NS_OK;
  if (!mChromeDataSource) {
    mChromeDataSource = do_CreateInstance(
        "@mozilla.org/rdf/datasource;1?name=composite-datasource", &rv);
    if (NS_FAILED(rv))
      return rv;

    
    rv = NS_NewChromeUIDataSource(mChromeDataSource, getter_AddRefs(mUIDataSource));
    if (NS_FAILED(rv)) return rv;
  }

  if (aUseProfile) {
    
    nsCOMPtr<nsIRDFDataSource> dataSource;
    LoadDataSource(kChromeFileName, getter_AddRefs(dataSource), PR_TRUE, nsnull);
    mChromeDataSource->AddDataSource(dataSource);
  }

  
  LoadDataSource(kChromeFileName, getter_AddRefs(mInstallDirChromeDataSource), PR_FALSE, nsnull);
  mChromeDataSource->AddDataSource(mInstallDirChromeDataSource);

  return rv;
}

nsresult
nsChromeRegistry::FlagXPCNativeWrappers()
{
  nsresult rv;
  
  nsCOMPtr<nsIXPConnect> xpc(do_GetService("@mozilla.org/js/xpc/XPConnect;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> arcs;
  nsCOMPtr<nsIRDFLiteral> trueLiteral;
  mRDFService->GetLiteral(NS_LITERAL_STRING("true").get(), getter_AddRefs(trueLiteral));
  rv = mChromeDataSource->GetSources(mXPCNativeWrappers, trueLiteral, PR_TRUE,
                                     getter_AddRefs(arcs));
  if (NS_FAILED(rv)) return rv;

  nsCAutoString uri;
  PRBool more;
  rv = arcs->HasMoreElements(&more);
  if (NS_FAILED(rv)) return rv;
  while (more) {
    nsCOMPtr<nsISupports> supp;
    rv = arcs->GetNext(getter_AddRefs(supp));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIRDFResource> package(do_QueryInterface(supp));
    if (package) {
      const char urn[] = "urn:mozilla:package:";
      const char* source;
      package->GetValueConst(&source);
      if (!memcmp(source, urn, sizeof urn - 1)) {
        uri.AssignLiteral("chrome://");
        uri.Append(source + sizeof urn - 1);
        uri.Append('/');
        rv = xpc->FlagSystemFilenamePrefix(uri.get(), PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    rv = arcs->HasMoreElements(&more);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}

nsresult nsChromeRegistry::LoadStyleSheetWithURL(nsIURI* aURL, PRBool aEnableUnsafeRules, nsICSSStyleSheet** aSheet)
{
  *aSheet = nsnull;

  if (!mCSSLoader) {
    nsresult rv;
    mCSSLoader = do_CreateInstance(kCSSLoaderCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return mCSSLoader->LoadSheetSync(aURL, aEnableUnsafeRules, aSheet);
}

nsresult nsChromeRegistry::LoadInstallDataSource()
{
  nsCOMPtr<nsIFile> installRootFile;
  
  nsresult rv = GetInstallRoot(getter_AddRefs(installRootFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = NS_GetURLSpecFromFile(installRootFile, mInstallRoot);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mInstallInitialized = PR_TRUE;
  return AddToCompositeDataSource(PR_FALSE);
}

nsresult nsChromeRegistry::LoadProfileDataSource()
{
  mLegacyOverlayinfo = PR_FALSE;
  nsresult rv = GetProfileRoot(mProfileRoot);
  if (NS_SUCCEEDED(rv)) {
    
    
    mProfileInitialized = mInstallInitialized = PR_TRUE;
    mChromeDataSource = nsnull;
    rv = AddToCompositeDataSource(PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = FlagXPCNativeWrappers();
    if (NS_FAILED(rv)) return rv;

    
    
    
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (prefBranch) {
      nsXPIDLCString skinToSelect;
      rv = prefBranch->GetCharPref("general.skins.selectedSkin", getter_Copies(skinToSelect));
      if (NS_SUCCEEDED(rv)) {
        rv = SelectSkin(skinToSelect, PR_TRUE);
        if (NS_SUCCEEDED(rv))
          prefBranch->DeleteBranch("general.skins.selectedSkin");
      }
    }

    
    FlushSkinCaches();
    
    
    
    nsCOMPtr<nsIFile> overlayinfoDir;
    rv = NS_GetSpecialDirectory(NS_APP_USER_CHROME_DIR, getter_AddRefs(overlayinfoDir));
    if (NS_SUCCEEDED(rv))
    {
      rv = overlayinfoDir->AppendNative(NS_LITERAL_CSTRING("overlayinfo"));
      if (NS_SUCCEEDED(rv))
      {
        PRBool isLegacyOverlayinfo;
        rv = overlayinfoDir->IsDirectory(&isLegacyOverlayinfo);
        mLegacyOverlayinfo = NS_SUCCEEDED(rv) && isLegacyOverlayinfo;
      }
    }

  }
  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::AllowScriptsForPackage(nsIURI* aChromeURI, PRBool *aResult)
{
  *aResult = PR_TRUE;

  
  nsCAutoString package, provider, file;
  nsresult rv;
  rv = SplitURL(aChromeURI, package, provider, file);
  if (NS_FAILED(rv)) return NS_OK;

  
  if (!provider.Equals("skin"))
    return NS_OK;

  
  
  nsCOMPtr<nsIRDFNode> selectedProvider;

  nsCAutoString resourceStr("urn:mozilla:package:");
  resourceStr += package;

  
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(resourceStr, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }

  if (NS_FAILED(rv = mChromeDataSource->GetTarget(resource, mSelectedSkin, PR_TRUE, getter_AddRefs(selectedProvider))))
    return NS_OK;

  if (!selectedProvider) {
    rv = FindProvider(package, provider, mSelectedSkin, getter_AddRefs(selectedProvider));
    if (NS_FAILED(rv)) return rv;
  }
  if (!selectedProvider)
    return NS_OK;

  resource = do_QueryInterface(selectedProvider, &rv);
  if (NS_SUCCEEDED(rv)) {
    
    nsCOMPtr<nsIRDFNode> scriptAccessNode;
    mChromeDataSource->GetTarget(resource, mAllowScripts, PR_TRUE,
                                 getter_AddRefs(scriptAccessNode));
    if (scriptAccessNode)
      *aResult = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsChromeRegistry::CheckForNewChrome()
{
  nsresult rv;

  rv = LoadInstallDataSource();
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsILocalFile> listFile;
  nsCOMPtr<nsIProperties> directoryService =
           do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  rv = directoryService->Get(NS_APP_CHROME_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(listFile));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> chromeFile;
  rv = listFile->Clone(getter_AddRefs(chromeFile));
  if (NS_FAILED(rv)) return rv;
  rv = chromeFile->AppendNative(kChromeFileName);
  if (NS_FAILED(rv)) return rv;
  
  
  
  nsInt64 chromeDate;
  (void)chromeFile->GetLastModifiedTime(&chromeDate.mValue);

  rv = listFile->AppendRelativeNativePath(kInstalledChromeFileName);
  if (NS_FAILED(rv)) return rv;
  nsInt64 listFileDate;
  (void)listFile->GetLastModifiedTime(&listFileDate.mValue);

  if (listFileDate < chromeDate)
    return NS_OK;

  PRFileDesc *file;
  rv = listFile->OpenNSPRFileDesc(PR_RDONLY, 0, &file);
  if (NS_FAILED(rv)) return rv;

  

  PRFileInfo finfo;

  if (PR_GetOpenFileInfo(file, &finfo) == PR_SUCCESS) {
    char *dataBuffer = new char[finfo.size+1];
    if (dataBuffer) {
      PRInt32 bufferSize = PR_Read(file, dataBuffer, finfo.size);
      if (bufferSize > 0) {
        mBatchInstallFlushes = PR_TRUE;
        rv = ProcessNewChromeBuffer(dataBuffer, bufferSize);
        mBatchInstallFlushes = PR_FALSE;
      }
      delete [] dataBuffer;
    }
  }
  PR_Close(file);
  

  return rv;
}


nsresult
nsChromeRegistry::ProcessNewChromeBuffer(char *aBuffer, PRInt32 aLength)
{
  nsresult rv = NS_OK;
  char   *bufferEnd = aBuffer + aLength;
  char   *chromeType,      
         *chromeProfile,   
         *chromeLocType,   
         *chromeLocation;  
  PRBool isProfile;
  PRBool isSelection;

  NS_NAMED_LITERAL_CSTRING(content, "content");
  NS_NAMED_LITERAL_CSTRING(locale, "locale");
  NS_NAMED_LITERAL_CSTRING(skin, "skin");
  NS_NAMED_LITERAL_CSTRING(profile, "profile");
  NS_NAMED_LITERAL_CSTRING(select, "select");
  NS_NAMED_LITERAL_CSTRING(path, "path");
  nsCAutoString fileURL;
  nsCAutoString chromeURL;

  
  while (aBuffer < bufferEnd) {
    
    chromeType = aBuffer;
    while (aBuffer < bufferEnd && *aBuffer != ',')
      ++aBuffer;
    *aBuffer = '\0';

    chromeProfile = ++aBuffer;
    if (aBuffer >= bufferEnd)
      break;

    while (aBuffer < bufferEnd && *aBuffer != ',')
      ++aBuffer;
    *aBuffer = '\0';

    chromeLocType = ++aBuffer;
    if (aBuffer >= bufferEnd)
      break;

    while (aBuffer < bufferEnd && *aBuffer != ',')
      ++aBuffer;
    *aBuffer = '\0';

    chromeLocation = ++aBuffer;
    if (aBuffer >= bufferEnd)
      break;

    while (aBuffer < bufferEnd &&
           (*aBuffer != '\r' && *aBuffer != '\n' && *aBuffer != ' '))
      ++aBuffer;
    *aBuffer = '\0';

    
    isSelection = select.Equals(chromeLocType);
    isProfile = profile.Equals(chromeProfile);
    if (isProfile && !mProfileInitialized) 
    { 
      rv = LoadProfileDataSource();
      if (NS_FAILED(rv)) 
        return rv;
    }

    if (path.Equals(chromeLocType)) {
      

      




      nsCOMPtr<nsILocalFile> chromeFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
      if (NS_FAILED(rv))
        return rv;
      
      rv = chromeFile->InitWithNativePath(nsDependentCString(chromeLocation));
      if (NS_FAILED(rv))
        return rv;

      


      rv = NS_GetURLSpecFromFile(chromeFile, chromeURL);
      if (NS_FAILED(rv)) return rv;

      


      PRBool isFile;
      rv = chromeFile->IsFile(&isFile);
      if (NS_FAILED(rv))
        return rv;

      if (isFile) {
        fileURL = "jar:";
        fileURL += chromeURL;
        fileURL += "!/";
        chromeURL = fileURL;
      }
    }
    else {
      
      chromeURL = chromeLocation;
    }

    
    if (skin.Equals(chromeType)) {
      if (isSelection) {

        rv = SelectSkin(nsDependentCString(chromeLocation), isProfile);
#ifdef DEBUG
        printf("***** Chrome Registration: Selecting skin %s as default\n", (const char*)chromeLocation);
#endif
      }
      else
        rv = InstallSkin(chromeURL.get(), isProfile, PR_FALSE);
    }
    else if (content.Equals(chromeType))
      rv = InstallPackage(chromeURL.get(), isProfile);
    else if (locale.Equals(chromeType)) {
      if (isSelection) {

        rv = SelectLocale(nsDependentCString(chromeLocation), isProfile);
#ifdef DEBUG
        printf("***** Chrome Registration: Selecting locale %s as default\n", (const char*)chromeLocation);
#endif
      }
      else
        rv = InstallLocale(chromeURL.get(), isProfile);
    }
    
    while (aBuffer < bufferEnd && (*aBuffer == '\0' || *aBuffer == ' ' || *aBuffer == '\r' || *aBuffer == '\n'))
      ++aBuffer;
  }

  nsCOMPtr<nsIRDFDataSource> dataSource;
  LoadDataSource(kChromeFileName, getter_AddRefs(dataSource), PR_FALSE, nsnull);
  nsCOMPtr<nsIRDFRemoteDataSource> remote(do_QueryInterface(dataSource));
  remote->Flush();
  return NS_OK;
}

PRBool
nsChromeRegistry::GetProviderCount(const nsACString& aProviderType, nsIRDFDataSource* aDataSource)
{
  nsresult rv;

  nsCAutoString rootStr("urn:mozilla:");
  rootStr += aProviderType;
  rootStr += ":root";

  
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(rootStr, getter_AddRefs(resource));
  if (NS_FAILED(rv))
    return 0;

  
  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv)) return 0;

  rv = container->Init(aDataSource, resource);
  if (NS_FAILED(rv)) return 0;

  PRInt32 count;
  container->GetCount(&count);
  return count;
}


NS_IMETHODIMP nsChromeRegistry::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  nsresult rv = NS_OK;

  if (!strcmp("profile-before-change", aTopic)) {

    mChromeDataSource = nsnull;
    mInstallInitialized = mProfileInitialized = PR_FALSE;

    if (!strcmp("shutdown-cleanse", NS_ConvertUTF16toUTF8(someData).get())) {
      nsCOMPtr<nsIFile> userChromeDir;
      rv = NS_GetSpecialDirectory(NS_APP_USER_CHROME_DIR, getter_AddRefs(userChromeDir));
      if (NS_SUCCEEDED(rv) && userChromeDir)
        rv = userChromeDir->Remove(PR_TRUE);
    }
    FlushAllCaches();
  }
  else if (!strcmp("profile-after-change", aTopic)) {
    rv = LoadProfileDataSource();
  }

  return rv;
}

NS_IMETHODIMP nsChromeRegistry::CheckThemeVersion(const nsACString& aSkin,
                                                  PRBool* aResult)
{
  return CheckProviderVersion(NS_LITERAL_CSTRING("skin"), aSkin, mSkinVersion, aResult);
}


NS_IMETHODIMP nsChromeRegistry::CheckLocaleVersion(const nsACString& aLocale,
                                                   PRBool* aResult)
{
  nsCAutoString provider("locale");
  return CheckProviderVersion(NS_LITERAL_CSTRING("skin"), aLocale, mLocaleVersion, aResult);
}


nsresult
nsChromeRegistry::CheckProviderVersion (const nsACString& aProviderType,
                                        const nsACString& aProviderName,
                                        nsIRDFResource* aSelectionArc,
                                        PRBool *aCompatible)
{
  *aCompatible = PR_TRUE;

  
  
  nsCAutoString resourceStr( "urn:mozilla:" );
  resourceStr += aProviderType;
  resourceStr += ":";
  resourceStr += aProviderName;

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRDFResource> resource;
  rv = GetResource(resourceStr, getter_AddRefs(resource));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the package resource.");
    return rv;
  }

  
  nsCOMPtr<nsIRDFNode> packageList;
  rv = mChromeDataSource->GetTarget(resource, mPackages, PR_TRUE, getter_AddRefs(packageList));
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain the SEQ for the package list.");
    return rv;
  }
  

  nsCOMPtr<nsIRDFResource> packageSeq(do_QueryInterface(packageList, &rv));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIRDFContainer> container =
      do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = container->Init(mChromeDataSource, packageSeq);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> arcs;

  rv = container->GetElements(getter_AddRefs(arcs));
  if (NS_FAILED(rv))
    return NS_OK;

  
  
  PRBool more;
  rv = arcs->HasMoreElements(&more);
  if (NS_FAILED(rv)) return rv;
  while (more) {
    nsCOMPtr<nsISupports> packageSkinEntry;
    rv = arcs->GetNext(getter_AddRefs(packageSkinEntry));
    if (NS_SUCCEEDED(rv) && packageSkinEntry) {
      nsCOMPtr<nsIRDFResource> entry = do_QueryInterface(packageSkinEntry);
      if (entry) {
        
        nsCOMPtr<nsIRDFNode> packageNode;
        rv = mChromeDataSource->GetTarget(entry, mPackage, PR_TRUE, getter_AddRefs(packageNode));
        if (NS_FAILED(rv)) {
          NS_ERROR("Unable to obtain the package resource.");
          return rv;
        }

        nsCOMPtr<nsIRDFResource> packageResource(do_QueryInterface(packageNode));
        if (packageResource) {
          nsCOMPtr<nsIRDFNode> packageNameNode;
          mChromeDataSource->GetTarget(packageResource, mName, PR_TRUE,
                                       getter_AddRefs(packageNameNode));

          if (packageNameNode) {
            nsCOMPtr<nsIRDFNode> packageVersionNode;
            mChromeDataSource->GetTarget(packageResource, aSelectionArc, PR_TRUE,
                                         getter_AddRefs(packageVersionNode));

            if (packageVersionNode) {
              mChromeDataSource->HasAssertion(entry, aSelectionArc,
                                              packageVersionNode, PR_TRUE,
                                              aCompatible);
              
              if (!*aCompatible)
                return NS_OK;
            }
          }
        }
      }
    }
    rv = arcs->HasMoreElements(&more);
    if (NS_FAILED(rv))
      return rv;
  }

  return NS_OK;
}

