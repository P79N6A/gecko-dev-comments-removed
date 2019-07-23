








































#ifndef nsCSSOMFactory_h___
#define nsCSSOMFactory_h___

#include "nsICSSOMFactory.h"

class nsCSSOMFactory : public nsICSSOMFactory {

public:

    nsCSSOMFactory();
    virtual ~nsCSSOMFactory();

    NS_DECL_ISUPPORTS

    NS_IMETHOD CreateDOMCSSAttributeDeclaration(nsIContent *aContent,
                                                nsDOMCSSDeclaration **aResult);

};

#endif 
