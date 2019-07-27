




#ifndef txRtfHandler_h___
#define txRtfHandler_h___

#include "mozilla/Attributes.h"
#include "txBufferingHandler.h"
#include "txExprResult.h"
#include "txXPathNode.h"

class txResultTreeFragment : public txAExprResult
{
public:
    explicit txResultTreeFragment(nsAutoPtr<txResultBuffer>&& aBuffer);

    TX_DECL_EXPRRESULT

    nsresult flushToHandler(txAXMLEventHandler* aHandler);

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
    nsresult getAsRTF(txAExprResult** aResult);

    nsresult endDocument(nsresult aResult) MOZ_OVERRIDE;
    nsresult startDocument() MOZ_OVERRIDE;
};

#endif 
