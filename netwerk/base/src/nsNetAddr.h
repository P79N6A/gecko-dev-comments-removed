





#ifndef nsNetAddr_h__
#define nsNetAddr_h__

#include "nsINetAddr.h"
#include "mozilla/net/DNS.h"
#include "mozilla/Attributes.h"

class nsNetAddr MOZ_FINAL : public nsINetAddr
{
  ~nsNetAddr() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETADDR

  nsNetAddr(mozilla::net::NetAddr* addr);

private:
  mozilla::net::NetAddr mAddr;

protected:
  
};

#endif
