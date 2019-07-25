






































#include "txNodeSetAdaptor.h"
#include "txXPathTreeWalker.h"

txNodeSetAdaptor::txNodeSetAdaptor()
    : txXPathObjectAdaptor(),
      mWritable(true)
{
}

txNodeSetAdaptor::txNodeSetAdaptor(txNodeSet *aNodeSet)
    : txXPathObjectAdaptor(aNodeSet),
      mWritable(false)
{
}

NS_IMPL_ISUPPORTS_INHERITED1(txNodeSetAdaptor, txXPathObjectAdaptor, txINodeSet)

nsresult
txNodeSetAdaptor::Init()
{
    if (!mValue) {
        mValue = new txNodeSet(nsnull);
    }

    return mValue ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
txNodeSetAdaptor::Item(PRUint32 aIndex, nsIDOMNode **aResult)
{
    *aResult = nsnull;

    if (aIndex > (PRUint32)NodeSet()->size()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    return txXPathNativeNode::getNode(NodeSet()->get(aIndex), aResult);
}

NS_IMETHODIMP
txNodeSetAdaptor::ItemAsNumber(PRUint32 aIndex, double *aResult)
{
    if (aIndex > (PRUint32)NodeSet()->size()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    nsAutoString result;
    txXPathNodeUtils::appendNodeValue(NodeSet()->get(aIndex), result);

    *aResult = txDouble::toDouble(result);

    return NS_OK;
}

NS_IMETHODIMP
txNodeSetAdaptor::ItemAsString(PRUint32 aIndex, nsAString &aResult)
{
    if (aIndex > (PRUint32)NodeSet()->size()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    txXPathNodeUtils::appendNodeValue(NodeSet()->get(aIndex), aResult);

    return NS_OK;
}

NS_IMETHODIMP
txNodeSetAdaptor::GetLength(PRUint32 *aLength)
{
    *aLength = (PRUint32)NodeSet()->size();

    return NS_OK;
}

NS_IMETHODIMP
txNodeSetAdaptor::Add(nsIDOMNode *aNode)
{
    NS_ENSURE_TRUE(mWritable, NS_ERROR_FAILURE);

    nsAutoPtr<txXPathNode> node(txXPathNativeNode::createXPathNode(aNode,
                                                                   true));

    return node ? NodeSet()->add(*node) : NS_ERROR_OUT_OF_MEMORY;
}
