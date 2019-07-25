






































#ifndef WeaveCrypto_h_
#define WeaveCrypto_h_

#include "IWeaveCrypto.h"
#include "pk11pub.h"

#define WEAVE_CRYPTO_CONTRACTID "@labs.mozilla.com/Weave/Crypto;1"
#define WEAVE_CRYPTO_CLASSNAME "A Simple XPCOM Sample"
#define WEAVE_CRYPTO_CID { 0xd3b0f750, 0xc976, 0x46d0, \
                           { 0xbe, 0x20, 0x96, 0xb2, 0x4f, 0x46, 0x84, 0xbc } }

class WeaveCrypto : public IWeaveCrypto
{
public:
  WeaveCrypto();

  NS_DECL_ISUPPORTS
  NS_DECL_IWEAVECRYPTO

private:
  ~WeaveCrypto();

  SECOidTag mAlgorithm;

  nsresult DecodeBase64(const nsACString& base64, nsACString& retval);
  nsresult EncodeBase64(const nsACString& binary, nsACString& retval);

  static void StoreToStringCallback(void *arg, const char *buf, unsigned long len);
  static PK11SymKey *GetSymmetricKeyCallback(void *arg, SECAlgorithmID *algid);
  static PRBool DecryptionAllowedCallback(SECAlgorithmID *algid, PK11SymKey *key);
};

#endif 
