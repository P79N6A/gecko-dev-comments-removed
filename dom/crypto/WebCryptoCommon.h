





#ifndef mozilla_dom_WebCryptoCommon_h
#define mozilla_dom_WebCryptoCommon_h

#include "pk11pub.h"
#include "nsString.h"
#include "mozilla/dom/CryptoBuffer.h"
#include "js/StructuredClone.h"


#define WEBCRYPTO_ALG_AES_CBC       "AES-CBC"
#define WEBCRYPTO_ALG_AES_CTR       "AES-CTR"
#define WEBCRYPTO_ALG_AES_GCM       "AES-GCM"
#define WEBCRYPTO_ALG_SHA1          "SHA-1"
#define WEBCRYPTO_ALG_SHA256        "SHA-256"
#define WEBCRYPTO_ALG_SHA224        "SHA-224"
#define WEBCRYPTO_ALG_SHA384        "SHA-384"
#define WEBCRYPTO_ALG_SHA512        "SHA-512"
#define WEBCRYPTO_ALG_HMAC          "HMAC"
#define WEBCRYPTO_ALG_RSAES_PKCS1   "RSAES-PKCS1-v1_5"
#define WEBCRYPTO_ALG_RSASSA_PKCS1  "RSASSA-PKCS1-v1_5"


#define WEBCRYPTO_KEY_FORMAT_RAW    "raw"
#define WEBCRYPTO_KEY_FORMAT_PKCS8  "pkcs8"
#define WEBCRYPTO_KEY_FORMAT_SPKI   "spki"
#define WEBCRYPTO_KEY_FORMAT_JWK    "jwk"


#define WEBCRYPTO_KEY_TYPE_PUBLIC  "public"
#define WEBCRYPTO_KEY_TYPE_PRIVATE "private"
#define WEBCRYPTO_KEY_TYPE_SECRET  "secret"


#define WEBCRYPTO_KEY_USAGE_ENCRYPT     "encrypt"
#define WEBCRYPTO_KEY_USAGE_DECRYPT     "decrypt"
#define WEBCRYPTO_KEY_USAGE_SIGN        "sign"
#define WEBCRYPTO_KEY_USAGE_VERIFY      "verify"
#define WEBCRYPTO_KEY_USAGE_DERIVEKEY   "deriveKey"
#define WEBCRYPTO_KEY_USAGE_DERIVEBITS  "deriveBits"
#define WEBCRYPTO_KEY_USAGE_WRAPKEY     "wrapKey"
#define WEBCRYPTO_KEY_USAGE_UNWRAPKEY   "unwrapKey"


#define JWK_TYPE_SYMMETRIC          "oct"
#define JWK_TYPE_RSA                "RSA"
#define JWK_TYPE_EC                 "EC"


#define JWK_ALG_RS1                 "RS1"
#define JWK_ALG_RS256               "RS256"
#define JWK_ALG_RS384               "RS384"
#define JWK_ALG_RS512               "RS512"


#define UNKNOWN_CK_MECHANISM        CKM_VENDOR_DEFINED+1

namespace mozilla {
namespace dom {


inline bool
ReadString(JSStructuredCloneReader* aReader, nsString& aString)
{
  bool read;
  uint32_t nameLength, zero;
  read = JS_ReadUint32Pair(aReader, &nameLength, &zero);
  if (!read) {
    return false;
  }

  aString.SetLength(nameLength);
  size_t charSize = sizeof(nsString::char_type);
  read = JS_ReadBytes(aReader, (void*) aString.BeginWriting(), nameLength * charSize);
  if (!read) {
    return false;
  }

  return true;
}

inline bool
WriteString(JSStructuredCloneWriter* aWriter, const nsString& aString)
{
  size_t charSize = sizeof(nsString::char_type);
  return JS_WriteUint32Pair(aWriter, aString.Length(), 0) &&
         JS_WriteBytes(aWriter, aString.get(), aString.Length() * charSize);
}

inline bool
ReadBuffer(JSStructuredCloneReader* aReader, CryptoBuffer& aBuffer)
{
  uint32_t length, zero;
  bool ret = JS_ReadUint32Pair(aReader, &length, &zero);
  if (!ret) {
    return false;
  }

  if (length > 0) {
    if (!aBuffer.SetLength(length)) {
      return false;
    }
    ret = JS_ReadBytes(aReader, aBuffer.Elements(), aBuffer.Length());
  }
  return ret;
}

inline bool
WriteBuffer(JSStructuredCloneWriter* aWriter, const CryptoBuffer& aBuffer)
{
  bool ret = JS_WriteUint32Pair(aWriter, aBuffer.Length(), 0);
  if (ret && aBuffer.Length() > 0) {
    ret = JS_WriteBytes(aWriter, aBuffer.Elements(), aBuffer.Length());
  }
  return ret;
}

} 
} 

#endif 
