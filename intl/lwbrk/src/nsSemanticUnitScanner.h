




#ifndef nsSemanticUnitScanner_h__
#define nsSemanticUnitScanner_h__

#include "nsSampleWordBreaker.h"
#include "nsISemanticUnitScanner.h"


class nsSemanticUnitScanner : public nsISemanticUnitScanner
                            , public nsSampleWordBreaker
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISEMANTICUNITSCANNER

  nsSemanticUnitScanner();

private:
  virtual ~nsSemanticUnitScanner();
  
};

#endif
