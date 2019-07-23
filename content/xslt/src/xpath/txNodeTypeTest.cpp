





































#include "txExpr.h"
#include "nsIAtom.h"
#include "txIXPathContext.h"
#include "txXPathTreeWalker.h"

PRBool txNodeTypeTest::matches(const txXPathNode& aNode,
                               txIMatchContext* aContext)
{
    switch (mNodeType) {
        case COMMENT_TYPE:
        {
            return txXPathNodeUtils::isComment(aNode);
        }
        case TEXT_TYPE:
        {
            return txXPathNodeUtils::isText(aNode) &&
                   !aContext->isStripSpaceAllowed(aNode);
        }
        case PI_TYPE:
        {
            return txXPathNodeUtils::isProcessingInstruction(aNode) &&
                   (!mNodeName ||
                    txXPathNodeUtils::localNameEquals(aNode, mNodeName));
        }
        case NODE_TYPE:
        {
            return !txXPathNodeUtils::isText(aNode) ||
                   !aContext->isStripSpaceAllowed(aNode);
        }
    }
    return PR_TRUE;
}

txNodeTest::NodeTestType
txNodeTypeTest::getType()
{
    return NODETYPE_TEST;
}




double txNodeTypeTest::getDefaultPriority()
{
    return mNodeName ? 0 : -0.5;
}

PRBool
txNodeTypeTest::isSensitiveTo(Expr::ContextSensitivity aContext)
{
    return !!(aContext & Expr::NODE_CONTEXT);
}

#ifdef TX_TO_STRING
void
txNodeTypeTest::toString(nsAString& aDest)
{
    switch (mNodeType) {
        case COMMENT_TYPE:
            aDest.Append(NS_LITERAL_STRING("comment()"));
            break;
        case TEXT_TYPE:
            aDest.Append(NS_LITERAL_STRING("text()"));
            break;
        case PI_TYPE:
            aDest.AppendLiteral("processing-instruction(");
            if (mNodeName) {
                nsAutoString str;
                mNodeName->ToString(str);
                aDest.Append(PRUnichar('\''));
                aDest.Append(str);
                aDest.Append(PRUnichar('\''));
            }
            aDest.Append(PRUnichar(')'));
            break;
        case NODE_TYPE:
            aDest.Append(NS_LITERAL_STRING("node()"));
            break;
    }
}
#endif
