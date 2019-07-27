





#include "pk11pub.h"
#include "cryptohi.h"
#include "ScopedNSSTypes.h"
#include "mozilla/dom/CryptoKey.h"
#include "mozilla/dom/WebCryptoCommon.h"
#include "mozilla/dom/SubtleCryptoBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(CryptoKey, mGlobal, mAlgorithm)
NS_IMPL_CYCLE_COLLECTING_ADDREF(CryptoKey)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CryptoKey)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CryptoKey)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsresult
StringToUsage(const nsString& aUsage, CryptoKey::KeyUsage& aUsageOut)
{
  if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_ENCRYPT)) {
    aUsageOut = CryptoKey::ENCRYPT;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_DECRYPT)) {
    aUsageOut = CryptoKey::DECRYPT;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_SIGN)) {
    aUsageOut = CryptoKey::SIGN;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_VERIFY)) {
    aUsageOut = CryptoKey::VERIFY;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_DERIVEKEY)) {
    aUsageOut = CryptoKey::DERIVEKEY;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_DERIVEBITS)) {
    aUsageOut = CryptoKey::DERIVEBITS;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_WRAPKEY)) {
    aUsageOut = CryptoKey::WRAPKEY;
  } else if (aUsage.EqualsLiteral(WEBCRYPTO_KEY_USAGE_UNWRAPKEY)) {
    aUsageOut = CryptoKey::UNWRAPKEY;
  } else {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  return NS_OK;
}

CryptoKey::CryptoKey(nsIGlobalObject* aGlobal)
  : mGlobal(aGlobal)
  , mAttributes(0)
  , mSymKey()
  , mPrivateKey(nullptr)
  , mPublicKey(nullptr)
{
  SetIsDOMBinding();
}

CryptoKey::~CryptoKey()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return;
  }
  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

JSObject*
CryptoKey::WrapObject(JSContext* aCx)
{
  return CryptoKeyBinding::Wrap(aCx, this);
}

void
CryptoKey::GetType(nsString& aRetVal) const
{
  uint32_t type = mAttributes & TYPE_MASK;
  switch (type) {
    case PUBLIC:  aRetVal.AssignLiteral(WEBCRYPTO_KEY_TYPE_PUBLIC); break;
    case PRIVATE: aRetVal.AssignLiteral(WEBCRYPTO_KEY_TYPE_PRIVATE); break;
    case SECRET:  aRetVal.AssignLiteral(WEBCRYPTO_KEY_TYPE_SECRET); break;
  }
}

bool
CryptoKey::Extractable() const
{
  return (mAttributes & EXTRACTABLE);
}

KeyAlgorithm*
CryptoKey::Algorithm() const
{
  return mAlgorithm;
}

void
CryptoKey::GetUsages(nsTArray<nsString>& aRetVal) const
{
  if (mAttributes & ENCRYPT) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_ENCRYPT));
  }
  if (mAttributes & DECRYPT) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_DECRYPT));
  }
  if (mAttributes & SIGN) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_SIGN));
  }
  if (mAttributes & VERIFY) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_VERIFY));
  }
  if (mAttributes & DERIVEKEY) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_DERIVEKEY));
  }
  if (mAttributes & DERIVEBITS) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_DERIVEBITS));
  }
  if (mAttributes & WRAPKEY) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_WRAPKEY));
  }
  if (mAttributes & UNWRAPKEY) {
    aRetVal.AppendElement(NS_LITERAL_STRING(WEBCRYPTO_KEY_USAGE_UNWRAPKEY));
  }
}

CryptoKey::KeyType
CryptoKey::GetKeyType() const
{
  return static_cast<CryptoKey::KeyType>(mAttributes & TYPE_MASK);
}

nsresult
CryptoKey::SetType(const nsString& aType)
{
  mAttributes &= CLEAR_TYPE;
  if (aType.EqualsLiteral(WEBCRYPTO_KEY_TYPE_SECRET)) {
    mAttributes |= SECRET;
  } else if (aType.EqualsLiteral(WEBCRYPTO_KEY_TYPE_PUBLIC)) {
    mAttributes |= PUBLIC;
  } else if (aType.EqualsLiteral(WEBCRYPTO_KEY_TYPE_PRIVATE)) {
    mAttributes |= PRIVATE;
  } else {
    mAttributes |= UNKNOWN;
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  return NS_OK;
}

void
CryptoKey::SetType(CryptoKey::KeyType aType)
{
  mAttributes &= CLEAR_TYPE;
  mAttributes |= aType;
}

void
CryptoKey::SetExtractable(bool aExtractable)
{
  mAttributes &= CLEAR_EXTRACTABLE;
  if (aExtractable) {
    mAttributes |= EXTRACTABLE;
  }
}

void
CryptoKey::SetAlgorithm(KeyAlgorithm* aAlgorithm)
{
  mAlgorithm = aAlgorithm;
}

void
CryptoKey::ClearUsages()
{
  mAttributes &= CLEAR_USAGES;
}

nsresult
CryptoKey::AddUsage(const nsString& aUsage)
{
  return AddUsageIntersecting(aUsage, USAGES_MASK);
}

nsresult
CryptoKey::AddUsageIntersecting(const nsString& aUsage, uint32_t aUsageMask)
{
  KeyUsage usage;
  if (NS_FAILED(StringToUsage(aUsage, usage))) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  if (usage & aUsageMask) {
    AddUsage(usage);
    return NS_OK;
  }

  return NS_OK;
}

void
CryptoKey::AddUsage(CryptoKey::KeyUsage aUsage)
{
  mAttributes |= aUsage;
}

bool
CryptoKey::HasUsage(CryptoKey::KeyUsage aUsage)
{
  return !!(mAttributes & aUsage);
}

bool
CryptoKey::HasUsageOtherThan(uint32_t aUsages)
{
  return !!(mAttributes & USAGES_MASK & ~aUsages);
}

void CryptoKey::SetSymKey(const CryptoBuffer& aSymKey)
{
  mSymKey = aSymKey;
}

void
CryptoKey::SetPrivateKey(SECKEYPrivateKey* aPrivateKey)
{
  nsNSSShutDownPreventionLock locker;
  if (!aPrivateKey || isAlreadyShutDown()) {
    mPrivateKey = nullptr;
    return;
  }
  mPrivateKey = SECKEY_CopyPrivateKey(aPrivateKey);
}

void
CryptoKey::SetPublicKey(SECKEYPublicKey* aPublicKey)
{
  nsNSSShutDownPreventionLock locker;
  if (!aPublicKey || isAlreadyShutDown()) {
    mPublicKey = nullptr;
    return;
  }
  mPublicKey = SECKEY_CopyPublicKey(aPublicKey);
}

const CryptoBuffer&
CryptoKey::GetSymKey() const
{
  return mSymKey;
}

SECKEYPrivateKey*
CryptoKey::GetPrivateKey() const
{
  nsNSSShutDownPreventionLock locker;
  if (!mPrivateKey || isAlreadyShutDown()) {
    return nullptr;
  }
  return SECKEY_CopyPrivateKey(mPrivateKey.get());
}

SECKEYPublicKey*
CryptoKey::GetPublicKey() const
{
  nsNSSShutDownPreventionLock locker;
  if (!mPublicKey || isAlreadyShutDown()) {
    return nullptr;
  }
  return SECKEY_CopyPublicKey(mPublicKey.get());
}

void CryptoKey::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}

void CryptoKey::destructorSafeDestroyNSSReference()
{
  mPrivateKey.dispose();
  mPublicKey.dispose();
}




SECKEYPrivateKey*
CryptoKey::PrivateKeyFromPkcs8(CryptoBuffer& aKeyData,
                         const nsNSSShutDownPreventionLock& )
{
  SECKEYPrivateKey* privKey;
  ScopedPK11SlotInfo slot(PK11_GetInternalSlot());
  ScopedSECItem pkcs8Item(aKeyData.ToSECItem());
  if (!pkcs8Item) {
    return nullptr;
  }

  
  unsigned int usage = KU_ALL;

  SECStatus rv = PK11_ImportDERPrivateKeyInfoAndReturnKey(
                 slot.get(), pkcs8Item.get(), nullptr, nullptr, false, false,
                 usage, &privKey, nullptr);

  if (rv == SECFailure) {
    return nullptr;
  }
  return privKey;
}

SECKEYPublicKey*
CryptoKey::PublicKeyFromSpki(CryptoBuffer& aKeyData,
                       const nsNSSShutDownPreventionLock& )
{
  ScopedSECItem spkiItem(aKeyData.ToSECItem());
  if (!spkiItem) {
    return nullptr;
  }

  ScopedCERTSubjectPublicKeyInfo spki(SECKEY_DecodeDERSubjectPublicKeyInfo(spkiItem.get()));
  if (!spki) {
    return nullptr;
  }

  
  
  
  if (SECITEM_ItemsAreEqual(&SEC_OID_DATA_EC_DH, &spki->algorithm.algorithm)) {
    
    SECOidData* oidData = SECOID_FindOIDByTag(SEC_OID_ANSIX962_EC_PUBLIC_KEY);
    if (!oidData) {
      return nullptr;
    }

    SECStatus rv = SECITEM_CopyItem(spki->arena, &spki->algorithm.algorithm,
                                    &oidData->oid);
    if (rv != SECSuccess) {
      return nullptr;
    }
  }

  return SECKEY_ExtractPublicKey(spki.get());
}

nsresult
CryptoKey::PrivateKeyToPkcs8(SECKEYPrivateKey* aPrivKey,
                       CryptoBuffer& aRetVal,
                       const nsNSSShutDownPreventionLock& )
{
  ScopedSECItem pkcs8Item(PK11_ExportDERPrivateKeyInfo(aPrivKey, nullptr));
  if (!pkcs8Item.get()) {
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }
  aRetVal.Assign(pkcs8Item.get());
  return NS_OK;
}

nsresult
CryptoKey::PublicKeyToSpki(SECKEYPublicKey* aPubKey,
                     CryptoBuffer& aRetVal,
                     const nsNSSShutDownPreventionLock& )
{
  ScopedCERTSubjectPublicKeyInfo spki(SECKEY_CreateSubjectPublicKeyInfo(aPubKey));
  if (!spki) {
    return NS_ERROR_DOM_OPERATION_ERR;
  }

  
  
  
  if (aPubKey->keyType == ecKey) {
    SECStatus rv = SECITEM_CopyItem(spki->arena, &spki->algorithm.algorithm,
                                    &SEC_OID_DATA_EC_DH);
    if (rv != SECSuccess) {
      return NS_ERROR_DOM_OPERATION_ERR;
    }
  }

  const SEC_ASN1Template* tpl = SEC_ASN1_GET(CERT_SubjectPublicKeyInfoTemplate);
  ScopedSECItem spkiItem(SEC_ASN1EncodeItem(nullptr, nullptr, spki, tpl));

  aRetVal.Assign(spkiItem.get());
  return NS_OK;
}

SECItem*
CreateECPointForCoordinates(const CryptoBuffer& aX,
                            const CryptoBuffer& aY,
                            PLArenaPool* aArena)
{
  
  if (aX.Length() != aY.Length()) {
    return nullptr;
  }

  
  SECItem* point = ::SECITEM_AllocItem(aArena, nullptr, aX.Length() + aY.Length() + 1);
  if (!point) {
    return nullptr;
  }

  
  point->data[0] = EC_POINT_FORM_UNCOMPRESSED;
  memcpy(point->data + 1, aX.Elements(), aX.Length());
  memcpy(point->data + 1 + aX.Length(), aY.Elements(), aY.Length());

  return point;
}

SECKEYPrivateKey*
PrivateKeyFromPrivateKeyTemplate(SECItem* aObjID,
                                 CK_ATTRIBUTE* aTemplate,
                                 CK_ULONG aTemplateSize)
{
  
  ScopedPK11SlotInfo slot(PK11_GetInternalSlot());
  if (!slot.get()) {
    return nullptr;
  }

  ScopedPK11GenericObject obj(PK11_CreateGenericObject(slot.get(),
                                                       aTemplate,
                                                       aTemplateSize,
                                                       PR_FALSE));
  if (!obj.get()) {
    return nullptr;
  }

  
  
  ScopedSECKEYPrivateKey privKey(PK11_FindKeyByKeyID(slot.get(), aObjID,
                                                     nullptr));
  if (!privKey.get()) {
    return nullptr;
  }

  return SECKEY_CopyPrivateKey(privKey.get());
}

SECKEYPrivateKey*
CryptoKey::PrivateKeyFromJwk(const JsonWebKey& aJwk,
                             const nsNSSShutDownPreventionLock& )
{
  if (!aJwk.mKty.WasPassed()) {
    return nullptr;
  }

  CK_OBJECT_CLASS privateKeyValue = CKO_PRIVATE_KEY;
  CK_BBOOL falseValue = CK_FALSE;

  if (aJwk.mKty.Value().EqualsLiteral(JWK_TYPE_EC)) {
    
    CryptoBuffer x, y, d;
    if (!aJwk.mCrv.WasPassed() ||
        !aJwk.mX.WasPassed() || NS_FAILED(x.FromJwkBase64(aJwk.mX.Value())) ||
        !aJwk.mY.WasPassed() || NS_FAILED(y.FromJwkBase64(aJwk.mY.Value())) ||
        !aJwk.mD.WasPassed() || NS_FAILED(d.FromJwkBase64(aJwk.mD.Value()))) {
      return nullptr;
    }

    nsString namedCurve;
    if (!NormalizeNamedCurveValue(aJwk.mCrv.Value(), namedCurve)) {
      return nullptr;
    }

    ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
    if (!arena) {
      return nullptr;
    }

    
    SECItem* params = CreateECParamsForCurve(namedCurve, arena.get());
    if (!params) {
      return nullptr;
    }

    SECItem* ecPoint = CreateECPointForCoordinates(x, y, arena.get());
    if (!ecPoint) {
      return nullptr;
    }

    
    
    ScopedSECItem objID(PK11_MakeIDFromPubKey(ecPoint));
    if (!objID.get()) {
      return nullptr;
    }

    
    CK_KEY_TYPE ecValue = CKK_EC;
    CK_ATTRIBUTE keyTemplate[9] = {
      { CKA_CLASS,            &privateKeyValue,     sizeof(privateKeyValue) },
      { CKA_KEY_TYPE,         &ecValue,             sizeof(ecValue) },
      { CKA_TOKEN,            &falseValue,          sizeof(falseValue) },
      { CKA_SENSITIVE,        &falseValue,          sizeof(falseValue) },
      { CKA_PRIVATE,          &falseValue,          sizeof(falseValue) },
      { CKA_ID,               objID->data,          objID->len },
      { CKA_EC_PARAMS,        params->data,         params->len },
      { CKA_EC_POINT,         ecPoint->data,        ecPoint->len },
      { CKA_VALUE,            (void*) d.Elements(), d.Length() },
    };

    return PrivateKeyFromPrivateKeyTemplate(objID, keyTemplate,
                                            PR_ARRAY_SIZE(keyTemplate));
  }

  if (aJwk.mKty.Value().EqualsLiteral(JWK_TYPE_RSA)) {
    
    CryptoBuffer n, e, d, p, q, dp, dq, qi;
    if (!aJwk.mN.WasPassed() || NS_FAILED(n.FromJwkBase64(aJwk.mN.Value())) ||
        !aJwk.mE.WasPassed() || NS_FAILED(e.FromJwkBase64(aJwk.mE.Value())) ||
        !aJwk.mD.WasPassed() || NS_FAILED(d.FromJwkBase64(aJwk.mD.Value())) ||
        !aJwk.mP.WasPassed() || NS_FAILED(p.FromJwkBase64(aJwk.mP.Value())) ||
        !aJwk.mQ.WasPassed() || NS_FAILED(q.FromJwkBase64(aJwk.mQ.Value())) ||
        !aJwk.mDp.WasPassed() || NS_FAILED(dp.FromJwkBase64(aJwk.mDp.Value())) ||
        !aJwk.mDq.WasPassed() || NS_FAILED(dq.FromJwkBase64(aJwk.mDq.Value())) ||
        !aJwk.mQi.WasPassed() || NS_FAILED(qi.FromJwkBase64(aJwk.mQi.Value()))) {
      return nullptr;
    }

    
    
    ScopedSECItem nItem(n.ToSECItem());
    if (!nItem.get()) {
      return nullptr;
    }

    ScopedSECItem objID(PK11_MakeIDFromPubKey(nItem.get()));
    if (!objID.get()) {
      return nullptr;
    }

    
    CK_KEY_TYPE rsaValue = CKK_RSA;
    CK_ATTRIBUTE keyTemplate[14] = {
      { CKA_CLASS,            &privateKeyValue,      sizeof(privateKeyValue) },
      { CKA_KEY_TYPE,         &rsaValue,             sizeof(rsaValue) },
      { CKA_TOKEN,            &falseValue,           sizeof(falseValue) },
      { CKA_SENSITIVE,        &falseValue,           sizeof(falseValue) },
      { CKA_PRIVATE,          &falseValue,           sizeof(falseValue) },
      { CKA_ID,               objID->data,           objID->len },
      { CKA_MODULUS,          (void*) n.Elements(),  n.Length() },
      { CKA_PUBLIC_EXPONENT,  (void*) e.Elements(),  e.Length() },
      { CKA_PRIVATE_EXPONENT, (void*) d.Elements(),  d.Length() },
      { CKA_PRIME_1,          (void*) p.Elements(),  p.Length() },
      { CKA_PRIME_2,          (void*) q.Elements(),  q.Length() },
      { CKA_EXPONENT_1,       (void*) dp.Elements(), dp.Length() },
      { CKA_EXPONENT_2,       (void*) dq.Elements(), dq.Length() },
      { CKA_COEFFICIENT,      (void*) qi.Elements(), qi.Length() },
    };

    return PrivateKeyFromPrivateKeyTemplate(objID, keyTemplate,
                                            PR_ARRAY_SIZE(keyTemplate));
  }

  return nullptr;
}

bool ReadAndEncodeAttribute(SECKEYPrivateKey* aKey,
                            CK_ATTRIBUTE_TYPE aAttribute,
                            Optional<nsString>& aDst)
{
  ScopedSECItem item(new SECItem());
  if (PK11_ReadRawAttribute(PK11_TypePrivKey, aKey, aAttribute, item)
        != SECSuccess) {
    return false;
  }

  CryptoBuffer buffer;
  if (!buffer.Assign(item)) {
    return false;
  }

  if (NS_FAILED(buffer.ToJwkBase64(aDst.Value()))) {
    return false;
  }

  return true;
}

bool
ECKeyToJwk(const PK11ObjectType aKeyType, void* aKey, const SECItem* aEcParams,
           const SECItem* aPublicValue, JsonWebKey& aRetVal)
{
  aRetVal.mX.Construct();
  aRetVal.mY.Construct();

  
  if (!CheckEncodedECParameters(aEcParams)) {
    return false;
  }

  
  SECItem oid = { siBuffer, nullptr, 0 };
  oid.len = aEcParams->data[1];
  oid.data = aEcParams->data + 2;

  uint32_t flen;
  switch (SECOID_FindOIDTag(&oid)) {
    case SEC_OID_SECG_EC_SECP256R1:
      flen = 32; 
      aRetVal.mCrv.Construct(NS_LITERAL_STRING(WEBCRYPTO_NAMED_CURVE_P256));
      break;
    case SEC_OID_SECG_EC_SECP384R1:
      flen = 48; 
      aRetVal.mCrv.Construct(NS_LITERAL_STRING(WEBCRYPTO_NAMED_CURVE_P384));
      break;
    case SEC_OID_SECG_EC_SECP521R1:
      flen = 66; 
      aRetVal.mCrv.Construct(NS_LITERAL_STRING(WEBCRYPTO_NAMED_CURVE_P521));
      break;
    default:
      return false;
  }

  
  if (aPublicValue->data[0] != EC_POINT_FORM_UNCOMPRESSED) {
    return false;
  }

  
  if (aPublicValue->len != (2 * flen + 1)) {
    return false;
  }

  ScopedSECItem ecPointX(::SECITEM_AllocItem(nullptr, nullptr, flen));
  ScopedSECItem ecPointY(::SECITEM_AllocItem(nullptr, nullptr, flen));
  if (!ecPointX || !ecPointY) {
    return false;
  }

  
  memcpy(ecPointX->data, aPublicValue->data + 1, flen);
  memcpy(ecPointY->data, aPublicValue->data + 1 + flen, flen);

  CryptoBuffer x, y;
  if (!x.Assign(ecPointX) || NS_FAILED(x.ToJwkBase64(aRetVal.mX.Value())) ||
      !y.Assign(ecPointY) || NS_FAILED(y.ToJwkBase64(aRetVal.mY.Value()))) {
    return false;
  }

  aRetVal.mKty.Construct(NS_LITERAL_STRING(JWK_TYPE_EC));
  return true;
}

nsresult
CryptoKey::PrivateKeyToJwk(SECKEYPrivateKey* aPrivKey,
                           JsonWebKey& aRetVal,
                           const nsNSSShutDownPreventionLock& )
{
  switch (aPrivKey->keyType) {
    case rsaKey: {
      aRetVal.mN.Construct();
      aRetVal.mE.Construct();
      aRetVal.mD.Construct();
      aRetVal.mP.Construct();
      aRetVal.mQ.Construct();
      aRetVal.mDp.Construct();
      aRetVal.mDq.Construct();
      aRetVal.mQi.Construct();

      if (!ReadAndEncodeAttribute(aPrivKey, CKA_MODULUS, aRetVal.mN) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_PUBLIC_EXPONENT, aRetVal.mE) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_PRIVATE_EXPONENT, aRetVal.mD) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_PRIME_1, aRetVal.mP) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_PRIME_2, aRetVal.mQ) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_EXPONENT_1, aRetVal.mDp) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_EXPONENT_2, aRetVal.mDq) ||
          !ReadAndEncodeAttribute(aPrivKey, CKA_COEFFICIENT, aRetVal.mQi)) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }

      aRetVal.mKty.Construct(NS_LITERAL_STRING(JWK_TYPE_RSA));
      return NS_OK;
    }
    case ecKey: {
      
      ScopedSECItem params(::SECITEM_AllocItem(nullptr, nullptr, 0));
      SECStatus rv = PK11_ReadRawAttribute(PK11_TypePrivKey, aPrivKey,
                                           CKA_EC_PARAMS, params);
      if (rv != SECSuccess) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }

      
      ScopedSECItem ecPoint(::SECITEM_AllocItem(nullptr, nullptr, 0));
      rv = PK11_ReadRawAttribute(PK11_TypePrivKey, aPrivKey, CKA_EC_POINT,
                                 ecPoint);
      if (rv != SECSuccess) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }

      if (!ECKeyToJwk(PK11_TypePrivKey, aPrivKey, params, ecPoint, aRetVal)) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }

      aRetVal.mD.Construct();

      
      if (!ReadAndEncodeAttribute(aPrivKey, CKA_VALUE, aRetVal.mD)) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }

      return NS_OK;
    }
    default:
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
}

SECKEYPublicKey*
CryptoKey::PublicKeyFromJwk(const JsonWebKey& aJwk,
                            const nsNSSShutDownPreventionLock& )
{
  if (!aJwk.mKty.WasPassed()) {
    return nullptr;
  }

  if (aJwk.mKty.Value().EqualsLiteral(JWK_TYPE_RSA)) {
    
    CryptoBuffer n, e;
    if (!aJwk.mN.WasPassed() || NS_FAILED(n.FromJwkBase64(aJwk.mN.Value())) ||
        !aJwk.mE.WasPassed() || NS_FAILED(e.FromJwkBase64(aJwk.mE.Value()))) {
      return nullptr;
    }

    
    struct RSAPublicKeyData {
      SECItem n;
      SECItem e;
    };
    const RSAPublicKeyData input = {
      { siUnsignedInteger, n.Elements(), (unsigned int) n.Length() },
      { siUnsignedInteger, e.Elements(), (unsigned int) e.Length() }
    };
    const SEC_ASN1Template rsaPublicKeyTemplate[] = {
      {SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(RSAPublicKeyData)},
      {SEC_ASN1_INTEGER, offsetof(RSAPublicKeyData, n),},
      {SEC_ASN1_INTEGER, offsetof(RSAPublicKeyData, e),},
      {0,}
    };

    ScopedSECItem pkDer(SEC_ASN1EncodeItem(nullptr, nullptr, &input,
                                           rsaPublicKeyTemplate));
    if (!pkDer.get()) {
      return nullptr;
    }

    return SECKEY_ImportDERPublicKey(pkDer.get(), CKK_RSA);
  }

  if (aJwk.mKty.Value().EqualsLiteral(JWK_TYPE_EC)) {
    
    CryptoBuffer x, y;
    if (!aJwk.mCrv.WasPassed() ||
        !aJwk.mX.WasPassed() || NS_FAILED(x.FromJwkBase64(aJwk.mX.Value())) ||
        !aJwk.mY.WasPassed() || NS_FAILED(y.FromJwkBase64(aJwk.mY.Value()))) {
      return nullptr;
    }

    ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
    if (!arena) {
      return nullptr;
    }

    SECKEYPublicKey* key = PORT_ArenaZNew(arena, SECKEYPublicKey);
    if (!key) {
      return nullptr;
    }

    key->keyType = ecKey;
    key->pkcs11Slot = nullptr;
    key->pkcs11ID = CK_INVALID_HANDLE;

    nsString namedCurve;
    if (!NormalizeNamedCurveValue(aJwk.mCrv.Value(), namedCurve)) {
      return nullptr;
    }

    
    SECItem* params = CreateECParamsForCurve(namedCurve, arena.get());
    if (!params) {
      return nullptr;
    }
    key->u.ec.DEREncodedParams = *params;

    
    SECItem* point = CreateECPointForCoordinates(x, y, arena.get());
    if (!point) {
      return nullptr;
    }
    key->u.ec.publicValue = *point;

    return SECKEY_CopyPublicKey(key);
  }

  return nullptr;
}

nsresult
CryptoKey::PublicKeyToJwk(SECKEYPublicKey* aPubKey,
                          JsonWebKey& aRetVal,
                          const nsNSSShutDownPreventionLock& )
{
  switch (aPubKey->keyType) {
    case rsaKey: {
      CryptoBuffer n, e;
      aRetVal.mN.Construct();
      aRetVal.mE.Construct();

      if (!n.Assign(&aPubKey->u.rsa.modulus) ||
          !e.Assign(&aPubKey->u.rsa.publicExponent) ||
          NS_FAILED(n.ToJwkBase64(aRetVal.mN.Value())) ||
          NS_FAILED(e.ToJwkBase64(aRetVal.mE.Value()))) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }

      aRetVal.mKty.Construct(NS_LITERAL_STRING(JWK_TYPE_RSA));
      return NS_OK;
    }
    case ecKey:
      if (!ECKeyToJwk(PK11_TypePubKey, aPubKey, &aPubKey->u.ec.DEREncodedParams,
                      &aPubKey->u.ec.publicValue, aRetVal)) {
        return NS_ERROR_DOM_OPERATION_ERR;
      }
      return NS_OK;
    default:
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
}

bool
CryptoKey::WriteStructuredClone(JSStructuredCloneWriter* aWriter) const
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return false;
  }

  
  
  
  
  
  
  CryptoBuffer priv, pub;

  if (mPrivateKey) {
    CryptoKey::PrivateKeyToPkcs8(mPrivateKey, priv, locker);
  }

  if (mPublicKey) {
    CryptoKey::PublicKeyToSpki(mPublicKey, pub, locker);
  }

  return JS_WriteUint32Pair(aWriter, mAttributes, 0) &&
         WriteBuffer(aWriter, mSymKey) &&
         WriteBuffer(aWriter, priv) &&
         WriteBuffer(aWriter, pub) &&
         mAlgorithm->WriteStructuredClone(aWriter);
}

bool
CryptoKey::ReadStructuredClone(JSStructuredCloneReader* aReader)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return false;
  }

  uint32_t zero;
  CryptoBuffer sym, priv, pub;
  nsRefPtr<KeyAlgorithm> algorithm;

  bool read = JS_ReadUint32Pair(aReader, &mAttributes, &zero) &&
              ReadBuffer(aReader, sym) &&
              ReadBuffer(aReader, priv) &&
              ReadBuffer(aReader, pub) &&
              (algorithm = KeyAlgorithm::Create(mGlobal, aReader));
  if (!read) {
    return false;
  }

  if (sym.Length() > 0)  {
    mSymKey = sym;
  }
  if (priv.Length() > 0) {
    mPrivateKey = CryptoKey::PrivateKeyFromPkcs8(priv, locker);
  }
  if (pub.Length() > 0)  {
    mPublicKey = CryptoKey::PublicKeyFromSpki(pub, locker);
  }
  mAlgorithm = algorithm;

  
  
  if (!((GetKeyType() == SECRET  && mSymKey.Length() > 0) ||
        (GetKeyType() == PRIVATE && mPrivateKey) ||
        (GetKeyType() == PUBLIC  && mPublicKey))) {
    return false;
  }

  return true;
}

} 
} 
