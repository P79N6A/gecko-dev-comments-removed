




#ifndef _nsCrypto_h_
#define _nsCrypto_h_

#include "nsIPKCS11.h"

#define NS_PKCS11_CID \
  {0x74b7a390, 0x3b41, 0x11d4, { 0x8a, 0x80, 0x00, 0x60, 0x08, 0xc8, 0x44, 0xc3} }

class nsPkcs11 : public nsIPKCS11
{
public:
  nsPkcs11();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPKCS11

protected:
  virtual ~nsPkcs11();
};

#endif 
