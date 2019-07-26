





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif

#include "nsNSSComponent.h"
#include "nsNSSCallbacks.h"
#include "nsNSSIOLayer.h"
#include "nsCertVerificationThread.h"

#include "nsNetUtil.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryService.h"
#include "nsIStreamListener.h"
#include "nsIStringBundle.h"
#include "nsIDirectoryService.h"
#include "nsCURILoader.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsNSSCertificate.h"
#include "nsNSSHelper.h"
#include "prlog.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsThreadUtils.h"

#ifndef MOZ_DISABLE_CRYPTOLEGACY
#include "nsIDOMNode.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDocument.h"
#include "nsIDOMSmartCardEvent.h"
#include "nsSmartCardMonitor.h"
#include "nsIDOMCryptoLegacy.h"
#include "nsIPrincipal.h"
#else
#include "nsIDOMCrypto.h"
#endif

#include "nsCRT.h"
#include "nsCRLInfo.h"
#include "nsCertOverrideService.h"
#include "nsNTLMAuthModule.h"

#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsCertificatePrincipal.h"
#include "nsReadableUtils.h"
#include "nsIDateTimeFormat.h"
#include "prtypes.h"
#include "nsIEntropyCollector.h"
#include "nsIBufEntropyCollector.h"
#include "nsIServiceManager.h"
#include "nsIFile.h"
#include "nsITokenPasswordDialogs.h"
#include "nsICRLManager.h"
#include "nsNSSShutDown.h"
#include "GeneratedEvents.h"
#include "nsIKeyModule.h"
#include "ScopedNSSTypes.h"
#include "SharedSSLState.h"

#include "nss.h"
#include "pk11func.h"
#include "ssl.h"
#include "sslproto.h"
#include "secmod.h"
#include "sechash.h"
#include "secmime.h"
#include "ocsp.h"
#include "cms.h"
#include "nssckbi.h"
#include "base64.h"
#include "secerr.h"
#include "sslerr.h"
#include "cert.h"

#include "nsXULAppAPI.h"
#include <algorithm>

#ifdef XP_WIN
#include "nsILocalFileWin.h"
#endif

#include "pkcs12.h"
#include "p12plcy.h"

using namespace mozilla;
using namespace mozilla::psm;

#ifdef MOZ_LOGGING
PRLogModuleInfo* gPIPNSSLog = nullptr;
#endif

#define NS_CRYPTO_HASH_BUFFER_SIZE 4096

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);
int nsNSSComponent::mInstanceCount = 0;
bool nsNSSComponent::globalConstFlagUsePKIXVerification = false;


extern char* pk11PasswordPrompt(PK11SlotInfo *slot, PRBool retry, void *arg);

#define PIPNSS_STRBUNDLE_URL "chrome://pipnss/locale/pipnss.properties"
#define NSSERR_STRBUNDLE_URL "chrome://pipnss/locale/nsserrors.properties"

static PLHashNumber certHashtable_keyHash(const void *key)
{
  if (!key)
    return 0;
  
  SECItem *certKey = (SECItem*)key;
  
  
  
  PLHashNumber hash = 0;
  unsigned int i = 0;
  unsigned char *c = certKey->data;
  
  for (i = 0; i < certKey->len; ++i, ++c) {
    hash += *c;
  }
  
  return hash;
}

static int certHashtable_keyCompare(const void *k1, const void *k2)
{
  

  if (!k1 || !k2)
    return false;
  
  SECItem *certKey1 = (SECItem*)k1;
  SECItem *certKey2 = (SECItem*)k2;
  
  if (certKey1->len != certKey2->len) {
    return false;
  }
  
  unsigned int i = 0;
  unsigned char *c1 = certKey1->data;
  unsigned char *c2 = certKey2->data;
  
  for (i = 0; i < certKey1->len; ++i, ++c1, ++c2) {
    if (*c1 != *c2) {
      return false;
    }
  }
  
  return true;
}

static int certHashtable_valueCompare(const void *v1, const void *v2)
{
  
  
  if (!v1 || !v2)
    return false;
  
  CERTCertificate *cert1 = (CERTCertificate*)v1;
  CERTCertificate *cert2 = (CERTCertificate*)v2;
  
  return certHashtable_keyCompare(&cert1->certKey, &cert2->certKey);
}

static int certHashtable_clearEntry(PLHashEntry *he, int , void * )
{
  if (he && he->value) {
    CERT_DestroyCertificate((CERTCertificate*)he->value);
  }
  
  return HT_ENUMERATE_NEXT;
}

class CRLDownloadEvent : public nsRunnable {
public:
  CRLDownloadEvent(const nsCSubstring &urlString, nsIStreamListener *listener)
    : mURLString(urlString)
    , mListener(listener)
  {}

  
  
  
  NS_IMETHOD Run()
  {
    if (!mListener || mURLString.IsEmpty())
      return NS_OK;

    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), mURLString);
    if (NS_SUCCEEDED(rv)){
      NS_OpenURI(mListener, nullptr, uri);
    }

    return NS_OK;
  }

private:
  nsCString mURLString;
  nsCOMPtr<nsIStreamListener> mListener;
};

#ifndef MOZ_DISABLE_CRYPTOLEGACY


class nsTokenEventRunnable : public nsIRunnable {
public:
  nsTokenEventRunnable(const nsAString &aType, const nsAString &aTokenName);
  virtual ~nsTokenEventRunnable();

  NS_IMETHOD Run ();
  NS_DECL_ISUPPORTS
private:
  nsString mType;
  nsString mTokenName;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsTokenEventRunnable, nsIRunnable)

nsTokenEventRunnable::nsTokenEventRunnable(const nsAString &aType, 
   const nsAString &aTokenName): mType(aType), mTokenName(aTokenName) { }

nsTokenEventRunnable::~nsTokenEventRunnable() { }



NS_IMETHODIMP
nsTokenEventRunnable::Run()
{ 
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  return nssComponent->DispatchEvent(mType, mTokenName);
}
#endif 

bool nsPSMInitPanic::isPanic = false;



bool EnsureNSSInitialized(EnsureNSSOperator op)
{
  if (nsPSMInitPanic::GetPanic())
    return false;

  if (GeckoProcessType_Default != XRE_GetProcessType())
  {
    if (op == nssEnsureOnChromeOnly)
    {
      
      
      
      
      return true;
    }

    NS_ERROR("Trying to initialize PSM/NSS in a non-chrome process!");
    return false;
  }

  static bool loading = false;
  static int32_t haveLoaded = 0;

  switch (op)
  {
    
    
    
  case nssLoadingComponent:
    if (loading)
      return false; 
    loading = true;
    return true;

  case nssInitSucceeded:
    NS_ASSERTION(loading, "Bad call to EnsureNSSInitialized(nssInitSucceeded)");
    loading = false;
    PR_AtomicSet(&haveLoaded, 1);
    return true;

  case nssInitFailed:
    NS_ASSERTION(loading, "Bad call to EnsureNSSInitialized(nssInitFailed)");
    loading = false;
    

  case nssShutdown:
    PR_AtomicSet(&haveLoaded, 0);
    return false;

    
    
    
  case nssEnsure:
  case nssEnsureOnChromeOnly:
    
    if (PR_AtomicAdd(&haveLoaded, 0) || loading)
      return true;

    {
    nsCOMPtr<nsINSSComponent> nssComponent
      = do_GetService(PSM_COMPONENT_CONTRACTID);

    
    
    if (!nssComponent)
      return false;

    bool isInitialized;
    nsresult rv = nssComponent->IsNSSInitialized(&isInitialized);
    return NS_SUCCEEDED(rv) && isInitialized;
    }

  default:
    NS_ASSERTION(false, "Bad operator to EnsureNSSInitialized");
    return false;
  }
}

nsNSSComponent::nsNSSComponent()
  :mutex("nsNSSComponent.mutex"),
   mNSSInitialized(false),
   mCrlTimerLock("nsNSSComponent.mCrlTimerLock"),
#ifndef MOZ_DISABLE_CRYPTOLEGACY
   mThreadList(nullptr),
#endif
   mCertVerificationThread(nullptr)
{
#ifdef PR_LOGGING
  if (!gPIPNSSLog)
    gPIPNSSLog = PR_NewLogModule("pipnss");
#endif
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent::ctor\n"));
  mUpdateTimerInitialized = false;
  crlDownloadTimerOn = false;
  crlsScheduledForDownload = nullptr;
  mTimer = nullptr;
  mObserversRegistered = false;

  
  
  memset(&mIdentityInfoCallOnce, 0, sizeof(PRCallOnceType));

  NS_ASSERTION( (0 == mInstanceCount), "nsNSSComponent is a singleton, but instantiated multiple times!");
  ++mInstanceCount;
  hashTableCerts = nullptr;
  mShutdownObjectList = nsNSSShutDownList::construct();
  mIsNetworkDown = false;
}

void 
nsNSSComponent::deleteBackgroundThreads()
{
  if (mCertVerificationThread)
  {
    mCertVerificationThread->requestExit();
    delete mCertVerificationThread;
    mCertVerificationThread = nullptr;
  }
}

void
nsNSSComponent::createBackgroundThreads()
{
  NS_ASSERTION(!mCertVerificationThread,
               "Cert verification thread already created.");

  mCertVerificationThread = new nsCertVerificationThread;
  nsresult rv = mCertVerificationThread->startThread(
    NS_LITERAL_CSTRING("Cert Verify"));

  if (NS_FAILED(rv)) {
    delete mCertVerificationThread;
    mCertVerificationThread = nullptr;
  }
}

nsNSSComponent::~nsNSSComponent()
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent::dtor\n"));

  deleteBackgroundThreads();

  if (mUpdateTimerInitialized) {
    {
      MutexAutoLock lock(mCrlTimerLock);
      if (crlDownloadTimerOn) {
        mTimer->Cancel();
      }
      crlDownloadTimerOn = false;
    }
    if (crlsScheduledForDownload) {
      crlsScheduledForDownload->Reset();
      delete crlsScheduledForDownload;
    }

    mUpdateTimerInitialized = false;
  }

  

  ShutdownNSS();
  SharedSSLState::GlobalCleanup();
  RememberCertErrorsTable::Cleanup();
  --mInstanceCount;
  delete mShutdownObjectList;

  
  
  EnsureNSSInitialized(nssShutdown);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent::dtor finished\n"));
}

#ifndef MOZ_DISABLE_CRYPTOLEGACY
NS_IMETHODIMP
nsNSSComponent::PostEvent(const nsAString &eventType, 
                                                  const nsAString &tokenName)
{
  nsCOMPtr<nsIRunnable> runnable = 
                               new nsTokenEventRunnable(eventType, tokenName);

  return NS_DispatchToMainThread(runnable);
}


NS_IMETHODIMP
nsNSSComponent::DispatchEvent(const nsAString &eventType,
                                                 const nsAString &tokenName)
{
  
  
  nsresult rv;
  nsCOMPtr<nsIWindowWatcher> windowWatcher =
                            do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);

  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = windowWatcher->GetWindowEnumerator(getter_AddRefs(enumerator));
  if (NS_FAILED(rv)) {
    return rv;
  }

  bool hasMoreWindows;

  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreWindows))
         && hasMoreWindows) {
    nsCOMPtr<nsISupports> supports;
    enumerator->GetNext(getter_AddRefs(supports));
    nsCOMPtr<nsIDOMWindow> domWin(do_QueryInterface(supports));
    if (domWin) {
      nsresult rv2 = DispatchEventToWindow(domWin, eventType, tokenName);
      if (NS_FAILED(rv2)) {
        
        
        rv = rv2;
      }
    }
  }
  return rv;
}

nsresult
nsNSSComponent::DispatchEventToWindow(nsIDOMWindow *domWin,
                                      const nsAString &eventType,
                                      const nsAString &tokenName)
{
  if (!domWin) {
    return NS_OK;
  }

  
  nsresult rv;
  nsCOMPtr<nsIDOMWindowCollection> frames;
  rv = domWin->GetFrames(getter_AddRefs(frames));
  if (NS_FAILED(rv)) {
    return rv;
  }

  uint32_t length;
  frames->GetLength(&length);
  uint32_t i;
  for (i = 0; i < length; i++) {
    nsCOMPtr<nsIDOMWindow> childWin;
    frames->Item(i, getter_AddRefs(childWin));
    DispatchEventToWindow(childWin, eventType, tokenName);
  }

  
  
  
  nsCOMPtr<nsIDOMCrypto> crypto;
  domWin->GetCrypto(getter_AddRefs(crypto));
  if (!crypto) {
    return NS_OK; 
  }

  bool boolrv;
  crypto->GetEnableSmartCardEvents(&boolrv);
  if (!boolrv) {
    return NS_OK; 
  }

  

  
  nsCOMPtr<nsIDOMDocument> doc;
  rv = domWin->GetDocument(getter_AddRefs(doc));
  if (!doc) {
    return NS_FAILED(rv) ? rv : NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocument> d = do_QueryInterface(doc);

  
  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMSmartCardEvent(getter_AddRefs(event), d, nullptr, nullptr);
  nsCOMPtr<nsIDOMSmartCardEvent> smartCardEvent = do_QueryInterface(event);
  rv = smartCardEvent->InitSmartCardEvent(eventType, false, true, tokenName);
  NS_ENSURE_SUCCESS(rv, rv);
  smartCardEvent->SetTrusted(true);

  
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(doc, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return target->DispatchEvent(smartCardEvent, &boolrv);
}
#endif 

NS_IMETHODIMP
nsNSSComponent::PIPBundleFormatStringFromName(const char *name,
                                              const PRUnichar **params,
                                              uint32_t numParams,
                                              nsAString &outString)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mPIPNSSBundle && name) {
    nsXPIDLString result;
    rv = mPIPNSSBundle->FormatStringFromName(NS_ConvertASCIItoUTF16(name).get(),
                                             params, numParams,
                                             getter_Copies(result));
    if (NS_SUCCEEDED(rv)) {
      outString = result;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsNSSComponent::GetPIPNSSBundleString(const char *name,
                                      nsAString &outString)
{
  nsresult rv = NS_ERROR_FAILURE;

  outString.SetLength(0);
  if (mPIPNSSBundle && name) {
    nsXPIDLString result;
    rv = mPIPNSSBundle->GetStringFromName(NS_ConvertASCIItoUTF16(name).get(),
                                          getter_Copies(result));
    if (NS_SUCCEEDED(rv)) {
      outString = result;
      rv = NS_OK;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsNSSComponent::NSSBundleFormatStringFromName(const char *name,
                                              const PRUnichar **params,
                                              uint32_t numParams,
                                              nsAString &outString)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mNSSErrorsBundle && name) {
    nsXPIDLString result;
    rv = mNSSErrorsBundle->FormatStringFromName(NS_ConvertASCIItoUTF16(name).get(),
                                                params, numParams,
                                                getter_Copies(result));
    if (NS_SUCCEEDED(rv)) {
      outString = result;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsNSSComponent::GetNSSBundleString(const char *name,
                                   nsAString &outString)
{
  nsresult rv = NS_ERROR_FAILURE;

  outString.SetLength(0);
  if (mNSSErrorsBundle && name) {
    nsXPIDLString result;
    rv = mNSSErrorsBundle->GetStringFromName(NS_ConvertASCIItoUTF16(name).get(),
                                             getter_Copies(result));
    if (NS_SUCCEEDED(rv)) {
      outString = result;
      rv = NS_OK;
    }
  }

  return rv;
}

#ifndef MOZ_DISABLE_CRYPTOLEGACY
void
nsNSSComponent::LaunchSmartCardThreads()
{
  nsNSSShutDownPreventionLock locker;
  {
    SECMODModuleList *list;
    SECMODListLock *lock = SECMOD_GetDefaultModuleListLock();
    if (!lock) {
        PR_LOG(gPIPNSSLog, PR_LOG_ERROR,
               ("Couldn't get the module list lock, can't launch smart card threads\n"));
        return;
    }
    SECMOD_GetReadLock(lock);
    list = SECMOD_GetDefaultModuleList();

    while (list) {
      SECMODModule *module = list->module;
      LaunchSmartCardThread(module);
      list = list->next;
    }
    SECMOD_ReleaseReadLock(lock);
  }
}

NS_IMETHODIMP
nsNSSComponent::LaunchSmartCardThread(SECMODModule *module)
{
  SmartCardMonitoringThread *newThread;
  if (SECMOD_HasRemovableSlots(module)) {
    if (!mThreadList) {
      mThreadList = new SmartCardThreadList();
    }
    newThread = new SmartCardMonitoringThread(module);
    
    return mThreadList->Add(newThread);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSComponent::ShutdownSmartCardThread(SECMODModule *module)
{
  if (!mThreadList) {
    return NS_OK;
  }
  mThreadList->Remove(module);
  return NS_OK;
}

void
nsNSSComponent::ShutdownSmartCardThreads()
{
  delete mThreadList;
  mThreadList = nullptr;
}
#endif 

static char *
nss_addEscape(const char *string, char quote)
{
    char *newString = 0;
    int escapes = 0, size = 0;
    const char *src;
    char *dest;

    for (src=string; *src ; src++) {
        if ((*src == quote) || (*src == '\\')) {
          escapes++;
        }
        size++;
    }

    newString = (char*)PORT_ZAlloc(escapes+size+1);
    if (!newString) {
        return nullptr;
    }

    for (src=string, dest=newString; *src; src++,dest++) {
        if ((*src == quote) || (*src == '\\')) {
            *dest++ = '\\';
        }
        *dest = *src;
    }

    return newString;
}

void
nsNSSComponent::InstallLoadableRoots()
{
  nsNSSShutDownPreventionLock locker;
  SECMODModule *RootsModule = nullptr;

  
  
  
  
  
  

  {
    

    SECMODModuleList *list;
    SECMODListLock *lock = SECMOD_GetDefaultModuleListLock();
    if (!lock) {
        PR_LOG(gPIPNSSLog, PR_LOG_ERROR,
               ("Couldn't get the module list lock, can't install loadable roots\n"));
        return;
    }
    SECMOD_GetReadLock(lock);
    list = SECMOD_GetDefaultModuleList();

    while (!RootsModule && list) {
      SECMODModule *module = list->module;

      for (int i=0; i < module->slotCount; i++) {
        PK11SlotInfo *slot = module->slots[i];
        if (PK11_IsPresent(slot)) {
          if (PK11_HasRootCerts(slot)) {
            RootsModule = SECMOD_ReferenceModule(module);
            break;
          }
        }
      }

      list = list->next;
    }
    SECMOD_ReleaseReadLock(lock);
  }

  if (RootsModule) {
    int32_t modType;
    SECMOD_DeleteModule(RootsModule->commonName, &modType);
    SECMOD_DestroyModule(RootsModule);
    RootsModule = nullptr;
  }

  
  
  

  nsresult rv;
  nsAutoString modName;
  rv = GetPIPNSSBundleString("RootCertModuleName", modName);
  if (NS_FAILED(rv)) return;

  nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
  if (!directoryService)
    return;

  static const char nss_lib[] = "nss3";
  const char *possible_ckbi_locations[] = {
    nss_lib, 
             
    NS_XPCOM_CURRENT_PROCESS_DIR,
    NS_GRE_DIR,
    0 
      
      
  };

  for (size_t il = 0; il < sizeof(possible_ckbi_locations)/sizeof(const char*); ++il) {
    nsCOMPtr<nsIFile> mozFile;
    char *fullLibraryPath = nullptr;

    if (!possible_ckbi_locations[il])
    {
      fullLibraryPath = PR_GetLibraryName(nullptr, "nssckbi");
    }
    else
    {
      if (possible_ckbi_locations[il] == nss_lib) {
        
        char *nss_path = PR_GetLibraryFilePathname(DLL_PREFIX "nss3" DLL_SUFFIX,
                                                   (PRFuncPtr) NSS_Initialize);
        if (!nss_path) {
          continue;
        }
        
        nsCOMPtr<nsIFile> nssLib(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) {
          rv = nssLib->InitWithNativePath(nsDependentCString(nss_path));
        }
        PR_Free(nss_path);
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIFile> file;
          if (NS_SUCCEEDED(nssLib->GetParent(getter_AddRefs(file)))) {
            mozFile = do_QueryInterface(file);
          }
        }
      } else {
        directoryService->Get( possible_ckbi_locations[il],
                               NS_GET_IID(nsIFile), 
                               getter_AddRefs(mozFile));
      }
  
      if (!mozFile) {
        continue;
      }

      nsAutoCString processDir;
      mozFile->GetNativePath(processDir);
      fullLibraryPath = PR_GetLibraryName(processDir.get(), "nssckbi");
    }

    if (!fullLibraryPath) {
      continue;
    }

    char *escaped_fullLibraryPath = nss_addEscape(fullLibraryPath, '\"');
    if (!escaped_fullLibraryPath) {
      PR_FreeLibraryName(fullLibraryPath); 
      continue;
    }

    
    NS_ConvertUTF16toUTF8 modNameUTF8(modName);
    int modType;
    SECMOD_DeleteModule(const_cast<char*>(modNameUTF8.get()), &modType);

    nsCString pkcs11moduleSpec;
    pkcs11moduleSpec.Append(NS_LITERAL_CSTRING("name=\""));
    pkcs11moduleSpec.Append(modNameUTF8.get());
    pkcs11moduleSpec.Append(NS_LITERAL_CSTRING("\" library=\""));
    pkcs11moduleSpec.Append(escaped_fullLibraryPath);
    pkcs11moduleSpec.Append(NS_LITERAL_CSTRING("\""));

    PR_FreeLibraryName(fullLibraryPath); 
    PORT_Free(escaped_fullLibraryPath);

    RootsModule =
      SECMOD_LoadUserModule(const_cast<char*>(pkcs11moduleSpec.get()), 
                            nullptr, 
                            false); 

    if (RootsModule) {
      bool found = (RootsModule->loaded);

      SECMOD_DestroyModule(RootsModule);
      RootsModule = nullptr;

      if (found) {
        break;
      }
    }
  }
}

void 
nsNSSComponent::UnloadLoadableRoots()
{
  nsresult rv;
  nsAutoString modName;
  rv = GetPIPNSSBundleString("RootCertModuleName", modName);
  if (NS_FAILED(rv)) return;

  NS_ConvertUTF16toUTF8 modNameUTF8(modName);
  SECMODModule *RootsModule = SECMOD_FindModule(modNameUTF8.get());

  if (RootsModule) {
    SECMOD_UnloadUserModule(RootsModule);
    SECMOD_DestroyModule(RootsModule);
  }
}

nsresult
nsNSSComponent::ConfigureInternalPKCS11Token()
{
  nsNSSShutDownPreventionLock locker;
  nsAutoString manufacturerID;
  nsAutoString libraryDescription;
  nsAutoString tokenDescription;
  nsAutoString privateTokenDescription;
  nsAutoString slotDescription;
  nsAutoString privateSlotDescription;
  nsAutoString fips140TokenDescription;
  nsAutoString fips140SlotDescription;

  nsresult rv;
  rv = GetPIPNSSBundleString("ManufacturerID", manufacturerID);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("LibraryDescription", libraryDescription);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("TokenDescription", tokenDescription);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("PrivateTokenDescription", privateTokenDescription);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("SlotDescription", slotDescription);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("PrivateSlotDescription", privateSlotDescription);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("Fips140TokenDescription", fips140TokenDescription);
  if (NS_FAILED(rv)) return rv;

  rv = GetPIPNSSBundleString("Fips140SlotDescription", fips140SlotDescription);
  if (NS_FAILED(rv)) return rv;

  PK11_ConfigurePKCS11(NS_ConvertUTF16toUTF8(manufacturerID).get(),
                       NS_ConvertUTF16toUTF8(libraryDescription).get(),
                       NS_ConvertUTF16toUTF8(tokenDescription).get(),
                       NS_ConvertUTF16toUTF8(privateTokenDescription).get(),
                       NS_ConvertUTF16toUTF8(slotDescription).get(),
                       NS_ConvertUTF16toUTF8(privateSlotDescription).get(),
                       NS_ConvertUTF16toUTF8(fips140TokenDescription).get(),
                       NS_ConvertUTF16toUTF8(fips140SlotDescription).get(),
                       0, 0);
  return NS_OK;
}

nsresult
nsNSSComponent::InitializePIPNSSBundle()
{
  

  nsresult rv;
  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
  if (NS_FAILED(rv) || !bundleService) 
    return NS_ERROR_FAILURE;
  
  bundleService->CreateBundle(PIPNSS_STRBUNDLE_URL,
                              getter_AddRefs(mPIPNSSBundle));
  if (!mPIPNSSBundle)
    rv = NS_ERROR_FAILURE;

  bundleService->CreateBundle(NSSERR_STRBUNDLE_URL,
                              getter_AddRefs(mNSSErrorsBundle));
  if (!mNSSErrorsBundle)
    rv = NS_ERROR_FAILURE;

  return rv;
}

nsresult
nsNSSComponent::RegisterPSMContentListener()
{
  

  nsresult rv = NS_OK;
  if (!mPSMContentListener) {
    nsCOMPtr<nsIURILoader> dispatcher(do_GetService(NS_URI_LOADER_CONTRACTID));
    if (dispatcher) {
      mPSMContentListener = do_CreateInstance(NS_PSMCONTENTLISTEN_CONTRACTID);
      rv = dispatcher->RegisterContentListener(mPSMContentListener);
    }
  }
  return rv;
}


typedef struct {
  const char* pref;
  long id;
} CipherPref;

static CipherPref CipherPrefs[] = {
 
 {"security.ssl3.rsa_rc4_128_md5", SSL_RSA_WITH_RC4_128_MD5}, 
 {"security.ssl3.rsa_rc4_128_sha", SSL_RSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.rsa_fips_des_ede3_sha", SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.rsa_des_ede3_sha", SSL_RSA_WITH_3DES_EDE_CBC_SHA}, 
 
 {"security.ssl3.dhe_rsa_camellia_256_sha", TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA}, 
 {"security.ssl3.dhe_dss_camellia_256_sha", TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA}, 
 {"security.ssl3.rsa_camellia_256_sha", TLS_RSA_WITH_CAMELLIA_256_CBC_SHA}, 
 {"security.ssl3.dhe_rsa_aes_256_sha", TLS_DHE_RSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.dhe_dss_aes_256_sha", TLS_DHE_DSS_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.rsa_aes_256_sha", TLS_RSA_WITH_AES_256_CBC_SHA}, 
   

 {"security.ssl3.ecdhe_ecdsa_aes_256_sha", TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.ecdhe_ecdsa_aes_128_sha", TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.ecdhe_ecdsa_des_ede3_sha", TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.ecdhe_ecdsa_rc4_128_sha", TLS_ECDHE_ECDSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.ecdhe_rsa_aes_256_sha", TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.ecdhe_rsa_aes_128_sha", TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.ecdhe_rsa_des_ede3_sha", TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.ecdhe_rsa_rc4_128_sha", TLS_ECDHE_RSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.ecdh_ecdsa_aes_256_sha", TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_aes_128_sha", TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_des_ede3_sha", TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_rc4_128_sha", TLS_ECDH_ECDSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.ecdh_rsa_aes_256_sha", TLS_ECDH_RSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.ecdh_rsa_aes_128_sha", TLS_ECDH_RSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.ecdh_rsa_des_ede3_sha", TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.ecdh_rsa_rc4_128_sha", TLS_ECDH_RSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.dhe_rsa_camellia_128_sha", TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA}, 
 {"security.ssl3.dhe_dss_camellia_128_sha", TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA}, 
 {"security.ssl3.rsa_camellia_128_sha", TLS_RSA_WITH_CAMELLIA_128_CBC_SHA}, 
 {"security.ssl3.dhe_rsa_aes_128_sha", TLS_DHE_RSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.dhe_dss_aes_128_sha", TLS_DHE_DSS_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.rsa_aes_128_sha", TLS_RSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.dhe_rsa_des_ede3_sha", SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.dhe_dss_des_ede3_sha", SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.rsa_seed_sha", TLS_RSA_WITH_SEED_CBC_SHA}, 
 {nullptr, 0} 
};

static void
setNonPkixOcspEnabled(int32_t ocspEnabled, nsIPrefBranch * pref)
{
  
  
  if (!ocspEnabled) {
    CERT_DisableOCSPChecking(CERT_GetDefaultCertDB());
    CERT_DisableOCSPDefaultResponder(CERT_GetDefaultCertDB());
  } else {
    CERT_EnableOCSPChecking(CERT_GetDefaultCertDB());
    CERT_DisableOCSPDefaultResponder(CERT_GetDefaultCertDB());
  }
}

#define CRL_DOWNLOAD_DEFAULT false
#define OCSP_ENABLED_DEFAULT 1
#define OCSP_REQUIRED_DEFAULT 0
#define FRESH_REVOCATION_REQUIRED_DEFAULT false
#define MISSING_CERT_DOWNLOAD_DEFAULT false
#define FIRST_REVO_METHOD_DEFAULT "ocsp"
#define USE_NSS_LIBPKIX_DEFAULT false


void nsNSSComponent::setValidationOptions(nsIPrefBranch * pref)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv;

  bool crlDownloading;
  rv = pref->GetBoolPref("security.CRL_download.enabled", &crlDownloading);
  if (NS_FAILED(rv))
    crlDownloading = CRL_DOWNLOAD_DEFAULT;
  
  int32_t ocspEnabled;
  rv = pref->GetIntPref("security.OCSP.enabled", &ocspEnabled);
  
  
  if (NS_FAILED(rv))
    ocspEnabled = OCSP_ENABLED_DEFAULT;

  bool ocspRequired;
  rv = pref->GetBoolPref("security.OCSP.require", &ocspRequired);
  if (NS_FAILED(rv))
    ocspRequired = OCSP_REQUIRED_DEFAULT;

  bool anyFreshRequired;
  rv = pref->GetBoolPref("security.fresh_revocation_info.require", &anyFreshRequired);
  if (NS_FAILED(rv))
    anyFreshRequired = FRESH_REVOCATION_REQUIRED_DEFAULT;
  
  bool aiaDownloadEnabled;
  rv = pref->GetBoolPref("security.missing_cert_download.enabled", &aiaDownloadEnabled);
  if (NS_FAILED(rv))
    aiaDownloadEnabled = MISSING_CERT_DOWNLOAD_DEFAULT;

  nsCString firstNetworkRevo;
  rv = pref->GetCharPref("security.first_network_revocation_method", getter_Copies(firstNetworkRevo));
  if (NS_FAILED(rv))
    firstNetworkRevo = FIRST_REVO_METHOD_DEFAULT;
  
  setNonPkixOcspEnabled(ocspEnabled, pref);
  
  CERT_SetOCSPFailureMode( ocspRequired ?
                           ocspMode_FailureIsVerificationFailure
                           : ocspMode_FailureIsNotAVerificationFailure);

  RefPtr<nsCERTValInParamWrapper> newCVIN(new nsCERTValInParamWrapper);
  if (NS_SUCCEEDED(newCVIN->Construct(
      aiaDownloadEnabled ? 
        nsCERTValInParamWrapper::missing_cert_download_on : nsCERTValInParamWrapper::missing_cert_download_off,
      crlDownloading ?
        nsCERTValInParamWrapper::crl_download_allowed : nsCERTValInParamWrapper::crl_local_only,
      ocspEnabled ? 
        nsCERTValInParamWrapper::ocsp_on : nsCERTValInParamWrapper::ocsp_off,
      ocspRequired ? 
        nsCERTValInParamWrapper::ocsp_strict : nsCERTValInParamWrapper::ocsp_relaxed,
      anyFreshRequired ?
        nsCERTValInParamWrapper::any_revo_strict : nsCERTValInParamWrapper::any_revo_relaxed,
      firstNetworkRevo.get()))) {
    
    
    mDefaultCERTValInParam = newCVIN;
  }

  



  SSL_ClearSessionCache();
}

NS_IMETHODIMP
nsNSSComponent::SkipOcsp()
{
  nsNSSShutDownPreventionLock locker;
  CERTCertDBHandle *certdb = CERT_GetDefaultCertDB();

  SECStatus rv = CERT_DisableOCSPChecking(certdb);
  return (rv == SECSuccess) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSComponent::SkipOcspOff()
{
  nsNSSShutDownPreventionLock locker;
  int32_t ocspEnabled;
  if (NS_FAILED(mPrefBranch->GetIntPref("security.OCSP.enabled", &ocspEnabled)))
    ocspEnabled = OCSP_ENABLED_DEFAULT;
  
  
  
  setNonPkixOcspEnabled(ocspEnabled, mPrefBranch);

  if (ocspEnabled)
    SSL_ClearSessionCache();

  return NS_OK;
}

nsresult
nsNSSComponent::PostCRLImportEvent(const nsCSubstring &urlString,
                                   nsIStreamListener *listener)
{
  
  nsCOMPtr<nsIRunnable> event = new CRLDownloadEvent(urlString, listener);

  
  return NS_DispatchToMainThread(event);
}

nsresult
nsNSSComponent::DownloadCRLDirectly(nsAutoString url, nsAutoString key)
{
  
  
  nsCOMPtr<nsIStreamListener> listener =
      new PSMContentDownloader(PSMContentDownloader::PKCS7_CRL);
  
  NS_ConvertUTF16toUTF8 url8(url);
  return PostCRLImportEvent(url8, listener);
}

nsresult nsNSSComponent::DownloadCrlSilently()
{
  
  nsStringKey hashKey(mCrlUpdateKey.get());
  crlsScheduledForDownload->Put(&hashKey,(void *)nullptr);
    
  
  RefPtr<PSMContentDownloader> psmDownloader(
      new PSMContentDownloader(PSMContentDownloader::PKCS7_CRL));
  psmDownloader->setSilentDownload(true);
  psmDownloader->setCrlAutodownloadKey(mCrlUpdateKey);
  
  
  NS_ConvertUTF16toUTF8 url8(mDownloadURL);
  return PostCRLImportEvent(url8, psmDownloader);
}

nsresult nsNSSComponent::getParamsForNextCrlToDownload(nsAutoString *url, PRTime *time, nsAutoString *key)
{
  const char *updateEnabledPref = CRL_AUTOUPDATE_ENABLED_PREF;
  const char *updateTimePref = CRL_AUTOUPDATE_TIME_PREF;
  const char *updateURLPref = CRL_AUTOUPDATE_URL_PREF;
  char **allCrlsToBeUpdated;
  uint32_t noOfCrls;
  PRTime nearestUpdateTime = 0;
  nsAutoString crlKey;
  char *tempUrl;
  nsresult rv;
  
  nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID,&rv);
  if(NS_FAILED(rv)){
    return rv;
  }

  rv = pref->GetChildList(updateEnabledPref, &noOfCrls, &allCrlsToBeUpdated);
  if ( (NS_FAILED(rv)) || (noOfCrls==0) ){
    return NS_ERROR_FAILURE;
  }

  for(uint32_t i=0;i<noOfCrls;i++) {
    
    bool autoUpdateEnabled = false;
    rv = pref->GetBoolPref(*(allCrlsToBeUpdated+i), &autoUpdateEnabled);
    if (NS_FAILED(rv) || !autoUpdateEnabled) {
      continue;
    }

    nsAutoString tempCrlKey;

    
    nsAutoCString enabledPrefCString(*(allCrlsToBeUpdated+i));
    enabledPrefCString.ReplaceSubstring(updateEnabledPref,".");
    tempCrlKey.AssignWithConversion(enabledPrefCString.get());
      
    
    
    
    
    nsStringKey hashKey(tempCrlKey.get());
    if(crlsScheduledForDownload->Exists(&hashKey)){
      continue;
    }

    char *tempTimeString;
    PRTime tempTime;
    nsAutoCString timingPrefCString(updateTimePref);
    LossyAppendUTF16toASCII(tempCrlKey, timingPrefCString);
    
    rv = pref->GetCharPref(timingPrefCString.get(), &tempTimeString);
    if (NS_FAILED(rv)){
      
      tempTime = PR_Now();
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("get %s failed: forcing download\n", timingPrefCString.get()));
    } else {
      tempTime = (PRTime)nsCRT::atoll(tempTimeString);
      nsMemory::Free(tempTimeString);
      
      
      
      
      
      
      
      
      
      
      
      if (tempTime == 0)
        tempTime = PR_Now();
#ifdef PR_LOGGING
      PRExplodedTime explodedTime;
      PR_ExplodeTime(tempTime, PR_GMTParameters, &explodedTime);
      
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("%s tempTime(%lli) "
              "(m/d/y h:m:s = %02d/%02d/%d %02d:%02d:%02d GMT\n",
              timingPrefCString.get(), tempTime,
              explodedTime.tm_month+1, explodedTime.tm_mday,
              explodedTime.tm_year, explodedTime.tm_hour,
              explodedTime.tm_min, explodedTime.tm_sec));
#endif
    }

    if(nearestUpdateTime == 0 || tempTime < nearestUpdateTime){
      nsAutoCString urlPrefCString(updateURLPref);
      LossyAppendUTF16toASCII(tempCrlKey, urlPrefCString);
      rv = pref->GetCharPref(urlPrefCString.get(), &tempUrl);
      if (NS_FAILED(rv) || (!tempUrl)){
        continue;
      }
      nearestUpdateTime = tempTime;
      crlKey = tempCrlKey;
    }
  }

  if(noOfCrls > 0)
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(noOfCrls, allCrlsToBeUpdated);

  if(nearestUpdateTime > 0){
    *time = nearestUpdateTime;
    url->AssignWithConversion((const char *)tempUrl);
    nsMemory::Free(tempUrl);
    *key = crlKey;
    rv = NS_OK;
  } else{
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

NS_IMETHODIMP
nsNSSComponent::Notify(nsITimer *timer)
{
  
  {
    MutexAutoLock lock(mCrlTimerLock);
    crlDownloadTimerOn = false;
  }

  
  DownloadCrlSilently();

  
  
  DefineNextTimer();
  return NS_OK;
}

nsresult
nsNSSComponent::RemoveCrlFromList(nsAutoString key)
{
  nsStringKey hashKey(key.get());
  if(crlsScheduledForDownload->Exists(&hashKey)){
    crlsScheduledForDownload->Remove(&hashKey);
  }
  return NS_OK;
}

nsresult
nsNSSComponent::DefineNextTimer()
{
  PRTime nextFiring;
  PRTime now = PR_Now();
  uint32_t interval;
  uint32_t primaryDelay = CRL_AUTOUPDATE_DEFAULT_DELAY;
  nsresult rv;

  if(!mTimer){
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if(NS_FAILED(rv))
      return rv;
  }

  
  
  
  

  MutexAutoLock lock(mCrlTimerLock);

  if (crlDownloadTimerOn) {
    mTimer->Cancel();
  }

  rv = getParamsForNextCrlToDownload(&mDownloadURL, &nextFiring, &mCrlUpdateKey);
  
  if(NS_FAILED(rv)){
    
    return NS_OK;
  }
     
  
  if ( now < nextFiring) {
    interval = uint32_t(nextFiring - now);
    
    interval = interval/PR_USEC_PER_MSEC;
  }else {
    interval = primaryDelay;
  }
  
  mTimer->InitWithCallback(static_cast<nsITimerCallback*>(this), 
                           interval,
                           nsITimer::TYPE_ONE_SHOT);
  crlDownloadTimerOn = true;

  return NS_OK;
}




nsresult
nsNSSComponent::StopCRLUpdateTimer()
{
  
  
  if (mUpdateTimerInitialized) {
    if (crlsScheduledForDownload) {
      crlsScheduledForDownload->Reset();
      delete crlsScheduledForDownload;
      crlsScheduledForDownload = nullptr;
    }
    {
      MutexAutoLock lock(mCrlTimerLock);
      if (crlDownloadTimerOn) {
        mTimer->Cancel();
      }
      crlDownloadTimerOn = false;
    }
    mUpdateTimerInitialized = false;
  }

  return NS_OK;
}

nsresult
nsNSSComponent::InitializeCRLUpdateTimer()
{
  nsresult rv;
    
  
  if (!mUpdateTimerInitialized) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if(NS_FAILED(rv)){
      return rv;
    }
    crlsScheduledForDownload = new nsHashtable(16, true);
    DefineNextTimer();
    mUpdateTimerInitialized = true;  
  } 

  return NS_OK;
}

#ifdef XP_MACOSX
void
nsNSSComponent::TryCFM2MachOMigration(nsIFile *cfmPath, nsIFile *machoPath)
{
  
  
  
  
  
  

  NS_NAMED_LITERAL_CSTRING(cstr_key3db, "key3.db");
  NS_NAMED_LITERAL_CSTRING(cstr_cert7db, "cert7.db");
  NS_NAMED_LITERAL_CSTRING(cstr_cert8db, "cert8.db");
  NS_NAMED_LITERAL_CSTRING(cstr_keydatabase3, "Key Database3");
  NS_NAMED_LITERAL_CSTRING(cstr_certificate7, "Certificates7");
  NS_NAMED_LITERAL_CSTRING(cstr_certificate8, "Certificates8");

  bool bExists;
  nsresult rv;

  nsCOMPtr<nsIFile> macho_key3db;
  rv = machoPath->Clone(getter_AddRefs(macho_key3db));
  if (NS_FAILED(rv)) {
    return;
  }

  macho_key3db->AppendNative(cstr_key3db);
  rv = macho_key3db->Exists(&bExists);
  if (NS_FAILED(rv) || bExists) {
    return;
  }

  nsCOMPtr<nsIFile> macho_cert7db;
  rv = machoPath->Clone(getter_AddRefs(macho_cert7db));
  if (NS_FAILED(rv)) {
    return;
  }

  macho_cert7db->AppendNative(cstr_cert7db);
  rv = macho_cert7db->Exists(&bExists);
  if (NS_FAILED(rv) || bExists) {
    return;
  }

  nsCOMPtr<nsIFile> macho_cert8db;
  rv = machoPath->Clone(getter_AddRefs(macho_cert8db));
  if (NS_FAILED(rv)) {
    return;
  }

  macho_cert8db->AppendNative(cstr_cert8db);
  rv = macho_cert7db->Exists(&bExists);
  if (NS_FAILED(rv) || bExists) {
    return;
  }

  

  nsCOMPtr<nsIFile> cfm_key3;
  rv = cfmPath->Clone(getter_AddRefs(cfm_key3));
  if (NS_FAILED(rv)) {
    return;
  }

  cfm_key3->AppendNative(cstr_keydatabase3);
  rv = cfm_key3->Exists(&bExists);
  if (NS_FAILED(rv)) {
    return;
  }

  if (bExists) {
    cfm_key3->CopyToFollowingLinksNative(machoPath, cstr_key3db);
  }

  nsCOMPtr<nsIFile> cfm_cert7;
  rv = cfmPath->Clone(getter_AddRefs(cfm_cert7));
  if (NS_FAILED(rv)) {
    return;
  }

  cfm_cert7->AppendNative(cstr_certificate7);
  rv = cfm_cert7->Exists(&bExists);
  if (NS_FAILED(rv)) {
    return;
  }

  if (bExists) {
    cfm_cert7->CopyToFollowingLinksNative(machoPath, cstr_cert7db);
  }

  nsCOMPtr<nsIFile> cfm_cert8;
  rv = cfmPath->Clone(getter_AddRefs(cfm_cert8));
  if (NS_FAILED(rv)) {
    return;
  }

  cfm_cert8->AppendNative(cstr_certificate8);
  rv = cfm_cert8->Exists(&bExists);
  if (NS_FAILED(rv)) {
    return;
  }

  if (bExists) {
    cfm_cert8->CopyToFollowingLinksNative(machoPath, cstr_cert8db);
  }
}
#endif

static void configureMD5(bool enabled)
{
  if (enabled) { 
    NSS_SetAlgorithmPolicy(SEC_OID_MD5, 
        NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE, 0);
    NSS_SetAlgorithmPolicy(SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION,
        NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE, 0);
    NSS_SetAlgorithmPolicy(SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC,
        NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE, 0);
  }
  else { 
    NSS_SetAlgorithmPolicy(SEC_OID_MD5,
        0, NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE);
    NSS_SetAlgorithmPolicy(SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION,
        0, NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE);
    NSS_SetAlgorithmPolicy(SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC,
        0, NSS_USE_ALG_IN_CERT_SIGNATURE | NSS_USE_ALG_IN_CMS_SIGNATURE);
  }
}

nsresult
nsNSSComponent::InitializeNSS(bool showWarningBox)
{
  
  

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent::InitializeNSS\n"));

  MOZ_STATIC_ASSERT(nsINSSErrorsService::NSS_SEC_ERROR_BASE == SEC_ERROR_BASE &&
                    nsINSSErrorsService::NSS_SEC_ERROR_LIMIT == SEC_ERROR_LIMIT &&
                    nsINSSErrorsService::NSS_SSL_ERROR_BASE == SSL_ERROR_BASE &&
                    nsINSSErrorsService::NSS_SSL_ERROR_LIMIT == SSL_ERROR_LIMIT,
                    "You must update the values in nsINSSErrorsService.idl");

  

  enum { problem_none, problem_no_rw, problem_no_security_at_all }
    which_nss_problem = problem_none;

  {
    MutexAutoLock lock(mutex);

    

    if (mNSSInitialized) {
      PR_ASSERT(!"Trying to initialize NSS twice"); 
                                                    
                                                    
      return NS_ERROR_FAILURE;
    }
    
    nsresult rv;
    nsAutoCString profileStr;
    nsCOMPtr<nsIFile> profilePath;

    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(profilePath));
    if (NS_FAILED(rv)) {
      PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("Unable to get profile directory\n"));
      ConfigureInternalPKCS11Token();
      SECStatus init_rv = NSS_NoDB_Init(nullptr);
      if (init_rv != SECSuccess) {
        nsPSMInitPanic::SetPanic();
        return NS_ERROR_NOT_AVAILABLE;
      }
    }
    else
    {

  

  #if defined(XP_MACOSX)
    
    
    nsCOMPtr<nsIFile> cfmSecurityPath;
    cfmSecurityPath = profilePath; 
    cfmSecurityPath->AppendNative(NS_LITERAL_CSTRING("Security"));
  #endif

  #if defined(XP_MACOSX)
    
    
    
    rv = cfmSecurityPath->GetParent(getter_AddRefs(profilePath));
    if (NS_FAILED(rv)) {
      nsPSMInitPanic::SetPanic();
      return rv;
    }
  #endif

    const char *dbdir_override = getenv("MOZPSM_NSSDBDIR_OVERRIDE");
    if (dbdir_override && strlen(dbdir_override)) {
      profileStr = dbdir_override;
    }
    else {
  #if defined(XP_WIN)
      
      
      nsCOMPtr<nsILocalFileWin> profilePathWin(do_QueryInterface(profilePath, &rv));
      if (profilePathWin)
        rv = profilePathWin->GetNativeCanonicalPath(profileStr);
  #else
      rv = profilePath->GetNativePath(profileStr);
  #endif
      if (NS_FAILED(rv)) {
        nsPSMInitPanic::SetPanic();
        return rv;
      }
    }

    hashTableCerts = PL_NewHashTable( 0, certHashtable_keyHash, certHashtable_keyCompare,
      certHashtable_valueCompare, 0, 0 );

  #if defined(XP_MACOSX)
    
    
    TryCFM2MachOMigration(cfmSecurityPath, profilePath);
  #endif

    rv = mPrefBranch->GetBoolPref("security.use_libpkix_verification", &globalConstFlagUsePKIXVerification);
    if (NS_FAILED(rv))
      globalConstFlagUsePKIXVerification = USE_NSS_LIBPKIX_DEFAULT;

    bool supress_warning_preference = false;
    rv = mPrefBranch->GetBoolPref("security.suppress_nss_rw_impossible_warning", &supress_warning_preference);

    if (NS_FAILED(rv)) {
      supress_warning_preference = false;
    }

    

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS Initialization beginning\n"));

    
    
    
    
    

    ConfigureInternalPKCS11Token();

    
    
    
    
    
    uint32_t init_flags = NSS_INIT_NOROOTINIT | NSS_INIT_OPTIMIZESPACE;
    SECStatus init_rv = ::NSS_Initialize(profileStr.get(), "", "",
                                         SECMOD_DB, init_flags);

    if (init_rv != SECSuccess) {
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("can not init NSS r/w in %s\n", profileStr.get()));

      if (supress_warning_preference) {
        which_nss_problem = problem_none;
      }
      else {
        which_nss_problem = problem_no_rw;
      }

      
      init_flags |= NSS_INIT_READONLY;
      init_rv = ::NSS_Initialize(profileStr.get(), "", "",
                                 SECMOD_DB, init_flags);

      if (init_rv != SECSuccess) {
        PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("can not init in r/o either\n"));
        which_nss_problem = problem_no_security_at_all;

        init_rv = NSS_NoDB_Init(profileStr.get());
        if (init_rv != SECSuccess) {
          nsPSMInitPanic::SetPanic();
          return NS_ERROR_NOT_AVAILABLE;
        }
      }
    } 
    } 

    

    if (problem_no_security_at_all != which_nss_problem) {

      mNSSInitialized = true;

      ::NSS_SetDomesticPolicy();

      PK11_SetPasswordFunc(PK11PasswordPrompt);

      
      mPrefBranch->AddObserver("security.", this, false);

      SSL_OptionSetDefault(SSL_ENABLE_SSL2, false);
      SSL_OptionSetDefault(SSL_V2_COMPATIBLE_HELLO, false);
      bool enabled;
      mPrefBranch->GetBoolPref("security.enable_ssl3", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_SSL3, enabled);
      mPrefBranch->GetBoolPref("security.enable_tls", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_TLS, enabled);
      mPrefBranch->GetBoolPref("security.enable_md5_signatures", &enabled);
      configureMD5(enabled);

      
      mPrefBranch->GetBoolPref("security.enable_tls_session_tickets", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_SESSION_TICKETS, enabled);

      mPrefBranch->GetBoolPref("security.ssl.require_safe_negotiation", &enabled);
      SSL_OptionSetDefault(SSL_REQUIRE_SAFE_NEGOTIATION, enabled);

      mPrefBranch->GetBoolPref(
        "security.ssl.allow_unrestricted_renego_everywhere__temporarily_available_pref", 
        &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_RENEGOTIATION, 
        enabled ? SSL_RENEGOTIATE_UNRESTRICTED : SSL_RENEGOTIATE_REQUIRES_XTN);

#ifdef SSL_ENABLE_FALSE_START 
      mPrefBranch->GetBoolPref("security.ssl.enable_false_start", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_FALSE_START, enabled);
#endif

      
      for (uint16_t i = 0; i < SSL_NumImplementedCiphers; ++i)
      {
        uint16_t cipher_id = SSL_ImplementedCiphers[i];
        SSL_CipherPrefSetDefault(cipher_id, false);
      }

      
      for (CipherPref* cp = CipherPrefs; cp->pref; ++cp) {
        rv = mPrefBranch->GetBoolPref(cp->pref, &enabled);
        if (NS_FAILED(rv))
          enabled = false;

        SSL_CipherPrefSetDefault(cp->id, enabled);
      }

      
      SEC_PKCS12EnableCipher(PKCS12_RC4_40, 1);
      SEC_PKCS12EnableCipher(PKCS12_RC4_128, 1);
      SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_40, 1);
      SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_128, 1);
      SEC_PKCS12EnableCipher(PKCS12_DES_56, 1);
      SEC_PKCS12EnableCipher(PKCS12_DES_EDE3_168, 1);
      SEC_PKCS12SetPreferredCipher(PKCS12_DES_EDE3_168, 1);
      PORT_SetUCS2_ASCIIConversionFunction(pip_ucs2_ascii_conversion_fn);

      
      setValidationOptions(mPrefBranch);

      
      mDefaultCERTValInParamLocalOnly = new nsCERTValInParamWrapper;
      rv = mDefaultCERTValInParamLocalOnly->Construct(
          nsCERTValInParamWrapper::missing_cert_download_off,
          nsCERTValInParamWrapper::crl_local_only,
          nsCERTValInParamWrapper::ocsp_off,
          nsCERTValInParamWrapper::ocsp_relaxed,
          nsCERTValInParamWrapper::any_revo_relaxed,
          FIRST_REVO_METHOD_DEFAULT);
      if (NS_FAILED(rv)) {
        nsPSMInitPanic::SetPanic();
        return rv;
      }
      
      RegisterMyOCSPAIAInfoCallback();

      mHttpForNSS.initTable();
      mHttpForNSS.registerHttpClient();

      InstallLoadableRoots();

#ifndef MOZ_DISABLE_CRYPTOLEGACY
      LaunchSmartCardThreads();
#endif

      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS Initialization done\n"));
    }
  }

  if (problem_none != which_nss_problem) {
    nsPSMInitPanic::SetPanic();

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS problem, trying to bring up GUI error message\n"));

    
    
    if (showWarningBox) {
      ShowAlertFromStringBundle("NSSInitProblemX");
    }
  }

  return NS_OK;
}

void
nsNSSComponent::ShutdownNSS()
{
  
  
  
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent::ShutdownNSS\n"));

  MutexAutoLock lock(mutex);

  if (hashTableCerts) {
    PL_HashTableEnumerateEntries(hashTableCerts, certHashtable_clearEntry, 0);
    PL_HashTableDestroy(hashTableCerts);
    hashTableCerts = nullptr;
  }

  if (mNSSInitialized) {
    mNSSInitialized = false;

    PK11_SetPasswordFunc((PK11PasswordFunc)nullptr);
    mHttpForNSS.unregisterHttpClient();
    UnregisterMyOCSPAIAInfoCallback();

    if (mPrefBranch) {
      mPrefBranch->RemoveObserver("security.", this);
    }

#ifndef MOZ_DISABLE_CRYPTOLEGACY
    ShutdownSmartCardThreads();
#endif
    SSL_ClearSessionCache();
    UnloadLoadableRoots();
    CleanupIdentityInfo();
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("evaporating psm resources\n"));
    mShutdownObjectList->evaporateAllNSSResources();
    EnsureNSSInitialized(nssShutdown);
    if (SECSuccess != ::NSS_Shutdown()) {
      PR_LOG(gPIPNSSLog, PR_LOG_ALWAYS, ("NSS SHUTDOWN FAILURE\n"));
    }
    else {
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS shutdown =====>> OK <<=====\n"));
    }
  }
}
 
NS_IMETHODIMP
nsNSSComponent::Init()
{
  
  

  nsresult rv = NS_OK;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Beginning NSS initialization\n"));

  if (!mShutdownObjectList)
  {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS init, out of memory in constructor\n"));
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = InitializePIPNSSBundle();
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("Unable to create pipnss bundle.\n"));
    return rv;
  }      

  
  
  
  
  {
    NS_NAMED_LITERAL_STRING(dummy_name, "dummy");
    nsXPIDLString result;
    mPIPNSSBundle->GetStringFromName(dummy_name.get(),
                                     getter_Copies(result));
    mNSSErrorsBundle->GetStringFromName(dummy_name.get(),
                                        getter_Copies(result));
  }

  if (!mPrefBranch) {
    mPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    NS_ASSERTION(mPrefBranch, "Unable to get pref service");
  }

  bool sendLM = false;
  mPrefBranch->GetBoolPref("network.ntlm.send-lm-response", &sendLM);
  nsNTLMAuthModule::SetSendLM(sendLM);

  
  RegisterObservers();

  rv = InitializeNSS(true); 
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("Unable to Initialize NSS.\n"));

    DeregisterObservers();
    mPIPNSSBundle = nullptr;
    return rv;
  }

  RememberCertErrorsTable::Init();
  SharedSSLState::GlobalInit();
  
  createBackgroundThreads();
  if (!mCertVerificationThread)
  {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS init, could not create threads\n"));

    DeregisterObservers();
    mPIPNSSBundle = nullptr;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  InitializeCRLUpdateTimer();
  RegisterPSMContentListener();

  nsCOMPtr<nsIEntropyCollector> ec
      = do_GetService(NS_ENTROPYCOLLECTOR_CONTRACTID);

  nsCOMPtr<nsIBufEntropyCollector> bec;

  if (ec) {
    bec = do_QueryInterface(ec);
  }

  NS_ASSERTION(bec, "No buffering entropy collector.  "
                    "This means no entropy will be collected.");
  if (bec) {
    bec->ForwardTo(this);
  }

  return rv;
}


NS_IMPL_THREADSAFE_ISUPPORTS6(nsNSSComponent,
                              nsISignatureVerifier,
                              nsIEntropyCollector,
                              nsINSSComponent,
                              nsIObserver,
                              nsISupportsWeakReference,
                              nsITimerCallback)



static void ContentCallback(void *arg, 
                                           const char *buf,
                                           unsigned long len)
{
}

static PK11SymKey * GetDecryptKeyCallback(void *arg, 
                                                 SECAlgorithmID *algid)
{
  return nullptr;
}

static PRBool DecryptionAllowedCallback(SECAlgorithmID *algid,  
                                               PK11SymKey *bulkkey)
{
  return SECMIME_DecryptionAllowed(algid, bulkkey);
}

static void * GetPasswordKeyCallback(void *arg, void *handle)
{
  return nullptr;
}

NS_IMETHODIMP
nsNSSComponent::VerifySignature(const char* aRSABuf, uint32_t aRSABufLen,
                                const char* aPlaintext, uint32_t aPlaintextLen,
                                int32_t* aErrorCode,
                                nsICertificatePrincipal** aPrincipal)
{
  if (!aPrincipal || !aErrorCode) {
    return NS_ERROR_NULL_POINTER;
  }

  *aErrorCode = 0;
  *aPrincipal = nullptr;

  nsNSSShutDownPreventionLock locker;
  ScopedSEC_PKCS7ContentInfo p7_info; 
  unsigned char hash[SHA1_LENGTH]; 

  SECItem item;
  item.type = siEncodedCertBuffer;
  item.data = (unsigned char*)aRSABuf;
  item.len = aRSABufLen;
  p7_info = SEC_PKCS7DecodeItem(&item,
                                ContentCallback, nullptr,
                                GetPasswordKeyCallback, nullptr,
                                GetDecryptKeyCallback, nullptr,
                                DecryptionAllowedCallback);

  if (!p7_info) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  SECItem digest;
  digest.data = nullptr;
  digest.len = 0;

  if (aPlaintext) {
    HASHContext* hash_ctxt;
    uint32_t hashLen = 0;

    hash_ctxt = HASH_Create(HASH_AlgSHA1);
    HASH_Begin(hash_ctxt);
    HASH_Update(hash_ctxt,(const unsigned char*)aPlaintext, aPlaintextLen);
    HASH_End(hash_ctxt, hash, &hashLen, SHA1_LENGTH); 
    HASH_Destroy(hash_ctxt);

    digest.data = hash;
    digest.len = SHA1_LENGTH;
  }

  
  bool rv = SEC_PKCS7VerifyDetachedSignature(p7_info, certUsageObjectSigner,
                                               &digest, HASH_AlgSHA1, false);
  if (!rv) {
    *aErrorCode = PR_GetError();
  }

  
  CERTCertificate *cert = p7_info->content.signedData->signerInfos[0]->cert;
  nsresult rv2 = NS_OK;
  if (cert) {
    
    
    
    do {
      nsCOMPtr<nsIX509Cert> pCert = nsNSSCertificate::Create(cert);
      if (!pCert) {
        rv2 = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      if (!mScriptSecurityManager) {
        MutexAutoLock lock(mutex);
        
        if (!mScriptSecurityManager) {
          mScriptSecurityManager = 
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv2);
          if (NS_FAILED(rv2)) {
            break;
          }
        }
      }

      
      nsAutoString fingerprint;
      rv2 = pCert->GetSha1Fingerprint(fingerprint);
      if (NS_FAILED(rv2)) {
        break;
      }
      nsAutoString orgName;
      rv2 = pCert->GetOrganization(orgName);
      if (NS_FAILED(rv2)) {
        break;
      }
      nsAutoString subjectName;
      rv2 = pCert->GetSubjectName(subjectName);
      if (NS_FAILED(rv2)) {
        break;
      }
    
      nsCOMPtr<nsICertificatePrincipal> certPrincipal =
        new nsCertificatePrincipal(NS_ConvertUTF16toUTF8(fingerprint),
                                   NS_ConvertUTF16toUTF8(subjectName),
                                   NS_ConvertUTF16toUTF8(orgName),
                                   pCert);

      certPrincipal.swap(*aPrincipal);
    } while (0);
  }

  return rv2;
}

NS_IMETHODIMP
nsNSSComponent::RandomUpdate(void *entropy, int32_t bufLen)
{
  nsNSSShutDownPreventionLock locker;

  
  
  
  MutexAutoLock lock(mutex);

  if (!mNSSInitialized)
      return NS_ERROR_NOT_INITIALIZED;

  PK11_RandomUpdate(entropy, bufLen);
  return NS_OK;
}

#define PROFILE_CHANGE_NET_TEARDOWN_TOPIC "profile-change-net-teardown"
#define PROFILE_CHANGE_NET_RESTORE_TOPIC "profile-change-net-restore"
#define PROFILE_CHANGE_TEARDOWN_TOPIC "profile-change-teardown"
#define PROFILE_BEFORE_CHANGE_TOPIC "profile-before-change"
#define PROFILE_DO_CHANGE_TOPIC "profile-do-change"

NS_IMETHODIMP
nsNSSComponent::Observe(nsISupports *aSubject, const char *aTopic, 
                        const PRUnichar *someData)
{
  if (nsCRT::strcmp(aTopic, PROFILE_CHANGE_TEARDOWN_TOPIC) == 0) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("in PSM code, receiving change-teardown\n"));
    DoProfileChangeTeardown(aSubject);
  }
  else if (nsCRT::strcmp(aTopic, PROFILE_BEFORE_CHANGE_TOPIC) == 0) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("receiving profile change topic\n"));
    DoProfileBeforeChange(aSubject);
  }
  else if (nsCRT::strcmp(aTopic, PROFILE_DO_CHANGE_TOPIC) == 0) {
    if (someData && NS_LITERAL_STRING("startup").Equals(someData)) {
      
      
      
      
      
      
      
      DoProfileChangeNetTeardown();
      DoProfileChangeTeardown(aSubject);
      DoProfileBeforeChange(aSubject);
      DoProfileChangeNetRestore();
    }
  
    bool needsInit = true;

    {
      MutexAutoLock lock(mutex);

      if (mNSSInitialized) {
        
        
        needsInit = false;
      }
    }
    
    if (needsInit) {
      if (NS_FAILED(InitializeNSS(false))) { 
        PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("Unable to Initialize NSS after profile switch.\n"));
      }
    }

    InitializeCRLUpdateTimer();
  }
  else if (nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent: XPCom shutdown observed\n"));

    

    if (mPSMContentListener) {
      nsCOMPtr<nsIURILoader> dispatcher(do_GetService(NS_URI_LOADER_CONTRACTID));
      if (dispatcher) {
        dispatcher->UnRegisterContentListener(mPSMContentListener);
      }
      mPSMContentListener = nullptr;
    }

    nsCOMPtr<nsIEntropyCollector> ec
        = do_GetService(NS_ENTROPYCOLLECTOR_CONTRACTID);

    if (ec) {
      nsCOMPtr<nsIBufEntropyCollector> bec
        = do_QueryInterface(ec);
      if (bec) {
        bec->DontForward();
      }
    }
  }
  else if (nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) { 
    nsNSSShutDownPreventionLock locker;
    bool clearSessionCache = false;
    bool enabled;
    NS_ConvertUTF16toUTF8  prefName(someData);

    if (prefName.Equals("security.enable_ssl3")) {
      mPrefBranch->GetBoolPref("security.enable_ssl3", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_SSL3, enabled);
      clearSessionCache = true;
    } else if (prefName.Equals("security.enable_tls")) {
      mPrefBranch->GetBoolPref("security.enable_tls", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_TLS, enabled);
      clearSessionCache = true;
    } else if (prefName.Equals("security.enable_md5_signatures")) {
      mPrefBranch->GetBoolPref("security.enable_md5_signatures", &enabled);
      configureMD5(enabled);
      clearSessionCache = true;
    } else if (prefName.Equals("security.enable_tls_session_tickets")) {
      mPrefBranch->GetBoolPref("security.enable_tls_session_tickets", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_SESSION_TICKETS, enabled);
    } else if (prefName.Equals("security.ssl.require_safe_negotiation")) {
      mPrefBranch->GetBoolPref("security.ssl.require_safe_negotiation", &enabled);
      SSL_OptionSetDefault(SSL_REQUIRE_SAFE_NEGOTIATION, enabled);
    } else if (prefName.Equals("security.ssl.allow_unrestricted_renego_everywhere__temporarily_available_pref")) {
      mPrefBranch->GetBoolPref("security.ssl.allow_unrestricted_renego_everywhere__temporarily_available_pref", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_RENEGOTIATION, 
        enabled ? SSL_RENEGOTIATE_UNRESTRICTED : SSL_RENEGOTIATE_REQUIRES_XTN);
#ifdef SSL_ENABLE_FALSE_START 
    } else if (prefName.Equals("security.ssl.enable_false_start")) {
      mPrefBranch->GetBoolPref("security.ssl.enable_false_start", &enabled);
      SSL_OptionSetDefault(SSL_ENABLE_FALSE_START, enabled);
#endif
    } else if (prefName.Equals("security.OCSP.enabled")
               || prefName.Equals("security.CRL_download.enabled")
               || prefName.Equals("security.fresh_revocation_info.require")
               || prefName.Equals("security.missing_cert_download.enabled")
               || prefName.Equals("security.first_network_revocation_method")
               || prefName.Equals("security.OCSP.require")) {
      MutexAutoLock lock(mutex);
      setValidationOptions(mPrefBranch);
    } else if (prefName.Equals("network.ntlm.send-lm-response")) {
      bool sendLM = false;
      mPrefBranch->GetBoolPref("network.ntlm.send-lm-response", &sendLM);
      nsNTLMAuthModule::SetSendLM(sendLM);
    } else {
      
      for (CipherPref* cp = CipherPrefs; cp->pref; ++cp) {
        if (prefName.Equals(cp->pref)) {
          mPrefBranch->GetBoolPref(cp->pref, &enabled);
          SSL_CipherPrefSetDefault(cp->id, enabled);
          clearSessionCache = true;
          break;
        }
      }
    }
    if (clearSessionCache)
      SSL_ClearSessionCache();
  }
  else if (nsCRT::strcmp(aTopic, PROFILE_CHANGE_NET_TEARDOWN_TOPIC) == 0) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("receiving network teardown topic\n"));
    DoProfileChangeNetTeardown();
  }
  else if (nsCRT::strcmp(aTopic, PROFILE_CHANGE_NET_RESTORE_TOPIC) == 0) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("receiving network restore topic\n"));
    DoProfileChangeNetRestore();
  }

  return NS_OK;
}

 nsresult
nsNSSComponent::GetNewPrompter(nsIPrompt ** result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = nullptr;

  if (!NS_IsMainThread()) {
    NS_ERROR("nsSDRContext::GetNewPrompter called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsresult rv;
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = wwatch->GetNewPrompter(0, result);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

 nsresult
nsNSSComponent::ShowAlertWithConstructedString(const nsString & message)
{
  nsCOMPtr<nsIPrompt> prompter;
  nsresult rv = GetNewPrompter(getter_AddRefs(prompter));
  if (prompter) {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      NS_WARNING("Suppressing alert because PSM UI is forbidden");
      rv = NS_ERROR_UNEXPECTED;
    } else {
      rv = prompter->Alert(nullptr, message.get());
    }
  }
  return rv;
}

NS_IMETHODIMP
nsNSSComponent::ShowAlertFromStringBundle(const char * messageID)
{
  nsString message;
  nsresult rv;

  rv = GetPIPNSSBundleString(messageID, message);
  if (NS_FAILED(rv)) {
    NS_ERROR("GetPIPNSSBundleString failed");
    return rv;
  }

  return ShowAlertWithConstructedString(message);
}

nsresult nsNSSComponent::LogoutAuthenticatedPK11()
{
  nsCOMPtr<nsICertOverrideService> icos =
    do_GetService("@mozilla.org/security/certoverride;1");
  if (icos) {
    icos->ClearValidityOverride(
            NS_LITERAL_CSTRING("all:temporary-certificates"),
            0);
  }

  nsClientAuthRememberService::ClearAllRememberedDecisions();

  return mShutdownObjectList->doPK11Logout();
}

nsresult
nsNSSComponent::RegisterObservers()
{
  

  nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1"));
  NS_ASSERTION(observerService, "could not get observer service");
  if (observerService) {
    mObserversRegistered = true;
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent: adding observers\n"));

    
    
    

    
    

    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

    observerService->AddObserver(this, PROFILE_CHANGE_TEARDOWN_TOPIC, false);
    observerService->AddObserver(this, PROFILE_BEFORE_CHANGE_TOPIC, false);
    observerService->AddObserver(this, PROFILE_DO_CHANGE_TOPIC, false);
    observerService->AddObserver(this, PROFILE_CHANGE_NET_TEARDOWN_TOPIC, false);
    observerService->AddObserver(this, PROFILE_CHANGE_NET_RESTORE_TOPIC, false);
  }
  return NS_OK;
}

nsresult
nsNSSComponent::DeregisterObservers()
{
  if (!mObserversRegistered)
    return NS_OK;

  nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1"));
  NS_ASSERTION(observerService, "could not get observer service");
  if (observerService) {
    mObserversRegistered = false;
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent: removing observers\n"));

    observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

    observerService->RemoveObserver(this, PROFILE_CHANGE_TEARDOWN_TOPIC);
    observerService->RemoveObserver(this, PROFILE_BEFORE_CHANGE_TOPIC);
    observerService->RemoveObserver(this, PROFILE_DO_CHANGE_TOPIC);
    observerService->RemoveObserver(this, PROFILE_CHANGE_NET_TEARDOWN_TOPIC);
    observerService->RemoveObserver(this, PROFILE_CHANGE_NET_RESTORE_TOPIC);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSComponent::RememberCert(CERTCertificate *cert)
{
  nsNSSShutDownPreventionLock locker;

  

  MutexAutoLock lock(mutex);

  if (!hashTableCerts || !cert)
    return NS_OK;
  
  void *found = PL_HashTableLookup(hashTableCerts, (void*)&cert->certKey);
  
  if (found) {
    
    return NS_OK;
  }
  
  CERTCertificate *myDupCert = CERT_DupCertificate(cert);
  
  if (!myDupCert)
    return NS_ERROR_OUT_OF_MEMORY;
  
  if (!PL_HashTableAdd(hashTableCerts, (void*)&myDupCert->certKey, myDupCert)) {
    CERT_DestroyCertificate(myDupCert);
  }
  
  return NS_OK;
}

void
nsNSSComponent::DoProfileChangeNetTeardown()
{
  if (mCertVerificationThread)
    mCertVerificationThread->requestExit();
  mIsNetworkDown = true;
}

void
nsNSSComponent::DoProfileChangeTeardown(nsISupports* aSubject)
{
  mShutdownObjectList->ifPossibleDisallowUI();
}

void
nsNSSComponent::DoProfileBeforeChange(nsISupports* aSubject)
{
  NS_ASSERTION(mIsNetworkDown, "nsNSSComponent relies on profile manager to wait for synchronous shutdown of all network activity");

  bool needsCleanup = true;

  {
    MutexAutoLock lock(mutex);

    if (!mNSSInitialized) {
      
      
      
      needsCleanup = false;
    }
  }
    
  StopCRLUpdateTimer();

  if (needsCleanup) {
    ShutdownNSS();
  }
  mShutdownObjectList->allowUI();
}

void
nsNSSComponent::DoProfileChangeNetRestore()
{
  
  deleteBackgroundThreads();
  createBackgroundThreads();
  mIsNetworkDown = false;
}

NS_IMETHODIMP
nsNSSComponent::IsNSSInitialized(bool *initialized)
{
  MutexAutoLock lock(mutex);
  *initialized = mNSSInitialized;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSComponent::GetDefaultCERTValInParam(RefPtr<nsCERTValInParamWrapper> &out)
{
  MutexAutoLock lock(mutex);
  if (!mNSSInitialized)
      return NS_ERROR_NOT_INITIALIZED;
  out = mDefaultCERTValInParam;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSComponent::GetDefaultCERTValInParamLocalOnly(RefPtr<nsCERTValInParamWrapper> &out)
{
  MutexAutoLock lock(mutex);
  if (!mNSSInitialized)
      return NS_ERROR_NOT_INITIALIZED;
  out = mDefaultCERTValInParamLocalOnly;
  return NS_OK;
}





nsCryptoHash::nsCryptoHash()
  : mHashContext(nullptr)
  , mInitialized(false)
{
}

nsCryptoHash::~nsCryptoHash()
{
  nsNSSShutDownPreventionLock locker;

  if (isAlreadyShutDown())
    return;

  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void nsCryptoHash::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void nsCryptoHash::destructorSafeDestroyNSSReference()
{
  if (isAlreadyShutDown())
    return;

  if (mHashContext)
    HASH_Destroy(mHashContext);
  mHashContext = nullptr;
}

NS_IMPL_ISUPPORTS1(nsCryptoHash, nsICryptoHash)

NS_IMETHODIMP 
nsCryptoHash::Init(uint32_t algorithm)
{
  nsNSSShutDownPreventionLock locker;

  HASH_HashType hashType = (HASH_HashType)algorithm;
  if (mHashContext)
  {
    if ((!mInitialized) && (HASH_GetType(mHashContext) == hashType))
    {
      mInitialized = true;
      HASH_Begin(mHashContext);
      return NS_OK;
    }

    
    
    HASH_Destroy(mHashContext);
    mInitialized = false;
  }

  mHashContext = HASH_Create(hashType);
  if (!mHashContext)
    return NS_ERROR_INVALID_ARG;

  HASH_Begin(mHashContext);
  mInitialized = true;
  return NS_OK; 
}

NS_IMETHODIMP
nsCryptoHash::InitWithString(const nsACString & aAlgorithm)
{
  if (aAlgorithm.LowerCaseEqualsLiteral("md2"))
    return Init(nsICryptoHash::MD2);

  if (aAlgorithm.LowerCaseEqualsLiteral("md5"))
    return Init(nsICryptoHash::MD5);

  if (aAlgorithm.LowerCaseEqualsLiteral("sha1"))
    return Init(nsICryptoHash::SHA1);

  if (aAlgorithm.LowerCaseEqualsLiteral("sha256"))
    return Init(nsICryptoHash::SHA256);

  if (aAlgorithm.LowerCaseEqualsLiteral("sha384"))
    return Init(nsICryptoHash::SHA384);

  if (aAlgorithm.LowerCaseEqualsLiteral("sha512"))
    return Init(nsICryptoHash::SHA512);

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsCryptoHash::Update(const uint8_t *data, uint32_t len)
{
  nsNSSShutDownPreventionLock locker;
  
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  HASH_Update(mHashContext, data, len);
  return NS_OK; 
}

NS_IMETHODIMP
nsCryptoHash::UpdateFromStream(nsIInputStream *data, uint32_t aLen)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  if (!data)
    return NS_ERROR_INVALID_ARG;

  uint64_t n;
  nsresult rv = data->Available(&n);
  if (NS_FAILED(rv))
    return rv;

  
  

  uint64_t len = aLen;
  if (aLen == UINT32_MAX)
    len = n;

  
  
  
  
  
  

  if (n == 0 || n < len)
    return NS_ERROR_NOT_AVAILABLE;
  
  char buffer[NS_CRYPTO_HASH_BUFFER_SIZE];
  uint32_t read, readLimit;
  
  while(NS_SUCCEEDED(rv) && len>0)
  {
    readLimit = (uint32_t)std::min<uint64_t>(NS_CRYPTO_HASH_BUFFER_SIZE, len);
    
    rv = data->Read(buffer, readLimit, &read);
    
    if (NS_SUCCEEDED(rv))
      rv = Update((const uint8_t*)buffer, read);
    
    len -= read;
  }
  
  return rv;
}

NS_IMETHODIMP
nsCryptoHash::Finish(bool ascii, nsACString & _retval)
{
  nsNSSShutDownPreventionLock locker;
  
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;
  
  uint32_t hashLen = 0;
  unsigned char buffer[HASH_LENGTH_MAX];
  unsigned char* pbuffer = buffer;

  HASH_End(mHashContext, pbuffer, &hashLen, HASH_LENGTH_MAX);

  mInitialized = false;

  if (ascii)
  {
    char *asciiData = BTOA_DataToAscii(buffer, hashLen);
    NS_ENSURE_TRUE(asciiData, NS_ERROR_OUT_OF_MEMORY);

    _retval.Assign(asciiData);
    PORT_Free(asciiData);
  }
  else
  {
    _retval.Assign((const char*)buffer, hashLen);
  }

  return NS_OK;
}





NS_IMPL_ISUPPORTS1(nsCryptoHMAC, nsICryptoHMAC)

nsCryptoHMAC::nsCryptoHMAC()
{
  mHMACContext = nullptr;
}

nsCryptoHMAC::~nsCryptoHMAC()
{
  nsNSSShutDownPreventionLock locker;

  if (isAlreadyShutDown())
    return;

  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void nsCryptoHMAC::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void nsCryptoHMAC::destructorSafeDestroyNSSReference()
{
  if (isAlreadyShutDown())
    return;

  if (mHMACContext)
    PK11_DestroyContext(mHMACContext, true);
  mHMACContext = nullptr;
}


NS_IMETHODIMP nsCryptoHMAC::Init(uint32_t aAlgorithm, nsIKeyObject *aKeyObject)
{
  nsNSSShutDownPreventionLock locker;

  if (mHMACContext)
  {
    PK11_DestroyContext(mHMACContext, true);
    mHMACContext = nullptr;
  }

  CK_MECHANISM_TYPE HMACMechType;
  switch (aAlgorithm)
  {
  case nsCryptoHMAC::MD2:
    HMACMechType = CKM_MD2_HMAC; break;
  case nsCryptoHMAC::MD5:
    HMACMechType = CKM_MD5_HMAC; break;
  case nsCryptoHMAC::SHA1:
    HMACMechType = CKM_SHA_1_HMAC; break;
  case nsCryptoHMAC::SHA256:
    HMACMechType = CKM_SHA256_HMAC; break;
  case nsCryptoHMAC::SHA384:
    HMACMechType = CKM_SHA384_HMAC; break;
  case nsCryptoHMAC::SHA512:
    HMACMechType = CKM_SHA512_HMAC; break;
  default:
    return NS_ERROR_INVALID_ARG;
  }

  NS_ENSURE_ARG_POINTER(aKeyObject);

  nsresult rv;

  int16_t keyType;
  rv = aKeyObject->GetType(&keyType);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(keyType == nsIKeyObject::SYM_KEY, NS_ERROR_INVALID_ARG);

  PK11SymKey* key;
  
  rv = aKeyObject->GetKeyObj((void**)&key);
  NS_ENSURE_SUCCESS(rv, rv);

  SECItem rawData;
  rawData.data = 0;
  rawData.len = 0;
  mHMACContext = PK11_CreateContextBySymKey(
      HMACMechType, CKA_SIGN, key, &rawData);
  NS_ENSURE_TRUE(mHMACContext, NS_ERROR_FAILURE);

  SECStatus ss = PK11_DigestBegin(mHMACContext);
  NS_ENSURE_TRUE(ss == SECSuccess, NS_ERROR_FAILURE);

  return NS_OK;
}


NS_IMETHODIMP nsCryptoHMAC::Update(const uint8_t *aData, uint32_t aLen)
{
  nsNSSShutDownPreventionLock locker;

  if (!mHMACContext)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aData)
    return NS_ERROR_INVALID_ARG;

  SECStatus ss = PK11_DigestOp(mHMACContext, aData, aLen);
  NS_ENSURE_TRUE(ss == SECSuccess, NS_ERROR_FAILURE);
  
  return NS_OK;
}


NS_IMETHODIMP nsCryptoHMAC::UpdateFromStream(nsIInputStream *aStream, uint32_t aLen)
{
  if (!mHMACContext)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aStream)
    return NS_ERROR_INVALID_ARG;

  uint64_t n;
  nsresult rv = aStream->Available(&n);
  if (NS_FAILED(rv))
    return rv;

  
  

  uint64_t len = aLen;
  if (aLen == UINT32_MAX)
    len = n;

  
  
  
  
  
  

  if (n == 0 || n < len)
    return NS_ERROR_NOT_AVAILABLE;
  
  char buffer[NS_CRYPTO_HASH_BUFFER_SIZE];
  uint32_t read, readLimit;
  
  while(NS_SUCCEEDED(rv) && len > 0)
  {
    readLimit = (uint32_t)std::min<uint64_t>(NS_CRYPTO_HASH_BUFFER_SIZE, len);
    
    rv = aStream->Read(buffer, readLimit, &read);
    if (read == 0)
      return NS_BASE_STREAM_CLOSED;
    
    if (NS_SUCCEEDED(rv))
      rv = Update((const uint8_t*)buffer, read);
    
    len -= read;
  }
  
  return rv;
}


NS_IMETHODIMP nsCryptoHMAC::Finish(bool aASCII, nsACString & _retval)
{
  nsNSSShutDownPreventionLock locker;

  if (!mHMACContext)
    return NS_ERROR_NOT_INITIALIZED;
  
  uint32_t hashLen = 0;
  unsigned char buffer[HASH_LENGTH_MAX];
  unsigned char* pbuffer = buffer;

  PK11_DigestFinal(mHMACContext, pbuffer, &hashLen, HASH_LENGTH_MAX);
  if (aASCII)
  {
    char *asciiData = BTOA_DataToAscii(buffer, hashLen);
    NS_ENSURE_TRUE(asciiData, NS_ERROR_OUT_OF_MEMORY);

    _retval.Assign(asciiData);
    PORT_Free(asciiData);
  }
  else
  {
    _retval.Assign((const char*)buffer, hashLen);
  }

  return NS_OK;
}


NS_IMETHODIMP nsCryptoHMAC::Reset()
{
  nsNSSShutDownPreventionLock locker;

  SECStatus ss = PK11_DigestBegin(mHMACContext);
  NS_ENSURE_TRUE(ss == SECSuccess, NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(PipUIContext, nsIInterfaceRequestor)

PipUIContext::PipUIContext()
{
}

PipUIContext::~PipUIContext()
{
}


NS_IMETHODIMP PipUIContext::GetInterface(const nsIID & uuid, void * *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = nullptr;

  if (!NS_IsMainThread()) {
    NS_ERROR("PipUIContext::GetInterface called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  if (!uuid.Equals(NS_GET_IID(nsIPrompt)))
    return NS_ERROR_NO_INTERFACE;

  nsIPrompt * prompt = nullptr;
  nsresult rv = nsNSSComponent::GetNewPrompter(&prompt);
  *result = prompt;
  return rv;
}

nsresult 
getNSSDialogs(void **_result, REFNSIID aIID, const char *contract)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("getNSSDialogs called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsresult rv;

  nsCOMPtr<nsISupports> svc = do_GetService(contract, &rv);
  if (NS_FAILED(rv)) 
    return rv;

  rv = svc->QueryInterface(aIID, _result);

  return rv;
}

nsresult
setPassword(PK11SlotInfo *slot, nsIInterfaceRequestor *ctx)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  
  if (PK11_NeedUserInit(slot)) {
    nsITokenPasswordDialogs *dialogs;
    bool canceled;
    NS_ConvertUTF8toUTF16 tokenName(PK11_GetTokenName(slot));

    rv = getNSSDialogs((void**)&dialogs,
                       NS_GET_IID(nsITokenPasswordDialogs),
                       NS_TOKENPASSWORDSDIALOG_CONTRACTID);

    if (NS_FAILED(rv)) goto loser;

    {
      nsPSMUITracker tracker;
      if (tracker.isUIForbidden()) {
        rv = NS_ERROR_NOT_AVAILABLE;
      }
      else {
        rv = dialogs->SetPassword(ctx,
                                  tokenName.get(),
                                  &canceled);
      }
    }
    NS_RELEASE(dialogs);
    if (NS_FAILED(rv)) goto loser;

    if (canceled) { rv = NS_ERROR_NOT_AVAILABLE; goto loser; }
  }
 loser:
  return rv;
}


PSMContentDownloader::PSMContentDownloader(uint32_t type)
  : mByteData(nullptr),
    mType(type),
    mDoSilentDownload(false)
{
}

PSMContentDownloader::~PSMContentDownloader()
{
  if (mByteData)
    nsMemory::Free(mByteData);
}

NS_IMPL_ISUPPORTS2(PSMContentDownloader, nsIStreamListener, nsIRequestObserver)

const int32_t kDefaultCertAllocLength = 2048;

NS_IMETHODIMP
PSMContentDownloader::OnStartRequest(nsIRequest* request, nsISupports* context)
{
  nsresult rv;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CertDownloader::OnStartRequest\n"));
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  if (!channel) return NS_ERROR_FAILURE;

  
  channel->GetURI(getter_AddRefs(mURI));

  int64_t contentLength;
  rv = channel->GetContentLength(&contentLength);
  if (NS_FAILED(rv) || contentLength <= 0)
    contentLength = kDefaultCertAllocLength;
  if (contentLength > INT32_MAX)
    return NS_ERROR_OUT_OF_MEMORY;
  
  mBufferOffset = 0;
  mBufferSize = 0;
  mByteData = (char*) nsMemory::Alloc(contentLength);
  if (!mByteData)
    return NS_ERROR_OUT_OF_MEMORY;
  
  mBufferSize = int32_t(contentLength);
  return NS_OK;
}

NS_IMETHODIMP
PSMContentDownloader::OnDataAvailable(nsIRequest* request,
                                nsISupports* context,
                                nsIInputStream *aIStream,
                                uint64_t aSourceOffset,
                                uint32_t aLength)
{
  if (!mByteData)
    return NS_ERROR_OUT_OF_MEMORY;
  
  uint32_t amt;
  nsresult err;
  
  if ((mBufferOffset + (int32_t)aLength) > mBufferSize) {
      size_t newSize = (mBufferOffset + aLength) *2; 
      char *newBuffer;
      newBuffer = (char*)nsMemory::Realloc(mByteData, newSize);
      if (!newBuffer) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mByteData = newBuffer;
      mBufferSize = newSize;
  }
  
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CertDownloader::OnDataAvailable\n"));
  do {
    err = aIStream->Read(mByteData+mBufferOffset,
                         aLength, &amt);
    if (NS_FAILED(err)) return err;
    if (amt == 0) break;
    
    aLength -= amt;
    mBufferOffset += amt;
    
  } while (aLength > 0);
  
  return NS_OK;
}

NS_IMETHODIMP
PSMContentDownloader::OnStopRequest(nsIRequest* request,
                              nsISupports* context,
                              nsresult aStatus)
{
  nsNSSShutDownPreventionLock locker;
  
  
  if (NS_FAILED(aStatus)){
    handleContentDownloadError(aStatus);
    return aStatus;
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CertDownloader::OnStopRequest\n"));

  nsCOMPtr<nsIX509CertDB> certdb;
  nsCOMPtr<nsICRLManager> crlManager;

  nsresult rv;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  switch (mType) {
  case PSMContentDownloader::X509_CA_CERT:
  case PSMContentDownloader::X509_USER_CERT:
  case PSMContentDownloader::X509_EMAIL_CERT:
    certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
    break;

  case PSMContentDownloader::PKCS7_CRL:
    crlManager = do_GetService(NS_CRLMANAGER_CONTRACTID);

  default:
    break;
  }

  switch (mType) {
  case PSMContentDownloader::X509_CA_CERT:
    return certdb->ImportCertificates((uint8_t*)mByteData, mBufferOffset, mType, ctx); 
  case PSMContentDownloader::X509_USER_CERT:
    return certdb->ImportUserCertificate((uint8_t*)mByteData, mBufferOffset, ctx);
  case PSMContentDownloader::X509_EMAIL_CERT:
    return certdb->ImportEmailCertificate((uint8_t*)mByteData, mBufferOffset, ctx); 
  case PSMContentDownloader::PKCS7_CRL:
    return crlManager->ImportCrl((uint8_t*)mByteData, mBufferOffset, mURI, SEC_CRL_TYPE, mDoSilentDownload, mCrlAutoDownloadKey.get());
  default:
    rv = NS_ERROR_FAILURE;
    break;
  }
  
  return rv;
}


nsresult
PSMContentDownloader::handleContentDownloadError(nsresult errCode)
{
  nsString tmpMessage;
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if(NS_FAILED(rv)){
    return rv;
  }
      
  
  switch (mType){
  case PSMContentDownloader::PKCS7_CRL:

    
    
    nssComponent->GetPIPNSSBundleString("CrlImportFailureNetworkProblem", tmpMessage);
      
    if (mDoSilentDownload) {
      
      nsAutoCString updateErrCntPrefStr(CRL_AUTOUPDATE_ERRCNT_PREF);
      nsAutoCString updateErrDetailPrefStr(CRL_AUTOUPDATE_ERRDETAIL_PREF);
      nsCString errMsg;
      int32_t errCnt;

      nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID,&rv);
      if(NS_FAILED(rv)){
        return rv;
      }
      
      LossyAppendUTF16toASCII(mCrlAutoDownloadKey, updateErrCntPrefStr);
      LossyAppendUTF16toASCII(mCrlAutoDownloadKey, updateErrDetailPrefStr);
      errMsg.AssignWithConversion(tmpMessage.get());
      
      rv = pref->GetIntPref(updateErrCntPrefStr.get(),&errCnt);
      if( (NS_FAILED(rv)) || (errCnt == 0) ){
        pref->SetIntPref(updateErrCntPrefStr.get(),1);
      }else{
        pref->SetIntPref(updateErrCntPrefStr.get(),errCnt+1);
      }
      pref->SetCharPref(updateErrDetailPrefStr.get(),errMsg.get());
      nsCOMPtr<nsIPrefService> prefSvc(do_QueryInterface(pref));
      prefSvc->SavePrefFile(nullptr);
    }else{
      nsString message;
      nssComponent->GetPIPNSSBundleString("CrlImportFailure1x", message);
      message.Append(NS_LITERAL_STRING("\n").get());
      message.Append(tmpMessage);
      nssComponent->GetPIPNSSBundleString("CrlImportFailure2", tmpMessage);
      message.Append(NS_LITERAL_STRING("\n").get());
      message.Append(tmpMessage);
      nsNSSComponent::ShowAlertWithConstructedString(message);
    }
    break;
  default:
    break;
  }

  return NS_OK;

}

void 
PSMContentDownloader::setSilentDownload(bool flag)
{
  mDoSilentDownload = flag;
}

void
PSMContentDownloader::setCrlAutodownloadKey(nsAutoString key)
{
  mCrlAutoDownloadKey = key;
}











uint32_t
getPSMContentType(const char * aContentType)
{ 
  
  
  
  if (!nsCRT::strcasecmp(aContentType, "application/x-x509-ca-cert"))
    return PSMContentDownloader::X509_CA_CERT;
  else if (!nsCRT::strcasecmp(aContentType, "application/x-x509-server-cert"))
    return PSMContentDownloader::X509_SERVER_CERT;
  else if (!nsCRT::strcasecmp(aContentType, "application/x-x509-user-cert"))
    return PSMContentDownloader::X509_USER_CERT;
  else if (!nsCRT::strcasecmp(aContentType, "application/x-x509-email-cert"))
    return PSMContentDownloader::X509_EMAIL_CERT;
  else if (!nsCRT::strcasecmp(aContentType, "application/x-pkcs7-crl"))
    return PSMContentDownloader::PKCS7_CRL;
  else if (!nsCRT::strcasecmp(aContentType, "application/x-x509-crl"))
    return PSMContentDownloader::PKCS7_CRL;
  else if (!nsCRT::strcasecmp(aContentType, "application/pkix-crl"))
    return PSMContentDownloader::PKCS7_CRL;
  return PSMContentDownloader::UNKNOWN_TYPE;
}


NS_IMPL_ISUPPORTS2(PSMContentListener,
                   nsIURIContentListener,
                   nsISupportsWeakReference) 

PSMContentListener::PSMContentListener()
{
  mLoadCookie = nullptr;
  mParentContentListener = nullptr;
}

PSMContentListener::~PSMContentListener()
{
}

nsresult
PSMContentListener::init()
{
  return NS_OK;
}

NS_IMETHODIMP
PSMContentListener::OnStartURIOpen(nsIURI *aURI, bool *aAbortOpen)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
PSMContentListener::IsPreferred(const char * aContentType,
                                 char ** aDesiredContentType,
                                 bool * aCanHandleContent)
{
  return CanHandleContent(aContentType, true,
                          aDesiredContentType, aCanHandleContent);
}

NS_IMETHODIMP
PSMContentListener::CanHandleContent(const char * aContentType,
                                      bool aIsContentPreferred,
                                      char ** aDesiredContentType,
                                      bool * aCanHandleContent)
{
  uint32_t type;
  type = getPSMContentType(aContentType);
  if (type == PSMContentDownloader::UNKNOWN_TYPE) {
    *aCanHandleContent = false;
  } else {
    *aCanHandleContent = true;
  }
  return NS_OK;
}

NS_IMETHODIMP
PSMContentListener::DoContent(const char * aContentType,
                               bool aIsContentPreferred,
                               nsIRequest * aRequest,
                               nsIStreamListener ** aContentHandler,
                               bool * aAbortProcess)
{
  PSMContentDownloader *downLoader;
  uint32_t type;
  type = getPSMContentType(aContentType);
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("PSMContentListener::DoContent\n"));
  if (type != PSMContentDownloader::UNKNOWN_TYPE) {
    downLoader = new PSMContentDownloader(type);
    if (downLoader) {
      downLoader->QueryInterface(NS_GET_IID(nsIStreamListener), 
                                            (void **)aContentHandler);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
PSMContentListener::GetLoadCookie(nsISupports * *aLoadCookie)
{
  *aLoadCookie = mLoadCookie;
  NS_IF_ADDREF(*aLoadCookie);
  return NS_OK;
}

NS_IMETHODIMP
PSMContentListener::SetLoadCookie(nsISupports * aLoadCookie)
{
  mLoadCookie = aLoadCookie;
  return NS_OK;
}

NS_IMETHODIMP
PSMContentListener::GetParentContentListener(nsIURIContentListener ** aContentListener)
{
  *aContentListener = mParentContentListener;
  NS_IF_ADDREF(*aContentListener);
  return NS_OK;
}

NS_IMETHODIMP
PSMContentListener::SetParentContentListener(nsIURIContentListener * aContentListener)
{
  mParentContentListener = aContentListener;
  return NS_OK;
}
