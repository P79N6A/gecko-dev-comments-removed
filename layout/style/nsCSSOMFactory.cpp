








































#include "nsCSSOMFactory.h"
#include "nsDOMCSSAttrDeclaration.h"

nsCSSOMFactory::nsCSSOMFactory()
{
}

nsCSSOMFactory::~nsCSSOMFactory()
{
}

NS_IMPL_ISUPPORTS1(nsCSSOMFactory, nsICSSOMFactory)

NS_IMETHODIMP
nsCSSOMFactory::CreateDOMCSSAttributeDeclaration(nsIContent *aContent,
                                                 nsDOMCSSDeclaration **aResult)
{
    nsDOMCSSDeclaration *result = new nsDOMCSSAttributeDeclaration(aContent);
    if (!result) {
        *aResult = 0;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(result);
    *aResult = result;
    return NS_OK;
}
