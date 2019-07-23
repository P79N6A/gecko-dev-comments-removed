




































 
#ifndef nsIXPathEvaluatorInternal_h__
#define nsIXPathEvaluatorInternal_h__

#include "nsCOMArray.h"

class nsIDOMDocument;
class nsIDOMXPathExpression;
class nsIDOMXPathNSResolver;

#define NS_IXPATHEVALUATORINTERNAL_IID \
  {0xb4b72daa, 0x65d6, 0x440f, \
    { 0xb6, 0x08, 0xe2, 0xee, 0x9a, 0x82, 0xf3, 0x13 }}

class nsIXPathEvaluatorInternal : public nsISupports
{
public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXPATHEVALUATORINTERNAL_IID)

  


  NS_IMETHOD SetDocument(nsIDOMDocument* aDocument) = 0;

  NS_IMETHOD CreateExpression(const nsAString &aExpression,
                              nsIDOMXPathNSResolver *aResolver,
                              nsStringArray *aNamespaceURIs,
                              nsCStringArray *aContractIDs,
                              nsCOMArray<nsISupports> *aState,
                              nsIDOMXPathExpression **aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXPathEvaluatorInternal,
                              NS_IXPATHEVALUATORINTERNAL_IID)

#endif 
