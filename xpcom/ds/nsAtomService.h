





































#ifndef __nsAtomService_h
#define __nsAtomService_h

#include "nsIAtomService.h"

class nsAtomService : public nsIAtomService
{
 public:
  nsAtomService();
  NS_DECL_ISUPPORTS
    
  NS_DECL_NSIATOMSERVICE

 private:
  ~nsAtomService() {}
};

#endif
