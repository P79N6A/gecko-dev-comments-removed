






































#ifndef WeaveCrypto_h_
#define WeaveCrypto_h_

#include "IWeaveCrypto.h"
#include "pk11pub.h"

#define WEAVE_CRYPTO_CONTRACTID "@labs.mozilla.com/Weave/Crypto;1"
#define WEAVE_CRYPTO_CLASSNAME "Weave crypto module"
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
  PRUint32  mKeypairBits;

  nsresult DecodeBase64(const nsACString& base64, nsACString& retval);
  nsresult DecodeBase64(const nsACString& base64, char *aData, PRUint32 *aLength);
  void EncodeBase64(const nsACString& binary, nsACString& retval);
  void EncodeBase64(const char *aData, PRUint32 aLength, nsACString& retval);

  nsresult CommonCrypt(const char *input, PRUint32 inputSize,
                       char *output, PRUint32 *outputSize,
                       const nsACString& aSymmetricKey,
                       const nsACString& aIV,
                       CK_ATTRIBUTE_TYPE aOperation);


  nsresult DeriveKeyFromPassphrase(const nsACString& aPassphrase,
                                   const nsACString& aSalt,
                                   PK11SymKey **aSymKey);

  nsresult WrapPrivateKey(SECKEYPrivateKey *aPrivateKey,
                          const nsACString& aPassphrase,
                          const nsACString& aSalt,
                          const nsACString& aIV,
                          nsACString& aEncodedPublicKey);
  nsresult EncodePublicKey(SECKEYPublicKey *aPublicKey,
                           nsACString& aEncodedPublicKey);


};

#endif 
