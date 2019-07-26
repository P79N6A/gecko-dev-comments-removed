




#ifndef mozilla_dom_XPathEvaluator_h
#define mozilla_dom_XPathEvaluator_h

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

namespace mozilla {
namespace dom {

class GlobalObject;




class XPathEvaluator MOZ_FINAL : public nsIDOMXPathEvaluator,
                                 public nsIXPathEvaluatorInternal
{
public:
    XPathEvaluator(nsISupports *aOuter);

    nsresult Init();

    
    NS_DECL_AGGREGATED

    
    NS_DECL_NSIDOMXPATHEVALUATOR

    
    NS_IMETHOD SetDocument(nsIDOMDocument* aDocument) MOZ_OVERRIDE;
    NS_IMETHOD CreateExpression(const nsAString &aExpression, 
                                nsIDOMXPathNSResolver *aResolver,
                                nsTArray<nsString> *aNamespaceURIs,
                                nsTArray<nsCString> *aContractIDs,
                                nsCOMArray<nsISupports> *aState,
                                nsIDOMXPathExpression **aResult) MOZ_OVERRIDE;

    
    JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope);
    static already_AddRefed<XPathEvaluator>
        Constructor(const GlobalObject& aGlobal, ErrorResult& rv);
    already_AddRefed<nsIDOMXPathExpression>
        CreateExpression(const nsAString& aExpression,
                         nsIDOMXPathNSResolver* aResolver,
                         ErrorResult& rv);
    already_AddRefed<nsIDOMXPathNSResolver>
        CreateNSResolver(nsINode* aNodeResolver, ErrorResult& rv);
    already_AddRefed<nsISupports>
        Evaluate(const nsAString& aExpression, nsINode* aContextNode,
                 nsIDOMXPathNSResolver* aResolver, uint16_t aType,
                 nsISupports* aResult, ErrorResult& rv);
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

inline nsISupports*
ToSupports(XPathEvaluator* e)
{
    return static_cast<nsIDOMXPathEvaluator*>(e);
}


#define TRANSFORMIIX_XPATH_EVALUATOR_CID   \
{ 0xd0a75e02, 0xb5e7, 0x11d5, { 0xa7, 0xf2, 0xdf, 0x10, 0x9f, 0xb8, 0xa1, 0xfc } }

} 
} 

#endif 
