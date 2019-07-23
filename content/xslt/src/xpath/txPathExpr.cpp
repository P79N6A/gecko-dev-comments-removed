





































#include "txExpr.h"
#include "txNodeSet.h"
#include "txNodeSetContext.h"
#include "txSingleNodeContext.h"
#include "txXMLUtils.h"
#include "txXPathTreeWalker.h"

  
 






nsresult
PathExpr::addExpr(Expr* aExpr, PathOperator aPathOp)
{
    NS_ASSERTION(!mItems.IsEmpty() || aPathOp == RELATIVE_OP,
                 "First step has to be relative in PathExpr");
    PathExprItem* pxi = mItems.AppendElement();
    if (!pxi) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    pxi->expr = aExpr;
    pxi->pathOp = aPathOp;

    return NS_OK;
}

    
  









nsresult
PathExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;

    
    
    
    nsRefPtr<txAExprResult> res;
    nsresult rv = mItems[0].expr->evaluate(aContext, getter_AddRefs(res));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(res->getResultType() == txAExprResult::NODESET,
                   NS_ERROR_XSLT_NODESET_EXPECTED);

    nsRefPtr<txNodeSet> nodes = NS_STATIC_CAST(txNodeSet*,
                                               NS_STATIC_CAST(txAExprResult*,
                                                              res));
    if (nodes->isEmpty()) {
        res.swap(*aResult);

        return NS_OK;
    }
    res = nsnull; 

    
    PRUint32 i, len = mItems.Length();
    for (i = 1; i < len; ++i) {
        PathExprItem& pxi = mItems[i];
        nsRefPtr<txNodeSet> tmpNodes;
        txNodeSetContext eContext(nodes, aContext);
        while (eContext.hasNext()) {
            eContext.next();

            nsRefPtr<txNodeSet> resNodes;
            if (pxi.pathOp == DESCENDANT_OP) {
                rv = aContext->recycler()->getNodeSet(getter_AddRefs(resNodes));
                NS_ENSURE_SUCCESS(rv, rv);

                rv = evalDescendants(pxi.expr, eContext.getContextNode(),
                                     &eContext, resNodes);
                NS_ENSURE_SUCCESS(rv, rv);
            }
            else {
                nsRefPtr<txAExprResult> res;
                rv = pxi.expr->evaluate(&eContext, getter_AddRefs(res));
                NS_ENSURE_SUCCESS(rv, rv);

                if (res->getResultType() != txAExprResult::NODESET) {
                    
                    return NS_ERROR_XSLT_NODESET_EXPECTED;
                }
                resNodes = NS_STATIC_CAST(txNodeSet*,
                                          NS_STATIC_CAST(txAExprResult*,
                                                         res));
            }

            if (tmpNodes) {
                if (!resNodes->isEmpty()) {
                    nsRefPtr<txNodeSet> oldSet;
                    oldSet.swap(tmpNodes);
                    rv = aContext->recycler()->
                        getNonSharedNodeSet(oldSet, getter_AddRefs(tmpNodes));
                    NS_ENSURE_SUCCESS(rv, rv);

                    oldSet.swap(resNodes);
                    rv = aContext->recycler()->
                        getNonSharedNodeSet(oldSet, getter_AddRefs(resNodes));
                    NS_ENSURE_SUCCESS(rv, rv);

                    tmpNodes->addAndTransfer(resNodes);
                }
            }
            else {
                tmpNodes = resNodes;
            }
        }
        nodes = tmpNodes;
        if (nodes->isEmpty()) {
            break;
        }
    }

    *aResult = nodes;
    NS_ADDREF(*aResult);
    
    return NS_OK;
} 





nsresult
PathExpr::evalDescendants(Expr* aStep, const txXPathNode& aNode,
                          txIMatchContext* aContext, txNodeSet* resNodes)
{
    txSingleNodeContext eContext(aNode, aContext);
    nsRefPtr<txAExprResult> res;
    nsresult rv = aStep->evaluate(&eContext, getter_AddRefs(res));
    NS_ENSURE_SUCCESS(rv, rv);

    if (res->getResultType() != txAExprResult::NODESET) {
        
        return NS_ERROR_XSLT_NODESET_EXPECTED;
    }

    txNodeSet* oldSet = NS_STATIC_CAST(txNodeSet*,
                                       NS_STATIC_CAST(txAExprResult*, res));
    nsRefPtr<txNodeSet> newSet;
    rv = aContext->recycler()->getNonSharedNodeSet(oldSet,
                                                   getter_AddRefs(newSet));
    NS_ENSURE_SUCCESS(rv, rv);

    resNodes->addAndTransfer(newSet);

    MBool filterWS = aContext->isStripSpaceAllowed(aNode);

    txXPathTreeWalker walker(aNode);
    if (!walker.moveToFirstChild()) {
        return NS_OK;
    }

    do {
        const txXPathNode& node = walker.getCurrentPosition();
        if (!(filterWS && txXPathNodeUtils::isText(node) &&
              txXPathNodeUtils::isWhitespace(node))) {
            rv = evalDescendants(aStep, node, aContext, resNodes);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    } while (walker.moveToNextSibling());

    return NS_OK;
} 

Expr::ExprType
PathExpr::getType()
{
  return PATH_EXPR;
}

TX_IMPL_EXPR_STUBS_BASE(PathExpr, NODESET_RESULT)

Expr*
PathExpr::getSubExprAt(PRUint32 aPos)
{
    return aPos < mItems.Length() ? mItems[aPos].expr.get() : nsnull;
}
void
PathExpr::setSubExprAt(PRUint32 aPos, Expr* aExpr)
{
    NS_ASSERTION(aPos < mItems.Length(), "setting bad subexpression index");
    mItems[aPos].expr.forget();
    mItems[aPos].expr = aExpr;
}


PRBool
PathExpr::isSensitiveTo(ContextSensitivity aContext)
{
    if (mItems[0].expr->isSensitiveTo(aContext)) {
        return PR_TRUE;
    }

    
    Expr::ContextSensitivity context =
        aContext & ~(Expr::NODE_CONTEXT | Expr::NODESET_CONTEXT);
    if (context == NO_CONTEXT) {
        return PR_FALSE;
    }

    PRUint32 i, len = mItems.Length();
    for (i = 0; i < len; ++i) {
        NS_ASSERTION(!mItems[i].expr->isSensitiveTo(Expr::NODESET_CONTEXT),
                     "Step cannot depend on nodeset-context");
        if (mItems[i].expr->isSensitiveTo(context)) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

#ifdef TX_TO_STRING
void
PathExpr::toString(nsAString& dest)
{
    if (!mItems.IsEmpty()) {
        NS_ASSERTION(mItems[0].pathOp == RELATIVE_OP,
                     "First step should be relative");
        mItems[0].expr->toString(dest);
    }
    
    PRUint32 i, len = mItems.Length();
    for (i = 1; i < len; ++i) {
        switch (mItems[i].pathOp) {
            case DESCENDANT_OP:
                dest.AppendLiteral("//");
                break;
            case RELATIVE_OP:
                dest.Append(PRUnichar('/'));
                break;
        }
        mItems[i].expr->toString(dest);
    }
}
#endif
