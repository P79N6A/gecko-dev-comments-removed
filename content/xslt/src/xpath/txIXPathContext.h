




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

    


    virtual nsresult resolveNamespacePrefix(nsIAtom* aPrefix, int32_t& aID) = 0;

    



    virtual nsresult resolveFunctionCall(nsIAtom* aName, int32_t aID,
                                         FunctionCall** aFunction) = 0;

    


    virtual bool caseInsensitiveNameTests() = 0;

    


    virtual void SetErrorOffset(uint32_t aOffset) = 0;
};










class txIMatchContext
{
public:
    virtual ~txIMatchContext()
    {
    }

    



    virtual nsresult getVariable(int32_t aNamespace, nsIAtom* aLName,
                                 txAExprResult*& aResult) = 0;

    



    virtual bool isStripSpaceAllowed(const txXPathNode& aNode) = 0;

    


    virtual void* getPrivateContext() = 0;

    virtual txResultRecycler* recycler() = 0;

    


    virtual void receiveError(const nsAString& aMsg, nsresult aRes) = 0;
};

#define TX_DECL_MATCH_CONTEXT \
    nsresult getVariable(int32_t aNamespace, nsIAtom* aLName, \
                         txAExprResult*& aResult); \
    bool isStripSpaceAllowed(const txXPathNode& aNode); \
    void* getPrivateContext(); \
    txResultRecycler* recycler(); \
    void receiveError(const nsAString& aMsg, nsresult aRes)

class txIEvalContext : public txIMatchContext
{
public:
    


    virtual const txXPathNode& getContextNode() = 0;

    


    virtual uint32_t size() = 0;

    



    virtual uint32_t position() = 0;
};

#define TX_DECL_EVAL_CONTEXT \
    TX_DECL_MATCH_CONTEXT; \
    const txXPathNode& getContextNode(); \
    uint32_t size(); \
    uint32_t position()

#endif 
