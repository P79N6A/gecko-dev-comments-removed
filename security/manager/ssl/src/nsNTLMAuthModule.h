




































#ifndef nsNTLMAuthModule_h__
#define nsNTLMAuthModule_h__

#include "nsIAuthModule.h"
#include "nsString.h"

class nsNTLMAuthModule : public nsIAuthModule
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTHMODULE

  nsNTLMAuthModule() {}
  virtual ~nsNTLMAuthModule();

  nsresult InitTest();

private:
  nsString mDomain;
  nsString mUsername;
  nsString mPassword;
};

#define NS_NTLMAUTHMODULE_CLASSNAME \
  "nsNTLMAuthModule"
#define NS_NTLMAUTHMODULE_CONTRACTID \
  NS_AUTH_MODULE_CONTRACTID_PREFIX "ntlm"
#define NS_NTLMAUTHMODULE_CID \
{ /* a4e5888f-4fe4-4632-8e7e-745196ea7c70 */       \
  0xa4e5888f,                                      \
  0x4fe4,                                          \
  0x4632,                                          \
  {0x8e, 0x7e, 0x74, 0x51, 0x96, 0xea, 0x7c, 0x70} \
}

#endif 
