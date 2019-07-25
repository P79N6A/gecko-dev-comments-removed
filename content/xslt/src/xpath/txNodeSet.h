









































#ifndef txNodeSet_h__
#define txNodeSet_h__

#include "txExprResult.h"
#include "txError.h"
#include "txXPathNode.h"

class txNodeSet : public txAExprResult
{
public:
    


    txNodeSet(txResultRecycler* aRecycler);

    


    txNodeSet(const txXPathNode& aNode, txResultRecycler* aRecycler);

    



    txNodeSet(const txNodeSet& aSource, txResultRecycler* aRecycler);

    


    virtual ~txNodeSet();

    






    nsresult add(const txXPathNode& aNode);

    







    nsresult add(const txNodeSet& aNodes);
    nsresult addAndTransfer(txNodeSet* aNodes);

    









    




    nsresult append(const txXPathNode& aNode);

    




    nsresult append(const txNodeSet& aNodes);

    









    void setReverse()
    {
        mDirection = -1;
    }
    void unsetReverse()
    {
        mDirection = 1;
    }

    






    nsresult mark(PRInt32 aIndex);
    nsresult sweep();

    


    void clear();

    






    PRInt32 indexOf(const txXPathNode& aNode, PRUint32 aStart = 0) const;

    




    bool contains(const txXPathNode& aNode) const
    {
        return indexOf(aNode) >= 0;
    }

    




    const txXPathNode& get(PRInt32 aIndex) const;

    



    bool isEmpty() const
    {
        return mStart ? mStart == mEnd : true;
    }

    



    PRInt32 size() const
    {
        return mStart ? mEnd - mStart : 0;
    }

    TX_DECL_EXPRRESULT

private:
    




    bool ensureGrowSize(PRInt32 aSize);

    













    txXPathNode* findPosition(const txXPathNode& aNode, 
                              txXPathNode* aFirst,
                              txXPathNode* aLast, bool& aDupe) const;

    static void copyElements(txXPathNode* aDest, const txXPathNode* aStart,
                             const txXPathNode* aEnd);
    static void transferElements(txXPathNode* aDest, const txXPathNode* aStart,
                                 const txXPathNode* aEnd);
    static void destroyElements(const txXPathNode* aStart,
                                const txXPathNode* aEnd)
    {
        while (aStart < aEnd) {
            aStart->~txXPathNode();
            ++aStart;
        }
    }

    typedef void (*transferOp) (txXPathNode* aDest, const txXPathNode* aStart,
                                const txXPathNode* aEnd);
    typedef void (*destroyOp) (const txXPathNode* aStart,
                               const txXPathNode* aEnd);
    nsresult add(const txNodeSet& aNodes, transferOp aTransfer,
                 destroyOp aDestroy);

    txXPathNode *mStart, *mEnd, *mStartBuffer, *mEndBuffer;
    PRInt32 mDirection;
    
    bool* mMarks;
};

#endif
