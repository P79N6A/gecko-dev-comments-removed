







































#ifndef nsTreeUtils_h__
#define nsTreeUtils_h__

#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsIContent.h"

class nsTreeUtils
{
  public:
    



    static nsresult
    TokenizeProperties(const nsAString& aProperties, nsISupportsArray* aPropertiesArray);

    static nsIContent*
    GetImmediateChild(nsIContent* aContainer, nsIAtom* aTag);

    static nsIContent*
    GetDescendantChild(nsIContent* aContainer, nsIAtom* aTag);

    static nsresult
    UpdateSortIndicators(nsIContent* aColumn, const nsAString& aDirection);

    static nsresult
    GetColumnIndex(nsIContent* aColumn, PRInt32* aResult);
};

#endif 
