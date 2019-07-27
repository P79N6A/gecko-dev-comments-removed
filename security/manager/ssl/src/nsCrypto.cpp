




#include "nsCrypto.h"
#include "nsNSSComponent.h"
#include "secmod.h"

#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsXPIDLString.h"
#include "nsISaveAsCharset.h"
#include "nsNativeCharsetUtils.h"
#include "nsServiceManagerUtils.h"

#ifndef MOZ_DISABLE_CRYPTOLEGACY
#include "mozilla/dom/ScriptSettings.h"
#include "nsKeygenHandler.h"
#include "nsKeygenThread.h"
#include "nsNSSCertificate.h"
#include "nsNSSCertificateDB.h"
#include "nsPKCS12Blob.h"
#include "nsPK11TokenDB.h"
#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsIMemory.h"
#include "nsAlgorithm.h"
#include "prprf.h"
#include "nsDOMCID.h"
#include "nsIDOMWindow.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIGlobalObject.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsDOMJSUtils.h"
#include "nsJSUtils.h"
#include "nsIXPConnect.h"
#include "nsIRunnable.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsIFilePicker.h"
#include "nsJSPrincipals.h"
#include "nsJSUtils.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsIGenKeypairInfoDlg.h"
#include "nsIDOMCryptoDialogs.h"
#include "nsIFormSigningDialog.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIURI.h"
#include "jsapi.h"
#include "js/OldDebugAPI.h"
#include <ctype.h>
#include "pk11func.h"
#include "keyhi.h"
#include "cryptohi.h"
#include "seccomon.h"
#include "secerr.h"
#include "sechash.h"
#include "crmf.h"
#include "pk11pqg.h"
#include "cmmf.h"
#include "nssb64.h"
#include "base64.h"
#include "cert.h"
#include "certdb.h"
#include "secmod.h"
#include "ScopedNSSTypes.h"
#include "pkix/pkixtypes.h"

#include "ssl.h" 

#include "nsNSSCleaner.h"

#include "nsNSSCertHelper.h"
#include <algorithm>
#include "nsWrapperCacheInlines.h"
#endif
#ifndef MOZ_DISABLE_CRYPTOLEGACY
#include "mozilla/dom/CRMFObjectBinding.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;






#define JS_ERROR       "error:"
#define JS_ERROR_INTERNAL  JS_ERROR"internalError"

#undef REPORT_INCORRECT_NUM_ARGS

#define JS_OK_ADD_MOD                      3
#define JS_OK_DEL_EXTERNAL_MOD             2
#define JS_OK_DEL_INTERNAL_MOD             1

#define JS_ERR_INTERNAL                   -1
#define JS_ERR_USER_CANCEL_ACTION         -2
#define JS_ERR_INCORRECT_NUM_OF_ARGUMENTS -3
#define JS_ERR_DEL_MOD                    -4
#define JS_ERR_ADD_MOD                    -5
#define JS_ERR_BAD_MODULE_NAME            -6
#define JS_ERR_BAD_DLL_NAME               -7
#define JS_ERR_BAD_MECHANISM_FLAGS        -8
#define JS_ERR_BAD_CIPHER_ENABLE_FLAGS    -9
#define JS_ERR_ADD_DUPLICATE_MOD          -10

#ifndef MOZ_DISABLE_CRYPTOLEGACY

NSSCleanupAutoPtrClass_WithParam(PK11Context, PK11_DestroyContext, TrueParam, true)








typedef enum {
  rsaEnc, rsaDualUse, rsaSign, rsaNonrepudiation, rsaSignNonrepudiation,
  ecEnc, ecDualUse, ecSign, ecNonrepudiation, ecSignNonrepudiation,
  dhEx, dsaSignNonrepudiation, dsaSign, dsaNonrepudiation, invalidKeyGen
} nsKeyGenType;

bool isECKeyGenType(nsKeyGenType kgt)
{
  switch (kgt)
  {
    case ecEnc:
    case ecDualUse:
    case ecSign:
    case ecNonrepudiation:
    case ecSignNonrepudiation:
      return true;
    
    default:
      break;
  }

  return false;
}

typedef struct nsKeyPairInfoStr {
  SECKEYPublicKey  *pubKey;     

  SECKEYPrivateKey *privKey;     
  nsKeyGenType      keyGenType; 

  CERTCertificate *ecPopCert;
                   



  SECKEYPublicKey  *ecPopPubKey;
                   
} nsKeyPairInfo;




class nsCryptoRunArgs : public nsISupports {
public:
  nsCryptoRunArgs();
  nsCOMPtr<nsIGlobalObject> m_globalObject;
  nsXPIDLCString m_jsCallback;
  NS_DECL_ISUPPORTS
protected:
  virtual ~nsCryptoRunArgs();
};





class nsCryptoRunnable : public nsIRunnable {
public:
  nsCryptoRunnable(nsCryptoRunArgs *args);

  NS_IMETHOD Run ();
  NS_DECL_ISUPPORTS
private:
  virtual ~nsCryptoRunnable();

  nsCryptoRunArgs *m_args;
};





class nsP12Runnable : public nsIRunnable {
public:
  nsP12Runnable(nsIX509Cert **certArr, int32_t numCerts, nsIPK11Token *token);

  NS_IMETHOD Run();
  NS_DECL_ISUPPORTS
protected:
  virtual ~nsP12Runnable();
private:
  nsCOMPtr<nsIPK11Token> mToken;
  nsIX509Cert **mCertArr;
  int32_t       mNumCerts;
};


NS_INTERFACE_MAP_BEGIN(nsCrypto)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCrypto)
NS_INTERFACE_MAP_END_INHERITING(mozilla::dom::Crypto)

NS_IMPL_ADDREF_INHERITED(nsCrypto, mozilla::dom::Crypto)
NS_IMPL_RELEASE_INHERITED(nsCrypto, mozilla::dom::Crypto)


#endif 

NS_INTERFACE_MAP_BEGIN(nsPkcs11)
  NS_INTERFACE_MAP_ENTRY(nsIPKCS11)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPkcs11)
NS_IMPL_RELEASE(nsPkcs11)

#ifndef MOZ_DISABLE_CRYPTOLEGACY


NS_IMPL_ISUPPORTS(nsCryptoRunnable, nsIRunnable)


NS_IMPL_ISUPPORTS(nsP12Runnable, nsIRunnable)


NS_IMPL_ISUPPORTS0(nsCryptoRunArgs)

nsCrypto::nsCrypto() :
  mEnableSmartCardEvents(false)
{
}

nsCrypto::~nsCrypto()
{
}

void
nsCrypto::Init(nsIDOMWindow* aWindow)
{
  mozilla::dom::Crypto::Init(aWindow);
}

void
nsCrypto::SetEnableSmartCardEvents(bool aEnable, ErrorResult& aRv)
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsresult rv = NS_OK;

  
  
  
  if (aEnable) {
    nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  mEnableSmartCardEvents = aEnable;
}

NS_IMETHODIMP
nsCrypto::SetEnableSmartCardEvents(bool aEnable)
{
  ErrorResult rv;
  SetEnableSmartCardEvents(aEnable, rv);
  return rv.ErrorCode();
}

bool
nsCrypto::EnableSmartCardEvents()
{
  return mEnableSmartCardEvents;
}

NS_IMETHODIMP
nsCrypto::GetEnableSmartCardEvents(bool *aEnable)
{
  *aEnable = EnableSmartCardEvents();
  return NS_OK;
}



static bool
ns_can_escrow(nsKeyGenType keyGenType)
{
  
  return (bool)(keyGenType == rsaEnc || keyGenType == ecEnc);
}



void
nsCrypto::GetVersion(nsString& aVersion)
{
  aVersion.Assign(NS_LITERAL_STRING(PSM_VERSION_STRING));
}





static uint32_t
cryptojs_convert_to_mechanism(nsKeyGenType keyGenType)
{
  uint32_t retMech;

  switch (keyGenType) {
  case rsaEnc:
  case rsaDualUse:
  case rsaSign:
  case rsaNonrepudiation:
  case rsaSignNonrepudiation:
    retMech = CKM_RSA_PKCS_KEY_PAIR_GEN;
    break;
  case ecEnc:
  case ecDualUse:
  case ecSign:
  case ecNonrepudiation:
  case ecSignNonrepudiation:
    retMech = CKM_EC_KEY_PAIR_GEN;
    break;
  case dhEx:
    retMech = CKM_DH_PKCS_KEY_PAIR_GEN;
    break;
  case dsaSign:
  case dsaSignNonrepudiation:
  case dsaNonrepudiation:
    retMech = CKM_DSA_KEY_PAIR_GEN;
    break;
  default:
    retMech = CKM_INVALID_MECHANISM;
  }
  return retMech;
}






static nsKeyGenType
cryptojs_interpret_key_gen_type(const nsAString& keyAlg)
{
  if (keyAlg.EqualsLiteral("rsa-ex")) {
    return rsaEnc;
  }
  if (keyAlg.EqualsLiteral("rsa-dual-use")) {
    return rsaDualUse;
  }
  if (keyAlg.EqualsLiteral("rsa-sign")) {
    return rsaSign;
  }
  if (keyAlg.EqualsLiteral("rsa-sign-nonrepudiation")) {
    return rsaSignNonrepudiation;
  }
  if (keyAlg.EqualsLiteral("rsa-nonrepudiation")) {
    return rsaNonrepudiation;
  }
  if (keyAlg.EqualsLiteral("ec-ex")) {
    return ecEnc;
  }
  if (keyAlg.EqualsLiteral("ec-dual-use")) {
    return ecDualUse;
  }
  if (keyAlg.EqualsLiteral("ec-sign")) {
    return ecSign;
  }
  if (keyAlg.EqualsLiteral("ec-sign-nonrepudiation")) {
    return ecSignNonrepudiation;
  }
  if (keyAlg.EqualsLiteral("ec-nonrepudiation")) {
    return ecNonrepudiation;
  }
  if (keyAlg.EqualsLiteral("dsa-sign-nonrepudiation")) {
    return dsaSignNonrepudiation;
  }
  if (keyAlg.EqualsLiteral("dsa-sign")) {
    return dsaSign;
  }
  if (keyAlg.EqualsLiteral("dsa-nonrepudiation")) {
    return dsaNonrepudiation;
  }
  if (keyAlg.EqualsLiteral("dh-ex")) {
    return dhEx;
  }
  return invalidKeyGen;
}















bool getNextNameValueFromECKeygenParamString(char *input,
                                               char *&name,
                                               int &name_len,
                                               char *&value,
                                               int &value_len,
                                               char *&next_call)
{
  if (!input || !*input)
    return false;

  

  while (*input && *input == ';')
    ++input;

  while (*input && *input == ' ')
    ++input;

  name = input;

  while (*input && *input != '=')
    ++input;

  if (*input != '=')
    return false;

  name_len = input - name;
  ++input;

  value = input;

  while (*input && *input != ';')
    ++input;

  value_len = input - value;
  next_call = input;

  return true;
}




static void*
nsConvertToActualKeyGenParams(uint32_t keyGenMech, char *params,
                              uint32_t paramLen, int32_t keySize,
                              nsKeyPairInfo *keyPairInfo)
{
  void *returnParams = nullptr;


  switch (keyGenMech) {
  case CKM_RSA_PKCS_KEY_PAIR_GEN:
  {
    
    
    if (params)
      return nullptr;

    PK11RSAGenParams *rsaParams;
    rsaParams = static_cast<PK11RSAGenParams*>
                           (nsMemory::Alloc(sizeof(PK11RSAGenParams)));
                              
    if (!rsaParams) {
      return nullptr;
    }
    


    if (keySize > 0) {
      rsaParams->keySizeInBits = keySize;
    } else {
      rsaParams->keySizeInBits = 1024;
    }
    rsaParams->pe = DEFAULT_RSA_KEYGEN_PE;
    returnParams = rsaParams;
    break;
  }
  case CKM_EC_KEY_PAIR_GEN:
  {
    



























    char *curve = nullptr;

    {
      

      char *next_input = params;
      char *name = nullptr;
      char *value = nullptr;
      int name_len = 0;
      int value_len = 0;
  
      while (getNextNameValueFromECKeygenParamString(
              next_input, name, name_len, value, value_len,
              next_input))
      {
        
        if (!curve && PL_strncmp(name, "curve", std::min(name_len, 5)) == 0)
        {
          curve = PL_strndup(value, value_len);
        }
        
        else if (!keyPairInfo->ecPopCert &&
                 PL_strncmp(name, "popcert", std::min(name_len, 7)) == 0)
        {
          char *certstr = PL_strndup(value, value_len);
          if (certstr) {
            keyPairInfo->ecPopCert = CERT_ConvertAndDecodeCertificate(certstr);
            PL_strfree(certstr);

            if (keyPairInfo->ecPopCert)
            {
              keyPairInfo->ecPopPubKey = CERT_ExtractPublicKey(keyPairInfo->ecPopCert);
            }
          }
        }
      }
    }

    
    if (keyPairInfo->ecPopPubKey && keyPairInfo->ecPopPubKey->keyType == ecKey)
    {
      returnParams = SECITEM_DupItem(&keyPairInfo->ecPopPubKey->u.ec.DEREncodedParams);
    }

    
    if (!returnParams && curve)
    {
      returnParams = decode_ec_params(curve);
    }

    
    if (!returnParams)
    {
      switch (keySize) {
      case 512:
      case 1024:
          returnParams = decode_ec_params("secp256r1");
          break;
      case 2048:
      default:
          returnParams = decode_ec_params("secp384r1");
          break;
      }
    }

    if (curve)
      PL_strfree(curve);

    break;
  }
  case CKM_DSA_KEY_PAIR_GEN:
  {
    
    
    if (params)
      return nullptr;

    PQGParams *pqgParams = nullptr;
    PQGVerify *vfy = nullptr;
    SECStatus  rv;
    int        index;
       
    index = PQG_PBITS_TO_INDEX(keySize);
    if (index == -1) {
      returnParams = nullptr;
      break;
    }
    rv = PK11_PQG_ParamGen(0, &pqgParams, &vfy);
    if (vfy) {
      PK11_PQG_DestroyVerify(vfy);
    }
    if (rv != SECSuccess) {
      if (pqgParams) {
        PK11_PQG_DestroyParams(pqgParams);
      }
      return nullptr;
    }
    returnParams = pqgParams;
    break;
  }
  default:
    returnParams = nullptr;
  }
  return returnParams;
}




static PK11SlotInfo*
nsGetSlotForKeyGen(nsKeyGenType keyGenType, nsIInterfaceRequestor *ctx)
{
  nsNSSShutDownPreventionLock locker;
  uint32_t mechanism = cryptojs_convert_to_mechanism(keyGenType);
  PK11SlotInfo *slot = nullptr;
  nsresult rv = GetSlotWithMechanism(mechanism,ctx, &slot);
  if (NS_FAILED(rv)) {
    if (slot)
      PK11_FreeSlot(slot);
    slot = nullptr;
  }
  return slot;
}



static void
nsFreeKeyGenParams(CK_MECHANISM_TYPE keyGenMechanism, void *params)
{
  switch (keyGenMechanism) {
  case CKM_RSA_PKCS_KEY_PAIR_GEN:
    nsMemory::Free(params);
    break;
  case CKM_EC_KEY_PAIR_GEN:
    SECITEM_FreeItem(reinterpret_cast<SECItem*>(params), true);
    break;
  case CKM_DSA_KEY_PAIR_GEN:
    PK11_PQG_DestroyParams(static_cast<PQGParams*>(params));
    break;
  }
}






static nsresult
cryptojs_generateOneKeyPair(JSContext *cx, nsKeyPairInfo *keyPairInfo, 
                            int32_t keySize, char *params, 
                            nsIInterfaceRequestor *uiCxt,
                            PK11SlotInfo *slot, bool willEscrow)
                            
{
  const PK11AttrFlags sensitiveFlags = (PK11_ATTR_SENSITIVE | PK11_ATTR_PRIVATE);
  const PK11AttrFlags temporarySessionFlags = PK11_ATTR_SESSION;
  const PK11AttrFlags permanentTokenFlags = PK11_ATTR_TOKEN;
  const PK11AttrFlags extractableFlags = PK11_ATTR_EXTRACTABLE;
  
  nsIGeneratingKeypairInfoDialogs * dialogs;
  nsKeygenThread *KeygenRunnable = 0;
  nsCOMPtr<nsIKeygenThread> runnable;

  uint32_t mechanism = cryptojs_convert_to_mechanism(keyPairInfo->keyGenType);
  void *keyGenParams = nsConvertToActualKeyGenParams(mechanism, params, 
                                                     (params) ? strlen(params):0, 
                                                     keySize, keyPairInfo);

  if (!keyGenParams || !slot) {
    return NS_ERROR_INVALID_ARG;
  }

  
  

  nsresult rv = setPassword(slot, uiCxt);
  if (NS_FAILED(rv))
    return rv;

  if (PK11_Authenticate(slot, true, uiCxt) != SECSuccess)
    return NS_ERROR_FAILURE;
 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ScopedPK11SlotInfo intSlot;
  
  if (willEscrow && !PK11_IsInternal(slot)) {
    intSlot = PK11_GetInternalSlot();
    NS_ASSERTION(intSlot,"Couldn't get the internal slot");
    
    if (!PK11_DoesMechanism(intSlot, mechanism)) {
      
      intSlot = nullptr;
    }
  }

  rv = getNSSDialogs((void**)&dialogs,
                     NS_GET_IID(nsIGeneratingKeypairInfoDialogs),
                     NS_GENERATINGKEYPAIRINFODIALOGS_CONTRACTID);

  if (NS_SUCCEEDED(rv)) {
    KeygenRunnable = new nsKeygenThread();
    if (KeygenRunnable) {
      NS_ADDREF(KeygenRunnable);
    }
  }
  
  
  
  
  PK11SlotInfo *firstAttemptSlot = nullptr;
  PK11AttrFlags firstAttemptFlags = 0;

  PK11SlotInfo *secondAttemptSlot = slot;
  PK11AttrFlags secondAttemptFlags = sensitiveFlags | permanentTokenFlags;
  
  if (willEscrow) {
    secondAttemptFlags |= extractableFlags;
  }
  
  if (!intSlot || PK11_IsInternal(slot)) {
    
    
    firstAttemptSlot = secondAttemptSlot;
    firstAttemptFlags = secondAttemptFlags;
    secondAttemptSlot = nullptr;
    secondAttemptFlags = 0;
  }
  else {
    firstAttemptSlot = intSlot;
    firstAttemptFlags = sensitiveFlags | temporarySessionFlags;
    
    
    
    firstAttemptFlags |= extractableFlags;
  }

  bool mustMoveKey = false;
  
  if (NS_FAILED(rv) || !KeygenRunnable) {
    
    rv = NS_OK;
    
    keyPairInfo->privKey = 
      PK11_GenerateKeyPairWithFlags(firstAttemptSlot, mechanism,
                                    keyGenParams, &keyPairInfo->pubKey,
                                    firstAttemptFlags, uiCxt);
    
    if (keyPairInfo->privKey) {
      
      if (secondAttemptSlot) {
        mustMoveKey = true;
      }
    }
    else {
      keyPairInfo->privKey = 
        PK11_GenerateKeyPairWithFlags(secondAttemptSlot, mechanism,
                                      keyGenParams, &keyPairInfo->pubKey,
                                      secondAttemptFlags, uiCxt);
    }
    
  } else {
    
    KeygenRunnable->SetParams( firstAttemptSlot, firstAttemptFlags,
                               secondAttemptSlot, secondAttemptFlags,
                               mechanism, keyGenParams, uiCxt );

    runnable = do_QueryInterface(KeygenRunnable);

    if (runnable) {
      {
        nsPSMUITracker tracker;
        if (tracker.isUIForbidden()) {
          rv = NS_ERROR_NOT_AVAILABLE;
        }
        else {
          rv = dialogs->DisplayGeneratingKeypairInfo(uiCxt, runnable);
          
          
          KeygenRunnable->Join();
        }
      }

      NS_RELEASE(dialogs);
      if (NS_SUCCEEDED(rv)) {
        PK11SlotInfo *used_slot = nullptr;
        rv = KeygenRunnable->ConsumeResult(&used_slot, 
                                           &keyPairInfo->privKey, &keyPairInfo->pubKey);

        if (NS_SUCCEEDED(rv)) {
          if ((used_slot == firstAttemptSlot) && secondAttemptSlot) {
            mustMoveKey = true;
          }
        
          if (used_slot) {
            PK11_FreeSlot(used_slot);
          }
        }
      }
    }
  }

  firstAttemptSlot = nullptr;
  secondAttemptSlot = nullptr;
  
  nsFreeKeyGenParams(mechanism, keyGenParams);

  if (KeygenRunnable) {
    NS_RELEASE(KeygenRunnable);
  }

  if (!keyPairInfo->privKey || !keyPairInfo->pubKey) {
    return NS_ERROR_FAILURE;
  }
 
  
  
  if (mustMoveKey) {
    ScopedSECKEYPrivateKey newPrivKey(PK11_LoadPrivKey(slot,
                                                    keyPairInfo->privKey,
                                                    keyPairInfo->pubKey,
                                                    true, true));
    if (!newPrivKey)
      return NS_ERROR_FAILURE;

    
    
    
    
  }  

  return NS_OK;
}




















































static nsresult
cryptojs_ReadArgsAndGenerateKey(JSContext *cx,
                                JS::Value *argv,
                                nsKeyPairInfo *keyGenType,
                                nsIInterfaceRequestor *uiCxt,
                                PK11SlotInfo **slot, bool willEscrow)
{
  JSString  *jsString;
  JSAutoByteString params;
  int    keySize;
  nsresult  rv;

  if (!argv[0].isInt32()) {
    JS_ReportError(cx, "%s%s", JS_ERROR,
                   "passed in non-integer for key size");
    return NS_ERROR_FAILURE;
  }
  keySize = argv[0].toInt32();
  if (!argv[1].isNull()) {
    JS::Rooted<JS::Value> v(cx, argv[1]);
    jsString = JS::ToString(cx, v);
    NS_ENSURE_TRUE(jsString, NS_ERROR_OUT_OF_MEMORY);
    argv[1] = STRING_TO_JSVAL(jsString);
    params.encodeLatin1(cx, jsString);
    NS_ENSURE_TRUE(!!params, NS_ERROR_OUT_OF_MEMORY);
  }

  if (argv[2].isNull()) {
    JS_ReportError(cx,"%s%s", JS_ERROR,
             "key generation type not specified");
    return NS_ERROR_FAILURE;
  }
  JS::Rooted<JS::Value> v(cx, argv[2]);
  jsString = JS::ToString(cx, v);
  NS_ENSURE_TRUE(jsString, NS_ERROR_OUT_OF_MEMORY);
  argv[2] = STRING_TO_JSVAL(jsString);
  nsAutoJSString autoJSKeyGenAlg;
  NS_ENSURE_TRUE(autoJSKeyGenAlg.init(cx, jsString), NS_ERROR_UNEXPECTED);
  nsAutoString keyGenAlg(autoJSKeyGenAlg);
  keyGenAlg.Trim("\r\n\t ");
  keyGenType->keyGenType = cryptojs_interpret_key_gen_type(keyGenAlg);
  if (keyGenType->keyGenType == invalidKeyGen) {
    NS_LossyConvertUTF16toASCII keyGenAlgNarrow(autoJSKeyGenAlg);
    JS_ReportError(cx, "%s%s%s", JS_ERROR,
                   "invalid key generation argument:",
                   keyGenAlgNarrow.get());
    goto loser;
  }
  if (!*slot) {
    *slot = nsGetSlotForKeyGen(keyGenType->keyGenType, uiCxt);
    if (!*slot)
      goto loser;
  }

  rv = cryptojs_generateOneKeyPair(cx,keyGenType,keySize,params.ptr(),uiCxt,
                                   *slot,willEscrow);

  if (rv != NS_OK) {
    NS_LossyConvertUTF16toASCII keyGenAlgNarrow(autoJSKeyGenAlg);
    JS_ReportError(cx,"%s%s%s", JS_ERROR,
                   "could not generate the key for algorithm ",
                   keyGenAlgNarrow.get());
    goto loser;
  }
  return NS_OK;
loser:
  return NS_ERROR_FAILURE;
}



static void
nsFreeKeyPairInfo(nsKeyPairInfo *keyids, int numIDs)
{
  NS_ASSERTION(keyids, "NULL pointer passed to nsFreeKeyPairInfo");
  if (!keyids)
    return;
  int i;
  for (i=0; i<numIDs; i++) {
    if (keyids[i].pubKey)
      SECKEY_DestroyPublicKey(keyids[i].pubKey);
    if (keyids[i].privKey)
      SECKEY_DestroyPrivateKey(keyids[i].privKey);
    if (keyids[i].ecPopCert)
      CERT_DestroyCertificate(keyids[i].ecPopCert);
    if (keyids[i].ecPopPubKey)
      SECKEY_DestroyPublicKey(keyids[i].ecPopPubKey);
  }
  delete []keyids;
}


static void
nsFreeCertReqMessages(CRMFCertReqMsg **certReqMsgs, int32_t numMessages)
{
  int32_t i;
  for (i=0; i<numMessages && certReqMsgs[i]; i++) {
    CRMF_DestroyCertReqMsg(certReqMsgs[i]);
  }
  delete []certReqMsgs;
}





static nsresult
nsSetEscrowAuthority(CRMFCertRequest *certReq, nsKeyPairInfo *keyInfo,
                     nsNSSCertificate *wrappingCert)
{
  if (!wrappingCert ||
      CRMF_CertRequestIsControlPresent(certReq, crmfPKIArchiveOptionsControl)){
    return NS_ERROR_FAILURE;
  }
  ScopedCERTCertificate cert(wrappingCert->GetCert());
  if (!cert)
    return NS_ERROR_FAILURE;

  CRMFEncryptedKey *encrKey = 
      CRMF_CreateEncryptedKeyWithEncryptedValue(keyInfo->privKey, cert.get());
  if (!encrKey)
    return NS_ERROR_FAILURE;

  CRMFPKIArchiveOptions *archOpt = 
      CRMF_CreatePKIArchiveOptions(crmfEncryptedPrivateKey, encrKey);
  if (!archOpt) {
    CRMF_DestroyEncryptedKey(encrKey);
    return NS_ERROR_FAILURE;
  }
  SECStatus srv = CRMF_CertRequestSetPKIArchiveOptions(certReq, archOpt);
  CRMF_DestroyEncryptedKey(encrKey);
  CRMF_DestroyPKIArchiveOptions(archOpt);
  if (srv != SECSuccess)
    return NS_ERROR_FAILURE;

  return NS_OK;
}



static nsresult
nsSetDNForRequest(CRMFCertRequest *certReq, char *reqDN)
{
  if (!reqDN || CRMF_CertRequestIsFieldPresent(certReq, crmfSubject)) {
    return NS_ERROR_FAILURE;
  }
  ScopedCERTName subjectName(CERT_AsciiToName(reqDN));
  if (!subjectName) {
    return NS_ERROR_FAILURE;
  }
  SECStatus srv = CRMF_CertRequestSetTemplateField(certReq, crmfSubject,
                                                   static_cast<void*>
                                                              (subjectName));
  return (srv == SECSuccess) ? NS_OK : NS_ERROR_FAILURE;
}


static nsresult
nsSetRegToken(CRMFCertRequest *certReq, char *regToken)
{
  
  NS_ASSERTION(certReq, "A bogus certReq passed to nsSetRegToken");
  if (regToken){
    if (CRMF_CertRequestIsControlPresent(certReq, crmfRegTokenControl))
      return NS_ERROR_FAILURE;
  
    SECItem src;
    src.data = (unsigned char*)regToken;
    src.len  = strlen(regToken);
    SECItem *derEncoded = SEC_ASN1EncodeItem(nullptr, nullptr, &src, 
                                        SEC_ASN1_GET(SEC_UTF8StringTemplate));

    if (!derEncoded)
      return NS_ERROR_FAILURE;

    SECStatus srv = CRMF_CertRequestSetRegTokenControl(certReq, derEncoded);
    SECITEM_FreeItem(derEncoded,true);
    if (srv != SECSuccess)
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}



static nsresult
nsSetAuthenticator(CRMFCertRequest *certReq, char *authenticator)
{
  
  NS_ASSERTION(certReq, "Bogus certReq passed to nsSetAuthenticator");
  if (authenticator) {
    if (CRMF_CertRequestIsControlPresent(certReq, crmfAuthenticatorControl))
      return NS_ERROR_FAILURE;
    
    SECItem src;
    src.data = (unsigned char*)authenticator;
    src.len  = strlen(authenticator);
    SECItem *derEncoded = SEC_ASN1EncodeItem(nullptr, nullptr, &src,
                                     SEC_ASN1_GET(SEC_UTF8StringTemplate));
    if (!derEncoded)
      return NS_ERROR_FAILURE;

    SECStatus srv = CRMF_CertRequestSetAuthenticatorControl(certReq, 
                                                            derEncoded);
    SECITEM_FreeItem(derEncoded, true);
    if (srv != SECSuccess)
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}





static void
nsPrepareBitStringForEncoding (SECItem *bitsmap, SECItem *value)
{
  unsigned char onebyte;
  unsigned int i, len = 0;

  
  onebyte = '\0';
  
  for (i = 0; i < (value->len ) * 8; ++i) {
    if (i % 8 == 0)
      onebyte = value->data[i/8];
    if (onebyte & 0x80)
      len = i;
    onebyte <<= 1;
  }

  bitsmap->data = value->data;
  
  bitsmap->len = len + 1;
}









static nsresult
nsSetKeyUsageExtension(CRMFCertRequest *crmfReq,
                       unsigned char   keyUsage)
{
  SECItem                 *encodedExt= nullptr;
  SECItem                  keyUsageValue = { (SECItemType) 0, nullptr, 0 };
  SECItem                  bitsmap = { (SECItemType) 0, nullptr, 0 };
  SECStatus                srv;
  CRMFCertExtension       *ext = nullptr;
  CRMFCertExtCreationInfo  extAddParams;
  SEC_ASN1Template         bitStrTemplate = {SEC_ASN1_BIT_STRING, 0, nullptr,
                                             sizeof(SECItem)};

  keyUsageValue.data = &keyUsage;
  keyUsageValue.len  = 1;
  nsPrepareBitStringForEncoding(&bitsmap, &keyUsageValue);

  encodedExt = SEC_ASN1EncodeItem(nullptr, nullptr, &bitsmap,&bitStrTemplate);
  if (!encodedExt) {
    goto loser;
  }
  ext = CRMF_CreateCertExtension(SEC_OID_X509_KEY_USAGE, true, encodedExt);
  if (!ext) {
      goto loser;
  }
  extAddParams.numExtensions = 1;
  extAddParams.extensions = &ext;
  srv = CRMF_CertRequestSetTemplateField(crmfReq, crmfExtension,
                                         &extAddParams);
  if (srv != SECSuccess) {
      goto loser;
  }
  CRMF_DestroyCertExtension(ext);
  SECITEM_FreeItem(encodedExt, true);
  return NS_OK;
 loser:
  if (ext) {
    CRMF_DestroyCertExtension(ext);
  }
  if (encodedExt) {
      SECITEM_FreeItem(encodedExt, true);
  }
  return NS_ERROR_FAILURE;
}

static nsresult
nsSetRSADualUse(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage =   KU_DIGITAL_SIGNATURE
                           | KU_NON_REPUDIATION
                           | KU_KEY_ENCIPHERMENT;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetRSAKeyEx(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_KEY_ENCIPHERMENT;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetRSASign(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_DIGITAL_SIGNATURE;


  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetRSANonRepudiation(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_NON_REPUDIATION;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetRSASignNonRepudiation(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_DIGITAL_SIGNATURE |
                           KU_NON_REPUDIATION;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetECDualUse(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage =   KU_DIGITAL_SIGNATURE
                           | KU_NON_REPUDIATION
                           | KU_KEY_AGREEMENT;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetECKeyEx(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_KEY_AGREEMENT;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetECSign(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_DIGITAL_SIGNATURE;


  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetECNonRepudiation(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_NON_REPUDIATION;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetECSignNonRepudiation(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_DIGITAL_SIGNATURE |
                           KU_NON_REPUDIATION;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetDH(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_KEY_AGREEMENT;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetDSASign(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_DIGITAL_SIGNATURE;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetDSANonRepudiation(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_NON_REPUDIATION;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetDSASignNonRepudiation(CRMFCertRequest *crmfReq)
{
  unsigned char keyUsage = KU_DIGITAL_SIGNATURE |
                           KU_NON_REPUDIATION;

  return nsSetKeyUsageExtension(crmfReq, keyUsage);
}

static nsresult
nsSetKeyUsageExtension(CRMFCertRequest *crmfReq, nsKeyGenType keyGenType)
{
  nsresult rv;

  switch (keyGenType) {
  case rsaDualUse:
    rv = nsSetRSADualUse(crmfReq);
    break;
  case rsaEnc:
    rv = nsSetRSAKeyEx(crmfReq);
    break;
  case rsaSign:
    rv = nsSetRSASign(crmfReq);
    break;
  case rsaNonrepudiation:
    rv = nsSetRSANonRepudiation(crmfReq);
    break;
  case rsaSignNonrepudiation:
    rv = nsSetRSASignNonRepudiation(crmfReq);
    break;
  case ecDualUse:
    rv = nsSetECDualUse(crmfReq);
    break;
  case ecEnc:
    rv = nsSetECKeyEx(crmfReq);
    break;
  case ecSign:
    rv = nsSetECSign(crmfReq);
    break;
  case ecNonrepudiation:
    rv = nsSetECNonRepudiation(crmfReq);
    break;
  case ecSignNonrepudiation:
    rv = nsSetECSignNonRepudiation(crmfReq);
    break;
  case dhEx:
    rv = nsSetDH(crmfReq);
    break;
  case dsaSign:
    rv = nsSetDSASign(crmfReq);
    break;
  case dsaNonrepudiation:
    rv = nsSetDSANonRepudiation(crmfReq);
    break;
  case dsaSignNonrepudiation:
    rv = nsSetDSASignNonRepudiation(crmfReq);
    break;
  default:
    rv = NS_ERROR_FAILURE;
    break;
  }
  return rv;
}





static CRMFCertRequest*
nsCreateSingleCertReq(nsKeyPairInfo *keyInfo, char *reqDN, char *regToken, 
                      char *authenticator, nsNSSCertificate *wrappingCert)
{
  uint32_t reqID;
  nsresult rv;

  
  
  
  PK11_GenerateRandom((unsigned char*)&reqID, sizeof(reqID));
  CRMFCertRequest *certReq = CRMF_CreateCertRequest(reqID);
  if (!certReq)
    return nullptr;

  long version = SEC_CERTIFICATE_VERSION_3;
  SECStatus srv;
  CERTSubjectPublicKeyInfo *spki = nullptr;
  srv = CRMF_CertRequestSetTemplateField(certReq, crmfVersion, &version);
  if (srv != SECSuccess)
    goto loser;
  
  spki = SECKEY_CreateSubjectPublicKeyInfo(keyInfo->pubKey);
  if (!spki)
    goto loser;

  srv = CRMF_CertRequestSetTemplateField(certReq, crmfPublicKey, spki);
  SECKEY_DestroySubjectPublicKeyInfo(spki);
  if (srv != SECSuccess)
    goto loser;

  if (wrappingCert && ns_can_escrow(keyInfo->keyGenType)) {
    rv = nsSetEscrowAuthority(certReq, keyInfo, wrappingCert);
    if (NS_FAILED(rv))
      goto loser;
  }
  rv = nsSetDNForRequest(certReq, reqDN);
  if (NS_FAILED(rv))
    goto loser;

  rv = nsSetRegToken(certReq, regToken);
  if (NS_FAILED(rv))
    goto loser;

  rv = nsSetAuthenticator(certReq, authenticator);
  if (NS_FAILED(rv))
    goto loser;

 rv = nsSetKeyUsageExtension(certReq, keyInfo->keyGenType); 
  if (NS_FAILED(rv))
    goto loser;

  return certReq;
loser:
  if (certReq) {
    CRMF_DestroyCertRequest(certReq);
  }
  return nullptr;
}






static nsresult
nsSetKeyEnciphermentPOP(CRMFCertReqMsg *certReqMsg, bool isEscrowed)
{
  SECItem       bitString;
  unsigned char der[2];
  SECStatus     srv;

  if (isEscrowed) {
    






    der[0] = 0x03; 
    der[1] = 0x00; 
    bitString.data = der;
    bitString.len  = 2;
    srv = CRMF_CertReqMsgSetKeyEnciphermentPOP(certReqMsg, crmfThisMessage,
                                              crmfNoSubseqMess, &bitString);
  } else {
    


    srv = CRMF_CertReqMsgSetKeyEnciphermentPOP(certReqMsg,
                                              crmfSubsequentMessage,
                                              crmfChallengeResp, nullptr);
  }
  return (srv == SECSuccess) ? NS_OK : NS_ERROR_FAILURE;
}

static void
nsCRMFEncoderItemCount(void *arg, const char *buf, unsigned long len);

static void
nsCRMFEncoderItemStore(void *arg, const char *buf, unsigned long len);

static nsresult
nsSet_EC_DHMAC_ProofOfPossession(CRMFCertReqMsg *certReqMsg, 
                                 nsKeyPairInfo  *keyInfo,
                                 CRMFCertRequest *certReq)
{
  
  
  
  
  
  
  

  unsigned long der_request_len = 0;
  ScopedSECItem der_request;

  if (SECSuccess != CRMF_EncodeCertRequest(certReq, 
                                           nsCRMFEncoderItemCount, 
                                           &der_request_len))
    return NS_ERROR_FAILURE;

  der_request = SECITEM_AllocItem(nullptr, nullptr, der_request_len);
  if (!der_request)
    return NS_ERROR_FAILURE;

  
  
  

  der_request->len = 0;

  if (SECSuccess != CRMF_EncodeCertRequest(certReq, 
                                           nsCRMFEncoderItemStore, 
                                           der_request))
    return NS_ERROR_FAILURE;

  
  
  
  

  ScopedPK11SymKey shared_secret;
  ScopedPK11SymKey subject_and_secret;
  ScopedPK11SymKey subject_and_secret_and_issuer;
  ScopedPK11SymKey sha1_of_subject_and_secret_and_issuer;

  shared_secret = 
    PK11_PubDeriveWithKDF(keyInfo->privKey, 
                          keyInfo->ecPopPubKey,  
                          false, 
                          nullptr, 
                          nullptr, 
                          CKM_ECDH1_DERIVE, 
                          CKM_CONCATENATE_DATA_AND_BASE, 
                          CKA_DERIVE, 
                          0, 
                          CKD_NULL, 
                          nullptr, 
                          nullptr); 

  if (!shared_secret)
    return NS_ERROR_FAILURE;

  CK_KEY_DERIVATION_STRING_DATA concat_data_base;
  concat_data_base.pData = keyInfo->ecPopCert->derSubject.data;
  concat_data_base.ulLen = keyInfo->ecPopCert->derSubject.len;
  SECItem concat_data_base_item;
  concat_data_base_item.data = (unsigned char*)&concat_data_base;
  concat_data_base_item.len = sizeof(CK_KEY_DERIVATION_STRING_DATA);

  subject_and_secret =
    PK11_Derive(shared_secret, 
                CKM_CONCATENATE_DATA_AND_BASE, 
                &concat_data_base_item, 
                CKM_CONCATENATE_BASE_AND_DATA, 
                CKA_DERIVE, 
                0); 

  if (!subject_and_secret)
    return NS_ERROR_FAILURE;

  CK_KEY_DERIVATION_STRING_DATA concat_base_data;
  concat_base_data.pData = keyInfo->ecPopCert->derSubject.data;
  concat_base_data.ulLen = keyInfo->ecPopCert->derSubject.len;
  SECItem concat_base_data_item;
  concat_base_data_item.data = (unsigned char*)&concat_base_data;
  concat_base_data_item.len = sizeof(CK_KEY_DERIVATION_STRING_DATA);

  subject_and_secret_and_issuer =
    PK11_Derive(subject_and_secret, 
                CKM_CONCATENATE_BASE_AND_DATA, 
                &concat_base_data_item, 
                CKM_SHA1_KEY_DERIVATION, 
                CKA_DERIVE, 
                0); 

  if (!subject_and_secret_and_issuer)
    return NS_ERROR_FAILURE;

  sha1_of_subject_and_secret_and_issuer =
    PK11_Derive(subject_and_secret_and_issuer, 
                CKM_SHA1_KEY_DERIVATION, 
                nullptr, 
                CKM_SHA_1_HMAC, 
                CKA_SIGN, 
                0); 

  if (!sha1_of_subject_and_secret_and_issuer)
    return NS_ERROR_FAILURE;

  PK11Context *context = nullptr;
  PK11ContextCleanerTrueParam context_cleaner(context);

  SECItem ignore;
  ignore.data = 0;
  ignore.len = 0;

  context = 
    PK11_CreateContextBySymKey(CKM_SHA_1_HMAC, 
                               CKA_SIGN, 
                               sha1_of_subject_and_secret_and_issuer, 
                               &ignore); 

  if (!context)
    return NS_ERROR_FAILURE;

  if (SECSuccess != PK11_DigestBegin(context))
    return NS_ERROR_FAILURE;

  if (SECSuccess != 
      PK11_DigestOp(context, der_request->data, der_request->len))
    return NS_ERROR_FAILURE;

  ScopedAutoSECItem result_hmac_sha1_item(SHA1_LENGTH);

  if (SECSuccess !=
      PK11_DigestFinal(context, 
                       result_hmac_sha1_item.data, 
                       &result_hmac_sha1_item.len, 
                       SHA1_LENGTH))
    return NS_ERROR_FAILURE;

  if (SECSuccess !=
      CRMF_CertReqMsgSetKeyAgreementPOP(certReqMsg, crmfDHMAC,
                                        crmfNoSubseqMess, &result_hmac_sha1_item))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

static nsresult
nsSetProofOfPossession(CRMFCertReqMsg *certReqMsg, 
                       nsKeyPairInfo  *keyInfo,
                       CRMFCertRequest *certReq)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool isEncryptionOnlyCertRequest = false;
  bool escrowEncryptionOnlyCert = false;
  
  switch (keyInfo->keyGenType)
  {
    case rsaEnc:
    case ecEnc:
      isEncryptionOnlyCertRequest = true;
      break;
    
    case rsaSign:
    case rsaDualUse:
    case rsaNonrepudiation:
    case rsaSignNonrepudiation:
    case ecSign:
    case ecDualUse:
    case ecNonrepudiation:
    case ecSignNonrepudiation:
    case dsaSign:
    case dsaNonrepudiation:
    case dsaSignNonrepudiation:
      break;
    
    case dhEx:
    


    default:
      return NS_ERROR_FAILURE;
  };
    
  if (isEncryptionOnlyCertRequest)
  {
    escrowEncryptionOnlyCert = 
      CRMF_CertRequestIsControlPresent(certReq,crmfPKIArchiveOptionsControl);
  }
    
  bool gotDHMACParameters = false;
  
  if (isECKeyGenType(keyInfo->keyGenType) && 
      keyInfo->ecPopCert && 
      keyInfo->ecPopPubKey)
  {
    gotDHMACParameters = true;
  }
  
  if (isEncryptionOnlyCertRequest)
  {
    if (escrowEncryptionOnlyCert)
      return nsSetKeyEnciphermentPOP(certReqMsg, true); 
    
    if (gotDHMACParameters)
      return nsSet_EC_DHMAC_ProofOfPossession(certReqMsg, keyInfo, certReq);
    
    return nsSetKeyEnciphermentPOP(certReqMsg, false); 
  }
  
  
  
  SECStatus srv = CRMF_CertReqMsgSetSignaturePOP(certReqMsg,
                                                 keyInfo->privKey,
                                                 keyInfo->pubKey, nullptr,
                                                 nullptr, nullptr);

  if (srv == SECSuccess)
    return NS_OK;
  
  if (!gotDHMACParameters)
    return NS_ERROR_FAILURE;
  
  return nsSet_EC_DHMAC_ProofOfPossession(certReqMsg, keyInfo, certReq);
}

static void
nsCRMFEncoderItemCount(void *arg, const char *buf, unsigned long len)
{
  unsigned long *count = (unsigned long *)arg;
  *count += len;
}

static void
nsCRMFEncoderItemStore(void *arg, const char *buf, unsigned long len)
{
  SECItem *dest = (SECItem *)arg;
  memcpy(dest->data + dest->len, buf, len);
  dest->len += len;
}

static SECItem*
nsEncodeCertReqMessages(CRMFCertReqMsg **certReqMsgs)
{
  unsigned long len = 0;
  if (CRMF_EncodeCertReqMessages(certReqMsgs, nsCRMFEncoderItemCount, &len)
      != SECSuccess) {
    return nullptr;
  }
  SECItem *dest = (SECItem *)PORT_Alloc(sizeof(SECItem));
  if (!dest) {
    return nullptr;
  }
  dest->type = siBuffer;
  dest->data = (unsigned char *)PORT_Alloc(len);
  if (!dest->data) {
    PORT_Free(dest);
    return nullptr;
  }
  dest->len = 0;

  if (CRMF_EncodeCertReqMessages(certReqMsgs, nsCRMFEncoderItemStore, dest)
      != SECSuccess) {
    SECITEM_FreeItem(dest, true);
    return nullptr;
  }
  return dest;
}





static char*
nsCreateReqFromKeyPairs(nsKeyPairInfo *keyids, int32_t numRequests,
                        char *reqDN, char *regToken, char *authenticator,
                        nsNSSCertificate *wrappingCert) 
{
  
  
  int32_t i;
  
  
  CRMFCertReqMsg **certReqMsgs = new CRMFCertReqMsg*[numRequests+1];
  CRMFCertRequest *certReq;
  if (!certReqMsgs)
    return nullptr;
  memset(certReqMsgs, 0, sizeof(CRMFCertReqMsg*)*(1+numRequests));
  SECStatus srv;
  nsresult rv;
  SECItem *encodedReq;
  char *retString;
  for (i=0; i<numRequests; i++) {
    certReq = nsCreateSingleCertReq(&keyids[i], reqDN, regToken, authenticator,
                                    wrappingCert);
    if (!certReq)
      goto loser;

    certReqMsgs[i] = CRMF_CreateCertReqMsg();
    if (!certReqMsgs[i])
      goto loser;
    srv = CRMF_CertReqMsgSetCertRequest(certReqMsgs[i], certReq);
    if (srv != SECSuccess)
      goto loser;

    rv = nsSetProofOfPossession(certReqMsgs[i], &keyids[i], certReq);
    if (NS_FAILED(rv))
      goto loser;
    CRMF_DestroyCertRequest(certReq);
  }
  encodedReq = nsEncodeCertReqMessages(certReqMsgs);
  nsFreeCertReqMessages(certReqMsgs, numRequests);

  retString = NSSBase64_EncodeItem (nullptr, nullptr, 0, encodedReq);
  SECITEM_FreeItem(encodedReq, true);
  return retString;
loser:
  nsFreeCertReqMessages(certReqMsgs,numRequests);
  return nullptr;
}



CRMFObject*
nsCrypto::GenerateCRMFRequest(JSContext* aContext,
                              const nsCString& aReqDN,
                              const nsCString& aRegToken,
                              const nsCString& aAuthenticator,
                              const nsCString& aEaCert,
                              const nsCString& aJsCallback,
                              const Sequence<JS::Value>& aArgs,
                              ErrorResult& aRv)
{
  nsNSSShutDownPreventionLock locker;
  nsresult nrv;

  uint32_t argc = aArgs.Length();

  


  if (argc % 3 != 0) {
    aRv.ThrowNotEnoughArgsError();
    return nullptr;
  }

  if (aReqDN.IsVoid()) {
    NS_WARNING("no DN specified");
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (aJsCallback.IsVoid()) {
    NS_WARNING("no completion function specified");
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsIGlobalObject> globalObject = do_QueryInterface(GetParentObject());
  if (MOZ_UNLIKELY(!globalObject)) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIContentSecurityPolicy> csp;
  if (!nsContentUtils::GetContentSecurityPolicy(getter_AddRefs(csp))) {
    NS_ERROR("Error: failed to get CSP");
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  bool evalAllowed = true;
  bool reportEvalViolations = false;
  if (csp && NS_FAILED(csp->GetAllowsEval(&reportEvalViolations, &evalAllowed))) {
    NS_WARNING("CSP: failed to get allowsEval");
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (reportEvalViolations) {
    NS_NAMED_LITERAL_STRING(scriptSample, "window.crypto.generateCRMFRequest: call to eval() or related function blocked by CSP");

    const char *fileName;
    uint32_t lineNum;
    nsJSUtils::GetCallingLocation(aContext, &fileName, &lineNum);
    csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_EVAL,
                             NS_ConvertASCIItoUTF16(fileName),
                             scriptSample,
                             lineNum,
                             EmptyString(),
                             EmptyString());
  }

  if (!evalAllowed) {
    NS_WARNING("eval() not allowed by Content Security Policy");
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  
  
  
  
  nsNSSCertificate *escrowCert = nullptr;
  nsCOMPtr<nsIX509Cert> nssCert;
  bool willEscrow = false;
  if (!aEaCert.IsVoid()) {
    SECItem certDer = {siBuffer, nullptr, 0};
    SECStatus srv = ATOB_ConvertAsciiToItem(&certDer, aEaCert.get());
    if (srv != SECSuccess) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }
    ScopedCERTCertificate cert(
      CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                              &certDer, nullptr, false, true));
    if (!cert) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    escrowCert = nsNSSCertificate::Create(cert.get());
    nssCert = escrowCert;
    if (!nssCert) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }

    nsCOMPtr<nsIDOMCryptoDialogs> dialogs;
    nsresult rv = getNSSDialogs(getter_AddRefs(dialogs),
                                NS_GET_IID(nsIDOMCryptoDialogs),
                                NS_DOMCRYPTODIALOGS_CONTRACTID);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return nullptr;
    }

    bool okay=false;
    {
      nsPSMUITracker tracker;
      if (tracker.isUIForbidden()) {
        okay = false;
      }
      else {
        dialogs->ConfirmKeyEscrow(nssCert, &okay);
      }
    }
    if (!okay) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }
    willEscrow = true;
  }
  nsCOMPtr<nsIInterfaceRequestor> uiCxt = new PipUIContext;
  int32_t numRequests = argc / 3;
  nsKeyPairInfo *keyids = new nsKeyPairInfo[numRequests];
  memset(keyids, 0, sizeof(nsKeyPairInfo)*numRequests);
  int keyInfoIndex;
  uint32_t i;
  PK11SlotInfo *slot = nullptr;
  
  for (i=0,keyInfoIndex=0; i<argc; i+=3,keyInfoIndex++) {
    nrv = cryptojs_ReadArgsAndGenerateKey(aContext,
                                          const_cast<JS::Value*>(&aArgs[i]),
                                          &keyids[keyInfoIndex],
                                          uiCxt, &slot, willEscrow);

    if (NS_FAILED(nrv)) {
      if (slot)
        PK11_FreeSlot(slot);
      nsFreeKeyPairInfo(keyids,numRequests);
      aRv.Throw(nrv);
      return nullptr;
    }
  }
  
  NS_ASSERTION(slot, "There was no slot selected for key generation");
  if (slot)
    PK11_FreeSlot(slot);

  char *encodedRequest = nsCreateReqFromKeyPairs(keyids,numRequests,
                                                 const_cast<char*>(aReqDN.get()),
                                                 const_cast<char*>(aRegToken.get()),
                                                 const_cast<char*>(aAuthenticator.get()),
                                                 escrowCert);
  if (!encodedRequest) {
    nsFreeKeyPairInfo(keyids, numRequests);
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  CRMFObject* newObject = new CRMFObject();
  newObject->SetCRMFRequest(encodedRequest);
  PORT_Free(encodedRequest);
  nsFreeKeyPairInfo(keyids, numRequests);

  
  
  
  
  
  
  
  
  

  MOZ_ASSERT(nsContentUtils::GetCurrentJSContext());
  nsCryptoRunArgs *args = new nsCryptoRunArgs();

  args->m_globalObject = globalObject;
  if (!aJsCallback.IsVoid()) {
    args->m_jsCallback = aJsCallback;
  }

  nsRefPtr<nsCryptoRunnable> cryptoRunnable(new nsCryptoRunnable(args));

  nsresult rv = NS_DispatchToMainThread(cryptoRunnable);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }

  return newObject;
}



nsP12Runnable::nsP12Runnable(nsIX509Cert **certArr, int32_t numCerts,
                             nsIPK11Token *token)
{
  mCertArr  = certArr;
  mNumCerts = numCerts;
  mToken = token;
}

nsP12Runnable::~nsP12Runnable()
{
  int32_t i;
  for (i=0; i<mNumCerts; i++) {
      NS_IF_RELEASE(mCertArr[i]);
  }
  delete []mCertArr;
}



NS_IMETHODIMP
nsP12Runnable::Run()
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  NS_ASSERTION(NS_IsMainThread(), "nsP12Runnable dispatched to the wrong thread");

  nsNSSShutDownPreventionLock locker;
  NS_ASSERTION(mCertArr, "certArr is NULL while trying to back up");

  nsString final;
  nsString temp;
  nsresult rv;

  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  
  
  nssComponent->GetPIPNSSBundleString("ForcedBackup1", final);
  final.Append(MOZ_UTF16("\n\n"));
  nssComponent->GetPIPNSSBundleString("ForcedBackup2", temp);
  final.Append(temp.get());
  final.Append(MOZ_UTF16("\n\n"));

  nssComponent->GetPIPNSSBundleString("ForcedBackup3", temp);

  final.Append(temp.get());
  nsNSSComponent::ShowAlertWithConstructedString(final);

  nsCOMPtr<nsIFilePicker> filePicker = 
                        do_CreateInstance("@mozilla.org/filepicker;1", &rv);
  if (!filePicker) {
    NS_ERROR("Could not create a file picker when backing up certs.");
    return rv;
  }

  nsCOMPtr<nsIWindowWatcher> wwatch =
    (do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> window;
  wwatch->GetActiveWindow(getter_AddRefs(window));

  nsString filePickMessage;
  nssComponent->GetPIPNSSBundleString("chooseP12BackupFileDialog",
                                      filePickMessage);
  rv = filePicker->Init(window, filePickMessage, nsIFilePicker::modeSave);
  NS_ENSURE_SUCCESS(rv, rv);

  filePicker->AppendFilter(NS_LITERAL_STRING("PKCS12"),
                           NS_LITERAL_STRING("*.p12"));
  filePicker->AppendFilters(nsIFilePicker::filterAll);

  int16_t dialogReturn;
  filePicker->Show(&dialogReturn);
  if (dialogReturn == nsIFilePicker::returnCancel)
    return NS_OK;  
                   

  nsCOMPtr<nsIFile> localFile;
  rv = filePicker->GetFile(getter_AddRefs(localFile));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsPKCS12Blob p12Cxt;
  
  p12Cxt.SetToken(mToken);
  p12Cxt.ExportToFile(localFile, mCertArr, mNumCerts);
  return NS_OK;
}

nsCryptoRunArgs::nsCryptoRunArgs() {}

nsCryptoRunArgs::~nsCryptoRunArgs() {}

nsCryptoRunnable::nsCryptoRunnable(nsCryptoRunArgs *args)
{
  nsNSSShutDownPreventionLock locker;
  NS_ASSERTION(args,"Passed nullptr to nsCryptoRunnable constructor.");
  m_args = args;
  NS_IF_ADDREF(m_args);
}

nsCryptoRunnable::~nsCryptoRunnable()
{
  nsNSSShutDownPreventionLock locker;
  NS_IF_RELEASE(m_args);
}



NS_IMETHODIMP
nsCryptoRunnable::Run()
{
  nsNSSShutDownPreventionLock locker;

  
  
  AutoEntryScript aes(m_args->m_globalObject);
  JSContext* cx = aes.cx();
  JS::RootedObject scope(cx, JS::CurrentGlobalOrNull(cx));

  bool ok =
    JS_EvaluateScript(cx, scope, m_args->m_jsCallback,
                      strlen(m_args->m_jsCallback), nullptr, 0);
  return ok ? NS_OK : NS_ERROR_FAILURE;
}



static bool
nsCertAlreadyExists(SECItem *derCert)
{
  CERTCertDBHandle *handle = CERT_GetDefaultCertDB();
  bool retVal = false;

  ScopedCERTCertificate cert(CERT_FindCertByDERCert(handle, derCert));
  if (cert) {
    if (cert->isperm && !cert->nickname && !cert->emailAddr) {
      
      
      SEC_DeletePermCertificate(cert.get());
    } else if (cert->isperm) {
      retVal = true;
    }
  }
  return retVal;
}

static int32_t
nsCertListCount(CERTCertList *certList)
{
  int32_t numCerts = 0;
  CERTCertListNode *node;

  node = CERT_LIST_HEAD(certList);
  while (!CERT_LIST_END(node, certList)) {
    numCerts++;
    node = CERT_LIST_NEXT(node);
  }
  return numCerts;
}



void
nsCrypto::ImportUserCertificates(const nsAString& aNickname,
                                 const nsAString& aCmmfResponse,
                                 bool aDoForcedBackup,
                                 nsAString& aReturn,
                                 ErrorResult& aRv)
{
  nsNSSShutDownPreventionLock locker;
  char *nickname=nullptr, *cmmfResponse=nullptr;
  CMMFCertRepContent *certRepContent = nullptr;
  int numResponses = 0;
  nsIX509Cert **certArr = nullptr;
  int i;
  CMMFCertResponse *currResponse;
  CMMFPKIStatus reqStatus;
  CERTCertificate *currCert;
  PK11SlotInfo *slot;
  nsAutoCString localNick;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPK11Token> token;

  nickname = ToNewCString(aNickname);
  cmmfResponse = ToNewCString(aCmmfResponse);
  if (nsCRT::strcmp("null", nickname) == 0) {
    nsMemory::Free(nickname);
    nickname = nullptr;
  }

  SECItem cmmfDer = {siBuffer, nullptr, 0};
  SECStatus srv = ATOB_ConvertAsciiToItem(&cmmfDer, cmmfResponse);

  if (srv != SECSuccess) {
    rv = NS_ERROR_FAILURE;
    goto loser;
  }

  certRepContent = CMMF_CreateCertRepContentFromDER(CERT_GetDefaultCertDB(),
                                                    (const char*)cmmfDer.data,
                                                    cmmfDer.len);
  if (!certRepContent) {
    rv = NS_ERROR_FAILURE;
    goto loser;
  }

  numResponses = CMMF_CertRepContentGetNumResponses(certRepContent);

  if (aDoForcedBackup) {
    
    
    
    certArr = new nsIX509Cert*[numResponses];
    
    
    if (!certArr)
      aDoForcedBackup = false;

    memset(certArr, 0, sizeof(nsIX509Cert*)*numResponses);
  }
  for (i=0; i<numResponses; i++) {
    currResponse = CMMF_CertRepContentGetResponseAtIndex(certRepContent,i);
    if (!currResponse) {
      rv = NS_ERROR_FAILURE;
      goto loser;
    }
    reqStatus = CMMF_CertResponseGetPKIStatusInfoStatus(currResponse);
    if (!(reqStatus == cmmfGranted || reqStatus == cmmfGrantedWithMods)) {
      
      rv = NS_ERROR_FAILURE;
      goto loser;
    }
    currCert = CMMF_CertResponseGetCertificate(currResponse, 
                                               CERT_GetDefaultCertDB());
    if (!currCert) {
      rv = NS_ERROR_FAILURE;
      goto loser;
    }

    if (nsCertAlreadyExists(&currCert->derCert)) {
      if (aDoForcedBackup) {
        certArr[i] = nsNSSCertificate::Create(currCert);
        if (!certArr[i])
          goto loser;
        NS_ADDREF(certArr[i]);
      }
      CERT_DestroyCertificate(currCert);
      CMMF_DestroyCertResponse(currResponse);
      continue;
    }
    
    
    
    if (currCert->nickname) {
      localNick = currCert->nickname;
    }
    else if (!nickname || nickname[0] == '\0') {
      nsNSSCertificateDB::get_default_nickname(currCert, ctx, localNick, locker);
    } else {
      
      
      
      
      localNick = nickname;
    }
    {
      char *cast_const_away = const_cast<char*>(localNick.get());
      slot = PK11_ImportCertForKey(currCert, cast_const_away, ctx);
    }
    if (!slot) {
      rv = NS_ERROR_FAILURE;
      goto loser;
    }
    if (aDoForcedBackup) {
      certArr[i] = nsNSSCertificate::Create(currCert);
      if (!certArr[i])
        goto loser;
      NS_ADDREF(certArr[i]);
    }
    CERT_DestroyCertificate(currCert);

    if (!token)
      token = new nsPK11Token(slot);

    PK11_FreeSlot(slot);
    CMMF_DestroyCertResponse(currResponse);
  }
  
  
  

  
 {
  ScopedCERTCertList caPubs(CMMF_CertRepContentGetCAPubs(certRepContent));
  if (caPubs) {
    int32_t numCAs = nsCertListCount(caPubs.get());
    
    NS_ASSERTION(numCAs > 0, "Invalid number of CA's");
    if (numCAs > 0) {
      CERTCertListNode *node;
      SECItem *derCerts;

      derCerts = static_cast<SECItem*>
                            (nsMemory::Alloc(sizeof(SECItem)*numCAs));
      if (!derCerts) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto loser;
      }
      for (node = CERT_LIST_HEAD(caPubs), i=0; 
           !CERT_LIST_END(node, caPubs);
           node = CERT_LIST_NEXT(node), i++) {
        derCerts[i] = node->cert->derCert;
      }
      nsNSSCertificateDB::ImportValidCACerts(numCAs, derCerts, ctx, locker);
      nsMemory::Free(derCerts);
    }
  }
 }

  if (aDoForcedBackup) {
    
    
    
    nsCOMPtr<nsIRunnable> p12Runnable = new nsP12Runnable(certArr, numResponses,
                                                          token);
    if (!p12Runnable) {
      rv = NS_ERROR_FAILURE;
      goto loser;
    }

    
    
    
    certArr = nullptr;

    rv = NS_DispatchToMainThread(p12Runnable);
    if (NS_FAILED(rv))
      goto loser;
  }

 loser:
  if (certArr) {
    for (i=0; i<numResponses; i++) {
      NS_IF_RELEASE(certArr[i]);
    }
    delete []certArr;
  }
  aReturn.Assign(EmptyString());
  if (nickname) {
    NS_Free(nickname);
  }
  if (cmmfResponse) {
    NS_Free(cmmfResponse);
  }
  if (certRepContent) {
    CMMF_DestroyCertRepContent(certRepContent);
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
}

static void
GetDocumentFromContext(JSContext *cx, nsIDocument **aDocument)
{
  
  nsIScriptContext* scriptContext = GetScriptContextFromJSContext(cx);
  if (!scriptContext) {
    return;
  }

  nsCOMPtr<nsIDOMWindow> domWindow = 
    do_QueryInterface(scriptContext->GetGlobalObject());
  if (!domWindow) {
    return;
  }

  nsCOMPtr<nsIDOMDocument> domDocument;
  domWindow->GetDocument(getter_AddRefs(domDocument));
  if (!domDocument) {
    return;
  }

  CallQueryInterface(domDocument, aDocument);

  return;
}

void signTextOutputCallback(void *arg, const char *buf, unsigned long len)
{
  ((nsCString*)arg)->Append(buf, len);
}

void
nsCrypto::SignText(JSContext* aContext,
                   const nsAString& aStringToSign,
                   const nsAString& aCaOption,
                   const Sequence<nsCString>& aArgs,
                   nsAString& aReturn)
{
  
  
  NS_NAMED_LITERAL_STRING(internalError, "error:internalError");

  aReturn.Truncate();

  uint32_t argc = aArgs.Length();

  if (!aCaOption.EqualsLiteral("auto") &&
      !aCaOption.EqualsLiteral("ask")) {
    NS_WARNING("caOption argument must be ask or auto");
    aReturn.Append(internalError);

    return;
  }

  
  

  nsCOMPtr<nsIInterfaceRequestor> uiContext = new PipUIContext;
  if (!uiContext) {
    aReturn.Append(internalError);

    return;
  }

  bool bestOnly = true;
  bool validOnly = true;
  CERTCertList* certList =
    CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), certUsageEmailSigner,
                              bestOnly, validOnly, uiContext);

  uint32_t numCAs = argc;
  if (numCAs > 0) {
    nsAutoArrayPtr<char*> caNames(new char*[numCAs]);
    if (!caNames) {
      aReturn.Append(internalError);
      return;
    }

    uint32_t i;
    for (i = 0; i < numCAs; ++i)
      caNames[i] = const_cast<char*>(aArgs[i].get());

    if (certList &&
        CERT_FilterCertListByCANames(certList, numCAs, caNames,
                                     certUsageEmailSigner) != SECSuccess) {
      aReturn.Append(internalError);

      return;
    }
  }

  if (!certList || CERT_LIST_EMPTY(certList)) {
    aReturn.AppendLiteral("error:noMatchingCert");

    return;
  }

  nsCOMPtr<nsIFormSigningDialog> fsd =
    do_CreateInstance(NS_FORMSIGNINGDIALOG_CONTRACTID);
  if (!fsd) {
    aReturn.Append(internalError);

    return;
  }

  nsCOMPtr<nsIDocument> document;
  GetDocumentFromContext(aContext, getter_AddRefs(document));
  if (!document) {
    aReturn.Append(internalError);

    return;
  }

  
  nsIURI* uri = document->GetDocumentURI();
  if (!uri) {
    aReturn.Append(internalError);

    return;
  }

  nsresult rv;

  nsCString host;
  rv = uri->GetHost(host);
  if (NS_FAILED(rv)) {
    aReturn.Append(internalError);

    return;
  }

  int32_t numberOfCerts = 0;
  CERTCertListNode* node;
  for (node = CERT_LIST_HEAD(certList); !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    ++numberOfCerts;
  }

  ScopedCERTCertNicknames nicknames(getNSSCertNicknamesFromCertList(certList));

  if (!nicknames) {
    aReturn.Append(internalError);

    return;
  }

  NS_ASSERTION(nicknames->numnicknames == numberOfCerts,
               "nicknames->numnicknames != numberOfCerts");

  nsAutoArrayPtr<char16_t*> certNicknameList(new char16_t*[nicknames->numnicknames * 2]);
  if (!certNicknameList) {
    aReturn.Append(internalError);

    return;
  }

  char16_t** certDetailsList = certNicknameList.get() + nicknames->numnicknames;

  int32_t certsToUse;
  for (node = CERT_LIST_HEAD(certList), certsToUse = 0;
       !CERT_LIST_END(node, certList) && certsToUse < nicknames->numnicknames;
       node = CERT_LIST_NEXT(node)) {
    RefPtr<nsNSSCertificate> tempCert(nsNSSCertificate::Create(node->cert));
    if (tempCert) {
      nsAutoString nickWithSerial, details;
      rv = tempCert->FormatUIStrings(NS_ConvertUTF8toUTF16(nicknames->nicknames[certsToUse]),
                                     nickWithSerial, details);
      if (NS_SUCCEEDED(rv)) {
        certNicknameList[certsToUse] = ToNewUnicode(nickWithSerial);
        if (certNicknameList[certsToUse]) {
          certDetailsList[certsToUse] = ToNewUnicode(details);
          if (!certDetailsList[certsToUse]) {
            nsMemory::Free(certNicknameList[certsToUse]);
            continue;
          }
          ++certsToUse;
        }
      }
    }
  }

  if (certsToUse == 0) {
    aReturn.Append(internalError);

    return;
  }

  NS_ConvertUTF8toUTF16 utf16Host(host);

  CERTCertificate *signingCert = nullptr;
  bool tryAgain, canceled;
  nsAutoString password;
  do {
    
    
    int32_t selectedIndex = -1;
    rv = fsd->ConfirmSignText(uiContext, utf16Host, aStringToSign,
                              const_cast<const char16_t**>(certNicknameList.get()),
                              const_cast<const char16_t**>(certDetailsList),
                              certsToUse, &selectedIndex, password,
                              &canceled);
    if (NS_FAILED(rv) || canceled) {
      break; 
    }

    int32_t j = 0;
    for (node = CERT_LIST_HEAD(certList); !CERT_LIST_END(node, certList);
         node = CERT_LIST_NEXT(node)) {
      if (j == selectedIndex) {
        signingCert = CERT_DupCertificate(node->cert);
        break; 
      }
      ++j;
    }

    if (!signingCert) {
      rv = NS_ERROR_FAILURE;
      break; 
    }

    NS_ConvertUTF16toUTF8 pwUtf8(password);

    tryAgain =
      PK11_CheckUserPassword(signingCert->slot,
                             const_cast<char *>(pwUtf8.get())) != SECSuccess;
    
  } while (tryAgain);

  int32_t k;
  for (k = 0; k < certsToUse; ++k) {
    nsMemory::Free(certNicknameList[k]);
    nsMemory::Free(certDetailsList[k]);
  }

  if (NS_FAILED(rv)) { 
    aReturn.Append(internalError);

    return;
  }

  if (canceled) {
    aReturn.AppendLiteral("error:userCancel");

    return;
  }

  SECKEYPrivateKey* privKey = PK11_FindKeyByAnyCert(signingCert, uiContext);
  if (!privKey) {
    aReturn.Append(internalError);

    return;
  }

  nsAutoCString charset(document->GetDocumentCharacterSet());

  
  
  if (charset.EqualsLiteral("ISO-8859-1")) {
    charset.AssignLiteral("windows-1252");
  }

  nsCOMPtr<nsISaveAsCharset> encoder =
    do_CreateInstance(NS_SAVEASCHARSET_CONTRACTID);
  if (encoder) {
    rv = encoder->Init(charset.get(),
                       (nsISaveAsCharset::attr_EntityAfterCharsetConv + 
                       nsISaveAsCharset::attr_FallbackDecimalNCR),
                       0);
  }

  nsXPIDLCString buffer;
  if (aStringToSign.Length() > 0) {
    if (encoder && NS_SUCCEEDED(rv)) {
      rv = encoder->Convert(PromiseFlatString(aStringToSign).get(),
                            getter_Copies(buffer));
      if (NS_FAILED(rv)) {
        aReturn.Append(internalError);

        return;
      }
    }
    else {
      AppendUTF16toUTF8(aStringToSign, buffer);
    }
  }

  HASHContext *hc = HASH_Create(HASH_AlgSHA1);
  if (!hc) {
    aReturn.Append(internalError);

    return;
  }

  unsigned char hash[SHA1_LENGTH];

  SECItem digest;
  digest.data = hash;

  HASH_Begin(hc);
  HASH_Update(hc, reinterpret_cast<const unsigned char*>(buffer.get()),
              buffer.Length());
  HASH_End(hc, digest.data, &digest.len, SHA1_LENGTH);
  HASH_Destroy(hc);

  nsCString p7;
  SECStatus srv = SECFailure;

  SEC_PKCS7ContentInfo *ci = SEC_PKCS7CreateSignedData(signingCert,
                                                       certUsageEmailSigner,
                                                       nullptr, SEC_OID_SHA1,
                                                       &digest, nullptr, uiContext);
  if (ci) {
    srv = SEC_PKCS7IncludeCertChain(ci, nullptr);
    if (srv == SECSuccess) {
      srv = SEC_PKCS7AddSigningTime(ci);
      if (srv == SECSuccess) {
        srv = SEC_PKCS7Encode(ci, signTextOutputCallback, &p7, nullptr, nullptr,
                              uiContext);
      }
    }

    SEC_PKCS7DestroyContentInfo(ci);
  }

  if (srv != SECSuccess) {
    aReturn.Append(internalError);

    return;
  }

  SECItem binary_item;
  binary_item.data = reinterpret_cast<unsigned char*>
                                     (const_cast<char*>(p7.get()));
  binary_item.len = p7.Length();

  char *result = NSSBase64_EncodeItem(nullptr, nullptr, 0, &binary_item);
  if (result) {
    AppendASCIItoUTF16(result, aReturn);
  }
  else {
    aReturn.Append(internalError);
  }

  PORT_Free(result);

  return;
}

void
nsCrypto::Logout(ErrorResult& aRv)
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }

  {
    nsNSSShutDownPreventionLock locker;
    PK11_LogoutAll();
    SSL_ClearSessionCache();
  }

  rv = nssComponent->LogoutAuthenticatedPK11();
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
}

CRMFObject::CRMFObject()
{
  MOZ_COUNT_CTOR(CRMFObject);
}

CRMFObject::~CRMFObject()
{
  MOZ_COUNT_DTOR(CRMFObject);
}

JSObject*
CRMFObject::WrapObject(JSContext *aCx, bool* aTookOwnership)
{
  return CRMFObjectBinding::Wrap(aCx, this, aTookOwnership);
}

void
CRMFObject::GetRequest(nsAString& aRequest)
{
  aRequest.Assign(mBase64Request);
}

nsresult
CRMFObject::SetCRMFRequest(char *inRequest)
{
  mBase64Request.AssignWithConversion(inRequest);  
  return NS_OK;
}

#endif 

nsPkcs11::nsPkcs11()
{
}

nsPkcs11::~nsPkcs11()
{
}


NS_IMETHODIMP
nsPkcs11::DeleteModule(const nsAString& aModuleName)
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  nsString errorMessage;

  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  if (aModuleName.IsEmpty()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  NS_ConvertUTF16toUTF8 modName(aModuleName);
  int32_t modType;
  SECStatus srv = SECMOD_DeleteModule(modName.get(), &modType);
  if (srv == SECSuccess) {
    SECMODModule *module = SECMOD_FindModule(modName.get());
    if (module) {
#ifndef MOZ_DISABLE_CRYPTOLEGACY
      nssComponent->ShutdownSmartCardThread(module);
#endif
      SECMOD_DestroyModule(module);
    }
    rv = NS_OK;
  } else {
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}


NS_IMETHODIMP
nsPkcs11::AddModule(const nsAString& aModuleName, 
                    const nsAString& aLibraryFullPath, 
                    int32_t aCryptoMechanismFlags, 
                    int32_t aCipherFlags)
{
  NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));

  NS_ConvertUTF16toUTF8 moduleName(aModuleName);
  nsCString fullPath;
  
  NS_CopyUnicodeToNative(aLibraryFullPath, fullPath);
  uint32_t mechFlags = SECMOD_PubMechFlagstoInternal(aCryptoMechanismFlags);
  uint32_t cipherFlags = SECMOD_PubCipherFlagstoInternal(aCipherFlags);
  SECStatus srv = SECMOD_AddNewModule(moduleName.get(), fullPath.get(), 
                                      mechFlags, cipherFlags);
  if (srv == SECSuccess) {
    SECMODModule *module = SECMOD_FindModule(moduleName.get());
    if (module) {
#ifndef MOZ_DISABLE_CRYPTOLEGACY
      nssComponent->LaunchSmartCardThread(module);
#endif
      SECMOD_DestroyModule(module);
    }
  }

  
  
  switch (srv) {
  case SECSuccess:
    return NS_OK;
  case SECFailure:
    return NS_ERROR_FAILURE;
  case -2:
    return NS_ERROR_ILLEGAL_VALUE;
  }
  NS_ERROR("Bogus return value, this should never happen");
  return NS_ERROR_FAILURE;
}

