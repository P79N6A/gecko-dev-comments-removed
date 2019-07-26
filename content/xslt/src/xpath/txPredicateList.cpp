




#include "txExpr.h"
#include "txNodeSet.h"
#include "txNodeSetContext.h"






nsresult
PredicateList::evaluatePredicates(txNodeSet* nodes,
                                  txIMatchContext* aContext)
{
    NS_ASSERTION(nodes, "called evaluatePredicates with nullptr NodeSet");
    nsresult rv = NS_OK;

    uint32_t i, len = mPredicates.Length();
    for (i = 0; i < len && !nodes->isEmpty(); ++i) {
        txNodeSetContext predContext(nodes, aContext);
        




        int32_t index = 0;
        while (predContext.hasNext()) {
            predContext.next();
            nsRefPtr<txAExprResult> exprResult;
            rv = mPredicates[i]->evaluate(&predContext,
                                          getter_AddRefs(exprResult));
            NS_ENSURE_SUCCESS(rv, rv);

            
            if (exprResult->getResultType() == txAExprResult::NUMBER) {
                if ((double)predContext.position() == exprResult->numberValue()) {
                    nodes->mark(index);
                }
            }
            else if (exprResult->booleanValue()) {
                nodes->mark(index);
            }
            ++index;
        }
        
        nodes->sweep();
    }

    return NS_OK;
}

bool
PredicateList::isSensitiveTo(Expr::ContextSensitivity aContext)
{
    
    Expr::ContextSensitivity context =
        aContext & ~(Expr::NODE_CONTEXT | Expr::NODESET_CONTEXT);
    if (context == Expr::NO_CONTEXT) {
        return false;
    }

    uint32_t i, len = mPredicates.Length();
    for (i = 0; i < len; ++i) {
        if (mPredicates[i]->isSensitiveTo(context)) {
            return true;
        }
    }

    return false;
}

#ifdef TX_TO_STRING
void PredicateList::toString(nsAString& dest)
{
    for (uint32_t i = 0; i < mPredicates.Length(); ++i) {
        dest.Append(PRUnichar('['));
        mPredicates[i]->toString(dest);
        dest.Append(PRUnichar(']'));
    }
}
#endif
