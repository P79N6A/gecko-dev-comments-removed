








































#include "nsTextNode.h"
#include "nsIDOM3Text.h"
#include "nsContentUtils.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMutationEvent.h"
#include "nsIAttribute.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"

using namespace mozilla::dom;




class nsAttributeTextNode : public nsTextNode,
                            public nsStubMutationObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  
  nsAttributeTextNode(already_AddRefed<nsINodeInfo> aNodeInfo,
                      PRInt32 aNameSpaceID,
                      nsIAtom* aAttrName) :
    nsTextNode(aNodeInfo),
    mGrandparent(nsnull),
    mNameSpaceID(aNameSpaceID),
    mAttrName(aAttrName)
  {
    NS_ASSERTION(mNameSpaceID != kNameSpaceID_Unknown, "Must know namespace");
    NS_ASSERTION(mAttrName, "Must have attr name");
  }

  virtual ~nsAttributeTextNode() {
    NS_ASSERTION(!mGrandparent, "We were not unbound!");
  }

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

  virtual nsGenericDOMDataNode *CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const
  {
    nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
    nsAttributeTextNode *it = new nsAttributeTextNode(ni.forget(),
                                                      mNameSpaceID,
                                                      mAttrName);
    if (it && aCloneText) {
      it->mText = mText;
    }

    return it;
  }

  
  void UpdateText() {
    UpdateText(PR_TRUE);
  }

private:
  
  void UpdateText(PRBool aNotify);

  
  
  
  
  nsIContent* mGrandparent;
  
  PRInt32 mNameSpaceID;
  nsCOMPtr<nsIAtom> mAttrName;
};

nsresult
NS_NewTextNode(nsIContent** aInstancePtrResult,
               nsNodeInfoManager *aNodeInfoManager)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeInfoManager");

  *aInstancePtrResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfoManager->GetTextNodeInfo();
  if (!ni) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTextNode *instance = new nsTextNode(ni.forget());
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}

nsTextNode::nsTextNode(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericTextNode(aNodeInfo)
{
}

nsTextNode::~nsTextNode()
{
}

NS_IMPL_ADDREF_INHERITED(nsTextNode, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsTextNode, nsGenericDOMDataNode)

DOMCI_NODE_DATA(Text, nsTextNode)


NS_INTERFACE_TABLE_HEAD(nsTextNode)
  NS_NODE_INTERFACE_TABLE3(nsTextNode, nsIDOMNode, nsIDOMText,
                           nsIDOMCharacterData)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOM3Text, new nsText3Tearoff(this))
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsTextNode)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Text)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)

NS_IMETHODIMP
nsTextNode::GetNodeName(nsAString& aNodeName)
{
  aNodeName.AssignLiteral("#text");
  return NS_OK;
}

NS_IMETHODIMP
nsTextNode::GetNodeValue(nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::GetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsTextNode::SetNodeValue(const nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::SetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsTextNode::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::TEXT_NODE;
  return NS_OK;
}

PRBool
nsTextNode::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eTEXT | eDATA_NODE));
}

nsGenericDOMDataNode*
nsTextNode::CloneDataNode(nsINodeInfo *aNodeInfo, PRBool aCloneText) const
{
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsTextNode *it = new nsTextNode(ni.forget());
  if (it && aCloneText) {
    it->mText = mText;
  }

  return it;
}

nsresult
nsTextNode::BindToAttribute(nsIAttribute* aAttr)
{
  NS_ASSERTION(!IsInDoc(), "Unbind before binding!");
  NS_ASSERTION(!GetNodeParent(), "Unbind before binding!");
  NS_ASSERTION(HasSameOwnerDoc(aAttr), "Wrong owner document!");

  mParent = aAttr;
  SetParentIsContent(false);
  ClearInDocument();
  return NS_OK;
}

nsresult
nsTextNode::UnbindFromAttribute()
{
  NS_ASSERTION(GetNodeParent(), "Bind before unbinding!");
  NS_ASSERTION(GetNodeParent() &&
               GetNodeParent()->IsNodeOfType(nsINode::eATTRIBUTE),
               "Use this method only to unbind from an attribute!");
  mParent = nsnull;
  return NS_OK;
}

#ifdef DEBUG
void
nsTextNode::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "Text@%p", static_cast<const void*>(this));
  fprintf(out, " intrinsicstate=[%llx]", IntrinsicState().GetInternalValue());
  fprintf(out, " flags=[%08x]", static_cast<unsigned int>(GetFlags()));
  fprintf(out, " primaryframe=%p", static_cast<void*>(GetPrimaryFrame()));
  fprintf(out, " refcount=%d<", mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);

  fputs(">\n", out);
}

void
nsTextNode::DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const
{
  if(aDumpAll) {
    PRInt32 index;
    for (index = aIndent; --index >= 0; ) fputs("  ", out);

    nsAutoString tmp;
    ToCString(tmp, 0, mText.GetLength());

    if(!tmp.EqualsLiteral("\\n")) {
      fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
      if(aIndent) fputs("\n", out);
    }
  }
}
#endif

nsresult
NS_NewAttributeContent(nsNodeInfoManager *aNodeInfoManager,
                       PRInt32 aNameSpaceID, nsIAtom* aAttrName,
                       nsIContent** aResult)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeInfoManager");
  NS_PRECONDITION(aAttrName, "Must have an attr name");
  NS_PRECONDITION(aNameSpaceID != kNameSpaceID_Unknown, "Must know namespace");
  
  *aResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfoManager->GetTextNodeInfo();
  if (!ni) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsAttributeTextNode* textNode = new nsAttributeTextNode(ni.forget(),
                                                          aNameSpaceID,
                                                          aAttrName);
  if (!textNode) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult = textNode);

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED1(nsAttributeTextNode, nsTextNode,
                             nsIMutationObserver)

nsresult
nsAttributeTextNode::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers)
{
  NS_PRECONDITION(aParent && aParent->GetParent(),
                  "This node can't be a child of the document or of the document root");

  nsresult rv = nsTextNode::BindToTree(aDocument, aParent,
                                       aBindingParent, aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!mGrandparent, "We were already bound!");
  mGrandparent = aParent->GetParent();
  mGrandparent->AddMutationObserver(this);

  
  
  UpdateText(PR_FALSE);

  return NS_OK;
}

void
nsAttributeTextNode::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  
  if (mGrandparent) {
    
    
    
    mGrandparent->RemoveMutationObserver(this);
    mGrandparent = nsnull;
  }
  nsTextNode::UnbindFromTree(aDeep, aNullParent);
}

void
nsAttributeTextNode::AttributeChanged(nsIDocument* aDocument,
                                      Element* aElement,
                                      PRInt32 aNameSpaceID,
                                      nsIAtom* aAttribute,
                                      PRInt32 aModType)
{
  if (aNameSpaceID == mNameSpaceID && aAttribute == mAttrName &&
      aElement == mGrandparent) {
    
    
    
    void (nsAttributeTextNode::*update)() = &nsAttributeTextNode::UpdateText;
    nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(this, update);
    nsContentUtils::AddScriptRunner(ev);
  }
}

void
nsAttributeTextNode::NodeWillBeDestroyed(const nsINode* aNode)
{
  NS_ASSERTION(aNode == static_cast<nsINode*>(mGrandparent), "Wrong node!");
  mGrandparent = nsnull;
}

void
nsAttributeTextNode::UpdateText(PRBool aNotify)
{
  if (mGrandparent) {
    nsAutoString attrValue;
    mGrandparent->GetAttr(mNameSpaceID, mAttrName, attrValue);
    SetText(attrValue, aNotify);
  }  
}
