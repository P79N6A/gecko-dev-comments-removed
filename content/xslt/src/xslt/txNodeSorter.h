






































#ifndef TRANSFRMX_NODESORTER_H
#define TRANSFRMX_NODESORTER_H

#include "txCore.h"
#include "txList.h"

class Expr;
class txExecutionState;
class txNodeSet;
class TxObject;
class txXPathResultComparator;
class txIEvalContext;
class txNodeSetContext;





class txNodeSorter
{
public:
    txNodeSorter();
    ~txNodeSorter();

    nsresult addSortElement(Expr* aSelectExpr, Expr* aLangExpr,
                            Expr* aDataTypeExpr, Expr* aOrderExpr,
                            Expr* aCaseOrderExpr, txIEvalContext* aContext);
    nsresult sortNodeSet(txNodeSet* aNodes, txExecutionState* aEs,
                         txNodeSet** aResult);

private:
    struct SortData
    {
        txNodeSorter* mNodeSorter;
        txNodeSetContext* mContext;
        TxObject** mSortValues;
        nsresult mRv;
    };
    struct SortKey
    {
        Expr* mExpr;
        txXPathResultComparator* mComparator;
    };

    static int compareNodes(const void* aIndexA, const void* aIndexB,
                            void* aSortData);
    static bool calcSortValue(TxObject*& aSortValue, SortKey* aKey,
                                SortData* aSortData, PRUint32 aNodeIndex);
    txList mSortKeys;
    unsigned int mNKeys;
};

#endif
