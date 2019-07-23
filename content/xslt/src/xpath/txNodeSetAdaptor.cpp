






































#include "txNodeSetAdaptor.h"
#include "txXPathTreeWalker.h"

txNodeSetAdaptor::txNodeSetAdaptor()
    : mWritable(PR_TRUE)
{
}

txNodeSetAdaptor::txNodeSetAdaptor(txNodeSet *aNodeSet)
    : mNodeSet(aNodeSet),
      mWritable(PR_FALSE)
{
    NS_ASSERTION(aNodeSet,
                 "Don't create an adaptor if you don't have a txNodeSet");
}

NS_IMPL_ISUPPORTS1(txNodeSetAdaptor, txINodeSet)

nsresult
txNodeSetAdaptor::Init()
{
    if (!mNodeSet) {
        mNodeSet = new txNodeSet(nsnull);
    }

    return mNodeSet ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
txNodeSetAdaptor::Item(PRUint32 aIndex, nsIDOMNode **aResult)
{
    *aResult = nsnull;

    if (aIndex > (PRUint32)mNodeSet->size()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    return txXPathNativeNode::getNode(mNodeSet->get(aIndex), aResult);
}

NS_IMETHODIMP
txNodeSetAdaptor::ItemAsNumber(PRUint32 aIndex, double *aResult)
{
    if (aIndex > (PRUint32)mNodeSet->size()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    nsAutoString result;
    txXPathNodeUtils::appendNodeValue(mNodeSet->get(aIndex), result);

    *aResult = Double::toDouble(result);

    return NS_OK;
}

NS_IMETHODIMP
txNodeSetAdaptor::ItemAsString(PRUint32 aIndex, nsAString &aResult)
{
    if (aIndex > (PRUint32)mNodeSet->size()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    txXPathNodeUtils::appendNodeValue(mNodeSet->get(aIndex), aResult);

    return NS_OK;
}

NS_IMETHODIMP
txNodeSetAdaptor::GetLength(PRUint32 *aLength)
{
    *aLength = (PRUint32)mNodeSet->size();

    return NS_OK;
}

NS_IMETHODIMP
txNodeSetAdaptor::Add(nsIDOMNode *aNode)
{
    NS_ENSURE_TRUE(mWritable, NS_ERROR_FAILURE);

    nsAutoPtr<txXPathNode> node(txXPathNativeNode::createXPathNode(aNode,
                                                                   PR_TRUE));

    return node ? mNodeSet->add(*node) : NS_ERROR_OUT_OF_MEMORY;
}

txAExprResult*
txNodeSetAdaptor::GetTxNodeSet()
{
    return mNodeSet;
}
