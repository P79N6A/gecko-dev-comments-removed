




































#ifndef _NSRANDOMGENERATOR_H_
#define _NSRANDOMGENERATOR_H_

#include "nsIRandomGenerator.h"

#define NS_RANDOMGENERATOR_CID \
  {0xbe65e2b7, 0xfe46, 0x4e0f, {0x88, 0xe0, 0x4b, 0x38, 0x5d, 0xb4, 0xd6, 0x8a}}

#define NS_RANDOMGENERATOR_CONTRACTID \
  "@mozilla.org/security/random-generator;1"

class nsRandomGenerator : public nsIRandomGenerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRANDOMGENERATOR
};

#endif
