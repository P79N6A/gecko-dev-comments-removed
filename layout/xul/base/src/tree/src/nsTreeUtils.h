







































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

    static nsresult
    GetImmediateChild(nsIContent* aContainer, nsIAtom* aTag, nsIContent** aResult);

    static nsresult
    GetDescendantChild(nsIContent* aContainer, nsIAtom* aTag, nsIContent** aResult);

    static nsresult
    UpdateSortIndicators(nsIContent* aColumn, const nsAString& aDirection);

    static nsresult
    GetColumnIndex(nsIContent* aColumn, PRInt32* aResult);
};

#endif 
