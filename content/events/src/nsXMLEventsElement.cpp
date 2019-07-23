





































#include "nsXMLElement.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"

class nsXMLEventsElement : public nsXMLElement {
public:
  nsXMLEventsElement(nsINodeInfo *aNodeInfo);
  virtual ~nsXMLEventsElement();
  NS_FORWARD_NSIDOMNODE(nsXMLElement::)

  virtual nsIAtom *GetIDAttributeName(PRInt32& aNameSpaceID) const;
  virtual PRBool IsPotentialIDAttributeName(PRInt32 aNameSpaceID,
                                            nsIAtom* aAtom) const;
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

nsXMLEventsElement::nsXMLEventsElement(nsINodeInfo *aNodeInfo)
  : nsXMLElement(aNodeInfo)
{
}

nsXMLEventsElement::~nsXMLEventsElement()
{
}

nsIAtom *
nsXMLEventsElement::GetIDAttributeName(PRInt32& aNameSpaceID) const
{
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::id)) {
    aNameSpaceID = kNameSpaceID_None;
    return nsGkAtoms::id;
  }
  return nsGenericElement::GetIDAttributeName(aNameSpaceID);
}

PRBool
nsXMLEventsElement::IsPotentialIDAttributeName(PRInt32 aNameSpaceID,
                                               nsIAtom* aAtom) const
{
  return (aNameSpaceID == kNameSpaceID_None && aAtom == nsGkAtoms::id) ||
    nsGenericElement::IsPotentialIDAttributeName(aNameSpaceID, aAtom);
}

nsresult
nsXMLEventsElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                            const nsAString& aValue, PRBool aNotify)
{
  if (mNodeInfo->Equals(nsGkAtoms::listener) && 
      mNodeInfo->GetDocument() && aNameSpaceID == kNameSpaceID_None && 
      aName == nsGkAtoms::event)
    mNodeInfo->GetDocument()->AddXMLEventsContent(this);
  return nsXMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                   aNotify);
}

NS_IMPL_ELEMENT_CLONE(nsXMLEventsElement)

nsresult
NS_NewXMLEventsElement(nsIContent** aInstancePtrResult, nsINodeInfo *aNodeInfo)
{
  nsXMLEventsElement* it = new nsXMLEventsElement(aNodeInfo);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aInstancePtrResult = it);
  return NS_OK;
}

