





































#ifndef nsXPathExpression_h__
#define nsXPathExpression_h__

#include "nsIDOMXPathExpression.h"
#include "nsIDOMNSXPathExpression.h"
#include "txIXPathContext.h"
#include "txResultRecycler.h"
#include "nsAutoPtr.h"

class Expr;
class txXPathNode;




class nsXPathExpression : public nsIDOMXPathExpression,
                          public nsIDOMNSXPathExpression
{
public:
    nsXPathExpression(nsAutoPtr<Expr>& aExpression, txResultRecycler* aRecycler,
                      nsIDOMDocument *aDocument);
    virtual ~nsXPathExpression();

    
    NS_DECL_ISUPPORTS

    
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
                        PRUint32 aContextPosition, PRUint32 aContextSize,
                        txResultRecycler* aRecycler)
            : mContextNode(aContextNode),
              mContextPosition(aContextPosition),
              mContextSize(aContextSize),
              mLastError(NS_OK),
              mRecycler(aRecycler)
        {
        }

        ~EvalContextImpl()
        {
        }

        nsresult getError()
        {
            return mLastError;
        }

        TX_DECL_EVAL_CONTEXT;

    private:
        const txXPathNode& mContextNode;
        PRUint32 mContextPosition;
        PRUint32 mContextSize;
        nsresult mLastError;
        nsRefPtr<txResultRecycler> mRecycler;
    };
};

#endif
