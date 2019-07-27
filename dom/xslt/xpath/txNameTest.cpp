




#include "txExpr.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "txXPathTreeWalker.h"
#include "txIXPathContext.h"

txNameTest::txNameTest(nsIAtom* aPrefix, nsIAtom* aLocalName, int32_t aNSID,
                       uint16_t aNodeType)
    :mPrefix(aPrefix), mLocalName(aLocalName), mNamespace(aNSID),
     mNodeType(aNodeType)
{
    if (aPrefix == nsGkAtoms::_empty)
        mPrefix = 0;
    NS_ASSERTION(aLocalName, "txNameTest without a local name?");
    NS_ASSERTION(aNodeType == txXPathNodeType::DOCUMENT_NODE ||
                 aNodeType == txXPathNodeType::ELEMENT_NODE ||
                 aNodeType == txXPathNodeType::ATTRIBUTE_NODE,
                 "Go fix txNameTest::matches");
}

bool txNameTest::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    if ((mNodeType == txXPathNodeType::ELEMENT_NODE &&
         !txXPathNodeUtils::isElement(aNode)) ||
        (mNodeType == txXPathNodeType::ATTRIBUTE_NODE &&
         !txXPathNodeUtils::isAttribute(aNode)) ||
        (mNodeType == txXPathNodeType::DOCUMENT_NODE &&
         !txXPathNodeUtils::isRoot(aNode))) {
        return false;
    }

    
    if (mLocalName == nsGkAtoms::_asterisk && !mPrefix)
        return true;

    
    if (mNamespace != txXPathNodeUtils::getNamespaceID(aNode) 
        && !(mNamespace == kNameSpaceID_None &&
             txXPathNodeUtils::isHTMLElementInHTMLDocument(aNode))
       )
        return false;

    
    if (mLocalName == nsGkAtoms::_asterisk)
        return true;

    
    return txXPathNodeUtils::localNameEquals(aNode, mLocalName);
}




double txNameTest::getDefaultPriority()
{
    if (mLocalName == nsGkAtoms::_asterisk) {
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

bool
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
        aDest.Append(char16_t(':'));
    }
    nsAutoString localName;
    mLocalName->ToString(localName);
    aDest.Append(localName);
}
#endif
