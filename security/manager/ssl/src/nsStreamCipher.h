



































#ifndef _NS_STREAMCIPHER_H_
#define _NS_STREAMCIPHER_H_

#include "nsIStreamCipher.h"
#include "nsString.h"
#include "pk11func.h"

#define NS_STREAMCIPHER_CLASSNAME  "Stream Cipher Component"

#define NS_STREAMCIPHER_CID   \
{ 0xdbfcbe4a, 0x10f7, 0x4d6f, {0xa4, 0x81, 0x68, 0xe6, 0xd6, 0xb7, 0x1d, 0x21}}
#define NS_STREAMCIPHER_CONTRACTID "@mozilla.org/security/streamcipher;1"

class nsStreamCipher : public nsIStreamCipher
{
public:
  nsStreamCipher();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMCIPHER

private:
  ~nsStreamCipher();

  
  
  nsresult InitWithIV_(nsIKeyObject *aKey, SECItem* aIV);
  
  
  nsStreamCipher(nsStreamCipher&);

  
  PK11Context* mContext;

  
  nsCString mValue;
};

#endif 
