









































#include "nsGenericDOMDataNode.h"
#include "nsGenericElement.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMDocument.h"
#include "nsReadableUtils.h"
#include "nsMutationEvent.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMText.h"
#include "nsCOMPtr.h"
#include "nsDOMString.h"
#include "nsIDOMUserDataHandler.h"
#include "nsChangeHint.h"
#include "nsEventDispatcher.h"
#include "nsCOMArray.h"
#include "nsNodeUtils.h"
#include "nsBindingManager.h"
#include "nsCCUncollectableMarker.h"
#include "mozAutoDocUpdate.h"
#include "nsPLDOMEvent.h"

#include "pldhash.h"
#include "prprf.h"

namespace css = mozilla::css;

nsGenericDOMDataNode::nsGenericDOMDataNode(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsIContent(aNodeInfo)
{
  NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::TEXT_NODE ||
                    mNodeInfo->NodeType() == nsIDOMNode::CDATA_SECTION_NODE ||
                    mNodeInfo->NodeType() == nsIDOMNode::COMMENT_NODE ||
                    mNodeInfo->NodeType() ==
                      nsIDOMNode::PROCESSING_INSTRUCTION_NODE ||
                    mNodeInfo->NodeType() == nsIDOMNode::DOCUMENT_TYPE_NODE,
                    "Bad NodeType in aNodeInfo");
}

nsGenericDOMDataNode::~nsGenericDOMDataNode()
{
  NS_PRECONDITION(!IsInDoc(),
                  "Please remove this from the document properly");
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsGenericDOMDataNode)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsGenericDOMDataNode)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsGenericDOMDataNode)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS

  nsIDocument* currentDoc = tmp->GetCurrentDoc();
  if (currentDoc && nsCCUncollectableMarker::InGeneration(
                      cb, currentDoc->GetMarkedCCGeneration())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNodeInfo)

  nsIDocument* ownerDoc = tmp->GetOwnerDoc();
  if (ownerDoc) {
    ownerDoc->BindingManager()->Traverse(tmp, cb);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_LISTENERMANAGER
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_USERDATA
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGenericDOMDataNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  NS_IMPL_CYCLE_COLLECTION_UNLINK_LISTENERMANAGER
  NS_IMPL_CYCLE_COLLECTION_UNLINK_USERDATA
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN(nsGenericDOMDataNode)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsGenericDOMDataNode)
  NS_INTERFACE_MAP_ENTRY(nsIContent)
  NS_INTERFACE_MAP_ENTRY(nsINode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOM3EventTarget,
                                 nsDOMEventRTTearoff::Create(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsISupportsWeakReference,
                                 new nsNodeSupportsWeakRefTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMXPathNSResolver,
                                 new nsNode3Tearoff(this))
  
  
  
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGenericDOMDataNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsGenericDOMDataNode,
                                              nsNodeUtils::LastRelease(this))


nsresult
nsGenericDOMDataNode::GetNodeValue(nsAString& aNodeValue)
{
  return GetData(aNodeValue);
}

nsresult
nsGenericDOMDataNode::SetNodeValue(const nsAString& aNodeValue)
{
  return SetTextInternal(0, mText.GetLength(), aNodeValue.BeginReading(),
                         aNodeValue.Length(), PR_TRUE);
}

nsresult
nsGenericDOMDataNode::GetNamespaceURI(nsAString& aNamespaceURI)
{
  SetDOMStringToNull(aNamespaceURI);

  return NS_OK;
}

nsresult
nsGenericDOMDataNode::GetPrefix(nsAString& aPrefix)
{
  SetDOMStringToNull(aPrefix);

  return NS_OK;
}

nsresult
nsGenericDOMDataNode::Normalize()
{
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::IsSupported(const nsAString& aFeature,
                                  const nsAString& aVersion,
                                  PRBool* aReturn)
{
  return nsGenericElement::InternalIsSupported(static_cast<nsIContent*>(this),
                                               aFeature, aVersion, aReturn);
}





nsresult
nsGenericDOMDataNode::GetData(nsAString& aData) const
{
  if (mText.Is2b()) {
    aData.Assign(mText.Get2b(), mText.GetLength());
  } else {
    
    

    const char *data = mText.Get1b();

    if (data) {
      CopyASCIItoUTF16(Substring(data, data + mText.GetLength()), aData);
    } else {
      aData.Truncate();
    }
  }

  return NS_OK;
}

nsresult
nsGenericDOMDataNode::SetData(const nsAString& aData)
{
  return SetTextInternal(0, mText.GetLength(), aData.BeginReading(),
                         aData.Length(), PR_TRUE);
}

nsresult
nsGenericDOMDataNode::GetLength(PRUint32* aLength)
{
  *aLength = mText.GetLength();
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::SubstringData(PRUint32 aStart, PRUint32 aCount,
                                    nsAString& aReturn)
{
  aReturn.Truncate();

  PRUint32 textLength = mText.GetLength();
  if (aStart > textLength) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  PRUint32 amount = aCount;
  if (amount > textLength - aStart) {
    amount = textLength - aStart;
  }

  if (mText.Is2b()) {
    aReturn.Assign(mText.Get2b() + aStart, amount);
  } else {
    
    

    const char *data = mText.Get1b() + aStart;
    CopyASCIItoUTF16(Substring(data, data + amount), aReturn);
  }

  return NS_OK;
}



nsresult
nsGenericDOMDataNode::AppendData(const nsAString& aData)
{
  return SetTextInternal(mText.GetLength(), 0, aData.BeginReading(),
                         aData.Length(), PR_TRUE);
}

nsresult
nsGenericDOMDataNode::InsertData(PRUint32 aOffset,
                                 const nsAString& aData)
{
  return SetTextInternal(aOffset, 0, aData.BeginReading(),
                         aData.Length(), PR_TRUE);
}

nsresult
nsGenericDOMDataNode::DeleteData(PRUint32 aOffset, PRUint32 aCount)
{
  return SetTextInternal(aOffset, aCount, nsnull, 0, PR_TRUE);
}

nsresult
nsGenericDOMDataNode::ReplaceData(PRUint32 aOffset, PRUint32 aCount,
                                  const nsAString& aData)
{
  return SetTextInternal(aOffset, aCount, aData.BeginReading(),
                         aData.Length(), PR_TRUE);
}

nsresult
nsGenericDOMDataNode::SetTextInternal(PRUint32 aOffset, PRUint32 aCount,
                                      const PRUnichar* aBuffer,
                                      PRUint32 aLength, PRBool aNotify)
{
  NS_PRECONDITION(aBuffer || !aLength,
                  "Null buffer passed to SetTextInternal!");

  
  PRUint32 textLength = mText.GetLength();
  if (aOffset > textLength) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  if (aCount > textLength - aOffset) {
    aCount = textLength - aOffset;
  }

  PRUint32 endOffset = aOffset + aCount;

  
  if (aLength > aCount && !mText.CanGrowBy(aLength - aCount)) {
    
    

    return NS_ERROR_DOM_DOMSTRING_SIZE_ERR;
  }

  nsIDocument *document = GetCurrentDoc();
  mozAutoDocUpdate updateBatch(document, UPDATE_CONTENT_MODEL, aNotify);

  PRBool haveMutationListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
      NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED,
      this);

  nsCOMPtr<nsIAtom> oldValue;
  if (haveMutationListeners) {
    oldValue = GetCurrentValueAtom();
  }
    
  if (aNotify) {
    CharacterDataChangeInfo info = {
      aOffset == textLength,
      aOffset,
      endOffset,
      aLength
    };
    nsNodeUtils::CharacterDataWillChange(this, &info);
  }

  if (aOffset == 0 && endOffset == textLength) {
    
    mText.SetTo(aBuffer, aLength);
  }
  else if (aOffset == textLength) {
    
    mText.Append(aBuffer, aLength);
  }
  else {
    

    
    PRInt32 newLength = textLength - aCount + aLength;
    PRUnichar* to = new PRUnichar[newLength];
    NS_ENSURE_TRUE(to, NS_ERROR_OUT_OF_MEMORY);

    
    if (aOffset) {
      mText.CopyTo(to, 0, aOffset);
    }
    if (aLength) {
      memcpy(to + aOffset, aBuffer, aLength * sizeof(PRUnichar));
    }
    if (endOffset != textLength) {
      mText.CopyTo(to + aOffset + aLength, endOffset, textLength - endOffset);
    }

    
    mText.SetTo(to, newLength);

    delete [] to;
  }

  UpdateBidiStatus(aBuffer, aLength);

  
  if (aNotify) {
    CharacterDataChangeInfo info = {
      aOffset == textLength,
      aOffset,
      endOffset,
      aLength
    };
    nsNodeUtils::CharacterDataChanged(this, &info);

    if (haveMutationListeners) {
      nsMutationEvent mutation(PR_TRUE, NS_MUTATION_CHARACTERDATAMODIFIED);

      mutation.mPrevAttrValue = oldValue;
      if (aLength > 0) {
        nsAutoString val;
        mText.AppendTo(val);
        mutation.mNewAttrValue = do_GetAtom(val);
      }

      mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
      (new nsPLDOMEvent(this, mutation))->RunDOMEventWhenSafe();
    }
  }

  return NS_OK;
}





#ifdef DEBUG
void
nsGenericDOMDataNode::ToCString(nsAString& aBuf, PRInt32 aOffset,
                                PRInt32 aLen) const
{
  if (mText.Is2b()) {
    const PRUnichar* cp = mText.Get2b() + aOffset;
    const PRUnichar* end = cp + aLen;

    while (cp < end) {
      PRUnichar ch = *cp++;
      if (ch == '&') {
        aBuf.AppendLiteral("&amp;");
      } else if (ch == '<') {
        aBuf.AppendLiteral("&lt;");
      } else if (ch == '>') {
        aBuf.AppendLiteral("&gt;");
      } else if ((ch < ' ') || (ch >= 127)) {
        char buf[10];
        PR_snprintf(buf, sizeof(buf), "\\u%04x", ch);
        AppendASCIItoUTF16(buf, aBuf);
      } else {
        aBuf.Append(ch);
      }
    }
  } else {
    unsigned char* cp = (unsigned char*)mText.Get1b() + aOffset;
    const unsigned char* end = cp + aLen;

    while (cp < end) {
      PRUnichar ch = *cp++;
      if (ch == '&') {
        aBuf.AppendLiteral("&amp;");
      } else if (ch == '<') {
        aBuf.AppendLiteral("&lt;");
      } else if (ch == '>') {
        aBuf.AppendLiteral("&gt;");
      } else if ((ch < ' ') || (ch >= 127)) {
        char buf[10];
        PR_snprintf(buf, sizeof(buf), "\\u%04x", ch);
        AppendASCIItoUTF16(buf, aBuf);
      } else {
        aBuf.Append(ch);
      }
    }
  }
}
#endif


nsresult
nsGenericDOMDataNode::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                 nsIContent* aBindingParent,
                                 PRBool aCompileEventHandlers)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION(HasSameOwnerDoc(NODE_FROM(aParent, aDocument)),
                  "Must have the same owner document");
  NS_PRECONDITION(!aParent || aDocument == aParent->GetCurrentDoc(),
                  "aDocument must be current doc of aParent");
  NS_PRECONDITION(!GetCurrentDoc() && !IsInDoc(),
                  "Already have a document.  Unbind first!");
  
  
  NS_PRECONDITION(!GetParent() || aParent == GetParent(),
                  "Already have a parent.  Unbind first!");
  NS_PRECONDITION(!GetBindingParent() ||
                  aBindingParent == GetBindingParent() ||
                  (!aBindingParent && aParent &&
                   aParent->GetBindingParent() == GetBindingParent()),
                  "Already have a binding parent.  Unbind first!");
  NS_PRECONDITION(aBindingParent != this,
                  "Content must not be its own binding parent");
  NS_PRECONDITION(!IsRootOfNativeAnonymousSubtree() || 
                  aBindingParent == aParent,
                  "Native anonymous content must have its parent as its "
                  "own binding parent");

  if (!aBindingParent && aParent) {
    aBindingParent = aParent->GetBindingParent();
  }

  
  if (aBindingParent) {
    nsDataSlots *slots = GetDataSlots();
    NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

    NS_ASSERTION(IsRootOfNativeAnonymousSubtree() ||
                 !HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE) ||
                 (aParent && aParent->IsInNativeAnonymousSubtree()),
                 "Trying to re-bind content from native anonymous subtree to "
                 "non-native anonymous parent!");
    slots->mBindingParent = aBindingParent; 
    if (aParent->IsInNativeAnonymousSubtree()) {
      SetFlags(NODE_IS_IN_ANONYMOUS_SUBTREE);
    }
  }

  
  if (aParent) {
    mParent = aParent;
  }
  else {
    mParent = aDocument;
  }
  SetParentIsContent(aParent);

  

  
  if (aDocument) {
    
    SetInDocument();
    if (mText.IsBidi()) {
      aDocument->SetBidiEnabled();
    }
    
    UnsetFlags(NODE_NEEDS_FRAME | NODE_DESCENDANTS_NEED_FRAMES);
  }

  nsNodeUtils::ParentChainChanged(this);

  UpdateEditableState(PR_FALSE);

  NS_POSTCONDITION(aDocument == GetCurrentDoc(), "Bound to wrong document");
  NS_POSTCONDITION(aParent == GetParent(), "Bound to wrong parent");
  NS_POSTCONDITION(aBindingParent == GetBindingParent(),
                   "Bound to wrong binding parent");

  return NS_OK;
}

void
nsGenericDOMDataNode::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  
  UnsetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE |
             NS_REFRAME_IF_WHITESPACE);
  
  nsIDocument *document = GetCurrentDoc();
  if (document) {
    
    
    
    document->BindingManager()->RemovedFromDocument(this, document);
  }

  if (aNullParent) {
    mParent = nsnull;
    SetParentIsContent(false);
  }
  ClearInDocument();

  nsDataSlots *slots = GetExistingDataSlots();
  if (slots) {
    slots->mBindingParent = nsnull;
  }

  nsNodeUtils::ParentChainChanged(this);
}

already_AddRefed<nsINodeList>
nsGenericDOMDataNode::GetChildren(PRUint32 aFilter)
{
  return nsnull;
}

nsIAtom *
nsGenericDOMDataNode::GetIDAttributeName() const
{
  return nsnull;
}

already_AddRefed<nsINodeInfo>
nsGenericDOMDataNode::GetExistingAttrNameFromQName(const nsAString& aStr) const
{
  return nsnull;
}

nsresult
nsGenericDOMDataNode::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr,
                              nsIAtom* aPrefix, const nsAString& aValue,
                              PRBool aNotify)
{
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr,
                                PRBool aNotify)
{
  return NS_OK;
}

PRBool
nsGenericDOMDataNode::GetAttr(PRInt32 aNameSpaceID, nsIAtom *aAttr,
                              nsAString& aResult) const
{
  aResult.Truncate();

  return PR_FALSE;
}

PRBool
nsGenericDOMDataNode::HasAttr(PRInt32 aNameSpaceID, nsIAtom *aAttribute) const
{
  return PR_FALSE;
}

const nsAttrName*
nsGenericDOMDataNode::GetAttrNameAt(PRUint32 aIndex) const
{
  return nsnull;
}

PRUint32
nsGenericDOMDataNode::GetAttrCount() const
{
  return 0;
}

NS_IMPL_DOMTARGET_DEFAULTS(nsGenericDOMDataNode)

NS_IMETHODIMP
nsGenericDOMDataNode::AddEventListener(const nsAString& aType,
                                       nsIDOMEventListener* aListener,
                                       PRBool aUseCapture,
                                       PRBool aWantsUntrusted,
                                       PRUint8 optional_argc)
{
  return nsIContent::AddEventListener(aType, aListener, aUseCapture,
                                      aWantsUntrusted, optional_argc);
}

NS_IMETHODIMP
nsGenericDOMDataNode::RemoveEventListener(const nsAString& aType,
                                          nsIDOMEventListener* aListener,
                                          PRBool aUseCapture)
{
  nsCOMPtr<nsIDOMEventTarget> event_target =
    do_QueryInterface(GetListenerManager(PR_TRUE));
  NS_ENSURE_STATE(event_target);

  return event_target->RemoveEventListener(aType, aListener, aUseCapture);
}

NS_IMETHODIMP
nsGenericDOMDataNode::DispatchEvent(nsIDOMEvent *aEvt, PRBool* _retval)
{
  nsCOMPtr<nsIDOMEventTarget> target =
    do_QueryInterface(GetListenerManager(PR_TRUE));
  NS_ENSURE_STATE(target);
  return target->DispatchEvent(aEvt, _retval);
}

nsresult
nsGenericDOMDataNode::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  return nsGenericElement::doPreHandleEvent(this, aVisitor);
}

nsresult
nsGenericDOMDataNode::PostHandleEvent(nsEventChainPostVisitor& )
{
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::DispatchDOMEvent(nsEvent* aEvent,
                                       nsIDOMEvent* aDOMEvent,
                                       nsPresContext* aPresContext,
                                       nsEventStatus* aEventStatus)
{
  return nsEventDispatcher::DispatchDOMEvent(static_cast<nsINode*>(this),
                                             aEvent, aDOMEvent,
                                             aPresContext, aEventStatus);
}

nsIEventListenerManager*
nsGenericDOMDataNode::GetListenerManager(PRBool aCreateIfNotFound)
{
  return nsContentUtils::GetListenerManager(this, aCreateIfNotFound);
}

nsresult
nsGenericDOMDataNode::AddEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID)
{
  nsIEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

nsresult
nsGenericDOMDataNode::RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                               const nsIID& aIID)
{
  nsIEventListenerManager* elm = GetListenerManager(PR_FALSE);
  return elm ?
    elm->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE) :
    NS_OK;
}

nsresult
nsGenericDOMDataNode::GetSystemEventGroup(nsIDOMEventGroup** aGroup)
{
  nsIEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->GetSystemEventGroupLM(aGroup);
}

nsIScriptContext*
nsGenericDOMDataNode::GetContextForEventHandlers(nsresult* aRv)
{
  return nsContentUtils::GetContextForEventHandlers(this, aRv);
}

PRUint32
nsGenericDOMDataNode::GetChildCount() const
{
  return 0;
}

nsIContent *
nsGenericDOMDataNode::GetChildAt(PRUint32 aIndex) const
{
  return nsnull;
}

nsIContent * const *
nsGenericDOMDataNode::GetChildArray(PRUint32* aChildCount) const
{
  *aChildCount = 0;
  return nsnull;
}

PRInt32
nsGenericDOMDataNode::IndexOf(nsINode* aPossibleChild) const
{
  return -1;
}

nsresult
nsGenericDOMDataNode::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                    PRBool aNotify)
{
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  return NS_OK;
}

nsIContent *
nsGenericDOMDataNode::GetBindingParent() const
{
  nsDataSlots *slots = GetExistingDataSlots();
  return slots ? slots->mBindingParent : nsnull;
}

PRBool
nsGenericDOMDataNode::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eDATA_NODE));
}

void
nsGenericDOMDataNode::SaveSubtreeState()
{
}

void
nsGenericDOMDataNode::DestroyContent()
{
  
  
  nsContentUtils::ReleaseWrapper(this, this);
}

#ifdef DEBUG
void
nsGenericDOMDataNode::List(FILE* out, PRInt32 aIndent) const
{
}

void
nsGenericDOMDataNode::DumpContent(FILE* out, PRInt32 aIndent,
                                  PRBool aDumpAll) const 
{
}
#endif

PRBool
nsGenericDOMDataNode::IsLink(nsIURI** aURI) const
{
  *aURI = nsnull;
  return PR_FALSE;
}

nsINode::nsSlots*
nsGenericDOMDataNode::CreateSlots()
{
  return new nsDataSlots();
}





nsresult
nsGenericDOMDataNode::SplitData(PRUint32 aOffset, nsIContent** aReturn,
                                PRBool aCloneAfterOriginal)
{
  *aReturn = nsnull;
  nsresult rv = NS_OK;
  nsAutoString cutText;
  PRUint32 length = TextLength();

  if (aOffset > length) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  PRUint32 cutStartOffset = aCloneAfterOriginal ? aOffset : 0;
  PRUint32 cutLength = aCloneAfterOriginal ? length - aOffset : aOffset;
  rv = SubstringData(cutStartOffset, cutLength, cutText);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = DeleteData(cutStartOffset, cutLength);
  if (NS_FAILED(rv)) {
    return rv;
  }

  




  nsCOMPtr<nsIContent> newContent = CloneDataNode(mNodeInfo, PR_FALSE);
  if (!newContent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  newContent->SetText(cutText, PR_TRUE);

  nsCOMPtr<nsINode> parent = GetNodeParent();

  if (parent) {
    PRInt32 insertionIndex = parent->IndexOf(this);
    if (aCloneAfterOriginal) {
      ++insertionIndex;
    }
    parent->InsertChildAt(newContent, insertionIndex, PR_TRUE);
  }

  newContent.swap(*aReturn);
  return rv;
}

nsresult
nsGenericDOMDataNode::SplitText(PRUint32 aOffset, nsIDOMText** aReturn)
{
  nsCOMPtr<nsIContent> newChild;
  nsresult rv = SplitData(aOffset, getter_AddRefs(newChild));
  if (NS_SUCCEEDED(rv)) {
    rv = CallQueryInterface(newChild, aReturn);
  }
  return rv;
}

 PRInt32
nsGenericDOMDataNode::FirstLogicallyAdjacentTextNode(nsIContent* aParent,
                                                     PRInt32 aIndex)
{
  while (aIndex-- > 0) {
    nsIContent* sibling = aParent->GetChildAt(aIndex);
    if (!sibling->IsNodeOfType(nsINode::eTEXT))
      return aIndex + 1;
  }
  return 0;
}

 PRInt32
nsGenericDOMDataNode::LastLogicallyAdjacentTextNode(nsIContent* aParent,
                                                    PRInt32 aIndex,
                                                    PRUint32 aCount)
{
  while (++aIndex < PRInt32(aCount)) {
    nsIContent* sibling = aParent->GetChildAt(aIndex);
    if (!sibling->IsNodeOfType(nsINode::eTEXT))
      return aIndex - 1;
  }
  return aCount - 1;
}

nsresult
nsGenericDOMDataNode::GetWholeText(nsAString& aWholeText)
{
  nsIContent* parent = GetParent();

  
  if (!parent)
    return GetData(aWholeText);

  PRInt32 index = parent->IndexOf(this);
  NS_WARN_IF_FALSE(index >= 0,
                   "Trying to use .wholeText with an anonymous"
                    "text node child of a binding parent?");
  NS_ENSURE_TRUE(index >= 0, NS_ERROR_DOM_NOT_SUPPORTED_ERR);
  PRInt32 first =
    FirstLogicallyAdjacentTextNode(parent, index);
  PRInt32 last =
    LastLogicallyAdjacentTextNode(parent, index, parent->GetChildCount());

  aWholeText.Truncate();

  nsCOMPtr<nsIDOMText> node;
  nsAutoString tmp;
  do {
    node = do_QueryInterface(parent->GetChildAt(first));
    node->GetData(tmp);
    aWholeText.Append(tmp);
  } while (first++ < last);

  return NS_OK;
}

nsresult
nsGenericDOMDataNode::ReplaceWholeText(const nsAString& aContent,
                                       nsIDOMText **aResult)
{
  *aResult = nsnull;

  
  nsCOMPtr<nsIContent> parent = GetParent();
  if (!parent) {
    if (aContent.IsEmpty()) {
      return NS_OK;
    }

    SetNodeValue(aContent);
    return CallQueryInterface(this, aResult);
  }

  
  nsIDocument* doc = GetOwnerDoc();

  
  mozAutoSubtreeModified subtree(doc, nsnull);

  PRInt32 index = parent->IndexOf(this);
  if (index < 0) {
    NS_WARNING("Trying to use .replaceWholeText with an anonymous text node "
               "child of a binding parent?");
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  
  
  
  PRInt32 first =
    FirstLogicallyAdjacentTextNode(parent, index);
  PRInt32 last =
    LastLogicallyAdjacentTextNode(parent, index, parent->GetChildCount());

  
  
  if (nsContentUtils::
        HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEREMOVED)) {
    for (PRInt32 i = first; i <= last; ++i) {
      nsCOMPtr<nsIContent> child = parent->GetChildAt((PRUint32)i);
      if (child &&
          (i != index || aContent.IsEmpty())) {
        nsContentUtils::MaybeFireNodeRemoved(child, parent, doc);
      }
    }
  }

  
  
  
  mozAutoDocUpdate updateBatch(parent->GetCurrentDoc(), UPDATE_CONTENT_MODEL,
                               PR_TRUE);

  do {
    if (last == index && !aContent.IsEmpty())
      continue;

    parent->RemoveChildAt(last, PR_TRUE);
  } while (last-- > first);

  
  if (aContent.IsEmpty()) {
    return NS_OK;
  }

  SetText(aContent.BeginReading(), aContent.Length(), PR_TRUE);
  return CallQueryInterface(this, aResult);
}





const nsTextFragment *
nsGenericDOMDataNode::GetText()
{
  return &mText;
}

PRUint32
nsGenericDOMDataNode::TextLength()
{
  return mText.GetLength();
}

nsresult
nsGenericDOMDataNode::SetText(const PRUnichar* aBuffer,
                              PRUint32 aLength,
                              PRBool aNotify)
{
  return SetTextInternal(0, mText.GetLength(), aBuffer, aLength, aNotify);
}

nsresult
nsGenericDOMDataNode::AppendText(const PRUnichar* aBuffer,
                                 PRUint32 aLength,
                                 PRBool aNotify)
{
  return SetTextInternal(mText.GetLength(), 0, aBuffer, aLength, aNotify);
}

PRBool
nsGenericDOMDataNode::TextIsOnlyWhitespace()
{
  if (mText.Is2b()) {
    
    
    return PR_FALSE;
  }

  const char* cp = mText.Get1b();
  const char* end = cp + mText.GetLength();

  while (cp < end) {
    char ch = *cp;

    if (!XP_IS_SPACE(ch)) {
      return PR_FALSE;
    }

    ++cp;
  }

  return PR_TRUE;
}

void
nsGenericDOMDataNode::AppendTextTo(nsAString& aResult)
{
  mText.AppendTo(aResult);
}

void nsGenericDOMDataNode::UpdateBidiStatus(const PRUnichar* aBuffer, PRUint32 aLength)
{
  nsIDocument *document = GetCurrentDoc();
  if (document && document->GetBidiEnabled()) {
    
    return;
  }

  mText.UpdateBidiFlag(aBuffer, aLength);

  if (document && mText.IsBidi()) {
    document->SetBidiEnabled();
  }
}

already_AddRefed<nsIAtom>
nsGenericDOMDataNode::GetCurrentValueAtom()
{
  nsAutoString val;
  GetData(val);
  return NS_NewAtom(val);
}

nsIAtom*
nsGenericDOMDataNode::DoGetID() const
{
  return nsnull;
}

const nsAttrValue*
nsGenericDOMDataNode::DoGetClasses() const
{
  NS_NOTREACHED("Shouldn't ever be called");
  return nsnull;
}

NS_IMETHODIMP
nsGenericDOMDataNode::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  return NS_OK;
}

#ifdef MOZ_SMIL
nsIDOMCSSStyleDeclaration*
nsGenericDOMDataNode::GetSMILOverrideStyle()
{
  return nsnull;
}

css::StyleRule*
nsGenericDOMDataNode::GetSMILOverrideStyleRule()
{
  return nsnull;
}

nsresult
nsGenericDOMDataNode::SetSMILOverrideStyleRule(css::StyleRule* aStyleRule,
                                               PRBool aNotify)
{
  NS_NOTREACHED("How come we're setting SMILOverrideStyle on a non-element?");
  return NS_ERROR_UNEXPECTED;
}
#endif 

css::StyleRule*
nsGenericDOMDataNode::GetInlineStyleRule()
{
  return nsnull;
}

NS_IMETHODIMP
nsGenericDOMDataNode::SetInlineStyleRule(css::StyleRule* aStyleRule,
                                         PRBool aNotify)
{
  NS_NOTREACHED("How come we're setting inline style on a non-element?");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP_(PRBool)
nsGenericDOMDataNode::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  return PR_FALSE;
}

nsChangeHint
nsGenericDOMDataNode::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                             PRInt32 aModType) const
{
  NS_NOTREACHED("Shouldn't be calling this!");
  return nsChangeHint(0);
}

nsIAtom*
nsGenericDOMDataNode::GetClassAttributeName() const
{
  return nsnull;
}
