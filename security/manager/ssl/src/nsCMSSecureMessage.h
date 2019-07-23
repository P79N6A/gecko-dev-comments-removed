




































#ifndef _NSCMSSECUREMESSAGE_H_
#define _NSCMSSECUREMESSAGE_H_

#include "nsICMSSecureMessage.h"

#include "cms.h"





#define NS_CMSSECUREMESSAGE_CLASSNAME "CMS Secure Message"
#define NS_CMSSECUREMESSAGE_CID \
  { 0x5fb907e0, 0x1dd2, 0x11b2, { 0xa7, 0xc0, 0xf1, 0x4c, 0x41, 0x6a, 0x62, 0xa1 } }

class nsCMSSecureMessage
: public nsICMSSecureMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICMSSECUREMESSAGE

  nsCMSSecureMessage();
  virtual ~nsCMSSecureMessage();

private:
  NS_METHOD encode(const unsigned char *data, PRInt32 dataLen, char **_retval);
  NS_METHOD decode(const char *data, unsigned char **result, PRInt32 * _retval);
};


#endif 
