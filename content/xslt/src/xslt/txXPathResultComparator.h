






































#ifndef TRANSFRMX_XPATHRESULTCOMPARATOR_H
#define TRANSFRMX_XPATHRESULTCOMPARATOR_H

#include "txCore.h"
#ifndef TX_EXE
#include "nsCOMPtr.h"
#include "nsICollation.h"
#endif
#include "nsString.h"

class Expr;
class txIEvalContext;




class txXPathResultComparator
{
public:
    virtual ~txXPathResultComparator()
    {
    }

    



    virtual int compareValues(TxObject* val1, TxObject* val2) = 0;
    
    


    virtual nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                         TxObject *&aResult) = 0;
};




class txResultStringComparator : public txXPathResultComparator
{
public:
    txResultStringComparator(MBool aAscending, MBool aUpperFirst,
                             const nsAFlatString& aLanguage);
    virtual ~txResultStringComparator();

    int compareValues(TxObject* aVal1, TxObject* aVal2);
    nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                 TxObject *&aResult);
private:
#ifndef TX_EXE
    nsCOMPtr<nsICollation> mCollation;
    nsresult init(const nsAFlatString& aLanguage);
    nsresult createRawSortKey(const PRInt32 aStrength,
                              const nsString& aString,
                              PRUint8** aKey,
                              PRUint32* aLength);
#endif
    int mSorting;

    class StringValue : public TxObject
    {
    public:
#ifdef TX_EXE
        nsString mStr;
#else
        StringValue();
        ~StringValue();

        PRUint8* mKey;
        void* mCaseKey;
        PRUint32 mLength, mCaseLength;
#endif
    };
};




class txResultNumberComparator : public txXPathResultComparator
{
public:
    txResultNumberComparator(MBool aAscending);
    virtual ~txResultNumberComparator();

    int compareValues(TxObject* aVal1, TxObject* aVal2);
    nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                 TxObject *&aResult);

private:
    int mAscending;

    class NumberValue : public TxObject
    {
    public:
        double mVal;
    };
};

#endif
