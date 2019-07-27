




#ifndef TRANSFRMX_EXPR_H
#define TRANSFRMX_EXPR_H

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "txExprResult.h"
#include "txCore.h"
#include "nsString.h"
#include "txOwningArray.h"
#include "nsIAtom.h"

#ifdef DEBUG
#define TX_TO_STRING
#endif






class nsIAtom;
class txIMatchContext;
class txIEvalContext;
class txNodeSet;
class txXPathNode;




class Expr
{
public:
    Expr()
    {
        MOZ_COUNT_CTOR(Expr);
    }
    virtual ~Expr()
    {
        MOZ_COUNT_DTOR(Expr);
    }

    






    virtual nsresult evaluate(txIEvalContext* aContext,
                              txAExprResult** aResult) = 0;


    


    enum ExprType {
        LOCATIONSTEP_EXPR,
        PATH_EXPR,
        UNION_EXPR,
        LITERAL_EXPR,
        OTHER_EXPR
    };
    virtual ExprType getType()
    {
      return OTHER_EXPR;
    }

    


    typedef uint16_t ResultType;
    enum {
        NODESET_RESULT = 0x01,
        BOOLEAN_RESULT = 0x02,
        NUMBER_RESULT = 0x04,
        STRING_RESULT = 0x08,
        RTF_RESULT = 0x10,
        ANY_RESULT = 0xFFFF
    };
    virtual ResultType getReturnType() = 0;
    bool canReturnType(ResultType aType)
    {
        return (getReturnType() & aType) != 0;
    }

    typedef uint16_t ContextSensitivity;
    enum {
        NO_CONTEXT = 0x00,
        NODE_CONTEXT = 0x01,
        POSITION_CONTEXT = 0x02,
        SIZE_CONTEXT = 0x04,
        NODESET_CONTEXT = POSITION_CONTEXT | SIZE_CONTEXT,
        VARIABLES_CONTEXT = 0x08,
        PRIVATE_CONTEXT = 0x10,
        ANY_CONTEXT = 0xFFFF
    };

    



    virtual bool isSensitiveTo(ContextSensitivity aContexts) = 0;

    


    virtual Expr* getSubExprAt(uint32_t aPos) = 0;

    



    virtual void setSubExprAt(uint32_t aPos, Expr* aExpr) = 0;

    virtual nsresult evaluateToBool(txIEvalContext* aContext,
                                    bool& aResult);

    virtual nsresult evaluateToString(txIEvalContext* aContext,
                                      nsString& aResult);

#ifdef TX_TO_STRING
    







    virtual void toString(nsAString& str) = 0;
#endif
}; 

#ifdef TX_TO_STRING
#define TX_DECL_TOSTRING \
    void toString(nsAString& aDest) override;
#define TX_DECL_GETNAMEATOM \
    nsresult getNameAtom(nsIAtom** aAtom) override;
#else
#define TX_DECL_TOSTRING
#define TX_DECL_GETNAMEATOM
#endif

#define TX_DECL_EXPR_BASE \
    nsresult evaluate(txIEvalContext* aContext, txAExprResult** aResult) override; \
    ResultType getReturnType() override; \
    bool isSensitiveTo(ContextSensitivity aContexts) override;

#define TX_DECL_EXPR \
    TX_DECL_EXPR_BASE \
    TX_DECL_TOSTRING \
    Expr* getSubExprAt(uint32_t aPos) override; \
    void setSubExprAt(uint32_t aPos, Expr* aExpr) override;

#define TX_DECL_OPTIMIZABLE_EXPR \
    TX_DECL_EXPR \
    ExprType getType() override;

#define TX_DECL_FUNCTION \
    TX_DECL_GETNAMEATOM \
    TX_DECL_EXPR_BASE

#define TX_IMPL_EXPR_STUBS_BASE(_class, _ReturnType)          \
Expr::ResultType                                              \
_class::getReturnType()                                       \
{                                                             \
    return _ReturnType;                                       \
}

#define TX_IMPL_EXPR_STUBS_0(_class, _ReturnType)             \
TX_IMPL_EXPR_STUBS_BASE(_class, _ReturnType)                  \
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

#define TX_IMPL_EXPR_STUBS_1(_class, _ReturnType, _Expr1)     \
TX_IMPL_EXPR_STUBS_BASE(_class, _ReturnType)                  \
Expr*                                                         \
_class::getSubExprAt(uint32_t aPos)                           \
{                                                             \
    if (aPos == 0) {                                          \
        return _Expr1;                                        \
    }                                                         \
    return nullptr;                                            \
}                                                             \
void                                                          \
_class::setSubExprAt(uint32_t aPos, Expr* aExpr)              \
{                                                             \
    NS_ASSERTION(aPos < 1, "setting bad subexpression index");\
    _Expr1.forget();                                          \
    _Expr1 = aExpr;                                           \
}

#define TX_IMPL_EXPR_STUBS_2(_class, _ReturnType, _Expr1, _Expr2) \
TX_IMPL_EXPR_STUBS_BASE(_class, _ReturnType)                  \
Expr*                                                         \
_class::getSubExprAt(uint32_t aPos)                           \
{                                                             \
    switch(aPos) {                                            \
        case 0:                                               \
            return _Expr1;                                    \
        case 1:                                               \
            return _Expr2;                                    \
        default:                                              \
            break;                                            \
    }                                                         \
    return nullptr;                                            \
}                                                             \
void                                                          \
_class::setSubExprAt(uint32_t aPos, Expr* aExpr)              \
{                                                             \
    NS_ASSERTION(aPos < 2, "setting bad subexpression index");\
    if (aPos == 0) {                                          \
        _Expr1.forget();                                      \
        _Expr1 = aExpr;                                       \
    }                                                         \
    else {                                                    \
        _Expr2.forget();                                      \
        _Expr2 = aExpr;                                       \
    }                                                         \
}

#define TX_IMPL_EXPR_STUBS_LIST(_class, _ReturnType, _ExprList) \
TX_IMPL_EXPR_STUBS_BASE(_class, _ReturnType)                  \
Expr*                                                         \
_class::getSubExprAt(uint32_t aPos)                           \
{                                                             \
    return _ExprList.SafeElementAt(aPos);                     \
}                                                             \
void                                                          \
_class::setSubExprAt(uint32_t aPos, Expr* aExpr)              \
{                                                             \
    NS_ASSERTION(aPos < _ExprList.Length(),                   \
                 "setting bad subexpression index");          \
    _ExprList[aPos] = aExpr;                                  \
}






class FunctionCall : public Expr
{
public:
    






    nsresult addParam(Expr* aExpr)
    {
        return mParams.AppendElement(aExpr) ?
            NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    










    virtual bool requireParams(int32_t aParamCountMin,
                                 int32_t aParamCountMax,
                                 txIEvalContext* aContext);

    TX_DECL_TOSTRING
    Expr* getSubExprAt(uint32_t aPos) override;
    void setSubExprAt(uint32_t aPos, Expr* aExpr) override;

protected:

    txOwningArray<Expr> mParams;

    


    static nsresult evaluateToNumber(Expr* aExpr, txIEvalContext* aContext,
                                     double* aResult);

    



    static nsresult evaluateToNodeSet(Expr* aExpr, txIEvalContext* aContext,
                                      txNodeSet** aResult);

    


    bool argsSensitiveTo(ContextSensitivity aContexts);


#ifdef TX_TO_STRING
    


    virtual nsresult getNameAtom(nsIAtom** aAtom) = 0;
#endif
};

class txCoreFunctionCall : public FunctionCall
{
public:

    
    
    enum eType {
        COUNT = 0,         
        ID,                
        LAST,              
        LOCAL_NAME,        
        NAMESPACE_URI,     
        NAME,              
        POSITION,          

        CONCAT,            
        CONTAINS,          
        NORMALIZE_SPACE,   
        STARTS_WITH,       
        STRING,            
        STRING_LENGTH,     
        SUBSTRING,         
        SUBSTRING_AFTER,   
        SUBSTRING_BEFORE,  
        TRANSLATE,         

        NUMBER,            
        ROUND,             
        FLOOR,             
        CEILING,           
        SUM,               

        BOOLEAN,           
        _FALSE,            
        LANG,              
        _NOT,              
        _TRUE              
    };

    


    explicit txCoreFunctionCall(eType aType) : mType(aType)
    {
    }

    TX_DECL_FUNCTION

    static bool getTypeFromAtom(nsIAtom* aName, eType& aType);

private:
    eType mType;
};





class txNodeTest
{
public:
    txNodeTest()
    {
        MOZ_COUNT_CTOR(txNodeTest);
    }
    virtual ~txNodeTest()
    {
        MOZ_COUNT_DTOR(txNodeTest);
    }

    





    virtual bool matches(const txXPathNode& aNode,
                           txIMatchContext* aContext) = 0;
    virtual double getDefaultPriority() = 0;

    


    enum NodeTestType {
        NAME_TEST,
        NODETYPE_TEST,
        OTHER_TEST
    };
    virtual NodeTestType getType()
    {
      return OTHER_TEST;
    }

    



    virtual bool isSensitiveTo(Expr::ContextSensitivity aContext) = 0;

#ifdef TX_TO_STRING
    virtual void toString(nsAString& aDest) = 0;
#endif
};

#define TX_DECL_NODE_TEST \
    TX_DECL_TOSTRING \
    bool matches(const txXPathNode& aNode, txIMatchContext* aContext) override; \
    double getDefaultPriority() override; \
    bool isSensitiveTo(Expr::ContextSensitivity aContext) override;




class txNameTest : public txNodeTest
{
public:
    



    txNameTest(nsIAtom* aPrefix, nsIAtom* aLocalName, int32_t aNSID,
               uint16_t aNodeType);

    NodeTestType getType() override;

    TX_DECL_NODE_TEST

    nsCOMPtr<nsIAtom> mPrefix;
    nsCOMPtr<nsIAtom> mLocalName;
    int32_t mNamespace;
private:
    uint16_t mNodeType;
};




class txNodeTypeTest : public txNodeTest
{
public:
    enum NodeType {
        COMMENT_TYPE,
        TEXT_TYPE,
        PI_TYPE,
        NODE_TYPE
    };

    


    explicit txNodeTypeTest(NodeType aNodeType)
        : mNodeType(aNodeType)
    {
    }

    


    void setNodeName(const nsAString& aName)
    {
        mNodeName = do_GetAtom(aName);
    }

    NodeType getNodeTestType()
    {
        return mNodeType;
    }

    NodeTestType getType() override;

    TX_DECL_NODE_TEST

private:
    NodeType mNodeType;
    nsCOMPtr<nsIAtom> mNodeName;
};





class txPredicatedNodeTest : public txNodeTest
{
public:
    txPredicatedNodeTest(txNodeTest* aNodeTest, Expr* aPredicate);
    TX_DECL_NODE_TEST

private:
    nsAutoPtr<txNodeTest> mNodeTest;
    nsAutoPtr<Expr> mPredicate;
};





class PredicateList  {
public:
    






    nsresult add(Expr* aExpr)
    {
        NS_ASSERTION(aExpr, "missing expression");
        return mPredicates.AppendElement(aExpr) ?
            NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult evaluatePredicates(txNodeSet* aNodes, txIMatchContext* aContext);

    


    void dropFirst()
    {
        mPredicates.RemoveElementAt(0);
    }

    


    bool isEmpty()
    {
        return mPredicates.IsEmpty();
    }

#ifdef TX_TO_STRING
    







    void toString(nsAString& dest);
#endif

protected:
    bool isSensitiveTo(Expr::ContextSensitivity aContext);
    Expr* getSubExprAt(uint32_t aPos)
    {
        return mPredicates.SafeElementAt(aPos);
    }
    void setSubExprAt(uint32_t aPos, Expr* aExpr)
    {
        NS_ASSERTION(aPos < mPredicates.Length(),
                     "setting bad subexpression index");
        mPredicates[aPos] = aExpr;
    }

    
    txOwningArray<Expr> mPredicates;
}; 

class LocationStep : public Expr,
                     public PredicateList
{
public:
    enum LocationStepType {
        ANCESTOR_AXIS = 0,
        ANCESTOR_OR_SELF_AXIS,
        ATTRIBUTE_AXIS,
        CHILD_AXIS,
        DESCENDANT_AXIS,
        DESCENDANT_OR_SELF_AXIS,
        FOLLOWING_AXIS,
        FOLLOWING_SIBLING_AXIS,
        NAMESPACE_AXIS,
        PARENT_AXIS,
        PRECEDING_AXIS,
        PRECEDING_SIBLING_AXIS,
        SELF_AXIS
    };

    




    LocationStep(txNodeTest* aNodeTest,
                 LocationStepType aAxisIdentifier)
        : mNodeTest(aNodeTest),
          mAxisIdentifier(aAxisIdentifier)
    {
    }

    TX_DECL_OPTIMIZABLE_EXPR

    txNodeTest* getNodeTest()
    {
        return mNodeTest;
    }
    void setNodeTest(txNodeTest* aNodeTest)
    {
        mNodeTest.forget();
        mNodeTest = aNodeTest;
    }
    LocationStepType getAxisIdentifier()
    {
        return mAxisIdentifier;
    }
    void setAxisIdentifier(LocationStepType aAxisIdentifier)
    {
        mAxisIdentifier = aAxisIdentifier;
    }

private:
    void fromDescendants(const txXPathNode& aNode, txIMatchContext* aCs,
                         txNodeSet* aNodes);
    void fromDescendantsRev(const txXPathNode& aNode, txIMatchContext* aCs,
                            txNodeSet* aNodes);

    nsAutoPtr<txNodeTest> mNodeTest;
    LocationStepType mAxisIdentifier;
};

class FilterExpr : public Expr,
                   public PredicateList
{
public:

    



    explicit FilterExpr(Expr* aExpr)
        : expr(aExpr)
    {
    }

    TX_DECL_EXPR

private:
    nsAutoPtr<Expr> expr;

}; 


class txLiteralExpr : public Expr {
public:
    explicit txLiteralExpr(double aDbl)
        : mValue(new NumberResult(aDbl, nullptr))
    {
    }
    explicit txLiteralExpr(const nsAString& aStr)
        : mValue(new StringResult(aStr, nullptr))
    {
    }
    explicit txLiteralExpr(txAExprResult* aValue)
        : mValue(aValue)
    {
    }

    TX_DECL_EXPR

private:
    nsRefPtr<txAExprResult> mValue;
};




class UnaryExpr : public Expr {

public:

    explicit UnaryExpr(Expr* aExpr)
        : expr(aExpr)
    {
    }

    TX_DECL_EXPR

private:
    nsAutoPtr<Expr> expr;
}; 





class BooleanExpr : public Expr
{
public:

    
    enum _BooleanExprType { AND = 1, OR };

     BooleanExpr(Expr* aLeftExpr, Expr* aRightExpr, short aOp)
         : leftExpr(aLeftExpr),
           rightExpr(aRightExpr),
           op(aOp)
    {
    }

    TX_DECL_EXPR

private:
    nsAutoPtr<Expr> leftExpr, rightExpr;
    short op;
}; 









class txNumberExpr : public Expr
{
public:

    enum eOp { ADD, SUBTRACT, DIVIDE, MULTIPLY, MODULUS };

    txNumberExpr(Expr* aLeftExpr, Expr* aRightExpr, eOp aOp)
        : mLeftExpr(aLeftExpr),
          mRightExpr(aRightExpr),
          mOp(aOp)
    {
    }

    TX_DECL_EXPR

private:
    nsAutoPtr<Expr> mLeftExpr, mRightExpr;
    eOp mOp;
}; 











class RelationalExpr : public Expr
{
public:
    enum RelationalExprType {
        EQUAL,
        NOT_EQUAL,
        LESS_THAN,
        GREATER_THAN,
        LESS_OR_EQUAL,
        GREATER_OR_EQUAL
    };

    RelationalExpr(Expr* aLeftExpr, Expr* aRightExpr, RelationalExprType aOp)
        : mLeftExpr(aLeftExpr),
          mRightExpr(aRightExpr),
          mOp(aOp)
    {
    }


    TX_DECL_EXPR

private:
    bool compareResults(txIEvalContext* aContext, txAExprResult* aLeft,
                          txAExprResult* aRight);

    nsAutoPtr<Expr> mLeftExpr;
    nsAutoPtr<Expr> mRightExpr;
    RelationalExprType mOp;
};





class VariableRefExpr : public Expr {

public:

    VariableRefExpr(nsIAtom* aPrefix, nsIAtom* aLocalName, int32_t aNSID);

    TX_DECL_EXPR

private:
    nsCOMPtr<nsIAtom> mPrefix;
    nsCOMPtr<nsIAtom> mLocalName;
    int32_t mNamespace;
};




class PathExpr : public Expr {

public:

    
    
    
    enum PathOperator { RELATIVE_OP, DESCENDANT_OP };

    






    nsresult addExpr(Expr* aExpr, PathOperator pathOp);

    


    void deleteExprAt(uint32_t aPos)
    {
        NS_ASSERTION(aPos < mItems.Length(),
                     "killing bad expression index");
        mItems.RemoveElementAt(aPos);
    }

    TX_DECL_OPTIMIZABLE_EXPR

    PathOperator getPathOpAt(uint32_t aPos)
    {
        NS_ASSERTION(aPos < mItems.Length(), "getting bad pathop index");
        return mItems[aPos].pathOp;
    }
    void setPathOpAt(uint32_t aPos, PathOperator aPathOp)
    {
        NS_ASSERTION(aPos < mItems.Length(), "setting bad pathop index");
        mItems[aPos].pathOp = aPathOp;
    }

private:
    class PathExprItem {
    public:
        nsAutoPtr<Expr> expr;
        PathOperator pathOp;
    };

    nsTArray<PathExprItem> mItems;

    



    nsresult evalDescendants(Expr* aStep, const txXPathNode& aNode,
                             txIMatchContext* aContext,
                             txNodeSet* resNodes);
};




class RootExpr : public Expr {
public:
    


    RootExpr()
#ifdef TX_TO_STRING
        : mSerialize(true)
#endif
    {
    }

    TX_DECL_EXPR

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




class UnionExpr : public Expr {
public:
    






    nsresult addExpr(Expr* aExpr)
    {
        return mExpressions.AppendElement(aExpr) ?
            NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    


    void deleteExprAt(uint32_t aPos)
    {
        NS_ASSERTION(aPos < mExpressions.Length(),
                     "killing bad expression index");

        delete mExpressions[aPos];
        mExpressions.RemoveElementAt(aPos);
    }

    TX_DECL_OPTIMIZABLE_EXPR

private:

   txOwningArray<Expr> mExpressions;

}; 





class txNamedAttributeStep : public Expr
{
public:
    txNamedAttributeStep(int32_t aNsID, nsIAtom* aPrefix,
                         nsIAtom* aLocalName);

    TX_DECL_EXPR

private:
    int32_t mNamespace;
    nsCOMPtr<nsIAtom> mPrefix;
    nsCOMPtr<nsIAtom> mLocalName;
};




class txUnionNodeTest : public txNodeTest
{
public:
    nsresult addNodeTest(txNodeTest* aNodeTest)
    {
        return mNodeTests.AppendElement(aNodeTest) ?
            NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    TX_DECL_NODE_TEST

private:
    txOwningArray<txNodeTest> mNodeTests;
};




class txErrorExpr : public Expr
{
public:
#ifdef TX_TO_STRING
    explicit txErrorExpr(const nsAString& aStr)
      : mStr(aStr)
    {
    }
#endif

    TX_DECL_EXPR

#ifdef TX_TO_STRING
private:
    nsString mStr;
#endif
};

#endif


