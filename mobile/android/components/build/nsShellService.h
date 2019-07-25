








































#ifndef __NS_SHELLSERVICE_H__
#define __NS_SHELLSERVICE_H__

#include "nsIShellService.h"

class nsShellService : public nsIShellService
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE

  nsShellService() {};
  ~nsShellService() {};

};

#define nsShellService_CID                          \
{0xae9ebe1c, 0x61e9, 0x45fa, {0x8f, 0x34, 0xc1, 0x07, 0x80, 0x3a, 0x5b, 0x44}}

#define nsShellService_ContractID "@mozilla.org/browser/shell-service;1"

#endif
