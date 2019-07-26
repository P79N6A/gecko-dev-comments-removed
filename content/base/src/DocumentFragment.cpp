








#include "nsIDOMDocumentFragment.h"
#include "mozilla/dom/FragmentOrElement.h"
#include "nsGenericElement.h" 
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsDOMString.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

class DocumentFragment : public FragmentOrElement,
                         public nsIDOMDocumentFragment
{
public:
  using FragmentOrElement::GetFirstChild;

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  

  DocumentFragment(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~DocumentFragment()
  {
  }

  
  virtual already_AddRefed<nsINodeInfo>
    GetExistingAttrNameFromQName(const nsAString& aStr) const
  {
    return nullptr;
  }

  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify)
  {
    return NS_OK;
  }
  virtual bool GetAttr(int32_t aNameSpaceID, nsIAtom* aName, 
                         nsAString& aResult) const
  {
    return false;
  }
  virtual bool HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const
  {
    return false;
  }
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute, 
                             bool aNotify)
  {
    return NS_OK;
  }
  virtual const nsAttrName* GetAttrNameAt(uint32_t aIndex) const
  {
    return nullptr;
  }
  virtual uint32_t GetAttrCount() const
  {
    return 0;
  }

  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual nsIAtom* DoGetID() const;
  virtual nsIAtom *GetIDAttributeName() const;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
  {
    NS_ASSERTION(false, "Trying to bind a fragment to a tree");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  virtual void UnbindFromTree(bool aDeep, bool aNullParent)
  {
    NS_ASSERTION(false, "Trying to unbind a fragment from a tree");
    return;
  }

  virtual Element* GetNameSpaceElement()
  {
    return nullptr;
  }

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
#endif

protected:
  nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

} 
} 

nsresult
NS_NewDocumentFragment(nsIDOMDocumentFragment** aInstancePtrResult,
                       nsNodeInfoManager *aNodeInfoManager)
{
  using namespace mozilla::dom;

  NS_ENSURE_ARG(aNodeInfoManager);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = aNodeInfoManager->GetNodeInfo(nsGkAtoms::documentFragmentNodeName,
                                           nullptr, kNameSpaceID_None,
                                           nsIDOMNode::DOCUMENT_FRAGMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  DocumentFragment *it = new DocumentFragment(nodeInfo.forget());
  NS_ADDREF(*aInstancePtrResult = it);

  return NS_OK;
}

DOMCI_NODE_DATA(DocumentFragment, mozilla::dom::DocumentFragment)

namespace mozilla {
namespace dom {

DocumentFragment::DocumentFragment(already_AddRefed<nsINodeInfo> aNodeInfo)
  : FragmentOrElement(aNodeInfo)
{
  NS_ABORT_IF_FALSE(mNodeInfo->NodeType() ==
                    nsIDOMNode::DOCUMENT_FRAGMENT_NODE &&
                    mNodeInfo->Equals(nsGkAtoms::documentFragmentNodeName,
                                      kNameSpaceID_None),
                    "Bad NodeType in aNodeInfo");
}

bool
DocumentFragment::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~(eCONTENT | eDOCUMENT_FRAGMENT));
}

nsIAtom*
DocumentFragment::DoGetID() const
{
  return nullptr;  
}

nsIAtom*
DocumentFragment::GetIDAttributeName() const
{
  return nullptr;
}

#ifdef DEBUG
void
DocumentFragment::List(FILE* out, int32_t aIndent) const
{
  int32_t indent;
  for (indent = aIndent; --indent >= 0; ) {
    fputs("  ", out);
  }

  fprintf(out, "DocumentFragment@%p", (void *)this);

  fprintf(out, " flags=[%08x]", static_cast<unsigned int>(GetFlags()));
  fprintf(out, " refcount=%d<", mRefCnt.get());

  nsIContent* child = GetFirstChild();
  if (child) {
    fputs("\n", out);

    for (; child; child = child->GetNextSibling()) {
      child->List(out, aIndent + 1);
    }

    for (indent = aIndent; --indent >= 0; ) {
      fputs("  ", out);
    }
  }

  fputs(">\n", out);
}

void
DocumentFragment::DumpContent(FILE* out, int32_t aIndent,
                              bool aDumpAll) const
{
  int32_t indent;
  for (indent = aIndent; --indent >= 0; ) {
    fputs("  ", out);
  }

  fputs("<DocumentFragment>", out);

  if(aIndent) {
    fputs("\n", out);
  }

  for (nsIContent* child = GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    int32_t indent = aIndent ? aIndent + 1 : 0;
    child->DumpContent(out, indent, aDumpAll);
  }
  for (indent = aIndent; --indent >= 0; ) {
    fputs("  ", out);
  }
  fputs("</DocumentFragment>", out);

  if(aIndent) {
    fputs("\n", out);
  }
}
#endif


NS_INTERFACE_MAP_BEGIN(DocumentFragment)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(DocumentFragment)
  NS_INTERFACE_MAP_ENTRY(nsIContent)
  NS_INTERFACE_MAP_ENTRY(nsINode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentFragment)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsISupportsWeakReference,
                                 new nsNodeSupportsWeakRefTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNodeSelector,
                                 new nsNodeSelectorTearoff(this))
  
  
  
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DocumentFragment)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(DocumentFragment, FragmentOrElement)
NS_IMPL_RELEASE_INHERITED(DocumentFragment, FragmentOrElement)

NS_IMPL_ELEMENT_CLONE(DocumentFragment)

} 
} 
