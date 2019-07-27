




#ifndef TX_XSLT_PATTERNS_H
#define TX_XSLT_PATTERNS_H

#include "mozilla/Attributes.h"
#include "txExpandedName.h"
#include "txExpr.h"
#include "txXMLUtils.h"

class txPattern
{
public:
    txPattern()
    {
        MOZ_COUNT_CTOR(txPattern);
    }
    virtual ~txPattern()
    {
        MOZ_COUNT_DTOR(txPattern);
    }

    


    virtual bool matches(const txXPathNode& aNode,
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

    


    virtual Expr* getSubExprAt(uint32_t aPos) = 0;

    



    virtual void setSubExprAt(uint32_t aPos, Expr* aExpr) = 0;

    


    virtual txPattern* getSubPatternAt(uint32_t aPos) = 0;

    



    virtual void setSubPatternAt(uint32_t aPos, txPattern* aPattern) = 0;

#ifdef TX_TO_STRING
    







    virtual void toString(nsAString& aDest) = 0;
#endif
};

#define TX_DECL_PATTERN_BASE \
    bool matches(const txXPathNode& aNode, txIMatchContext* aContext) override; \
    double getDefaultPriority() override; \
    virtual Expr* getSubExprAt(uint32_t aPos) override; \
    virtual void setSubExprAt(uint32_t aPos, Expr* aExpr) override; \
    virtual txPattern* getSubPatternAt(uint32_t aPos) override; \
    virtual void setSubPatternAt(uint32_t aPos, txPattern* aPattern) override

#ifndef TX_TO_STRING
#define TX_DECL_PATTERN TX_DECL_PATTERN_BASE
#else
#define TX_DECL_PATTERN \
    TX_DECL_PATTERN_BASE; \
    void toString(nsAString& aDest) override
#endif

#define TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(_class)             \
Expr*                                                         \
_class::getSubExprAt(uint32_t aPos)                           \
{                                                             \
    return nullptr;                                            \
}                                                             \
void                                                          \
_class::setSubExprAt(uint32_t aPos, Expr* aExpr)              \
{                                                             \
    NS_NOTREACHED("setting bad subexpression index");         \
}

#define TX_IMPL_PATTERN_STUBS_NO_SUB_PATTERN(_class)          \
txPattern*                                                    \
_class::getSubPatternAt(uint32_t aPos)                        \
{                                                             \
    return nullptr;                                            \
}                                                             \
void                                                          \
_class::setSubPatternAt(uint32_t aPos, txPattern* aPattern)   \
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
    Type getType() override;

private:
    txOwningArray<txPattern> mLocPathPatterns;
};

class txLocPathPattern : public txPattern
{
public:
    nsresult addStep(txPattern* aPattern, bool isChild);

    TX_DECL_PATTERN;

private:
    class Step {
    public:
        nsAutoPtr<txPattern> pattern;
        bool isChild;
    };

    nsTArray<Step> mSteps;
};

class txRootPattern : public txPattern
{
public:
#ifdef TX_TO_STRING
    txRootPattern()
        : mSerialize(true)
    {
    }
#endif

    TX_DECL_PATTERN;

#ifdef TX_TO_STRING
public:
    void setSerialize(bool aSerialize)
    {
        mSerialize = aSerialize;
    }

private:
    
    bool mSerialize;
#endif
};

class txIdPattern : public txPattern
{
public:
    explicit txIdPattern(const nsSubstring& aString);

    TX_DECL_PATTERN;

private:
    nsCOMArray<nsIAtom> mIds;
};

class txKeyPattern : public txPattern
{
public:
    txKeyPattern(nsIAtom* aPrefix, nsIAtom* aLocalName,
                 int32_t aNSID, const nsAString& aValue)
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
    txStepPattern(txNodeTest* aNodeTest, bool isAttr)
        : mNodeTest(aNodeTest), mIsAttr(isAttr)
    {
    }

    TX_DECL_PATTERN;
    Type getType() override;

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
    bool mIsAttr;
};

#endif 
