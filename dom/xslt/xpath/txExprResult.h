




#ifndef TRANSFRMX_EXPRRESULT_H
#define TRANSFRMX_EXPRRESULT_H

#include "nsString.h"
#include "nsAutoPtr.h"
#include "txCore.h"
#include "txResultRecycler.h"










class txAExprResult
{
public:
    friend class txResultRecycler;

    
    
    enum ResultType {
        NODESET = 0,
        BOOLEAN,
        NUMBER,
        STRING,
        RESULT_TREE_FRAGMENT
    };

    txAExprResult(txResultRecycler* aRecycler) : mRecycler(aRecycler)
    {
    }
    virtual ~txAExprResult()
    {
    }

    void AddRef()
    {
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "txAExprResult", sizeof(*this));
    }
    
    void Release(); 

    



    virtual short getResultType()      = 0;

    




    virtual void stringValue(nsString& aResult) = 0;

    



    virtual const nsString* stringValuePointer() = 0;

    



    virtual bool booleanValue()          = 0;

    



    virtual double numberValue()          = 0;

private:
    nsAutoRefCnt mRefCnt;
    nsRefPtr<txResultRecycler> mRecycler;
};

#define TX_DECL_EXPRRESULT                                        \
    virtual short getResultType();                                \
    virtual void stringValue(nsString& aString);                  \
    virtual const nsString* stringValuePointer();                 \
    virtual bool booleanValue();                                \
    virtual double numberValue();                                 \


class BooleanResult : public txAExprResult {

public:
    BooleanResult(bool aValue);

    TX_DECL_EXPRRESULT

private:
    bool value;
};

class NumberResult : public txAExprResult {

public:
    NumberResult(double aValue, txResultRecycler* aRecycler);

    TX_DECL_EXPRRESULT

    double value;

};


class StringResult : public txAExprResult {
public:
    StringResult(txResultRecycler* aRecycler);
    StringResult(const nsAString& aValue, txResultRecycler* aRecycler);

    TX_DECL_EXPRRESULT

    nsString mValue;
};

#endif

