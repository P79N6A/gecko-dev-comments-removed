





#ifndef nsNetAddr_h__
#define nsNetAddr_h__

#include "nsINetAddr.h"
#include "prio.h"
#include "mozilla/Attributes.h"

class nsNetAddr MOZ_FINAL : public nsINetAddr
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETADDR

  nsNetAddr(PRNetAddr* addr);

private:
  PRNetAddr mAddr;

protected:
  
};

#endif
