





































#ifndef nsXPathEvaluator_h__
#define nsXPathEvaluator_h__

#include "nsIDOMXPathEvaluator.h"
#include "nsIXPathEvaluatorInternal.h"
#include "nsIWeakReference.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "txResultRecycler.h"
#include "nsAgg.h"
#include "nsTArray.h"




class nsXPathEvaluator : public nsIDOMXPathEvaluator,
                         public nsIXPathEvaluatorInternal
{
public:
    nsXPathEvaluator(nsISupports *aOuter);
    virtual ~nsXPathEvaluator()
    {
    }

    nsresult Init();

    
    NS_DECL_AGGREGATED

    
    NS_DECL_NSIDOMXPATHEVALUATOR

    
    NS_IMETHOD SetDocument(nsIDOMDocument* aDocument);
    NS_IMETHOD CreateExpression(const nsAString &aExpression, 
                                nsIDOMXPathNSResolver *aResolver,
                                nsStringArray *aNamespaceURIs,
                                nsCStringArray *aContractIDs,
                                nsCOMArray<nsISupports> *aState,
                                nsIDOMXPathExpression **aResult);

private:
    nsresult CreateExpression(const nsAString & aExpression,
                              nsIDOMXPathNSResolver *aResolver,
                              nsTArray<PRInt32> *aNamespaceIDs,
                              nsCStringArray *aContractIDs,
                              nsCOMArray<nsISupports> *aState,
                              nsIDOMXPathExpression **aResult);

    nsWeakPtr mDocument;
    nsRefPtr<txResultRecycler> mRecycler;
};


#define TRANSFORMIIX_XPATH_EVALUATOR_CID   \
{ 0xd0a75e02, 0xb5e7, 0x11d5, { 0xa7, 0xf2, 0xdf, 0x10, 0x9f, 0xb8, 0xa1, 0xfc } }

#endif
