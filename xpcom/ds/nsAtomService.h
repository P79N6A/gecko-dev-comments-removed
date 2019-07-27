





#ifndef __nsAtomService_h
#define __nsAtomService_h

#include "nsIAtomService.h"
#include "mozilla/Attributes.h"

class nsAtomService final : public nsIAtomService
{
public:
  nsAtomService();
  NS_DECL_THREADSAFE_ISUPPORTS

  NS_DECL_NSIATOMSERVICE

private:
  ~nsAtomService() {}
};

#endif
