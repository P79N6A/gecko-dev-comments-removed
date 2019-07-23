





































#include "txXPathOptimizer.h"
#include "txExprResult.h"
#include "nsIAtom.h"
#include "txAtoms.h"
#include "txXPathNode.h"
#include "txExpr.h"
#include "txIXPathContext.h"

class txEarlyEvalContext : public txIEvalContext
{
public:
    txEarlyEvalContext(txResultRecycler* aRecycler)
        : mRecycler(aRecycler)
    {
    }

    
    nsresult getVariable(PRInt32 aNamespace, nsIAtom* aLName,
                         txAExprResult*& aResult)
    {
        NS_NOTREACHED("shouldn't depend on this context");
        return NS_ERROR_FAILURE;
    }
    PRBool isStripSpaceAllowed(const txXPathNode& aNode)
    {
        NS_NOTREACHED("shouldn't depend on this context");
        return PR_FALSE;
    }
    void* getPrivateContext()
    {
        NS_NOTREACHED("shouldn't depend on this context");
        return nsnull;
    }
    txResultRecycler* recycler()
    {
        return mRecycler;
    }
    void receiveError(const nsAString& aMsg, nsresult aRes)
    {
    }
    const txXPathNode& getContextNode()
    {
        NS_NOTREACHED("shouldn't depend on this context");

        
        

        return *NS_STATIC_CAST(txXPathNode*, nsnull);
    }
    PRUint32 size()
    {
        NS_NOTREACHED("shouldn't depend on this context");
        return 1;
    }
    PRUint32 position()
    {
        NS_NOTREACHED("shouldn't depend on this context");
        return 1;
    }

private:
    txResultRecycler* mRecycler;
};


nsresult
txXPathOptimizer::optimize(Expr* aInExpr, Expr** aOutExpr)
{
    *aOutExpr = nsnull;
    nsresult rv = NS_OK;

    
    
    Expr::ExprType exprType = aInExpr->getType();
    if (exprType != Expr::LITERAL_EXPR &&
        !aInExpr->isSensitiveTo(Expr::ANY_CONTEXT)) {
        nsRefPtr<txResultRecycler> recycler = new txResultRecycler;
        NS_ENSURE_TRUE(recycler, NS_ERROR_OUT_OF_MEMORY);

        rv = recycler->init();
        NS_ENSURE_SUCCESS(rv, rv);

        txEarlyEvalContext context(recycler);
        nsRefPtr<txAExprResult> exprRes;

        
        
        rv = aInExpr->evaluate(&context, getter_AddRefs(exprRes));
        if (NS_SUCCEEDED(rv)) {
            *aOutExpr = new txLiteralExpr(exprRes);
        }
        
        return NS_OK;
    }

    
    PRUint32 i = 0;
    Expr* subExpr;
    while ((subExpr = aInExpr->getSubExprAt(i))) {
        Expr* newExpr = nsnull;
        rv = optimize(subExpr, &newExpr);
        NS_ENSURE_SUCCESS(rv, rv);
        if (newExpr) {
            delete subExpr;
            aInExpr->setSubExprAt(i, newExpr);
        }

        ++i;
    }

    
    switch (exprType) {
        case Expr::LOCATIONSTEP_EXPR:
            return optimizeStep(aInExpr, aOutExpr);

        case Expr::PATH_EXPR:
            return optimizePath(aInExpr, aOutExpr);

        case Expr::UNION_EXPR:
            return optimizeUnion(aInExpr, aOutExpr);

        default:
            break;
    }

    return NS_OK;
}

nsresult
txXPathOptimizer::optimizeStep(Expr* aInExpr, Expr** aOutExpr)
{
    LocationStep* step = NS_STATIC_CAST(LocationStep*, aInExpr);

    if (step->getAxisIdentifier() == LocationStep::ATTRIBUTE_AXIS) {
        
        txNameTest* nameTest = nsnull;
        if (!step->getSubExprAt(0) &&
            step->getNodeTest()->getType() == txNameTest::NAME_TEST &&
            (nameTest = NS_STATIC_CAST(txNameTest*, step->getNodeTest()))->
                mLocalName != txXPathAtoms::_asterix) {

            *aOutExpr = new txNamedAttributeStep(nameTest->mNamespace,
                                                 nameTest->mPrefix,
                                                 nameTest->mLocalName);
            NS_ENSURE_TRUE(*aOutExpr, NS_ERROR_OUT_OF_MEMORY);

            return NS_OK; 
        }
    }

    
    Expr* pred;
    while ((pred = step->getSubExprAt(0)) &&
           !pred->canReturnType(Expr::NUMBER_RESULT) &&
           !pred->isSensitiveTo(Expr::NODESET_CONTEXT)) {
        txNodeTest* predTest = new txPredicatedNodeTest(step->getNodeTest(), pred);
        NS_ENSURE_TRUE(predTest, NS_ERROR_OUT_OF_MEMORY);

        step->dropFirst();
        step->setNodeTest(predTest);
    }

    return NS_OK;
}

nsresult
txXPathOptimizer::optimizePath(Expr* aInExpr, Expr** aOutExpr)
{
    PathExpr* path = NS_STATIC_CAST(PathExpr*, aInExpr);

    PRUint32 i;
    Expr* subExpr;
    
    
    for (i = 0; (subExpr = path->getSubExprAt(i)); ++i) {
        if (path->getPathOpAt(i) == PathExpr::DESCENDANT_OP &&
            subExpr->getType() == Expr::LOCATIONSTEP_EXPR &&
            !subExpr->getSubExprAt(0)) {
            LocationStep* step = NS_STATIC_CAST(LocationStep*, subExpr);
            if (step->getAxisIdentifier() == LocationStep::CHILD_AXIS) {
                step->setAxisIdentifier(LocationStep::DESCENDANT_AXIS);
                path->setPathOpAt(i, PathExpr::RELATIVE_OP);
            }
            else if (step->getAxisIdentifier() == LocationStep::SELF_AXIS) {
                step->setAxisIdentifier(LocationStep::DESCENDANT_OR_SELF_AXIS);
                path->setPathOpAt(i, PathExpr::RELATIVE_OP);
            }
        }
    }

    
    subExpr = path->getSubExprAt(0);
    LocationStep* step;
    if (subExpr->getType() == Expr::LOCATIONSTEP_EXPR &&
        path->getSubExprAt(1) &&
        path->getPathOpAt(1) != PathExpr::DESCENDANT_OP) {
        step = NS_STATIC_CAST(LocationStep*, subExpr);
        if (step->getAxisIdentifier() == LocationStep::SELF_AXIS &&
            !step->getSubExprAt(0)) {
            txNodeTest* test = step->getNodeTest();
            txNodeTypeTest* typeTest;
            if (test->getType() == txNodeTest::NODETYPE_TEST &&
                (typeTest = NS_STATIC_CAST(txNodeTypeTest*, test))->
                  getNodeTestType() == txNodeTypeTest::NODE_TYPE) {
                

                
                
                if (!path->getSubExprAt(2)) {
                    *aOutExpr = path->getSubExprAt(1);
                    path->setSubExprAt(1, nsnull);

                    return NS_OK;
                }

                
                path->deleteExprAt(0);
            }
        }
    }

    return NS_OK;
}

nsresult
txXPathOptimizer::optimizeUnion(Expr* aInExpr, Expr** aOutExpr)
{
    UnionExpr* uni = NS_STATIC_CAST(UnionExpr*, aInExpr);

    
    

    nsresult rv;
    PRUint32 current;
    Expr* subExpr;
    for (current = 0; (subExpr = uni->getSubExprAt(current)); ++current) {
        if (subExpr->getType() != Expr::LOCATIONSTEP_EXPR ||
            subExpr->getSubExprAt(0)) {
            continue;
        }

        LocationStep* currentStep = NS_STATIC_CAST(LocationStep*, subExpr);
        LocationStep::LocationStepType axis = currentStep->getAxisIdentifier();

        txUnionNodeTest* unionTest = nsnull;

        
        
        PRUint32 i;
        for (i = current + 1; (subExpr = uni->getSubExprAt(i)); ++i) {
            if (subExpr->getType() != Expr::LOCATIONSTEP_EXPR ||
                subExpr->getSubExprAt(0)) {
                continue;
            }

            LocationStep* step = NS_STATIC_CAST(LocationStep*, subExpr);
            if (step->getAxisIdentifier() != axis) {
                continue;
            }
            
            
            if (!unionTest) {
                nsAutoPtr<txNodeTest> owner(unionTest = new txUnionNodeTest);
                NS_ENSURE_TRUE(unionTest, NS_ERROR_OUT_OF_MEMORY);
                
                rv = unionTest->addNodeTest(currentStep->getNodeTest());
                NS_ENSURE_SUCCESS(rv, rv);

                currentStep->setNodeTest(unionTest);
                owner.forget();
            }

            
            rv = unionTest->addNodeTest(step->getNodeTest());
            NS_ENSURE_SUCCESS(rv, rv);

            step->setNodeTest(nsnull);

            
            uni->deleteExprAt(i);
            --i;
        }

        
        
        if (unionTest && current == 0 && !uni->getSubExprAt(1)) {
            
            uni->setSubExprAt(0, nsnull);
            *aOutExpr = currentStep;

            
            return NS_OK;
        }
    }

    return NS_OK;
}
