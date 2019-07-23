




































#ifndef _NSCERTPICKER_H_
#define _NSCERTPICKER_H_

#include "nsIUserCertPicker.h"

#define NS_CERT_PICKER_CID \
  { 0x735959a1, 0xaf01, 0x447e, { 0xb0, 0x2d, 0x56, 0xe9, 0x68, 0xfa, 0x52, 0xb4 } }

class nsCertPicker : public nsIUserCertPicker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUSERCERTPICKER

  nsCertPicker();
  virtual ~nsCertPicker();
  
private:
};

#endif
