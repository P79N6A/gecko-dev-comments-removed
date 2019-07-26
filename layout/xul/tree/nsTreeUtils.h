




#ifndef nsTreeUtils_h__
#define nsTreeUtils_h__

#include "nsError.h"
#include "nsString.h"
#include "nsTreeStyleCache.h"

class nsIAtom;
class nsIContent;
class nsISupportsArray;

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
