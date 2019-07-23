





































#ifndef txXPathOptimizer_h__
#define txXPathOptimizer_h__

#include "txCore.h"

class Expr;

class txXPathOptimizer
{
public:
    





    nsresult optimize(Expr* aInExpr, Expr** aOutExpr);

private:
    
    nsresult optimizeStep(Expr* aInExpr, Expr** aOutExpr);
    nsresult optimizePath(Expr* aInExpr, Expr** aOutExpr);
    nsresult optimizeUnion(Expr* aInExpr, Expr** aOutExpr);
};

#endif
