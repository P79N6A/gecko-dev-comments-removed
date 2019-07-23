





































#include "nsXPathResult.h"
#include "txExprResult.h"
#include "txNodeSet.h"
#include "nsDOMError.h"
#include "nsIContent.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsDOMString.h"
#include "txXPathTreeWalker.h"

nsXPathResult::nsXPathResult() : mDocument(nsnull),
                                 mCurrentPos(0),
                                 mResultType(ANY_TYPE),
                                 mInvalidIteratorState(PR_TRUE)
{
}

nsXPathResult::~nsXPathResult()
{
    if (mDocument) {
        mDocument->RemoveMutationObserver(this);
    }
}

NS_IMPL_ADDREF(nsXPathResult)
NS_IMPL_RELEASE(nsXPathResult)
NS_INTERFACE_MAP_BEGIN(nsXPathResult)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathResult)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsIXPathResult)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXPathResult)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XPathResult)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsXPathResult::GetResultType(PRUint16 *aResultType)
{
    *aResultType = mResultType;

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::GetNumberValue(double *aNumberValue)
{
    if (mResultType != NUMBER_TYPE) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    *aNumberValue = mResult.get()->numberValue();

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::GetStringValue(nsAString &aStringValue)
{
    if (mResultType != STRING_TYPE) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    nsAutoString stringValue;
    mResult.get()->stringValue(stringValue);

    aStringValue.Assign(stringValue);

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::GetBooleanValue(PRBool *aBooleanValue)
{
    if (mResultType != BOOLEAN_TYPE) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    *aBooleanValue = mResult.get()->booleanValue();

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::GetSingleNodeValue(nsIDOMNode **aSingleNodeValue)
{
    if (!isNode()) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    txNodeSet *nodeSet = NS_STATIC_CAST(txNodeSet*, mResult.get());
    if (nodeSet->size() > 0) {
        return txXPathNativeNode::getNode(nodeSet->get(0), aSingleNodeValue);
    }

    *aSingleNodeValue = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::GetInvalidIteratorState(PRBool *aInvalidIteratorState)
{
    *aInvalidIteratorState = isIterator() && mInvalidIteratorState;

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::GetSnapshotLength(PRUint32 *aSnapshotLength)
{
    if (!isSnapshot()) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    txNodeSet *nodeSet = NS_STATIC_CAST(txNodeSet*, mResult.get());
    *aSnapshotLength = (PRUint32)nodeSet->size();

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::IterateNext(nsIDOMNode **aResult)
{
    if (!isIterator()) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    if (mDocument) {
        mDocument->FlushPendingNotifications(Flush_Content);
    }

    if (mInvalidIteratorState) {
        return NS_ERROR_DOM_INVALID_STATE_ERR;
    }

    txNodeSet *nodeSet = NS_STATIC_CAST(txNodeSet*, mResult.get());
    if (mCurrentPos < (PRUint32)nodeSet->size()) {
        return txXPathNativeNode::getNode(nodeSet->get(mCurrentPos++),
                                          aResult);
    }

    *aResult = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsXPathResult::SnapshotItem(PRUint32 aIndex, nsIDOMNode **aResult)
{
    if (!isSnapshot()) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    txNodeSet *nodeSet = NS_STATIC_CAST(txNodeSet*, mResult.get());
    if (aIndex < (PRUint32)nodeSet->size()) {
        return txXPathNativeNode::getNode(nodeSet->get(aIndex), aResult);
    }

    *aResult = nsnull;

    return NS_OK;
}

void
nsXPathResult::NodeWillBeDestroyed(const nsINode* aNode)
{
    
    mDocument = nsnull;
    Invalidate();
}

void
nsXPathResult::CharacterDataChanged(nsIDocument* aDocument,
                                    nsIContent *aContent,
                                    CharacterDataChangeInfo* aInfo)
{
    Invalidate();
}

void
nsXPathResult::AttributeChanged(nsIDocument* aDocument,
                                nsIContent* aContent,
                                PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute,
                                PRInt32 aModType)
{
    Invalidate();
}

void
nsXPathResult::ContentAppended(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer)
{
    Invalidate();
}

void
nsXPathResult::ContentInserted(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer)
{
    Invalidate();
}

void
nsXPathResult::ContentRemoved(nsIDocument* aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer)
{
    Invalidate();
}

nsresult
nsXPathResult::SetExprResult(txAExprResult* aExprResult, PRUint16 aResultType)
{
    mResultType = aResultType;

    if ((isSnapshot() || isIterator() || isNode()) &&
        aExprResult->getResultType() != txAExprResult::NODESET) {
        return NS_ERROR_DOM_TYPE_ERR;
    }

    if (mDocument) {
        mDocument->RemoveMutationObserver(this);
        mDocument = nsnull;
    }
 
    mResult.set(aExprResult);

    if (!isIterator()) {
        return NS_OK;
    }

    mInvalidIteratorState = PR_FALSE;

    txNodeSet* nodeSet = NS_STATIC_CAST(txNodeSet*, aExprResult);
    nsCOMPtr<nsIDOMNode> node;
    if (nodeSet->size() > 0) {
        nsresult rv = txXPathNativeNode::getNode(nodeSet->get(0),
                                                 getter_AddRefs(node));
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        nsCOMPtr<nsIDOMDocument> document;
        node->GetOwnerDocument(getter_AddRefs(document));
        if (document) {
            mDocument = do_QueryInterface(document);
        }
        else {
            mDocument = do_QueryInterface(node);
        }

        NS_ASSERTION(mDocument, "We need a document!");
        if (mDocument) {
            mDocument->AddMutationObserver(this);
        }
    }

    return NS_OK;
}

void
nsXPathResult::Invalidate()
{
    if (mDocument) {
        mDocument->RemoveMutationObserver(this);
        mDocument = nsnull;
    }
    mInvalidIteratorState = PR_TRUE;
}

nsresult
nsXPathResult::GetExprResult(txAExprResult** aExprResult)
{
    if (isIterator() && mInvalidIteratorState) {
        return NS_ERROR_DOM_INVALID_STATE_ERR;
    }

    *aExprResult = mResult.get();
    if (!*aExprResult) {
        return NS_ERROR_DOM_INVALID_STATE_ERR;
    }

    NS_ADDREF(*aExprResult);

    return NS_OK;
}

nsresult
nsXPathResult::Clone(nsIXPathResult **aResult)
{
    *aResult = nsnull;

    if (isIterator() && mInvalidIteratorState) {
        return NS_ERROR_DOM_INVALID_STATE_ERR;
    }

    nsCOMPtr<nsIXPathResult> result = new nsXPathResult();
    if (!result) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv = result->SetExprResult(mResult.get(), mResultType);
    NS_ENSURE_SUCCESS(rv, rv);

    result.swap(*aResult);

    return NS_OK;
}

void
txResultHolder::set(txAExprResult *aResult)
{
    releaseNodeSet();

    
    mResult = aResult;

    if (mResult && mResult->getResultType() == txAExprResult::NODESET) {
        txNodeSet *nodeSet =
            NS_STATIC_CAST(txNodeSet*,
                           NS_STATIC_CAST(txAExprResult*, mResult));
        PRInt32 i, count = nodeSet->size();
        for (i = 0; i < count; ++i) {
            txXPathNativeNode::addRef(nodeSet->get(i));
        }
    }
}

void
txResultHolder::releaseNodeSet()
{
    if (mResult && mResult->getResultType() == txAExprResult::NODESET) {
        txNodeSet *nodeSet =
            NS_STATIC_CAST(txNodeSet*,
                           NS_STATIC_CAST(txAExprResult*, mResult));
        PRInt32 i, count = nodeSet->size();
        for (i = 0; i < count; ++i) {
            txXPathNativeNode::release(nodeSet->get(i));
        }
    }
}
