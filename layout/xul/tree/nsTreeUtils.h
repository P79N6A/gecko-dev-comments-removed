




#ifndef nsTreeUtils_h__
#define nsTreeUtils_h__

#include "nsString.h"
#include "nsTreeStyleCache.h"
#include "nsIContent.h"

class nsTreeUtils
{
  public:
    



    static nsresult
    TokenizeProperties(const nsAString& aProperties, AtomArray & aPropertiesArray);

    static nsIContent*
    GetImmediateChild(nsIContent* aContainer, nsIAtom* aTag);

    static nsIContent*
    GetDescendantChild(nsIContent* aContainer, nsIAtom* aTag);

    static nsresult
    UpdateSortIndicators(nsIContent* aColumn, const nsAString& aDirection);

    static nsresult
    GetColumnIndex(nsIContent* aColumn, int32_t* aResult);
};

#endif 
