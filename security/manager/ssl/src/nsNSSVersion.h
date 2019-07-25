





































#ifndef _NS_NSSVERSION_H_
#define _NS_NSSVERSION_H_

#include "nsINSSVersion.h"

class nsNSSVersion : public nsINSSVersion
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINSSVERSION
  
  nsNSSVersion();

private:
  ~nsNSSVersion();
};

#define NS_NSSVERSION_CID \
  { 0x23ad3531, 0x11d2, 0x4e8e, { 0x80, 0x5a, 0x6a, 0x75, 0x2e, 0x91, 0x68, 0x1a } }

#endif
