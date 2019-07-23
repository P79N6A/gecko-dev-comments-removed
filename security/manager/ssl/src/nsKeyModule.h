



































#ifndef _NS_KEYMODULE_H_
#define _NS_KEYMODULE_H_

#include "nsIKeyModule.h"
#include "pk11pub.h"

#define NS_KEYMODULEOBJECT_CLASSNAME "Key Object Component"

#define NS_KEYMODULEOBJECT_CID   \
{ 0xeae599aa, 0xecef, 0x49c6, {0xa8, 0xaf, 0x6d, 0xdc, 0xc6, 0xfe, 0xb4, 0x84} }
#define NS_KEYMODULEOBJECT_CONTRACTID "@mozilla.org/security/keyobject;1"

#define NS_KEYMODULEOBJECTFACTORY_CLASSNAME "Key Object Factory Component"

#define NS_KEYMODULEOBJECTFACTORY_CID   \
{ 0xa39e0e9d, 0xe567, 0x41e3, {0xb1, 0x2c, 0x5d, 0xf6, 0x7f, 0x18, 0x17, 0x4d} }
#define NS_KEYMODULEOBJECTFACTORY_CONTRACTID \
"@mozilla.org/security/keyobjectfactory;1"

class nsKeyObject : public nsIKeyObject
{
public:
  nsKeyObject();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIKEYOBJECT

private:
  ~nsKeyObject();
  
  
  nsKeyObject(nsKeyObject&);

  
  PRUint32 mKeyType;
  
  
  PK11SymKey* mSymKey;
  SECKEYPrivateKey* mPrivateKey;
  SECKEYPublicKey* mPublicKey;

  
  void CleanUp();
};


class nsKeyObjectFactory : public nsIKeyObjectFactory
{
public:
  nsKeyObjectFactory();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIKEYOBJECTFACTORY

private:
  ~nsKeyObjectFactory() {}

  
  nsKeyObjectFactory(nsKeyObjectFactory&);
};

#endif 
