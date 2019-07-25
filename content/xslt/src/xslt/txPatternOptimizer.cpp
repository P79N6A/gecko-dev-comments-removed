




#include "txPatternOptimizer.h"
#include "txXSLTPatterns.h"

nsresult
txPatternOptimizer::optimize(txPattern* aInPattern, txPattern** aOutPattern)
{
    *aOutPattern = nullptr;
    nsresult rv = NS_OK;

    
    uint32_t i = 0;
    Expr* subExpr;
    while ((subExpr = aInPattern->getSubExprAt(i))) {
        Expr* newExpr = nullptr;
        rv = mXPathOptimizer.optimize(subExpr, &newExpr);
        NS_ENSURE_SUCCESS(rv, rv);
        if (newExpr) {
            delete subExpr;
            aInPattern->setSubExprAt(i, newExpr);
        }

        ++i;
    }

    
    txPattern* subPattern;
    i = 0;
    while ((subPattern = aInPattern->getSubPatternAt(i))) {
        txPattern* newPattern = nullptr;
        rv = optimize(subPattern, &newPattern);
        NS_ENSURE_SUCCESS(rv, rv);
        if (newPattern) {
            delete subPattern;
            aInPattern->setSubPatternAt(i, newPattern);
        }

        ++i;
    }

    
    switch (aInPattern->getType()) {
        case txPattern::STEP_PATTERN:
            return optimizeStep(aInPattern, aOutPattern);

        default:
            break;
    }

    return NS_OK;
}


nsresult
txPatternOptimizer::optimizeStep(txPattern* aInPattern,
                                 txPattern** aOutPattern)
{
    txStepPattern* step = static_cast<txStepPattern*>(aInPattern);

    
    Expr* pred;
    while ((pred = step->getSubExprAt(0)) &&
           !pred->canReturnType(Expr::NUMBER_RESULT) &&
           !pred->isSensitiveTo(Expr::NODESET_CONTEXT)) {
        txNodeTest* predTest = new txPredicatedNodeTest(step->getNodeTest(),
                                                        pred);
        step->dropFirst();
        step->setNodeTest(predTest);
    }

    return NS_OK;
}
