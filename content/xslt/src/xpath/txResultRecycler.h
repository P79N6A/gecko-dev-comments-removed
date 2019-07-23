





































#ifndef txResultRecycler_h__
#define txResultRecycler_h__

#include "nsCOMPtr.h"
#include "txStack.h"

class txAExprResult;
class StringResult;
class txNodeSet;
class txXPathNode;
class NumberResult;
class BooleanResult;

class txResultRecycler
{
public:
    txResultRecycler();
    ~txResultRecycler();
    nsresult init();

    void AddRef()
    {
        ++mRefCnt;
    }
    void Release()
    {
        if (--mRefCnt == 0) {
            mRefCnt = 1; 
            delete this;
        }
    }

    



    void recycle(txAExprResult* aResult);

    



    nsresult getStringResult(StringResult** aResult);
    nsresult getStringResult(const nsAString& aValue, txAExprResult** aResult);
    nsresult getNodeSet(txNodeSet** aResult);
    nsresult getNodeSet(txNodeSet* aNodeSet, txNodeSet** aResult);
    nsresult getNodeSet(const txXPathNode& aNode, txAExprResult** aResult);
    nsresult getNodeSet(const txXPathNode& aNode, txNodeSet** aResult);
    nsresult getNumberResult(double aValue, txAExprResult** aResult);

    



    void getEmptyStringResult(txAExprResult** aResult);
    void getBoolResult(PRBool aValue, txAExprResult** aResult);

    


    nsresult getNonSharedNodeSet(txNodeSet* aNodeSet, txNodeSet** aResult);

private:
    nsAutoRefCnt mRefCnt;
    txStack mStringResults;
    txStack mNodeSetResults;
    txStack mNumberResults;
    StringResult* mEmptyStringResult;
    BooleanResult* mTrueResult;
    BooleanResult* mFalseResult;
};

#endif 
