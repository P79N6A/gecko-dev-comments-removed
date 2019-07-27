




#ifndef mozilla_dom_XPathExpression_h
#define mozilla_dom_XPathExpression_h

#include "nsIDOMXPathExpression.h"
#include "nsIDOMNSXPathExpression.h"
#include "txResultRecycler.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class Expr;
class txXPathNode;

namespace mozilla {
namespace dom {




class XPathExpression MOZ_FINAL : public nsIDOMXPathExpression,
                                  public nsIDOMNSXPathExpression
{
public:
    XPathExpression(nsAutoPtr<Expr>&& aExpression, txResultRecycler* aRecycler,
                    nsIDOMDocument *aDocument);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(XPathExpression,
                                             nsIDOMXPathExpression)

    
    NS_DECL_NSIDOMXPATHEXPRESSION

    
    NS_DECL_NSIDOMNSXPATHEXPRESSION

private:
    ~nsXPathExpression() {}

    nsAutoPtr<Expr> mExpression;
    nsRefPtr<txResultRecycler> mRecycler;
    nsCOMPtr<nsIDOMDocument> mDocument;
};

} 
} 

#endif 
