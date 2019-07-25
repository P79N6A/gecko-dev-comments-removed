









































#include "mozilla/ModuleUtils.h"

#include "nsNSSComponent.h"
#include "nsSSLSocketProvider.h"
#include "nsTLSSocketProvider.h"
#include "nsKeygenHandler.h"

#include "nsSDR.h"

#include "nsPK11TokenDB.h"
#include "nsPKCS11Slot.h"
#include "nsNSSCertificate.h"
#include "nsNSSCertificateDB.h"
#include "nsNSSCertCache.h"
#include "nsCMS.h"
#ifdef MOZ_XUL
#include "nsCertTree.h"
#endif
#include "nsCrypto.h"

#include "nsDOMCID.h"

#include "nsCMSSecureMessage.h"
#include "nsCertPicker.h"
#include "nsCURILoader.h"
#include "nsICategoryManager.h"
#include "nsCRLManager.h"
#include "nsCipherInfo.h"
#include "nsNTLMAuthModule.h"
#include "nsStreamCipher.h"
#include "nsKeyModule.h"
#include "nsDataSignatureVerifier.h"
#include "nsCertOverrideService.h"
#include "nsRandomGenerator.h"
#include "nsRecentBadCerts.h"
#include "nsSSLStatus.h"
#include "nsNSSIOLayer.h"



#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(triggeredByNSSComponent,           \
                                                      _InstanceClass)         \
static nsresult                                                               \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    if (triggeredByNSSComponent)                                              \
    {                                                                         \
        if (!EnsureNSSInitialized(nssLoading))                                \
            return NS_ERROR_FAILURE;                                          \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        if (!EnsureNSSInitialized(nssEnsure))                                 \
            return NS_ERROR_FAILURE;                                          \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        if (triggeredByNSSComponent)                                          \
            EnsureNSSInitialized(nssInitFailed);                              \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    NS_ADDREF(inst);                                                          \
    rv = inst->QueryInterface(aIID, aResult);                                 \
    NS_RELEASE(inst);                                                         \
                                                                              \
    if (triggeredByNSSComponent)                                              \
    {                                                                         \
        if (NS_SUCCEEDED(rv))                                                 \
            EnsureNSSInitialized(nssInitSucceeded);                           \
        else                                                                  \
            EnsureNSSInitialized(nssInitFailed);                              \
    }                                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \

 
#define NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(triggeredByNSSComponent,      \
                                                _InstanceClass, _InitMethod)  \
static nsresult                                                               \
_InstanceClass##Constructor(nsISupports *aOuter, REFNSIID aIID,               \
                            void **aResult)                                   \
{                                                                             \
    nsresult rv;                                                              \
    _InstanceClass * inst;                                                    \
                                                                              \
    *aResult = NULL;                                                          \
    if (NULL != aOuter) {                                                     \
        rv = NS_ERROR_NO_AGGREGATION;                                         \
        return rv;                                                            \
    }                                                                         \
                                                                              \
    if (triggeredByNSSComponent)                                              \
    {                                                                         \
        if (!EnsureNSSInitialized(nssLoading))                                \
            return NS_ERROR_FAILURE;                                          \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        if (!EnsureNSSInitialized(nssEnsure))                                 \
            return NS_ERROR_FAILURE;                                          \
    }                                                                         \
                                                                              \
    NS_NEWXPCOM(inst, _InstanceClass);                                        \
    if (NULL == inst) {                                                       \
        if (triggeredByNSSComponent)                                          \
            EnsureNSSInitialized(nssInitFailed);                              \
        rv = NS_ERROR_OUT_OF_MEMORY;                                          \
        return rv;                                                            \
    }                                                                         \
    NS_ADDREF(inst);                                                          \
    rv = inst->_InitMethod();                                                 \
    if(NS_SUCCEEDED(rv)) {                                                    \
        rv = inst->QueryInterface(aIID, aResult);                             \
    }                                                                         \
    NS_RELEASE(inst);                                                         \
                                                                              \
    if (triggeredByNSSComponent)                                              \
    {                                                                         \
        if (NS_SUCCEEDED(rv))                                                 \
            EnsureNSSInitialized(nssInitSucceeded);                           \
        else                                                                  \
            EnsureNSSInitialized(nssInitFailed);                              \
    }                                                                         \
                                                                              \
    return rv;                                                                \
}                                                                             \

NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_TRUE, nsNSSComponent, Init)







NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsSSLSocketProvider)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsTLSSocketProvider)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsSecretDecoderRing)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsPK11TokenDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsPKCS11ModuleDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_FALSE, PSMContentListener, init)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsNSSCertificate)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsNSSCertificateDB)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsNSSCertCache)
#ifdef MOZ_XUL
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCertTree)
#endif
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCrypto)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsPkcs11)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSSecureMessage)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSDecoder)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSEncoder)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCMSMessage)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCertPicker)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCRLManager)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCipherInfoService)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_FALSE, nsNTLMAuthModule, InitTest)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCryptoHash)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsCryptoHMAC)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsStreamCipher)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsKeyObject)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsKeyObjectFactory)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsDataSignatureVerifier)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_FALSE, nsCertOverrideService, Init)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsRandomGenerator)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR_INIT(PR_FALSE, nsRecentBadCertsService, Init)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsSSLStatus)
NS_NSS_GENERIC_FACTORY_CONSTRUCTOR(PR_FALSE, nsNSSSocketInfo)

NS_DEFINE_NAMED_CID(NS_NSSCOMPONENT_CID);
NS_DEFINE_NAMED_CID(NS_SSLSOCKETPROVIDER_CID);
NS_DEFINE_NAMED_CID(NS_STARTTLSSOCKETPROVIDER_CID);
NS_DEFINE_NAMED_CID(NS_SDR_CID);
NS_DEFINE_NAMED_CID(NS_PK11TOKENDB_CID);
NS_DEFINE_NAMED_CID(NS_PKCS11MODULEDB_CID);
NS_DEFINE_NAMED_CID(NS_PSMCONTENTLISTEN_CID);
NS_DEFINE_NAMED_CID(NS_X509CERT_CID);
NS_DEFINE_NAMED_CID(NS_X509CERTDB_CID);
NS_DEFINE_NAMED_CID(NS_NSSCERTCACHE_CID);
NS_DEFINE_NAMED_CID(NS_FORMPROCESSOR_CID);
#ifdef MOZ_XUL
NS_DEFINE_NAMED_CID(NS_CERTTREE_CID);
#endif
NS_DEFINE_NAMED_CID(NS_PKCS11_CID);
NS_DEFINE_NAMED_CID(NS_CRYPTO_CID);
NS_DEFINE_NAMED_CID(NS_CMSSECUREMESSAGE_CID);
NS_DEFINE_NAMED_CID(NS_CMSDECODER_CID);
NS_DEFINE_NAMED_CID(NS_CMSENCODER_CID);
NS_DEFINE_NAMED_CID(NS_CMSMESSAGE_CID);
NS_DEFINE_NAMED_CID(NS_CRYPTO_HASH_CID);
NS_DEFINE_NAMED_CID(NS_CRYPTO_HMAC_CID);
NS_DEFINE_NAMED_CID(NS_CERT_PICKER_CID);
NS_DEFINE_NAMED_CID(NS_CRLMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_CIPHERINFOSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_NTLMAUTHMODULE_CID);
NS_DEFINE_NAMED_CID(NS_STREAMCIPHER_CID);
NS_DEFINE_NAMED_CID(NS_KEYMODULEOBJECT_CID);
NS_DEFINE_NAMED_CID(NS_KEYMODULEOBJECTFACTORY_CID);
NS_DEFINE_NAMED_CID(NS_DATASIGNATUREVERIFIER_CID);
NS_DEFINE_NAMED_CID(NS_CERTOVERRIDE_CID);
NS_DEFINE_NAMED_CID(NS_RANDOMGENERATOR_CID);
NS_DEFINE_NAMED_CID(NS_RECENTBADCERTS_CID);
NS_DEFINE_NAMED_CID(NS_SSLSTATUS_CID);
NS_DEFINE_NAMED_CID(NS_NSSSOCKETINFO_CID);


static const mozilla::Module::CIDEntry kNSSCIDs[] = {
  { &kNS_NSSCOMPONENT_CID, false, NULL, nsNSSComponentConstructor },
  { &kNS_SSLSOCKETPROVIDER_CID, false, NULL, nsSSLSocketProviderConstructor },
  { &kNS_STARTTLSSOCKETPROVIDER_CID, false, NULL, nsTLSSocketProviderConstructor },
  { &kNS_SDR_CID, false, NULL, nsSecretDecoderRingConstructor },
  { &kNS_PK11TOKENDB_CID, false, NULL, nsPK11TokenDBConstructor },
  { &kNS_PKCS11MODULEDB_CID, false, NULL, nsPKCS11ModuleDBConstructor },
  { &kNS_PSMCONTENTLISTEN_CID, false, NULL, PSMContentListenerConstructor },
  { &kNS_X509CERT_CID, false, NULL, nsNSSCertificateConstructor },
  { &kNS_X509CERTDB_CID, false, NULL, nsNSSCertificateDBConstructor },
  { &kNS_NSSCERTCACHE_CID, false, NULL, nsNSSCertCacheConstructor },
  { &kNS_FORMPROCESSOR_CID, false, NULL, nsKeygenFormProcessor::Create },
#ifdef MOZ_XUL
  { &kNS_CERTTREE_CID, false, NULL, nsCertTreeConstructor },
#endif
  { &kNS_PKCS11_CID, false, NULL, nsPkcs11Constructor },
  { &kNS_CRYPTO_CID, false, NULL, nsCryptoConstructor },
  { &kNS_CMSSECUREMESSAGE_CID, false, NULL, nsCMSSecureMessageConstructor },
  { &kNS_CMSDECODER_CID, false, NULL, nsCMSDecoderConstructor },
  { &kNS_CMSENCODER_CID, false, NULL, nsCMSEncoderConstructor },
  { &kNS_CMSMESSAGE_CID, false, NULL, nsCMSMessageConstructor },
  { &kNS_CRYPTO_HASH_CID, false, NULL, nsCryptoHashConstructor },
  { &kNS_CRYPTO_HMAC_CID, false, NULL, nsCryptoHMACConstructor },
  { &kNS_CERT_PICKER_CID, false, NULL, nsCertPickerConstructor },
  { &kNS_CRLMANAGER_CID, false, NULL, nsCRLManagerConstructor },
  { &kNS_CIPHERINFOSERVICE_CID, false, NULL, nsCipherInfoServiceConstructor },
  { &kNS_NTLMAUTHMODULE_CID, false, NULL, nsNTLMAuthModuleConstructor },
  { &kNS_STREAMCIPHER_CID, false, NULL, nsStreamCipherConstructor },
  { &kNS_KEYMODULEOBJECT_CID, false, NULL, nsKeyObjectConstructor },
  { &kNS_KEYMODULEOBJECTFACTORY_CID, false, NULL, nsKeyObjectFactoryConstructor },
  { &kNS_DATASIGNATUREVERIFIER_CID, false, NULL, nsDataSignatureVerifierConstructor },
  { &kNS_CERTOVERRIDE_CID, false, NULL, nsCertOverrideServiceConstructor },
  { &kNS_RANDOMGENERATOR_CID, false, NULL, nsRandomGeneratorConstructor },
  { &kNS_RECENTBADCERTS_CID, false, NULL, nsRecentBadCertsServiceConstructor },
  { &kNS_SSLSTATUS_CID, false, NULL, nsSSLStatusConstructor },
  { &kNS_NSSSOCKETINFO_CID, false, NULL, nsNSSSocketInfoConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kNSSContracts[] = {
  { PSM_COMPONENT_CONTRACTID, &kNS_NSSCOMPONENT_CID },
  { NS_NSS_ERRORS_SERVICE_CONTRACTID, &kNS_NSSCOMPONENT_CID },
  { NS_SSLSOCKETPROVIDER_CONTRACTID, &kNS_SSLSOCKETPROVIDER_CID },
  { NS_STARTTLSSOCKETPROVIDER_CONTRACTID, &kNS_STARTTLSSOCKETPROVIDER_CID },
  { NS_SDR_CONTRACTID, &kNS_SDR_CID },
  { NS_PK11TOKENDB_CONTRACTID, &kNS_PK11TOKENDB_CID },
  { NS_PKCS11MODULEDB_CONTRACTID, &kNS_PKCS11MODULEDB_CID },
  { NS_PSMCONTENTLISTEN_CONTRACTID, &kNS_PSMCONTENTLISTEN_CID },
  { NS_X509CERTDB_CONTRACTID, &kNS_X509CERTDB_CID },
  { NS_NSSCERTCACHE_CONTRACTID, &kNS_NSSCERTCACHE_CID },
  { NS_FORMPROCESSOR_CONTRACTID, &kNS_FORMPROCESSOR_CID },
#ifdef MOZ_XUL
  { NS_CERTTREE_CONTRACTID, &kNS_CERTTREE_CID },
#endif
  { NS_PKCS11_CONTRACTID, &kNS_PKCS11_CID },
  { NS_CRYPTO_CONTRACTID, &kNS_CRYPTO_CID },
  { NS_CMSSECUREMESSAGE_CONTRACTID, &kNS_CMSSECUREMESSAGE_CID },
  { NS_CMSDECODER_CONTRACTID, &kNS_CMSDECODER_CID },
  { NS_CMSENCODER_CONTRACTID, &kNS_CMSENCODER_CID },
  { NS_CMSMESSAGE_CONTRACTID, &kNS_CMSMESSAGE_CID },
  { NS_CRYPTO_HASH_CONTRACTID, &kNS_CRYPTO_HASH_CID },
  { NS_CRYPTO_HMAC_CONTRACTID, &kNS_CRYPTO_HMAC_CID },
  { NS_CERT_PICKER_CONTRACTID, &kNS_CERT_PICKER_CID },
  { "@mozilla.org/uriloader/psm-external-content-listener;1", &kNS_PSMCONTENTLISTEN_CID },
  { NS_CRLMANAGER_CONTRACTID, &kNS_CRLMANAGER_CID },
  { NS_CIPHERINFOSERVICE_CONTRACTID, &kNS_CIPHERINFOSERVICE_CID },
  { NS_CRYPTO_FIPSINFO_SERVICE_CONTRACTID, &kNS_PKCS11MODULEDB_CID },
  { NS_NTLMAUTHMODULE_CONTRACTID, &kNS_NTLMAUTHMODULE_CID },
  { NS_STREAMCIPHER_CONTRACTID, &kNS_STREAMCIPHER_CID },
  { NS_KEYMODULEOBJECT_CONTRACTID, &kNS_KEYMODULEOBJECT_CID },
  { NS_KEYMODULEOBJECTFACTORY_CONTRACTID, &kNS_KEYMODULEOBJECTFACTORY_CID },
  { NS_DATASIGNATUREVERIFIER_CONTRACTID, &kNS_DATASIGNATUREVERIFIER_CID },
  { NS_CERTOVERRIDE_CONTRACTID, &kNS_CERTOVERRIDE_CID },
  { NS_RANDOMGENERATOR_CONTRACTID, &kNS_RANDOMGENERATOR_CID },
  { NS_RECENTBADCERTS_CONTRACTID, &kNS_RECENTBADCERTS_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kNSSCategories[] = {
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-ca-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-server-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-user-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-email-cert", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-pkcs7-crl", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/x-x509-crl", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NS_CONTENT_LISTENER_CATEGORYMANAGER_ENTRY, "application/pkix-crl", "@mozilla.org/uriloader/psm-external-content-listener;1" },
  { NULL }
};

static const mozilla::Module kNSSModule = {
  mozilla::Module::kVersion,
  kNSSCIDs,
  kNSSContracts,
  kNSSCategories
};

NSMODULE_DEFN(NSS) = &kNSSModule;
