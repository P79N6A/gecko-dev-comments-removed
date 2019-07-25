








































#ifndef __NS_PHONESUPPORT_H__
#define __NS_PHONESUPPORT_H__

#include "nsIPhoneSupport.h"

class nsPhoneSupport : public nsIPhoneSupport
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPHONESUPPORT

  nsPhoneSupport() {};
  ~nsPhoneSupport() {};

};

#define nsPhoneSupport_CID                          \
{ 0x2a08c9e4, 0xf853, 0x4f02,                       \
{0x88, 0xd8, 0xd6, 0x2f, 0x27, 0xca, 0x06, 0x85} }

#define nsPhoneSupport_ContractID "@mozilla.org/phone/support;1"

#endif
