




#ifndef TRANSFRMX_XPATHRESULTCOMPARATOR_H
#define TRANSFRMX_XPATHRESULTCOMPARATOR_H

#include "mozilla/Attributes.h"
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

    int compareValues(txObject* aVal1, txObject* aVal2) MOZ_OVERRIDE;
    nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                 txObject *&aResult) MOZ_OVERRIDE;
private:
    nsCOMPtr<nsICollation> mCollation;
    nsresult init(const nsAFlatString& aLanguage);
    nsresult createRawSortKey(const int32_t aStrength,
                              const nsString& aString,
                              uint8_t** aKey,
                              uint32_t* aLength);
    int mSorting;

    class StringValue : public txObject
    {
    public:
        StringValue();
        ~StringValue();

        uint8_t* mKey;
        void* mCaseKey;
        uint32_t mLength, mCaseLength;
    };
};




class txResultNumberComparator : public txXPathResultComparator
{
public:
    explicit txResultNumberComparator(bool aAscending);

    int compareValues(txObject* aVal1, txObject* aVal2) MOZ_OVERRIDE;
    nsresult createSortableValue(Expr *aExpr, txIEvalContext *aContext,
                                 txObject *&aResult) MOZ_OVERRIDE;

private:
    int mAscending;

    class NumberValue : public txObject
    {
    public:
        double mVal;
    };
};

#endif
