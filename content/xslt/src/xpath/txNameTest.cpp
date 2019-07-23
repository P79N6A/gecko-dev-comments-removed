





































#include "txExpr.h"
#include "nsIAtom.h"
#include "txAtoms.h"
#include "txXPathTreeWalker.h"
#include "txIXPathContext.h"

txNameTest::txNameTest(nsIAtom* aPrefix, nsIAtom* aLocalName, PRInt32 aNSID,
                       PRUint16 aNodeType)
    :mPrefix(aPrefix), mLocalName(aLocalName), mNamespace(aNSID),
     mNodeType(aNodeType)
{
    if (aPrefix == txXMLAtoms::_empty)
        mPrefix = 0;
    NS_ASSERTION(aLocalName, "txNameTest without a local name?");
    NS_ASSERTION(aNodeType == txXPathNodeType::DOCUMENT_NODE ||
                 aNodeType == txXPathNodeType::ELEMENT_NODE ||
                 aNodeType == txXPathNodeType::ATTRIBUTE_NODE,
                 "Go fix txNameTest::matches");
}

PRBool txNameTest::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    if ((mNodeType == txXPathNodeType::ELEMENT_NODE &&
         !txXPathNodeUtils::isElement(aNode)) ||
        (mNodeType == txXPathNodeType::ATTRIBUTE_NODE &&
         !txXPathNodeUtils::isAttribute(aNode)) ||
        (mNodeType == txXPathNodeType::DOCUMENT_NODE &&
         !txXPathNodeUtils::isRoot(aNode))) {
        return PR_FALSE;
    }

    
    if (mLocalName == txXPathAtoms::_asterix && !mPrefix)
        return MB_TRUE;

    
    if (txXPathNodeUtils::getNamespaceID(aNode) != mNamespace)
        return MB_FALSE;

    
    if (mLocalName == txXPathAtoms::_asterix)
        return MB_TRUE;

    
    return txXPathNodeUtils::localNameEquals(aNode, mLocalName);
}




double txNameTest::getDefaultPriority()
{
    if (mLocalName == txXPathAtoms::_asterix) {
        if (!mPrefix)
            return -0.5;
        return -0.25;
    }
    return 0;
}

txNodeTest::NodeTestType
txNameTest::getType()
{
    return NAME_TEST;
}

PRBool
txNameTest::isSensitiveTo(Expr::ContextSensitivity aContext)
{
    return !!(aContext & Expr::NODE_CONTEXT);
}

#ifdef TX_TO_STRING
void
txNameTest::toString(nsAString& aDest)
{
    if (mPrefix) {
        nsAutoString prefix;
        mPrefix->ToString(prefix);
        aDest.Append(prefix);
        aDest.Append(PRUnichar(':'));
    }
    nsAutoString localName;
    mLocalName->ToString(localName);
    aDest.Append(localName);
}
#endif
