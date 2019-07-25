




#ifndef nsXPathExpression_h__
#define nsXPathExpression_h__

#include "nsIDOMXPathExpression.h"
#include "nsIDOMNSXPathExpression.h"
#include "txIXPathContext.h"
#include "txResultRecycler.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class Expr;
class txXPathNode;




class nsXPathExpression MOZ_FINAL : public nsIDOMXPathExpression,
                                    public nsIDOMNSXPathExpression
{
public:
    nsXPathExpression(nsAutoPtr<Expr>& aExpression, txResultRecycler* aRecycler,
                      nsIDOMDocument *aDocument);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXPathExpression,
                                             nsIDOMXPathExpression)

    
    NS_DECL_NSIDOMXPATHEXPRESSION

    
    NS_DECL_NSIDOMNSXPATHEXPRESSION

private:
    nsAutoPtr<Expr> mExpression;
    nsRefPtr<txResultRecycler> mRecycler;
    nsCOMPtr<nsIDOMDocument> mDocument;

    class EvalContextImpl : public txIEvalContext
    {
    public:
        EvalContextImpl(const txXPathNode& aContextNode,
                        uint32_t aContextPosition, uint32_t aContextSize,
                        txResultRecycler* aRecycler)
            : mContextNode(aContextNode),
              mContextPosition(aContextPosition),
              mContextSize(aContextSize),
              mLastError(NS_OK),
              mRecycler(aRecycler)
        {
        }

        nsresult getError()
        {
            return mLastError;
        }

        TX_DECL_EVAL_CONTEXT;

    private:
        const txXPathNode& mContextNode;
        uint32_t mContextPosition;
        uint32_t mContextSize;
        nsresult mLastError;
        nsRefPtr<txResultRecycler> mRecycler;
    };
};

#endif
