





































#ifndef __TX_I_XPATH_CONTEXT
#define __TX_I_XPATH_CONTEXT

#include "txCore.h"

class FunctionCall;
class nsIAtom;
class txAExprResult;
class txResultRecycler;
class txXPathNode;










class txIParseContext
{
public:
    virtual ~txIParseContext()
    {
    }

    


    virtual nsresult resolveNamespacePrefix(nsIAtom* aPrefix, PRInt32& aID) = 0;

    



    virtual nsresult resolveFunctionCall(nsIAtom* aName, PRInt32 aID,
                                         FunctionCall** aFunction) = 0;

    


    virtual PRBool caseInsensitiveNameTests() = 0;

    


    virtual void SetErrorOffset(PRUint32 aOffset) = 0;
};










class txIMatchContext
{
public:
    virtual ~txIMatchContext()
    {
    }

    



    virtual nsresult getVariable(PRInt32 aNamespace, nsIAtom* aLName,
                                 txAExprResult*& aResult) = 0;

    



    virtual MBool isStripSpaceAllowed(const txXPathNode& aNode) = 0;

    


    virtual void* getPrivateContext() = 0;

    virtual txResultRecycler* recycler() = 0;

    


    virtual void receiveError(const nsAString& aMsg, nsresult aRes) = 0;
};

#define TX_DECL_MATCH_CONTEXT \
    nsresult getVariable(PRInt32 aNamespace, nsIAtom* aLName, \
                         txAExprResult*& aResult); \
    MBool isStripSpaceAllowed(const txXPathNode& aNode); \
    void* getPrivateContext(); \
    txResultRecycler* recycler(); \
    void receiveError(const nsAString& aMsg, nsresult aRes)

class txIEvalContext : public txIMatchContext
{
public:
    virtual ~txIEvalContext()
    {
    }

    


    virtual const txXPathNode& getContextNode() = 0;

    


    virtual PRUint32 size() = 0;

    



    virtual PRUint32 position() = 0;
};

#define TX_DECL_EVAL_CONTEXT \
    TX_DECL_MATCH_CONTEXT; \
    const txXPathNode& getContextNode(); \
    PRUint32 size(); \
    PRUint32 position()

#endif 
