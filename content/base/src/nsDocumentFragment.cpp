








































#include "nsISupports.h"
#include "nsIContent.h"
#include "nsIDOMDocumentFragment.h"
#include "nsGenericElement.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMAttr.h"
#include "nsDOMError.h"
#include "nsGkAtoms.h"
#include "nsDOMString.h"
#include "nsIDOMUserDataHandler.h"

class nsDocumentFragment : public nsGenericElement,
                           public nsIDOMDocumentFragment
{
public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericElement::)

  
  

  nsDocumentFragment(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsDocumentFragment()
  {
  }

  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify)
  {
    return NS_OK;
  }
  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                         nsAString& aResult) const
  {
    return PR_FALSE;
  }
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute, 
                             PRBool aNotify)
  {
    return NS_OK;
  }
  virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const
  {
    return nsnull;
  }

  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIAtom* DoGetID() const;
  virtual nsIAtom *GetIDAttributeName() const;

protected:
  nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

nsresult
NS_NewDocumentFragment(nsIDOMDocumentFragment** aInstancePtrResult,
                       nsNodeInfoManager *aNodeInfoManager)
{
  NS_ENSURE_ARG(aNodeInfoManager);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = aNodeInfoManager->GetNodeInfo(nsGkAtoms::documentFragmentNodeName,
                                           nsnull, kNameSpaceID_None,
                                           nsIDOMNode::DOCUMENT_FRAGMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsDocumentFragment *it = new nsDocumentFragment(nodeInfo.forget());
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = it);

  return NS_OK;
}

nsDocumentFragment::nsDocumentFragment(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericElement(aNodeInfo)
{
  ClearIsElement();
}

PRBool
nsDocumentFragment::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eDOCUMENT_FRAGMENT));
}

nsIAtom*
nsDocumentFragment::DoGetID() const
{
  return nsnull;  
}

nsIAtom*
nsDocumentFragment::GetIDAttributeName() const
{
  return nsnull;
}

DOMCI_NODE_DATA(DocumentFragment, nsDocumentFragment)


NS_INTERFACE_TABLE_HEAD(nsDocumentFragment)
  NS_NODE_INTERFACE_TABLE2(nsDocumentFragment, nsIDOMNode,
                           nsIDOMDocumentFragment)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DocumentFragment)
NS_INTERFACE_MAP_END_INHERITING(nsGenericElement)


NS_IMPL_ADDREF_INHERITED(nsDocumentFragment, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsDocumentFragment, nsGenericElement)

NS_IMPL_ELEMENT_CLONE(nsDocumentFragment)
