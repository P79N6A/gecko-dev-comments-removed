




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
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

class nsINode;




class nsXPathEvaluator MOZ_FINAL : public nsIDOMXPathEvaluator,
                                   public nsIXPathEvaluatorInternal
{
public:
    nsXPathEvaluator(nsISupports *aOuter);

    nsresult Init();

    
    NS_DECL_AGGREGATED

    
    NS_DECL_NSIDOMXPATHEVALUATOR

    
    NS_IMETHOD SetDocument(nsIDOMDocument* aDocument);
    NS_IMETHOD CreateExpression(const nsAString &aExpression, 
                                nsIDOMXPathNSResolver *aResolver,
                                nsTArray<nsString> *aNamespaceURIs,
                                nsTArray<nsCString> *aContractIDs,
                                nsCOMArray<nsISupports> *aState,
                                nsIDOMXPathExpression **aResult);

    
    JSObject* WrapObject(JSContext* aCx, JSObject* aScope);
    static already_AddRefed<nsXPathEvaluator>
        Constructor(nsISupports* aGlobal, mozilla::ErrorResult& rv);
    already_AddRefed<nsIDOMXPathExpression>
        CreateExpression(const nsAString& aExpression,
                         nsIDOMXPathNSResolver* aResolver,
                         mozilla::ErrorResult& rv);
    already_AddRefed<nsIDOMXPathNSResolver>
        CreateNSResolver(nsINode* aNodeResolver, mozilla::ErrorResult& rv);
    already_AddRefed<nsISupports>
        Evaluate(const nsAString& aExpression, nsINode* aContextNode,
                 nsIDOMXPathNSResolver* aResolver, uint16_t aType,
                 nsISupports* aResult, mozilla::ErrorResult& rv);
private:
    nsresult CreateExpression(const nsAString & aExpression,
                              nsIDOMXPathNSResolver *aResolver,
                              nsTArray<int32_t> *aNamespaceIDs,
                              nsTArray<nsCString> *aContractIDs,
                              nsCOMArray<nsISupports> *aState,
                              nsIDOMXPathExpression **aResult);

    nsWeakPtr mDocument;
    nsRefPtr<txResultRecycler> mRecycler;
};


#define TRANSFORMIIX_XPATH_EVALUATOR_CID   \
{ 0xd0a75e02, 0xb5e7, 0x11d5, { 0xa7, 0xf2, 0xdf, 0x10, 0x9f, 0xb8, 0xa1, 0xfc } }

#endif
