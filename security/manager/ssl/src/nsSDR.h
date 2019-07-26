





#ifndef _NSSDR_H_
#define _NSSDR_H_

#include "nsISecretDecoderRing.h"











#ifndef NS_SDR_CONTRACTID
#define NS_SDR_CONTRACTID "@mozilla.org/security/sdr;1"
#endif





#define NS_SDR_CID \
  { 0x0c4f1ddc, 0x1dd2, 0x11b2, { 0x9d, 0x95, 0xf2, 0xfd, 0xf1, 0x13, 0x04, 0x4b } }

class nsSecretDecoderRing
: public nsISecretDecoderRing,
  public nsISecretDecoderRingConfig
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISECRETDECODERRING
  NS_DECL_NSISECRETDECODERRINGCONFIG

  nsSecretDecoderRing();
  virtual ~nsSecretDecoderRing();

private:

  



  nsresult encode(const unsigned char *data, int32_t dataLen, char **_retval);
  nsresult decode(const char *data, unsigned char **result, int32_t * _retval);

};

#endif 
