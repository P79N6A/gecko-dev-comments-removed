
























#include "XMLDOMUtils.h"
#include "dom.h"
#include "nsString.h"

void XMLDOMUtils::getNodeValue(Node* aNode, nsAString& aResult)
{
    if (!aNode)
        return;

    unsigned short nodeType = aNode->getNodeType();

    switch (nodeType) {
        case Node::ATTRIBUTE_NODE:
        case Node::CDATA_SECTION_NODE:
        case Node::COMMENT_NODE:
        case Node::PROCESSING_INSTRUCTION_NODE:
        case Node::TEXT_NODE:
        {
            nsAutoString nodeValue;
            aNode->getNodeValue(nodeValue);
            aResult.Append(nodeValue);
            break;
        }
        case Node::DOCUMENT_NODE:
        {
            getNodeValue(((Document*)aNode)->getDocumentElement(),
                         aResult);
            break;
        }
        case Node::DOCUMENT_FRAGMENT_NODE:
        case Node::ELEMENT_NODE:
        {
            Node* tmpNode = aNode->getFirstChild();
            while (tmpNode) {
                nodeType = tmpNode->getNodeType();
                if ((nodeType == Node::TEXT_NODE) ||
                    (nodeType == Node::CDATA_SECTION_NODE)) {
                    nsAutoString nodeValue;
                    tmpNode->getNodeValue(nodeValue);
                    aResult.Append(nodeValue);
                }
                else if (nodeType == Node::ELEMENT_NODE) {
                    getNodeValue(tmpNode, aResult);
                }
                tmpNode = tmpNode->getNextSibling();
            }
            break;
        }
    }
}
