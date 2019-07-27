





#ifndef _nsNSSComponent_h_
#define _nsNSSComponent_h_

#include "mozilla/Mutex.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "nsIEntropyCollector.h"
#include "nsIStringBundle.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsINSSErrorsService.h"
#include "nsNSSCallbacks.h"
#include "SharedCertVerifier.h"
#include "nsNSSHelper.h"
#include "nsClientAuthRemember.h"
#include "prerror.h"

class nsIDOMWindow;
class nsIPrompt;
class SmartCardThreadList;

namespace mozilla { namespace psm {

MOZ_WARN_UNUSED_RESULT
  ::mozilla::TemporaryRef<mozilla::psm::SharedCertVerifier>
  GetDefaultCertVerifier();

} } 


#define NS_NSSCOMPONENT_CID \
{0x4cb64dfd, 0xca98, 0x4e24, {0xbe, 0xfd, 0x0d, 0x92, 0x85, 0xa3, 0x3b, 0xcb}}

#define PSM_COMPONENT_CONTRACTID "@mozilla.org/psm;1"




#define NS_INSSCOMPONENT_IID_STR "e60602a8-97a3-4fe7-b5b7-56bc6ce87ab4"
#define NS_INSSCOMPONENT_IID \
  { 0xe60602a8, 0x97a3, 0x4fe7, \
    { 0xb5, 0xb7, 0x56, 0xbc, 0x6c, 0xe8, 0x7a, 0xb4 } }

enum EnsureNSSOperator
{
  nssLoadingComponent = 0,
  nssInitSucceeded = 1,
  nssInitFailed = 2,
  nssShutdown = 3,
  nssEnsure = 100,
  nssEnsureOnChromeOnly = 101
};

extern bool EnsureNSSInitializedChromeOrContent();

extern bool EnsureNSSInitialized(EnsureNSSOperator op);

class nsNSSComponent;

class NS_NO_VTABLE nsINSSComponent : public nsISupports {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INSSCOMPONENT_IID)

  NS_IMETHOD ShowAlertFromStringBundle(const char* messageID) = 0;

  NS_IMETHOD GetPIPNSSBundleString(const char* name,
                                   nsAString& outString) = 0;
  NS_IMETHOD PIPBundleFormatStringFromName(const char* name,
                                           const char16_t** params,
                                           uint32_t numParams,
                                           nsAString& outString) = 0;

  NS_IMETHOD GetNSSBundleString(const char* name,
                                nsAString& outString) = 0;
  NS_IMETHOD NSSBundleFormatStringFromName(const char* name,
                                           const char16_t** params,
                                           uint32_t numParams,
                                           nsAString& outString) = 0;

  NS_IMETHOD LogoutAuthenticatedPK11() = 0;

#ifndef MOZ_NO_SMART_CARDS
  NS_IMETHOD LaunchSmartCardThread(SECMODModule* module) = 0;

  NS_IMETHOD ShutdownSmartCardThread(SECMODModule* module) = 0;
#endif

  NS_IMETHOD IsNSSInitialized(bool* initialized) = 0;

  virtual ::mozilla::TemporaryRef<mozilla::psm::SharedCertVerifier>
    GetDefaultCertVerifier() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINSSComponent, NS_INSSCOMPONENT_IID)

class nsNSSShutDownList;
class nsCertVerificationThread;


class nsNSSComponent MOZ_FINAL : public nsIEntropyCollector,
                                 public nsINSSComponent,
                                 public nsIObserver,
                                 public nsSupportsWeakReference
{
  typedef mozilla::Mutex Mutex;

public:
  NS_DEFINE_STATIC_CID_ACCESSOR( NS_NSSCOMPONENT_CID )

  nsNSSComponent();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIENTROPYCOLLECTOR
  NS_DECL_NSIOBSERVER

  NS_METHOD Init();

  static nsresult GetNewPrompter(nsIPrompt** result);
  static nsresult ShowAlertWithConstructedString(const nsString& message);
  NS_IMETHOD ShowAlertFromStringBundle(const char* messageID);

  NS_IMETHOD GetPIPNSSBundleString(const char* name,
                                   nsAString& outString);
  NS_IMETHOD PIPBundleFormatStringFromName(const char* name,
                                           const char16_t** params,
                                           uint32_t numParams,
                                           nsAString& outString);
  NS_IMETHOD GetNSSBundleString(const char* name, nsAString& outString);
  NS_IMETHOD NSSBundleFormatStringFromName(const char* name,
                                           const char16_t** params,
                                           uint32_t numParams,
                                           nsAString& outString);
  NS_IMETHOD LogoutAuthenticatedPK11();

#ifndef MOZ_NO_SMART_CARDS
  NS_IMETHOD LaunchSmartCardThread(SECMODModule* module);
  NS_IMETHOD ShutdownSmartCardThread(SECMODModule* module);
  void LaunchSmartCardThreads();
  void ShutdownSmartCardThreads();
  nsresult DispatchEventToWindow(nsIDOMWindow* domWin,
                                 const nsAString& eventType,
                                 const nsAString& token);
#endif

  NS_IMETHOD IsNSSInitialized(bool* initialized);

  ::mozilla::TemporaryRef<mozilla::psm::SharedCertVerifier>
    GetDefaultCertVerifier() MOZ_OVERRIDE;

  
  static bool AreAnyWeakCiphersEnabled();
  static void UseWeakCiphersOnSocket(PRFileDesc* fd);

protected:
  virtual ~nsNSSComponent();

private:
  nsresult InitializeNSS();
  void ShutdownNSS();

  void LoadLoadableRoots();
  void UnloadLoadableRoots();
  void setValidationOptions(bool isInitialSetting,
                            const mozilla::MutexAutoLock& lock);
  nsresult setEnabledTLSVersions();
  nsresult InitializePIPNSSBundle();
  nsresult ConfigureInternalPKCS11Token();
  nsresult RegisterObservers();
  nsresult DeregisterObservers();

  
  
  void DoProfileChangeNetTeardown();
  void DoProfileChangeTeardown(nsISupports* aSubject);
  void DoProfileBeforeChange(nsISupports* aSubject);
  void DoProfileChangeNetRestore();

  Mutex mutex;

  nsCOMPtr<nsIStringBundle> mPIPNSSBundle;
  nsCOMPtr<nsIStringBundle> mNSSErrorsBundle;
  bool mNSSInitialized;
  bool mObserversRegistered;
  static int mInstanceCount;
  nsNSSShutDownList* mShutdownObjectList;
#ifndef MOZ_NO_SMART_CARDS
  SmartCardThreadList* mThreadList;
#endif
  bool mIsNetworkDown;

  void deleteBackgroundThreads();
  void createBackgroundThreads();
  nsCertVerificationThread* mCertVerificationThread;

  nsNSSHttpInterface mHttpForNSS;
  mozilla::RefPtr<mozilla::psm::SharedCertVerifier> mDefaultCertVerifier;

  static PRStatus IdentityInfoInit(void);
};

class nsNSSErrors
{
public:
  static const char* getDefaultErrorStringName(PRErrorCode err);
  static const char* getOverrideErrorStringName(PRErrorCode aErrorCode);
  static nsresult getErrorMessageFromCode(PRErrorCode err,
                                          nsINSSComponent* component,
                                          nsString& returnedMessage);
};

class nsPSMInitPanic
{
private:
  static bool isPanic;
public:
  static void SetPanic() {isPanic = true;}
  static bool GetPanic() {return isPanic;}
};

#endif 
