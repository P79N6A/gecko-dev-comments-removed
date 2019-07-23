




































#ifndef nsSemanticUnitScanner_h__
#define nsSemanticUnitScanner_h__

#include "nsSampleWordBreaker.h"
#include "nsISemanticUnitScanner.h"


class nsSemanticUnitScanner : public nsISemanticUnitScanner
                            , public nsSampleWordBreaker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISEMANTICUNITSCANNER

  nsSemanticUnitScanner();
  virtual ~nsSemanticUnitScanner();
  
};

#endif
