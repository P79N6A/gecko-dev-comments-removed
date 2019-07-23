





































#ifndef TRANSFRMX_TXXSLTNUMBER_H
#define TRANSFRMX_TXXSLTNUMBER_H

#include "txError.h"
#include "txList.h"
#include "nsString.h"

class Expr;
class txPattern;
class txIEvalContext;
class txIMatchContext;
class txXPathTreeWalker;

class txXSLTNumber {
public:
    enum LevelType {
        eLevelSingle,
        eLevelMultiple,
        eLevelAny
    };

    static nsresult createNumber(Expr* aValueExpr, txPattern* aCountPattern,
                                 txPattern* aFromPattern, LevelType aLevel,
                                 Expr* aGroupSize, Expr* aGroupSeparator,
                                 Expr* aFormat, txIEvalContext* aContext,
                                 nsAString& aResult);

private:
    static nsresult getValueList(Expr* aValueExpr, txPattern* aCountPattern,
                                 txPattern* aFromPattern, LevelType aLevel,
                                 txIEvalContext* aContext, txList& aValues,
                                 nsAString& aValueString);

    static nsresult getCounters(Expr* aGroupSize, Expr* aGroupSeparator,
                                Expr* aFormat, txIEvalContext* aContext,
                                txList& aCounters, nsAString& aHead,
                                nsAString& aTail);

    




    static PRInt32 getSiblingCount(txXPathTreeWalker& aWalker,
                                   txPattern* aCountPattern,
                                   txIMatchContext* aContext);
    
    static PRBool getPrevInDocumentOrder(txXPathTreeWalker& aWalker);

    static MBool isAlphaNumeric(PRUnichar ch);
};

class txFormattedCounter {
public:
    virtual ~txFormattedCounter()
    {
    }
    
    virtual void appendNumber(PRInt32 aNumber, nsAString& aDest) = 0;

    static nsresult getCounterFor(const nsAFlatString& aToken, int aGroupSize,
                                  const nsAString& aGroupSeparator,
                                  txFormattedCounter*& aCounter);
    
    nsString mSeparator;
};

#endif
