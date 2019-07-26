








#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/Element.h" 
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsDOMString.h"
#include "nsContentUtils.h"
#include "mozilla/dom/DocumentFragmentBinding.h"

nsresult
NS_NewDocumentFragment(nsIDOMDocumentFragment** aInstancePtrResult,
                       nsNodeInfoManager *aNodeInfoManager)
{
  mozilla::ErrorResult rv;
  *aInstancePtrResult = NS_NewDocumentFragment(aNodeInfoManager, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<mozilla::dom::DocumentFragment>
NS_NewDocumentFragment(nsNodeInfoManager* aNodeInfoManager,
                       mozilla::ErrorResult& aRv)
{
  using namespace mozilla::dom;

  if (!aNodeInfoManager) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return nullptr;
  }

  nsCOMPtr<nsINodeInfo> nodeInfo =
    aNodeInfoManager->GetNodeInfo(nsGkAtoms::documentFragmentNodeName,
                                  nullptr, kNameSpaceID_None,
                                  nsIDOMNode::DOCUMENT_FRAGMENT_NODE);
  if (!nodeInfo) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsRefPtr<DocumentFragment> it = new DocumentFragment(nodeInfo.forget());
  return it.forget();
}

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

  SetIsDOMBinding();
}

JSObject*
DocumentFragment::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return DocumentFragmentBinding::Wrap(aCx, aScope, this);
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
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::EventTarget)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsISupportsWeakReference,
                                 new nsNodeSupportsWeakRefTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNodeSelector,
                                 new nsNodeSelectorTearoff(this))
  
  
  
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(DocumentFragment, FragmentOrElement)
NS_IMPL_RELEASE_INHERITED(DocumentFragment, FragmentOrElement)

NS_IMPL_ELEMENT_CLONE(DocumentFragment)

} 
} 
