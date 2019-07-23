








































#ifndef nsICSSOMFactory_h___
#define nsICSSOMFactory_h___

#include "nsISupports.h"
class nsDOMCSSDeclaration;
class nsIContent;


#define NS_ICSSOMFACTORY_IID \
  { 0xf2fb43bf, 0x81a1, 0x4b0d, \
    { 0x90, 0x7a, 0x89, 0x3f, 0xe6, 0x72, 0x7d, 0xbb } }


#define NS_CSSOMFACTORY_CID \
  { 0x5fcaa2c1, 0x7ca4, 0x4f73, \
    {0xa3, 0x57, 0x93, 0xe7, 0x9d, 0x70, 0x93, 0x76 } }

class nsICSSOMFactory : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSSOMFACTORY_IID)

    NS_IMETHOD CreateDOMCSSAttributeDeclaration(nsIContent *aContent,
                                                nsDOMCSSDeclaration **aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSOMFactory, NS_ICSSOMFACTORY_IID)

#endif 
