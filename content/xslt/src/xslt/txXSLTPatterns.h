





































#ifndef TX_XSLT_PATTERNS_H
#define TX_XSLT_PATTERNS_H

#include "txExpr.h"
#include "txXMLUtils.h"
#include "nsVoidArray.h"

class ProcessorState;

class txPattern
{
public:
    


    virtual MBool matches(const txXPathNode& aNode,
                          txIMatchContext* aContext) = 0;

    





    virtual double getDefaultPriority() = 0;

    


    enum Type {
        STEP_PATTERN,
        UNION_PATTERN,
        OTHER_PATTERN
    };
    virtual Type getType()
    {
      return OTHER_PATTERN;
    }

    


    virtual Expr* getSubExprAt(PRUint32 aPos) = 0;

    



    virtual void setSubExprAt(PRUint32 aPos, Expr* aExpr) = 0;

    


    virtual txPattern* getSubPatternAt(PRUint32 aPos) = 0;

    



    virtual void setSubPatternAt(PRUint32 aPos, txPattern* aPattern) = 0;

#ifdef TX_TO_STRING
    







    virtual void toString(nsAString& aDest) = 0;
#endif
};

#define TX_DECL_PATTERN_BASE \
    MBool matches(const txXPathNode& aNode, txIMatchContext* aContext); \
    double getDefaultPriority(); \
    virtual Expr* getSubExprAt(PRUint32 aPos); \
    virtual void setSubExprAt(PRUint32 aPos, Expr* aExpr); \
    virtual txPattern* getSubPatternAt(PRUint32 aPos); \
    virtual void setSubPatternAt(PRUint32 aPos, txPattern* aPattern)

#ifndef TX_TO_STRING
#define TX_DECL_PATTERN TX_DECL_PATTERN_BASE
#else
#define TX_DECL_PATTERN \
    TX_DECL_PATTERN_BASE; \
    void toString(nsAString& aDest)
#endif

#define TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(_class)             \
Expr*                                                         \
_class::getSubExprAt(PRUint32 aPos)                           \
{                                                             \
    return nsnull;                                            \
}                                                             \
void                                                          \
_class::setSubExprAt(PRUint32 aPos, Expr* aExpr)              \
{                                                             \
    NS_NOTREACHED("setting bad subexpression index");         \
}

#define TX_IMPL_PATTERN_STUBS_NO_SUB_PATTERN(_class)          \
txPattern*                                                    \
_class::getSubPatternAt(PRUint32 aPos)                        \
{                                                             \
    return nsnull;                                            \
}                                                             \
void                                                          \
_class::setSubPatternAt(PRUint32 aPos, txPattern* aPattern)   \
{                                                             \
    NS_NOTREACHED("setting bad subexpression index");         \
}

class txUnionPattern : public txPattern
{
public:
    nsresult addPattern(txPattern* aPattern)
    {
        return mLocPathPatterns.AppendElement(aPattern) ? 
            NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    TX_DECL_PATTERN;
    Type getType();

private:
    txOwningArray<txPattern> mLocPathPatterns;
};

class txLocPathPattern : public txPattern
{
public:
    nsresult addStep(txPattern* aPattern, PRBool isChild);

    TX_DECL_PATTERN;

private:
    class Step {
    public:
        nsAutoPtr<txPattern> pattern;
        PRBool isChild;
    };

    nsTArray<Step> mSteps;
};

class txRootPattern : public txPattern
{
public:
#ifdef TX_TO_STRING
    txRootPattern()
        : mSerialize(PR_TRUE)
    {
    }
#endif

    TX_DECL_PATTERN;

#ifdef TX_TO_STRING
public:
    void setSerialize(PRBool aSerialize)
    {
        mSerialize = aSerialize;
    }

private:
    
    PRBool mSerialize;
#endif
};

class txIdPattern : public txPattern
{
public:
    txIdPattern(const nsSubstring& aString);

    TX_DECL_PATTERN;

private:
    nsCOMArray<nsIAtom> mIds;
};

class txKeyPattern : public txPattern
{
public:
    txKeyPattern(nsIAtom* aPrefix, nsIAtom* aLocalName,
                 PRInt32 aNSID, const nsAString& aValue)
        : mName(aNSID, aLocalName),
#ifdef TX_TO_STRING
          mPrefix(aPrefix),
#endif
          mValue(aValue)
    {
    }

    TX_DECL_PATTERN;

private:
    txExpandedName mName;
#ifdef TX_TO_STRING
    nsCOMPtr<nsIAtom> mPrefix;
#endif
    nsString mValue;
};

class txStepPattern : public txPattern,
                      public PredicateList
{
public:
    txStepPattern(txNodeTest* aNodeTest, PRBool isAttr)
        : mNodeTest(aNodeTest), mIsAttr(isAttr)
    {
    }

    TX_DECL_PATTERN;
    Type getType();

    txNodeTest* getNodeTest()
    {
      return mNodeTest;
    }
    void setNodeTest(txNodeTest* aNodeTest)
    {
      mNodeTest.forget();
      mNodeTest = aNodeTest;
    }

private:
    nsAutoPtr<txNodeTest> mNodeTest;
    PRBool mIsAttr;
};

#endif 
