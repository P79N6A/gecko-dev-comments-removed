

















































#include "txDOM.h"
#include "txAtoms.h"





Document::Document() :
  NodeDefinition(Node::DOCUMENT_NODE, txXMLAtoms::document, EmptyString(),
                 nsnull),
  documentElement(nsnull)
{
  mIDMap.Init(0);
}




Element* Document::getDocumentElement()
{
  return documentElement;
}

Element*
Document::createElementNS(nsIAtom *aPrefix, nsIAtom *aLocalName,
                          PRInt32 aNamespaceID)
{
  return new Element(aPrefix, aLocalName, aNamespaceID, this);
}




Node* Document::createTextNode(const nsAString& theData)
{
  return new NodeDefinition(Node::TEXT_NODE, txXMLAtoms::text, theData, this);
}




Node* Document::createComment(const nsAString& theData)
{
  return new NodeDefinition(Node::COMMENT_NODE, txXMLAtoms::comment, theData,
                            this);
}




ProcessingInstruction*
  Document::createProcessingInstruction(nsIAtom *aTarget,
                                        const nsAString& data)
{
  return new ProcessingInstruction(aTarget, data, this);
}




DHASH_WRAPPER(txIDMap, txIDEntry, nsAString&)

Element* Document::getElementById(const nsAString& aID)
{
  txIDEntry* entry = mIDMap.GetEntry(aID);
  if (entry)
    return entry->mElement;
  return nsnull;
}




PRBool
Document::setElementID(const nsAString& aID, Element* aElement)
{
  txIDEntry* id = mIDMap.AddEntry(aID);
  
  if (id->mElement) {
    return PR_FALSE;
  }
  id->mElement = aElement;
  id->mElement->setIDValue(aID);
  return PR_TRUE;
}

Node* Document::appendChild(Node* newChild)
{
  unsigned short nodeType = newChild->getNodeType();

  
  NodeDefinition* pNewChild = (NodeDefinition*)newChild;

  if (pNewChild->parentNode == this)
    {
      pNewChild = implRemoveChild(pNewChild);
      if (nodeType == Node::ELEMENT_NODE)
        documentElement = nsnull;
    }

  switch (nodeType)
    {
      case Node::PROCESSING_INSTRUCTION_NODE :
      case Node::COMMENT_NODE :
      case Node::DOCUMENT_TYPE_NODE :
        return implAppendChild(pNewChild);

      case Node::ELEMENT_NODE :
        if (!documentElement)
          {
            Node* returnVal = implAppendChild(pNewChild);
            documentElement = (Element*)pNewChild;
            return returnVal;
          }

      default:
        break;
    }

  return nsnull;
}

nsresult Document::getBaseURI(nsAString& aURI)
{
  aURI = documentBaseURI;
  return NS_OK;
}
