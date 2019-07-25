






































#ifndef TRANSFRMX_XPATHRESULTCOMPARATOR_H
#define TRANSFRMX_XPATHRESULTCOMPARATOR_H

#include "txCore.h"
#include "nsCOMPtr.h"
#include "nsICollation.h"
#include "nsString.h"

class Expr;
class txIEvalContext;




class txXPathResultComparator
{
public:
    virtual ~txXPathResultComparator()
    {
    }

    



    virtual int compareValues(txObject* val1, txObject* val2) = 0;
    
    


    virtual nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                         txObject *&aResult) = 0;
};




class txResultStringComparator : public txXPathResultComparator
{
public:
    txResultStringComparator(bool aAscending, bool aUpperFirst,
                             const nsAFlatString& aLanguage);

    int compareValues(txObject* aVal1, txObject* aVal2);
    nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                 txObject *&aResult);
private:
    nsCOMPtr<nsICollation> mCollation;
    nsresult init(const nsAFlatString& aLanguage);
    nsresult createRawSortKey(const PRInt32 aStrength,
                              const nsString& aString,
                              PRUint8** aKey,
                              PRUint32* aLength);
    int mSorting;

    class StringValue : public txObject
    {
    public:
        StringValue();
        ~StringValue();

        PRUint8* mKey;
        void* mCaseKey;
        PRUint32 mLength, mCaseLength;
    };
};




class txResultNumberComparator : public txXPathResultComparator
{
public:
    txResultNumberComparator(bool aAscending);

    int compareValues(txObject* aVal1, txObject* aVal2);
    nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                 txObject *&aResult);

private:
    int mAscending;

    class NumberValue : public txObject
    {
    public:
        double mVal;
    };
};

#endif
