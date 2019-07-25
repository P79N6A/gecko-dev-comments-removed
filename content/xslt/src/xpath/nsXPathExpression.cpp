





































#include "nsXPathExpression.h"
#include "txExpr.h"
#include "txExprResult.h"
#include "nsDOMError.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXPathNamespace.h"
#include "nsXPathResult.h"
#include "nsDOMError.h"
#include "txURIUtils.h"
#include "txXPathTreeWalker.h"

NS_IMPL_CYCLE_COLLECTION_1(nsXPathExpression, mDocument)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXPathExpression)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXPathExpression)

DOMCI_DATA(XPathExpression, nsXPathExpression)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXPathExpression)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathExpression)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSXPathExpression)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXPathExpression)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XPathExpression)
NS_INTERFACE_MAP_END

nsXPathExpression::nsXPathExpression(nsAutoPtr<Expr>& aExpression,
                                     txResultRecycler* aRecycler,
                                     nsIDOMDocument *aDocument)
    : mExpression(aExpression),
      mRecycler(aRecycler),
      mDocument(aDocument)
{
}

NS_IMETHODIMP
nsXPathExpression::Evaluate(nsIDOMNode *aContextNode,
                            PRUint16 aType,
                            nsISupports *aInResult,
                            nsISupports **aResult)
{
    return EvaluateWithContext(aContextNode, 1, 1, aType, aInResult, aResult);
}

NS_IMETHODIMP
nsXPathExpression::EvaluateWithContext(nsIDOMNode *aContextNode,
                                       PRUint32 aContextPosition,
                                       PRUint32 aContextSize,
                                       PRUint16 aType,
                                       nsISupports *aInResult,
                                       nsISupports **aResult)
{
    nsCOMPtr<nsINode> context = do_QueryInterface(aContextNode);
    NS_ENSURE_ARG(context);

    if (aContextPosition > aContextSize)
        return NS_ERROR_FAILURE;

    if (!nsContentUtils::CanCallerAccess(aContextNode))
        return NS_ERROR_DOM_SECURITY_ERR;

    if (mDocument && mDocument != aContextNode) {
        nsCOMPtr<nsIDOMDocument> contextDocument;
        aContextNode->GetOwnerDocument(getter_AddRefs(contextDocument));

        if (mDocument != contextDocument) {
            return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;
        }
    }

    nsresult rv;
    PRUint16 nodeType;
    rv = aContextNode->GetNodeType(&nodeType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (nodeType == nsIDOMNode::TEXT_NODE ||
        nodeType == nsIDOMNode::CDATA_SECTION_NODE) {
        nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(aContextNode);
        NS_ENSURE_TRUE(textNode, NS_ERROR_FAILURE);

        if (textNode) {
            PRUint32 textLength;
            textNode->GetLength(&textLength);
            if (textLength == 0)
                return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
        }

        
        
    }
    else if (nodeType != nsIDOMNode::DOCUMENT_NODE &&
             nodeType != nsIDOMNode::ELEMENT_NODE &&
             nodeType != nsIDOMNode::ATTRIBUTE_NODE &&
             nodeType != nsIDOMNode::COMMENT_NODE &&
             nodeType != nsIDOMNode::PROCESSING_INSTRUCTION_NODE &&
             nodeType != nsIDOMXPathNamespace::XPATH_NAMESPACE_NODE) {
        return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }

    NS_ENSURE_ARG(aResult);
    *aResult = nsnull;

    nsAutoPtr<txXPathNode> contextNode(txXPathNativeNode::createXPathNode(aContextNode));
    if (!contextNode) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    EvalContextImpl eContext(*contextNode, aContextPosition, aContextSize,
                             mRecycler);
    nsRefPtr<txAExprResult> exprResult;
    rv = mExpression->evaluate(&eContext, getter_AddRefs(exprResult));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint16 resultType = aType;
    if (aType == nsIDOMXPathResult::ANY_TYPE) {
        short exprResultType = exprResult->getResultType();
        switch (exprResultType) {
            case txAExprResult::NUMBER:
                resultType = nsIDOMXPathResult::NUMBER_TYPE;
                break;
            case txAExprResult::STRING:
                resultType = nsIDOMXPathResult::STRING_TYPE;
                break;
            case txAExprResult::BOOLEAN:
                resultType = nsIDOMXPathResult::BOOLEAN_TYPE;
                break;
            case txAExprResult::NODESET:
                resultType = nsIDOMXPathResult::UNORDERED_NODE_ITERATOR_TYPE;
                break;
            case txAExprResult::RESULT_TREE_FRAGMENT:
                NS_ERROR("Can't return a tree fragment!");
                return NS_ERROR_FAILURE;
        }
    }

    
    nsCOMPtr<nsIXPathResult> xpathResult = do_QueryInterface(aInResult);
    if (!xpathResult) {
        
        xpathResult = new nsXPathResult();
        NS_ENSURE_TRUE(xpathResult, NS_ERROR_OUT_OF_MEMORY);
    }
    rv = xpathResult->SetExprResult(exprResult, resultType, context);
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(xpathResult, aResult);
}






nsresult
nsXPathExpression::EvalContextImpl::getVariable(PRInt32 aNamespace,
                                                nsIAtom* aLName,
                                                txAExprResult*& aResult)
{
    aResult = 0;
    return NS_ERROR_INVALID_ARG;
}

MBool nsXPathExpression::EvalContextImpl::isStripSpaceAllowed(const txXPathNode& aNode)
{
    return MB_FALSE;
}

void* nsXPathExpression::EvalContextImpl::getPrivateContext()
{
    
    return nsnull;
}

txResultRecycler* nsXPathExpression::EvalContextImpl::recycler()
{
    return mRecycler;
}

void nsXPathExpression::EvalContextImpl::receiveError(const nsAString& aMsg,
                                                      nsresult aRes)
{
    mLastError = aRes;
    
}

const txXPathNode& nsXPathExpression::EvalContextImpl::getContextNode()
{
    return mContextNode;
}

PRUint32 nsXPathExpression::EvalContextImpl::size()
{
    return mContextSize;
}

PRUint32 nsXPathExpression::EvalContextImpl::position()
{
    return mContextPosition;
}
