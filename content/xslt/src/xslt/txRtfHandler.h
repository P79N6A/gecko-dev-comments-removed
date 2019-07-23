






































#ifndef txRtfHandler_h___
#define txRtfHandler_h___

#include "txBufferingHandler.h"
#include "txExprResult.h"
#include "txXPathNode.h"

class txResultTreeFragment : public txAExprResult
{
public:
    txResultTreeFragment(nsAutoPtr<txResultBuffer>& aBuffer);
    ~txResultTreeFragment();

    TX_DECL_EXPRRESULT

    nsresult flushToHandler(txAXMLEventHandler** aHandler);

    void setNode(const txXPathNode* aNode)
    {
        NS_ASSERTION(!mNode, "Already converted!");

        mNode = aNode;
    }
    const txXPathNode *getNode() const
    {
        return mNode;
    }

private:
    nsAutoPtr<txResultBuffer> mBuffer;
    nsAutoPtr<const txXPathNode> mNode;
};

class txRtfHandler : public txBufferingHandler
{
public:
    txRtfHandler();
    virtual ~txRtfHandler();

    nsresult getAsRTF(txAExprResult** aResult);

    nsresult endDocument(nsresult aResult);
    nsresult startDocument();
};

#endif 
