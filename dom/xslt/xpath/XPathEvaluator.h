




#ifndef mozilla_dom_XPathEvaluator_h
#define mozilla_dom_XPathEvaluator_h

#include "nsIDOMXPathEvaluator.h"
#include "nsIWeakReference.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsIDocument.h"

class nsINode;
class txIParseContext;
class txResultRecycler;

namespace mozilla {
namespace dom {

class GlobalObject;
class XPathExpression;
class XPathNSResolver;
class XPathResult;




class XPathEvaluator MOZ_FINAL : public nsIDOMXPathEvaluator
{
    ~XPathEvaluator();

public:
    explicit XPathEvaluator(nsIDocument* aDocument = nullptr);

    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIDOMXPATHEVALUATOR

    
    JSObject* WrapObject(JSContext* aCx);
    nsIDocument* GetParentObject()
    {
        nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocument);
        return doc;
    }
    static already_AddRefed<XPathEvaluator>
        Constructor(const GlobalObject& aGlobal, ErrorResult& rv);
    XPathExpression*
        CreateExpression(const nsAString& aExpression,
                         XPathNSResolver* aResolver,
                         ErrorResult& rv);
    XPathExpression*
        CreateExpression(const nsAString& aExpression,
                         nsINode* aResolver,
                         ErrorResult& aRv);
    nsINode* CreateNSResolver(nsINode& aNodeResolver)
    {
        return &aNodeResolver;
    }
    already_AddRefed<XPathResult>
        Evaluate(JSContext* aCx, const nsAString& aExpression,
                 nsINode* aContextNode, XPathNSResolver* aResolver,
                 uint16_t aType, JS::Handle<JSObject*> aResult,
                 ErrorResult& rv);
private:
    XPathExpression*
        CreateExpression(const nsAString& aExpression,
                         txIParseContext* aContext,
                         nsIDocument* aDocument,
                         ErrorResult& aRv);

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
