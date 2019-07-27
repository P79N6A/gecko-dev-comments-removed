




#include "mozilla/Move.h"
#include "XPathExpression.h"
#include "txExpr.h"
#include "txExprResult.h"
#include "nsError.h"
#include "nsIDOMCharacterData.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMDocument.h"
#include "XPathResult.h"
#include "txURIUtils.h"
#include "txXPathTreeWalker.h"

using mozilla::Move;

DOMCI_DATA(XPathExpression, mozilla::dom::XPathExpression)
 
namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION(XPathExpression, mDocument)

NS_IMPL_CYCLE_COLLECTING_ADDREF(XPathExpression)
NS_IMPL_CYCLE_COLLECTING_RELEASE(XPathExpression)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(XPathExpression)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathExpression)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSXPathExpression)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXPathExpression)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XPathExpression)
NS_INTERFACE_MAP_END

XPathExpression::XPathExpression(nsAutoPtr<Expr>&& aExpression,
                                 txResultRecycler* aRecycler,
                                 nsIDOMDocument *aDocument)
    : mExpression(Move(aExpression)),
      mRecycler(aRecycler),
      mDocument(aDocument)
{
}

NS_IMETHODIMP
XPathExpression::Evaluate(nsIDOMNode *aContextNode,
                          uint16_t aType,
                          nsISupports *aInResult,
                          nsISupports **aResult)
{
    return EvaluateWithContext(aContextNode, 1, 1, aType, aInResult, aResult);
}

NS_IMETHODIMP
XPathExpression::EvaluateWithContext(nsIDOMNode *aContextNode,
                                     uint32_t aContextPosition,
                                     uint32_t aContextSize,
                                     uint16_t aType,
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

    uint16_t nodeType = context->NodeType();

    if (nodeType == nsIDOMNode::TEXT_NODE ||
        nodeType == nsIDOMNode::CDATA_SECTION_NODE) {
        nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(aContextNode);
        NS_ENSURE_TRUE(textNode, NS_ERROR_FAILURE);

        if (textNode) {
            uint32_t textLength;
            textNode->GetLength(&textLength);
            if (textLength == 0)
                return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
        }

        
        
    }
    else if (nodeType != nsIDOMNode::DOCUMENT_NODE &&
             nodeType != nsIDOMNode::ELEMENT_NODE &&
             nodeType != nsIDOMNode::ATTRIBUTE_NODE &&
             nodeType != nsIDOMNode::COMMENT_NODE &&
             nodeType != nsIDOMNode::PROCESSING_INSTRUCTION_NODE) {
        return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }

    NS_ENSURE_ARG(aResult);
    *aResult = nullptr;

    nsAutoPtr<txXPathNode> contextNode(txXPathNativeNode::createXPathNode(aContextNode));
    if (!contextNode) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    EvalContextImpl eContext(*contextNode, aContextPosition, aContextSize,
                             mRecycler);
    nsRefPtr<txAExprResult> exprResult;
    nsresult rv = mExpression->evaluate(&eContext, getter_AddRefs(exprResult));
    NS_ENSURE_SUCCESS(rv, rv);

    uint16_t resultType = aType;
    if (aType == XPathResult::ANY_TYPE) {
        short exprResultType = exprResult->getResultType();
        switch (exprResultType) {
            case txAExprResult::NUMBER:
                resultType = XPathResult::NUMBER_TYPE;
                break;
            case txAExprResult::STRING:
                resultType = XPathResult::STRING_TYPE;
                break;
            case txAExprResult::BOOLEAN:
                resultType = XPathResult::BOOLEAN_TYPE;
                break;
            case txAExprResult::NODESET:
                resultType = XPathResult::UNORDERED_NODE_ITERATOR_TYPE;
                break;
            case txAExprResult::RESULT_TREE_FRAGMENT:
                NS_ERROR("Can't return a tree fragment!");
                return NS_ERROR_FAILURE;
        }
    }

    
    nsCOMPtr<nsIXPathResult> xpathResult = do_QueryInterface(aInResult);
    if (!xpathResult) {
        
        xpathResult = new XPathResult(context);
    }
    rv = xpathResult->SetExprResult(exprResult, resultType, context);
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(xpathResult, aResult);
}






nsresult
XPathExpression::EvalContextImpl::getVariable(int32_t aNamespace,
                                              nsIAtom* aLName,
                                              txAExprResult*& aResult)
{
    aResult = 0;
    return NS_ERROR_INVALID_ARG;
}

bool
XPathExpression::EvalContextImpl::isStripSpaceAllowed(const txXPathNode& aNode)
{
    return false;
}

void*
XPathExpression::EvalContextImpl::getPrivateContext()
{
    
    return nullptr;
}

txResultRecycler*
XPathExpression::EvalContextImpl::recycler()
{
    return mRecycler;
}

void
XPathExpression::EvalContextImpl::receiveError(const nsAString& aMsg,
                                               nsresult aRes)
{
    mLastError = aRes;
    
}

const txXPathNode&
XPathExpression::EvalContextImpl::getContextNode()
{
    return mContextNode;
}

uint32_t
XPathExpression::EvalContextImpl::size()
{
    return mContextSize;
}

uint32_t
XPathExpression::EvalContextImpl::position()
{
    return mContextPosition;
}

} 
} 
