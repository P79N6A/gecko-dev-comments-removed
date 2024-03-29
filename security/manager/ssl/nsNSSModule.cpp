





#include "mozilla/ModuleUtils.h"

#include "nsNSSComponent.h"
#include "nsSSLSocketProvider.h"
#include "nsTLSSocketProvider.h"
#include "nsKeygenHandler.h"

#include "nsSDR.h"

#include "nsPK11TokenDB.h"
#include "nsPKCS11Slot.h"
#include "nsNSSCertificate.h"
#include "nsNSSCertificateFakeTransport.h"
#include "nsNSSCertificateDB.h"
#ifdef MOZ_XUL
#include "nsCertTree.h"
#endif
#include "nsCrypto.h"
#include "nsCryptoHash.h"

#include "nsDOMCID.h"
#include "nsNetCID.h"
#include "nsCertPicker.h"
#include "nsCURILoader.h"
#include "nsICategoryManager.h"
#include "nsNTLMAuthModule.h"
#include "nsKeyModule.h"
#include "nsDataSignatureVerifier.h"
#include "nsCertOverrideService.h"
#include "nsRandomGenerator.h"
#include "nsSSLStatus.h"
#include "TransportSecurityInfo.h"
#include "NSSErrorsService.h"
#include "nsNSSVersion.h"
#include "CertBlocklist.h"
#include "nsEntropyCollector.h"
#include "nsSecureBrowserUIImpl.h"
#include "nsSiteSecurityService.h"

#include "nsXULAppAPI.h"

#include "PSMContentListener.h"

#define NS_IS_PROCESS_DEFAULT                                                 \
    (GeckoProcessType_Default == XRE_GetProcessType())

#define NS_NSS_INSTANTIATE(ensureOperator, _InstanceClass)                    \
    PR_BEGIN_MACRO                                                            \
        _InstanceClass * inst;                                                \
        inst = new _InstanceClass();                                          \
        NS_ADDREF(inst);                                                      \
        rv = inst->QueryInterface(aIID, aResult);                             \
        NS_RELEASE(inst);                                                     \
    PR_END_MACRO

#define NS_NSS_INSTANTIATE_INIT(ensureOperator, _InstanceClass, _InitMethod)  \
    PR_BEGIN_MACRO                                                            \
        _InstanceClass * inst;                                                \
        inst = new _InstanceClass();                                          \
        NS_ADDREF(inst);                                                      \
        rv = inst->_InitMethod();                                             \
        if(NS_SUCCEEDED(rv)) {                                                \
            rv = inst->QueryInterface(aIID, aResult);                         \
        }                                                                     \
        NS_RELEASE(inst);                                                     \
   PR_END_MACRO


#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(ensureOperator,                    \
                                           _InstanceClass)                    \
   NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_BYPROCESS(ensureOperator,               \
                                                _InstanceClass,               \
                                                _InstanceClass)



#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_BYPROCESS(ensureOperator,          \
                                                     _InstanceClassChrome,    \
                                                     _InstanceClassContent)   \
static nsresult                                                               \
_InstanceClassChrome##Constructor(nsISupports *aOuter, REFNSIID aIID,         \
                                  void **aResult)                             \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    *aResult = nullptr;                                                          \
    if (nullptr != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    if (!NS_IS_PROCESS_DEFAULT &&                                             \
        ensureOperator == nssEnsureChromeOrContent) {                         \
        if (!EnsureNSSInitializedChromeOrContent()) {                         \
            return NS_ERROR_FAILURE;                                          \
        }                                                                     \
    } else if (!EnsureNSSInitialized(ensureOperator)) {                       \
        return NS_ERROR_FAILURE;                                              \
    }                                                                         \
                                                                              \
    if (NS_IS_PROCESS_DEFAULT)                                                \
        NS_NSS_INSTANTIATE(ensureOperator, _InstanceClassChrome);             \
    else                                                                      \
        NS_NSS_INSTANTIATE(ensureOperator, _InstanceClassContent);            \
                                                                              \
    if (ensureOperator == nssLoadingComponent)                                \
    {                                                                         \
        if (NS_SUCCEEDED(rv))                                                 \
            EnsureNSSInitialized(nssInitSucceeded);                           \
        else                                                                  \
            EnsureNSSInitialized(nssInitFailed);                              \
    }                                                                         \
                                                                              \
    return rv;                                                                \
}

 
#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(ensureOperator,               \
                                                _InstanceClass,               \
                                                _InitMethod)                  \
    NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT_BYPROCESS(ensureOperator,         \
                                                      _InstanceClass,         \
                                                      _InstanceClass,         \
                                                      _InitMethod)

#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT_BYPROCESS(ensureOperator,     \
                                                _InstanceClassChrome,         \
                                                _InstanceClassContent,        \
                                                _InitMethod)                  \
static nsresult                                                               \
_InstanceClassChrome##Constructor(nsISupports *aOuter, REFNSIID aIID,         \
                                  void **aResult)                             \
{                                                                             \
    nsresult rv;                                                              \
                                                                              \
    *aResult = nullptr;                                                       \
    if (nullptr != aOuter) {                                                  \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    if (!NS_IS_PROCESS_DEFAULT &&                                             \
        ensureOperator == nssEnsureChromeOrContent) {                         \
        if (!EnsureNSSInitializedChromeOrContent()) {                         \
            return NS_ERROR_FAILURE;                                          \
        }                                                                     \
    } else if (!EnsureNSSInitialized(ensureOperator)) {                       \
        return NS_ERROR_FAILURE;                                              \
    }                                                                         \
                                                                              \
    if (NS_IS_PROCESS_DEFAULT)                                                \
        NS_NSS_INSTANTIATE_INIT(ensureOperator,                               \
                                _InstanceClassChrome,                         \
                                _InitMethod);                                 \
    else                                                                      \
        NS_NSS_INSTANTIATE_INIT(ensureOperator,                               \
                                _InstanceClassContent,                        \
                                _InitMethod);                                 \
                                                                              \
    if (ensureOperator == nssLoadingComponent)                                \
    {                                                                         \
        if (NS_SUCCEEDED(rv))                                                 \
            EnsureNSSInitialized(nssInitSucceeded);                           \
        else                                                                  \
            EnsureNSSInitialized(nssInitFailed);                              \
    }                                                                         \
                                                                              \
    return rv;                                                                \
}

NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nssLoadingComponent, nsNSSComponent,
                                        Init)

using namespace mozilla::psm;

namespace {







NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsSSLSocketProvider)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsTLSSocketProvider)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsSecretDecoderRing)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsPK11TokenDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsPKCS11ModuleDB)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PSMContentListener, init)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_BYPROCESS(nssEnsureOnChromeOnly,
                                             nsNSSCertificate,
                                             nsNSSCertificateFakeTransport)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsNSSCertificateDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_BYPROCESS(nssEnsureOnChromeOnly,
                                             nsNSSCertList,
                                             nsNSSCertListFakeTransport)
#ifdef MOZ_XUL
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsCertTree)
#endif
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsPkcs11)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsCertPicker)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nssEnsure, nsNTLMAuthModule, InitTest)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsureChromeOrContent, nsCryptoHash)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsureChromeOrContent, nsCryptoHMAC)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsureChromeOrContent, nsKeyObject)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsureChromeOrContent, nsKeyObjectFactory)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsDataSignatureVerifier)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsure, nsRandomGenerator)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsureOnChromeOnly, nsSSLStatus)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(nssEnsureOnChromeOnly, TransportSecurityInfo)

typedef mozilla::psm::NSSErrorsService NSSErrorsService;
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(NSSErrorsService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNSSVersion)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsCertOverrideService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsEntropyCollector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSecureBrowserUIImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(CertBlocklist, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSiteSecurityService, Init)

NS_DEFINE_NAMED_CID(NS_NSSCOMPONENT_CID);
NS_DEFINE_NAMED_CID(NS_SSLSOCKETPROVIDER_CID);
NS_DEFINE_NAMED_CID(NS_STARTTLSSOCKETPROVIDER_CID);
NS_DEFINE_NAMED_CID(NS_SDR_CID);
NS_DEFINE_NAMED_CID(NS_PK11TOKENDB_CID);
NS_DEFINE_NAMED_CID(NS_PKCS11MODULEDB_CID);
NS_DEFINE_NAMED_CID(NS_PSMCONTENTLISTEN_CID);
NS_DEFINE_NAMED_CID(NS_X509CERT_CID);
NS_DEFINE_NAMED_CID(NS_X509CERTDB_CID);
NS_DEFINE_NAMED_CID(NS_X509CERTLIST_CID);
NS_DEFINE_NAMED_CID(NS_FORMPROCESSOR_CID);
#ifdef MOZ_XUL
NS_DEFINE_NAMED_CID(NS_CERTTREE_CID);
#endif
NS_DEFINE_NAMED_CID(NS_PKCS11_CID);
NS_DEFINE_NAMED_CID(NS_CRYPTO_HASH_CID);
NS_DEFINE_NAMED_CID(NS_CRYPTO_HMAC_CID);
NS_DEFINE_NAMED_CID(NS_CERT_PICKER_CID);
NS_DEFINE_NAMED_CID(NS_NTLMAUTHMODULE_CID);
NS_DEFINE_NAMED_CID(NS_KEYMODULEOBJECT_CID);
NS_DEFINE_NAMED_CID(NS_KEYMODULEOBJECTFACTORY_CID);
NS_DEFINE_NAMED_CID(NS_DATASIGNATUREVERIFIER_CID);
NS_DEFINE_NAMED_CID(NS_CERTOVERRIDE_CID);
NS_DEFINE_NAMED_CID(NS_RANDOMGENERATOR_CID);
NS_DEFINE_NAMED_CID(NS_SSLSTATUS_CID);
NS_DEFINE_NAMED_CID(TRANSPORTSECURITYINFO_CID);
NS_DEFINE_NAMED_CID(NS_NSSERRORSSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_NSSVERSION_CID);
NS_DEFINE_NAMED_CID(NS_ENTROPYCOLLECTOR_CID);
NS_DEFINE_NAMED_CID(NS_SECURE_BROWSER_UI_CID);
NS_DEFINE_NAMED_CID(NS_SITE_SECURITY_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_CERT_BLOCKLIST_CID);

static const mozilla::Module::CIDEntry kNSSCIDs[] = {
  { &kNS_NSSCOMPONENT_CID, false, nullptr, nsNSSComponentConstructor },
  { &kNS_SSLSOCKETPROVIDER_CID, false, nullptr, nsSSLSocketProviderConstructor },
  { &kNS_STARTTLSSOCKETPROVIDER_CID, false, nullptr, nsTLSSocketProviderConstructor },
  { &kNS_SDR_CID, false, nullptr, nsSecretDecoderRingConstructor },
  { &kNS_PK11TOKENDB_CID, false, nullptr, nsPK11TokenDBConstructor },
  { &kNS_PKCS11MODULEDB_CID, false, nullptr, nsPKCS11ModuleDBConstructor },
  { &kNS_PSMCONTENTLISTEN_CID, false, nullptr, PSMContentListenerConstructor },
  { &kNS_X509CERT_CID, false, nullptr, nsNSSCertificateConstructor },
  { &kNS_X509CERTDB_CID, false, nullptr, nsNSSCertificateDBConstructor },
  { &kNS_X509CERTLIST_CID, false, nullptr, nsNSSCertListConstructor },
  { &kNS_FORMPROCESSOR_CID, false, nullptr, nsKeygenFormProcessor::Create },
#ifdef MOZ_XUL
  { &kNS_CERTTREE_CID, false, nullptr, nsCertTreeConstructor },
#endif
  { &kNS_PKCS11_CID, false, nullptr, nsPkcs11Constructor },
  { &kNS_CRYPTO_HASH_CID, false, nullptr, nsCryptoHashConstructor },
  { &kNS_CRYPTO_HMAC_CID, false, nullptr, nsCryptoHMACConstructor },
  { &kNS_CERT_PICKER_CID, false, nullptr, nsCertPickerConstructor },
  { &kNS_NTLMAUTHMODULE_CID, false, nullptr, nsNTLMAuthModuleConstructor },
  { &kNS_KEYMODULEOBJECT_CID, false, nullptr, nsKeyObjectConstructor },
  { &kNS_KEYMODULEOBJECTFACTORY_CID, false, nullptr, nsKeyObjectFactoryConstructor },
  { &kNS_DATASIGNATUREVERIFIER_CID, false, nullptr, nsDataSignatureVerifierConstructor },
  { &kNS_CERTOVERRIDE_CID, false, nullptr, nsCertOverrideServiceConstructor },
  { &kNS_RANDOMGENERATOR_CID, false, nullptr, nsRandomGeneratorConstructor },
  { &kNS_SSLSTATUS_CID, false, nullptr, nsSSLStatusConstructor },
  { &kTRANSPORTSECURITYINFO_CID, false, nullptr, TransportSecurityInfoConstructor },
  { &kNS_NSSERRORSSERVICE_CID, false, nullptr, NSSErrorsServiceConstructor },
  { &kNS_NSSVERSION_CID, false, nullptr, nsNSSVersionConstructor },
  { &kNS_ENTROPYCOLLECTOR_CID, false, nullptr, nsEntropyCollectorConstructor },
  { &kNS_SECURE_BROWSER_UI_CID, false, nullptr, nsSecureBrowserUIImplConstructor },
  { &kNS_SITE_SECURITY_SERVICE_CID, false, nullptr, nsSiteSecurityServiceConstructor },
  { &kNS_CERT_BLOCKLIST_CID, false, nullptr, CertBlocklistConstructor},
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kNSSContracts[] = {
  { PSM_COMPONENT_CONTRACTID, &kNS_NSSCOMPONENT_CID },
  { NS_NSS_ERRORS_SERVICE_CONTRACTID, &kNS_NSSERRORSSERVICE_CID },
  { NS_NSSVERSION_CONTRACTID, &kNS_NSSVERSION_CID },
  { NS_SSLSOCKETPROVIDER_CONTRACTID, &kNS_SSLSOCKETPROVIDER_CID },
  { NS_STARTTLSSOCKETPROVIDER_CONTRACTID, &kNS_STARTTLSSOCKETPROVIDER_CID },
  { NS_SDR_CONTRACTID, &kNS_SDR_CID },
  { NS_PK11TOKENDB_CONTRACTID, &kNS_PK11TOKENDB_CID },
  { NS_PKCS11MODULEDB_CONTRACTID, &kNS_PKCS11MODULEDB_CID },
  { NS_PSMCONTENTLISTEN_CONTRACTID, &kNS_PSMCONTENTLISTEN_CID },
  { NS_X509CERTDB_CONTRACTID, &kNS_X509CERTDB_CID },
  { NS_X509CERTLIST_CONTRACTID, &kNS_X509CERTLIST_CID },
  { NS_FORMPROCESSOR_CONTRACTID, &kNS_FORMPROCESSOR_CID },
#ifdef MOZ_XUL
  { NS_CERTTREE_CONTRACTID, &kNS_CERTTREE_CID },
#endif
  { NS_PKCS11_CONTRACTID, &kNS_PKCS11_CID },
  { NS_CRYPTO_HASH_CONTRACTID, &kNS_CRYPTO_HASH_CID },
  { NS_CRYPTO_HMAC_CONTRACTID, &kNS_CRYPTO_HMAC_CID },
  { NS_CERT_PICKER_CONTRACTID, &kNS_CERT_PICKER_CID },
  { "@mozilla.org/uriloader/psm-external-content-listener;1", &kNS_PSMCONTENTLISTEN_CID },
  { NS_CRYPTO_FIPSINFO_SERVICE_CONTRACTID, &kNS_PKCS11MODULEDB_CID },
  { NS_NTLMAUTHMODULE_CONTRACTID, &kNS_NTLMAUTHMODULE_CID },
  { NS_KEYMODULEOBJECT_CONTRACTID, &kNS_KEYMODULEOBJECT_CID },
  { NS_KEYMODULEOBJECTFACTORY_CONTRACTID, &kNS_KEYMODULEOBJECTFACTORY_CID },
  { NS_DATASIGNATUREVERIFIER_CONTRACTID, &kNS_DATASIGNATUREVERIFIER_CID },
  { NS_CERTOVERRIDE_CONTRACTID, &kNS_CERTOVERRIDE_CID },
  { NS_RANDOMGENERATOR_CONTRACTID, &kNS_RANDOMGENERATOR_CID },
  { NS_ENTROPYCOLLECTOR_CONTRACTID, &kNS_ENTROPYCOLLECTOR_CID },
  { NS_SECURE_BROWSER_UI_CONTRACTID, &kNS_SECURE_BROWSER_UI_CID },
  { NS_SSSERVICE_CONTRACTID, &kNS_SITE_SECURITY_SERVICE_CID },
  { NS_CERTBLOCKLIST_CONTRACTID, &kNS_CERT_BLOCKLIST_CID },
  { nullptr }
};

static const mozilla::Module::CategoryEntry kNSSCategories[] = {
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-ca-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-server-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-user-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-email-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { nullptr }
};

static const mozilla::Module kNSSModule = {
  mozilla::Module::kVersion,
  kNSSCIDs,
  kNSSContracts,
  kNSSCategories
};

} 

NSMODULE_DEFN(NSS) = &kNSSModule;
