




#include "mozilla/Assertions.h"
#include "txXPathOptimizer.h"
#include "txExprResult.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "txXPathNode.h"
#include "txExpr.h"
#include "txIXPathContext.h"

class txEarlyEvalContext : public txIEvalContext
{
public:
    explicit txEarlyEvalContext(txResultRecycler* aRecycler)
        : mRecycler(aRecycler)
    {
    }

    
    nsresult getVariable(int32_t aNamespace, nsIAtom* aLName,
                         txAExprResult*& aResult)
    {
        MOZ_CRASH("shouldn't depend on this context");
    }
    bool isStripSpaceAllowed(const txXPathNode& aNode)
    {
        MOZ_CRASH("shouldn't depend on this context");
    }
    void* getPrivateContext()
    {
        MOZ_CRASH("shouldn't depend on this context");
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
        MOZ_CRASH("shouldn't depend on this context");
    }
    uint32_t size()
    {
        MOZ_CRASH("shouldn't depend on this context");
    }
    uint32_t position()
    {
        MOZ_CRASH("shouldn't depend on this context");
    }

private:
    txResultRecycler* mRecycler;
};


nsresult
txXPathOptimizer::optimize(Expr* aInExpr, Expr** aOutExpr)
{
    *aOutExpr = nullptr;
    nsresult rv = NS_OK;

    
    
    Expr::ExprType exprType = aInExpr->getType();
    if (exprType != Expr::LITERAL_EXPR &&
        !aInExpr->isSensitiveTo(Expr::ANY_CONTEXT)) {
        nsRefPtr<txResultRecycler> recycler = new txResultRecycler;
        txEarlyEvalContext context(recycler);
        nsRefPtr<txAExprResult> exprRes;

        
        
        rv = aInExpr->evaluate(&context, getter_AddRefs(exprRes));
        if (NS_SUCCEEDED(rv)) {
            *aOutExpr = new txLiteralExpr(exprRes);
        }
        
        return NS_OK;
    }

    
    uint32_t i = 0;
    Expr* subExpr;
    while ((subExpr = aInExpr->getSubExprAt(i))) {
        Expr* newExpr = nullptr;
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
    LocationStep* step = static_cast<LocationStep*>(aInExpr);

    if (step->getAxisIdentifier() == LocationStep::ATTRIBUTE_AXIS) {
        
        txNameTest* nameTest = nullptr;
        if (!step->getSubExprAt(0) &&
            step->getNodeTest()->getType() == txNameTest::NAME_TEST &&
            (nameTest = static_cast<txNameTest*>(step->getNodeTest()))->
                mLocalName != nsGkAtoms::_asterisk) {

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
    PathExpr* path = static_cast<PathExpr*>(aInExpr);

    uint32_t i;
    Expr* subExpr;
    
    
    for (i = 0; (subExpr = path->getSubExprAt(i)); ++i) {
        if (path->getPathOpAt(i) == PathExpr::DESCENDANT_OP &&
            subExpr->getType() == Expr::LOCATIONSTEP_EXPR &&
            !subExpr->getSubExprAt(0)) {
            LocationStep* step = static_cast<LocationStep*>(subExpr);
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
        step = static_cast<LocationStep*>(subExpr);
        if (step->getAxisIdentifier() == LocationStep::SELF_AXIS &&
            !step->getSubExprAt(0)) {
            txNodeTest* test = step->getNodeTest();
            txNodeTypeTest* typeTest;
            if (test->getType() == txNodeTest::NODETYPE_TEST &&
                (typeTest = static_cast<txNodeTypeTest*>(test))->
                  getNodeTestType() == txNodeTypeTest::NODE_TYPE) {
                

                
                
                if (!path->getSubExprAt(2)) {
                    *aOutExpr = path->getSubExprAt(1);
                    path->setSubExprAt(1, nullptr);

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
    UnionExpr* uni = static_cast<UnionExpr*>(aInExpr);

    
    

    nsresult rv;
    uint32_t current;
    Expr* subExpr;
    for (current = 0; (subExpr = uni->getSubExprAt(current)); ++current) {
        if (subExpr->getType() != Expr::LOCATIONSTEP_EXPR ||
            subExpr->getSubExprAt(0)) {
            continue;
        }

        LocationStep* currentStep = static_cast<LocationStep*>(subExpr);
        LocationStep::LocationStepType axis = currentStep->getAxisIdentifier();

        txUnionNodeTest* unionTest = nullptr;

        
        
        uint32_t i;
        for (i = current + 1; (subExpr = uni->getSubExprAt(i)); ++i) {
            if (subExpr->getType() != Expr::LOCATIONSTEP_EXPR ||
                subExpr->getSubExprAt(0)) {
                continue;
            }

            LocationStep* step = static_cast<LocationStep*>(subExpr);
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

            step->setNodeTest(nullptr);

            
            uni->deleteExprAt(i);
            --i;
        }

        
        
        if (unionTest && current == 0 && !uni->getSubExprAt(1)) {
            
            uni->setSubExprAt(0, nullptr);
            *aOutExpr = currentStep;

            
            return NS_OK;
        }
    }

    return NS_OK;
}
