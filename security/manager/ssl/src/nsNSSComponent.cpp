





#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1
#endif

#include "nsNSSComponent.h"

#include "CertVerifier.h"
#include "nsCertVerificationThread.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsICertOverrideService.h"
#include "mozilla/Preferences.h"
#include "nsThreadUtils.h"
#include "mozilla/PublicSSL.h"
#include "mozilla/StaticPtr.h"

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
#include "nsNTLMAuthModule.h"
#include "nsIFile.h"
#include "nsIProperties.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsCertificatePrincipal.h"
#include "nsIBufEntropyCollector.h"
#include "nsITokenPasswordDialogs.h"
#include "nsServiceManagerUtils.h"
#include "nsNSSShutDown.h"
#include "GeneratedEvents.h"
#include "SharedSSLState.h"

#include "nss.h"
#include "ssl.h"
#include "sslproto.h"
#include "secmod.h"
#include "secmime.h"
#include "ocsp.h"
#include "secerr.h"
#include "sslerr.h"

#include "nsXULAppAPI.h"

#ifdef XP_WIN
#include "nsILocalFileWin.h"
#endif

#include "p12plcy.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::psm;

#ifdef MOZ_LOGGING
PRLogModuleInfo* gPIPNSSLog = nullptr;
#endif

int nsNSSComponent::mInstanceCount = 0;

#ifndef NSS_NO_LIBPKIX
bool nsNSSComponent::globalConstFlagUsePKIXVerification = false;
#endif


extern char* pk11PasswordPrompt(PK11SlotInfo *slot, PRBool retry, void *arg);

#define PIPNSS_STRBUNDLE_URL "chrome://pipnss/locale/pipnss.properties"
#define NSSERR_STRBUNDLE_URL "chrome://pipnss/locale/nsserrors.properties"

#ifndef MOZ_DISABLE_CRYPTOLEGACY


class nsTokenEventRunnable : public nsIRunnable {
public:
  nsTokenEventRunnable(const nsAString &aType, const nsAString &aTokenName);
  virtual ~nsTokenEventRunnable();

  NS_IMETHOD Run ();
  NS_DECL_THREADSAFE_ISUPPORTS
private:
  nsString mType;
  nsString mTokenName;
};


NS_IMPL_ISUPPORTS1(nsTokenEventRunnable, nsIRunnable)

nsTokenEventRunnable::nsTokenEventRunnable(const nsAString &aType, 
   const nsAString &aTokenName): mType(aType), mTokenName(aTokenName) { }

nsTokenEventRunnable::~nsTokenEventRunnable() { }



NS_IMETHODIMP
nsTokenEventRunnable::Run()
{ 
  static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

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
  mObserversRegistered = false;

#ifndef NSS_NO_LIBPKIX
  
  
  memset(&mIdentityInfoCallOnce, 0, sizeof(PRCallOnceType));
#endif

  NS_ASSERTION( (0 == mInstanceCount), "nsNSSComponent is a singleton, but instantiated multiple times!");
  ++mInstanceCount;
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


typedef struct {
  const char* pref;
  long id;
  bool enabledByDefault;
} CipherPref;



static const CipherPref sCipherPrefs[] = {
 { "security.ssl3.ecdhe_rsa_aes_128_gcm_sha256",
   TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, true },
 { "security.ssl3.ecdhe_ecdsa_aes_128_gcm_sha256",
   TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, true },
 { "security.ssl3.ecdhe_rsa_aes_128_sha",
   TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA, true },
 { "security.ssl3.ecdhe_ecdsa_aes_128_sha",
   TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA, true },

 { "security.ssl3.ecdhe_rsa_aes_256_sha",
   TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA, true },
 { "security.ssl3.ecdhe_ecdsa_aes_256_sha",
   TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA, true },

 { "security.ssl3.ecdhe_rsa_des_ede3_sha",
   TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA, true }, 

 { "security.ssl3.dhe_rsa_aes_128_sha",
   TLS_DHE_RSA_WITH_AES_128_CBC_SHA, true },
 { "security.ssl3.dhe_rsa_camellia_128_sha",
   TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA, true },

 { "security.ssl3.dhe_rsa_aes_256_sha",
   TLS_DHE_RSA_WITH_AES_256_CBC_SHA, true },
 { "security.ssl3.dhe_rsa_camellia_256_sha",
   TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA, true },

 { "security.ssl3.dhe_rsa_des_ede3_sha",
   SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA, true }, 

 { "security.ssl3.dhe_dss_aes_128_sha",
   TLS_DHE_DSS_WITH_AES_128_CBC_SHA, true }, 
 { "security.ssl3.dhe_dss_aes_256_sha",
   TLS_DHE_DSS_WITH_AES_256_CBC_SHA, true }, 

 { "security.ssl3.ecdhe_rsa_rc4_128_sha",
   TLS_ECDHE_RSA_WITH_RC4_128_SHA, true }, 
 { "security.ssl3.ecdhe_ecdsa_rc4_128_sha",
   TLS_ECDHE_ECDSA_WITH_RC4_128_SHA, true }, 

 { "security.ssl3.rsa_aes_128_sha",
   TLS_RSA_WITH_AES_128_CBC_SHA, true }, 
 { "security.ssl3.rsa_camellia_128_sha",
   TLS_RSA_WITH_CAMELLIA_128_CBC_SHA, true }, 
 { "security.ssl3.rsa_aes_256_sha",
   TLS_RSA_WITH_AES_256_CBC_SHA, true }, 
 { "security.ssl3.rsa_camellia_256_sha",
   TLS_RSA_WITH_CAMELLIA_256_CBC_SHA, true }, 
 { "security.ssl3.rsa_des_ede3_sha",
   SSL_RSA_WITH_3DES_EDE_CBC_SHA, true }, 

 { "security.ssl3.rsa_rc4_128_sha",
   SSL_RSA_WITH_RC4_128_SHA, true }, 
 { "security.ssl3.rsa_rc4_128_md5",
   SSL_RSA_WITH_RC4_128_MD5, true }, 

 

 {"security.ssl3.rsa_fips_des_ede3_sha", SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA},
 {"security.ssl3.dhe_dss_camellia_256_sha", TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_aes_256_sha", TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_aes_128_sha", TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_des_ede3_sha", TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.ecdh_ecdsa_rc4_128_sha", TLS_ECDH_ECDSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.ecdh_rsa_aes_256_sha", TLS_ECDH_RSA_WITH_AES_256_CBC_SHA}, 
 {"security.ssl3.ecdh_rsa_aes_128_sha", TLS_ECDH_RSA_WITH_AES_128_CBC_SHA}, 
 {"security.ssl3.ecdh_rsa_des_ede3_sha", TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA}, 
 {"security.ssl3.ecdh_rsa_rc4_128_sha", TLS_ECDH_RSA_WITH_RC4_128_SHA}, 
 {"security.ssl3.dhe_dss_camellia_128_sha", TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA}, 
 {"security.ssl3.rsa_seed_sha", TLS_RSA_WITH_SEED_CBC_SHA}, 
 {nullptr, 0} 
};

static void
setNonPkixOcspEnabled(int32_t ocspEnabled)
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
#define OCSP_REQUIRED_DEFAULT false
#define FRESH_REVOCATION_REQUIRED_DEFAULT false
#define MISSING_CERT_DOWNLOAD_DEFAULT false
#define FIRST_REVO_METHOD_DEFAULT "ocsp"
#define USE_NSS_LIBPKIX_DEFAULT false
#define OCSP_STAPLING_ENABLED_DEFAULT true

static const bool SUPPRESS_WARNING_PREF_DEFAULT = false;
static const bool MD5_ENABLED_DEFAULT = false;
static const bool REQUIRE_SAFE_NEGOTIATION_DEFAULT = false;
static const bool ALLOW_UNRESTRICTED_RENEGO_DEFAULT = false;
static const bool FALSE_START_ENABLED_DEFAULT = true;
static const bool CIPHER_ENABLED_DEFAULT = false;

namespace {

class CipherSuiteChangeObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  virtual ~CipherSuiteChangeObserver() {}
  static nsresult StartObserve();
  static nsresult StopObserve();

private:
  static StaticRefPtr<CipherSuiteChangeObserver> sObserver;
  CipherSuiteChangeObserver() {}
};

NS_IMPL_ISUPPORTS1(CipherSuiteChangeObserver, nsIObserver)


StaticRefPtr<CipherSuiteChangeObserver> CipherSuiteChangeObserver::sObserver;


nsresult
CipherSuiteChangeObserver::StartObserve()
{
  NS_ASSERTION(NS_IsMainThread(), "CipherSuiteChangeObserver::StartObserve() can only be accessed in main thread");
  if (!sObserver) {
    nsRefPtr<CipherSuiteChangeObserver> observer = new CipherSuiteChangeObserver();
    nsresult rv = Preferences::AddStrongObserver(observer.get(), "security.");
    if (NS_FAILED(rv)) {
      sObserver = nullptr;
      return rv;
    }
    sObserver = observer;
  }
  return NS_OK;
}


nsresult
CipherSuiteChangeObserver::StopObserve()
{
  NS_ASSERTION(NS_IsMainThread(), "CipherSuiteChangeObserver::StopObserve() can only be accessed in main thread");
  if (sObserver) {
    nsresult rv = Preferences::RemoveObserver(sObserver.get(), "security.");
    sObserver = nullptr;
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  return NS_OK;
}

nsresult
CipherSuiteChangeObserver::Observe(nsISupports *aSubject,
                                   const char *aTopic,
                                   const PRUnichar *someData)
{
  NS_ASSERTION(NS_IsMainThread(), "CipherSuiteChangeObserver::Observe can only be accessed in main thread");
  if (nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    NS_ConvertUTF16toUTF8  prefName(someData);
    
    for (const CipherPref* cp = sCipherPrefs; cp->pref; ++cp) {
      if (prefName.Equals(cp->pref)) {
        bool cipherEnabled = Preferences::GetBool(cp->pref,
                                                  cp->enabledByDefault);
        SSL_CipherPrefSetDefault(cp->id, cipherEnabled);
        SSL_ClearSessionCache();
        break;
      }
    }
  }
  return NS_OK;
}

} 


void nsNSSComponent::setValidationOptions()
{
  nsNSSShutDownPreventionLock locker;

  bool crlDownloading = Preferences::GetBool("security.CRL_download.enabled",
                                             CRL_DOWNLOAD_DEFAULT);
  
  int32_t ocspEnabled = Preferences::GetInt("security.OCSP.enabled",
                                            OCSP_ENABLED_DEFAULT);

  bool ocspRequired = Preferences::GetBool("security.OCSP.require",
                                           OCSP_REQUIRED_DEFAULT);
  bool anyFreshRequired = Preferences::GetBool("security.fresh_revocation_info.require",
                                               FRESH_REVOCATION_REQUIRED_DEFAULT);
  bool aiaDownloadEnabled = Preferences::GetBool("security.missing_cert_download.enabled",
                                                 MISSING_CERT_DOWNLOAD_DEFAULT);

  nsCString firstNetworkRevo =
    Preferences::GetCString("security.first_network_revocation_method");
  if (firstNetworkRevo.IsEmpty()) {
    firstNetworkRevo = FIRST_REVO_METHOD_DEFAULT;
  }

  bool ocspStaplingEnabled = Preferences::GetBool("security.ssl.enable_ocsp_stapling",
                                                  OCSP_STAPLING_ENABLED_DEFAULT);
  if (!ocspEnabled) {
    ocspStaplingEnabled = false;
  }
  PublicSSLState()->SetOCSPStaplingEnabled(ocspStaplingEnabled);
  PrivateSSLState()->SetOCSPStaplingEnabled(ocspStaplingEnabled);

  setNonPkixOcspEnabled(ocspEnabled);

  CERT_SetOCSPFailureMode( ocspRequired ?
                           ocspMode_FailureIsVerificationFailure
                           : ocspMode_FailureIsNotAVerificationFailure);

  int OCSPTimeoutSeconds = 3;
  if (ocspRequired || anyFreshRequired) {
    OCSPTimeoutSeconds = 10;
  }
  CERT_SetOCSPTimeout(OCSPTimeoutSeconds);

  
  bool ocspGetEnabled = Preferences::GetBool("security.OCSP.GET.enabled", false);
  CERT_ForcePostMethodForOCSP(!ocspGetEnabled);

  mDefaultCertVerifier = new CertVerifier(
      aiaDownloadEnabled ? 
        CertVerifier::missing_cert_download_on : CertVerifier::missing_cert_download_off,
      crlDownloading ?
        CertVerifier::crl_download_allowed : CertVerifier::crl_local_only,
      ocspEnabled ? 
        CertVerifier::ocsp_on : CertVerifier::ocsp_off,
      ocspRequired ? 
        CertVerifier::ocsp_strict : CertVerifier::ocsp_relaxed,
      anyFreshRequired ?
        CertVerifier::any_revo_strict : CertVerifier::any_revo_relaxed,
      firstNetworkRevo.get(),
      ocspGetEnabled ?
        CertVerifier::ocsp_get_enabled : CertVerifier::ocsp_get_disabled);

  



  SSL_ClearSessionCache();
}




nsresult
nsNSSComponent::setEnabledTLSVersions()
{
  
  static const int32_t PSM_DEFAULT_MIN_TLS_VERSION = 0;
  static const int32_t PSM_DEFAULT_MAX_TLS_VERSION = 3;

  int32_t minVersion = Preferences::GetInt("security.tls.version.min",
                                           PSM_DEFAULT_MIN_TLS_VERSION);
  int32_t maxVersion = Preferences::GetInt("security.tls.version.max",
                                           PSM_DEFAULT_MAX_TLS_VERSION);

  
  minVersion += SSL_LIBRARY_VERSION_3_0;
  maxVersion += SSL_LIBRARY_VERSION_3_0;

  SSLVersionRange range = { (uint16_t) minVersion, (uint16_t) maxVersion };

  if (minVersion != (int32_t) range.min || 
      maxVersion != (int32_t) range.max || 
      SSL_VersionRangeSetDefault(ssl_variant_stream, &range) != SECSuccess) {
    range.min = SSL_LIBRARY_VERSION_3_0 + PSM_DEFAULT_MIN_TLS_VERSION;
    range.max = SSL_LIBRARY_VERSION_3_0 + PSM_DEFAULT_MAX_TLS_VERSION;
    if (SSL_VersionRangeSetDefault(ssl_variant_stream, &range)
          != SECSuccess) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
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
  
  int32_t ocspEnabled = Preferences::GetInt("security.OCSP.enabled",
                                            OCSP_ENABLED_DEFAULT);

  setNonPkixOcspEnabled(ocspEnabled);

  if (ocspEnabled)
    SSL_ClearSessionCache();

  return NS_OK;
}

nsresult
nsNSSComponent::InitializeNSS(bool showWarningBox)
{
  
  

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent::InitializeNSS\n"));

  static_assert(nsINSSErrorsService::NSS_SEC_ERROR_BASE == SEC_ERROR_BASE &&
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

#ifndef NSS_NO_LIBPKIX
    globalConstFlagUsePKIXVerification =
      Preferences::GetBool("security.use_libpkix_verification", USE_NSS_LIBPKIX_DEFAULT);
#endif

    bool suppressWarningPref =
      Preferences::GetBool("security.suppress_nss_rw_impossible_warning",
                           SUPPRESS_WARNING_PREF_DEFAULT);

    

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS Initialization beginning\n"));

    
    
    
    
    

    ConfigureInternalPKCS11Token();

    
    
    
    
    
    uint32_t init_flags = NSS_INIT_NOROOTINIT | NSS_INIT_OPTIMIZESPACE;
    SECStatus init_rv = ::NSS_Initialize(profileStr.get(), "", "",
                                         SECMOD_DB, init_flags);

    if (init_rv != SECSuccess) {
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("can not init NSS r/w in %s\n", profileStr.get()));

      if (suppressWarningPref) {
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

      PK11_SetPasswordFunc(PK11PasswordPrompt);

      SharedSSLState::GlobalInit();

      
      Preferences::AddStrongObserver(this, "security.");

      SSL_OptionSetDefault(SSL_ENABLE_SSL2, false);
      SSL_OptionSetDefault(SSL_V2_COMPATIBLE_HELLO, false);

      rv = setEnabledTLSVersions();
      if (NS_FAILED(rv)) {
        nsPSMInitPanic::SetPanic();
        return NS_ERROR_UNEXPECTED;
      }

      bool md5Enabled = Preferences::GetBool("security.enable_md5_signatures",
                                             MD5_ENABLED_DEFAULT);
      ConfigureMD5(md5Enabled);

      SSL_OptionSetDefault(SSL_ENABLE_SESSION_TICKETS, true);

      bool requireSafeNegotiation =
        Preferences::GetBool("security.ssl.require_safe_negotiation",
                             REQUIRE_SAFE_NEGOTIATION_DEFAULT);
      SSL_OptionSetDefault(SSL_REQUIRE_SAFE_NEGOTIATION, requireSafeNegotiation);

      bool allowUnrestrictedRenego =
        Preferences::GetBool("security.ssl.allow_unrestricted_renego_everywhere__temporarily_available_pref",
                             ALLOW_UNRESTRICTED_RENEGO_DEFAULT);
      SSL_OptionSetDefault(SSL_ENABLE_RENEGOTIATION,
                           allowUnrestrictedRenego ?
                             SSL_RENEGOTIATE_UNRESTRICTED :
                             SSL_RENEGOTIATE_REQUIRES_XTN);




      SSL_OptionSetDefault(SSL_ENABLE_FALSE_START, false);

      if (NS_FAILED(InitializeCipherSuite())) {
        PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("Unable to initialize cipher suite settings\n"));
        return NS_ERROR_FAILURE;
      }

      
      setValidationOptions();

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

  if (mNSSInitialized) {
    mNSSInitialized = false;

    PK11_SetPasswordFunc((PK11PasswordFunc)nullptr);
    mHttpForNSS.unregisterHttpClient();

    Preferences::RemoveObserver(this, "security.");
    if (NS_FAILED(CipherSuiteChangeObserver::StopObserve())) {
      PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("nsNSSComponent::ShutdownNSS cannot stop observing cipher suite change\n"));
    }

#ifndef MOZ_DISABLE_CRYPTOLEGACY
    ShutdownSmartCardThreads();
#endif
    SSL_ClearSessionCache();
    UnloadLoadableRoots();
#ifndef NSS_NO_LIBPKIX
    CleanupIdentityInfo();
#endif
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

static const bool SEND_LM_DEFAULT = false;

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

  bool sendLM = Preferences::GetBool("network.ntlm.send-lm-response",
                                     SEND_LM_DEFAULT);
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
  
  createBackgroundThreads();
  if (!mCertVerificationThread)
  {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("NSS init, could not create threads\n"));

    DeregisterObservers();
    mPIPNSSBundle = nullptr;
    return NS_ERROR_OUT_OF_MEMORY;
  }

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


NS_IMPL_ISUPPORTS5(nsNSSComponent,
                   nsISignatureVerifier,
                   nsIEntropyCollector,
                   nsINSSComponent,
                   nsIObserver,
                   nsISupportsWeakReference)



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
  }
  else if (nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {

    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsNSSComponent: XPCom shutdown observed\n"));

    

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
    NS_ConvertUTF16toUTF8  prefName(someData);

    if (prefName.Equals("security.tls.version.min") ||
        prefName.Equals("security.tls.version.max")) {
      (void) setEnabledTLSVersions();
      clearSessionCache = true;
    } else if (prefName.Equals("security.enable_md5_signatures")) {
      bool md5Enabled = Preferences::GetBool("security.enable_md5_signatures",
                                             MD5_ENABLED_DEFAULT);
      ConfigureMD5(md5Enabled);
      clearSessionCache = true;
    } else if (prefName.Equals("security.ssl.require_safe_negotiation")) {
      bool requireSafeNegotiation =
        Preferences::GetBool("security.ssl.require_safe_negotiation",
                             REQUIRE_SAFE_NEGOTIATION_DEFAULT);
      SSL_OptionSetDefault(SSL_REQUIRE_SAFE_NEGOTIATION, requireSafeNegotiation);
    } else if (prefName.Equals("security.ssl.allow_unrestricted_renego_everywhere__temporarily_available_pref")) {
      bool allowUnrestrictedRenego =
        Preferences::GetBool("security.ssl.allow_unrestricted_renego_everywhere__temporarily_available_pref",
                             ALLOW_UNRESTRICTED_RENEGO_DEFAULT);
      SSL_OptionSetDefault(SSL_ENABLE_RENEGOTIATION,
                           allowUnrestrictedRenego ?
                             SSL_RENEGOTIATE_UNRESTRICTED :
                             SSL_RENEGOTIATE_REQUIRES_XTN);
    } else if (prefName.Equals("security.ssl.enable_false_start")) {



      SSL_OptionSetDefault(SSL_ENABLE_FALSE_START, false);
    } else if (prefName.Equals("security.OCSP.enabled")
               || prefName.Equals("security.CRL_download.enabled")
               || prefName.Equals("security.fresh_revocation_info.require")
               || prefName.Equals("security.missing_cert_download.enabled")
               || prefName.Equals("security.first_network_revocation_method")
               || prefName.Equals("security.OCSP.require")
               || prefName.Equals("security.OCSP.GET.enabled")
               || prefName.Equals("security.ssl.enable_ocsp_stapling")) {
      MutexAutoLock lock(mutex);
      setValidationOptions();
    } else if (prefName.Equals("network.ntlm.send-lm-response")) {
      bool sendLM = Preferences::GetBool("network.ntlm.send-lm-response",
                                         SEND_LM_DEFAULT);
      nsNTLMAuthModule::SetSendLM(sendLM);
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
nsNSSComponent::GetDefaultCertVerifier(RefPtr<CertVerifier> &out)
{
  MutexAutoLock lock(mutex);
  if (!mNSSInitialized)
      return NS_ERROR_NOT_INITIALIZED;
  out = mDefaultCertVerifier;
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(PipUIContext, nsIInterfaceRequestor)

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

namespace mozilla {
namespace psm {

void ConfigureMD5(bool enabled)
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

nsresult InitializeCipherSuite()
{
  NS_ASSERTION(NS_IsMainThread(), "InitializeCipherSuite() can only be accessed in main thread");

  if (NSS_SetDomesticPolicy() != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

  
  for (uint16_t i = 0; i < SSL_NumImplementedCiphers; ++i) {
    uint16_t cipher_id = SSL_ImplementedCiphers[i];
    SSL_CipherPrefSetDefault(cipher_id, false);
  }

  
  for (const CipherPref* cp = sCipherPrefs; cp->pref; ++cp) {
    bool cipherEnabled = Preferences::GetBool(cp->pref, cp->enabledByDefault);
    SSL_CipherPrefSetDefault(cp->id, cipherEnabled);
  }

  
  SEC_PKCS12EnableCipher(PKCS12_RC4_40, 1);
  SEC_PKCS12EnableCipher(PKCS12_RC4_128, 1);
  SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_40, 1);
  SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_128, 1);
  SEC_PKCS12EnableCipher(PKCS12_DES_56, 1);
  SEC_PKCS12EnableCipher(PKCS12_DES_EDE3_168, 1);
  SEC_PKCS12SetPreferredCipher(PKCS12_DES_EDE3_168, 1);
  PORT_SetUCS2_ASCIIConversionFunction(pip_ucs2_ascii_conversion_fn);

  
  return CipherSuiteChangeObserver::StartObserve();
}

} 
} 
