




#include "Accessible-inl.h"

#include "nsIXBLAccessible.h"

#include "AccGroupInfo.h"
#include "AccIterator.h"
#include "nsAccUtils.h"
#include "nsAccEvent.h"
#include "nsAccessibleRelation.h"
#include "nsAccessibilityService.h"
#include "nsAccTreeWalker.h"
#include "nsIAccessibleRelation.h"
#include "nsEventShell.h"
#include "nsTextEquivUtils.h"
#include "Relation.h"
#include "Role.h"
#include "RootAccessible.h"
#include "States.h"
#include "StyleInfo.h"

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMTreeWalker.h"
#include "nsIDOMXULButtonElement.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULLabelElement.h"
#include "nsIDOMXULSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsPIDOMWindow.h"

#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIForm.h"
#include "nsIFormControl.h"

#include "nsLayoutUtils.h"
#include "nsIPresShell.h"
#include "nsIStringBundle.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsIView.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScrollableFrame.h"
#include "nsFocusManager.h"

#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "prdtoa.h"
#include "nsIAtom.h"
#include "nsIURI.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsWhitespaceTokenizer.h"
#include "nsAttrName.h"
#include "nsNetUtil.h"
#include "nsEventStates.h"

#ifdef NS_DEBUG
#include "nsIDOMCharacterData.h"
#endif

#include "mozilla/unused.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





NS_IMPL_CYCLE_COLLECTION_CLASS(nsAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsAccessible, nsAccessNode)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mParent");
  cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mParent.get()));

  PRUint32 i, length = tmp->mChildren.Length();
  for (i = 0; i < length; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChildren[i]");
    cb.NoteXPCOMChild(static_cast<nsIAccessible*>(tmp->mChildren[i].get()));
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsAccessible, nsAccessNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mChildren)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(nsAccessible, nsAccessNode)
NS_IMPL_RELEASE_INHERITED(nsAccessible, nsAccessNode)

nsresult nsAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  
  
  *aInstancePtr = nsnull;

  if (aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant))) {
    *aInstancePtr = &NS_CYCLE_COLLECTION_NAME(nsAccessible);
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessible))) {
    *aInstancePtr = static_cast<nsIAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsAccessible))) {
    *aInstancePtr = static_cast<nsAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleSelectable))) {
    if (IsSelect()) {
      *aInstancePtr = static_cast<nsIAccessibleSelectable*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }
    return NS_ERROR_NO_INTERFACE;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleValue))) {
    if (mRoleMapEntry && mRoleMapEntry->valueRule != eNoValue) {
      *aInstancePtr = static_cast<nsIAccessibleValue*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }
  }                       

  if (aIID.Equals(NS_GET_IID(nsIAccessibleHyperLink))) {
    if (IsLink()) {
      *aInstancePtr = static_cast<nsIAccessibleHyperLink*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }
    return NS_ERROR_NO_INTERFACE;
  }

  return nsAccessNodeWrap::QueryInterface(aIID, aInstancePtr);
}

nsAccessible::nsAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessNodeWrap(aContent, aDoc),
  mParent(nsnull), mIndexInParent(-1), mFlags(eChildrenUninitialized),
  mIndexOfEmbeddedChild(-1), mRoleMapEntry(nsnull)
{
#ifdef NS_DEBUG_X
   {
     nsCOMPtr<nsIPresShell> shell(do_QueryReferent(aShell));
     printf(">>> %p Created Acc - DOM: %p  PS: %p", 
            (void*)static_cast<nsIAccessible*>(this), (void*)aNode,
            (void*)shell.get());
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (content) {
      printf(" Con: %s@%p",
             NS_ConvertUTF16toUTF8(content->NodeInfo()->QualifiedName()).get(),
             (void *)content.get());
      nsAutoString buf;
      Name(buf);
      printf(" Name:[%s]", NS_ConvertUTF16toUTF8(buf).get());
     }
     printf("\n");
   }
#endif
}




nsAccessible::~nsAccessible()
{
}

void
nsAccessible::SetRoleMapEntry(nsRoleMapEntry* aRoleMapEntry)
{
  mRoleMapEntry = aRoleMapEntry;
}

NS_IMETHODIMP
nsAccessible::GetDocument(nsIAccessibleDocument **aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  NS_IF_ADDREF(*aDocument = Document());
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetDOMNode(nsIDOMNode **aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;

  nsINode *node = GetNode();
  if (node)
    CallQueryInterface(node, aDOMNode);

  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetRootDocument(nsIAccessibleDocument **aRootDocument)
{
  NS_ENSURE_ARG_POINTER(aRootDocument);

  NS_IF_ADDREF(*aRootDocument = RootAccessible());
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetLanguage(nsAString& aLanguage)
{
  Language(aLanguage);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAutoString name;
  Name(name);
  aName.Assign(name);

  return NS_OK;
}

ENameValueFlag
nsAccessible::Name(nsString& aName)
{
  aName.Truncate();

  GetARIAName(aName);
  if (!aName.IsEmpty())
    return eNameOK;

  nsCOMPtr<nsIXBLAccessible> xblAccessible(do_QueryInterface(mContent));
  if (xblAccessible) {
    xblAccessible->GetAccessibleName(aName);
    if (!aName.IsEmpty())
      return eNameOK;
  }

  nsresult rv = GetNameInternal(aName);
  if (!aName.IsEmpty())
    return eNameOK;

  
  if (mContent->IsHTML()) {
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::title, aName)) {
      aName.CompressWhitespace();
      return eNameFromTooltip;
    }
  } else if (mContent->IsXUL()) {
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltiptext, aName)) {
      aName.CompressWhitespace();
      return eNameFromTooltip;
    }
  } else {
    return eNameOK;
  }

  if (rv != NS_OK_EMPTY_NAME)
    aName.SetIsVoid(true);

  return eNameOK;
}

NS_IMETHODIMP
nsAccessible::GetDescription(nsAString& aDescription)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAutoString desc;
  Description(desc);
  aDescription.Assign(desc);

  return NS_OK;
}

void
nsAccessible::Description(nsString& aDescription)
{
  
  
  
  
  

  if (mContent->IsNodeOfType(nsINode::eTEXT))
    return;

  nsTextEquivUtils::
    GetTextEquivFromIDRefs(this, nsGkAtoms::aria_describedby,
                           aDescription);

  if (aDescription.IsEmpty()) {
    bool isXUL = mContent->IsXUL();
    if (isXUL) {
      
      XULDescriptionIterator iter(Document(), mContent);
      nsAccessible* descr = nsnull;
      while ((descr = iter.Next()))
        nsTextEquivUtils::AppendTextEquivFromContent(this, descr->GetContent(),
                                                     &aDescription);
      }

      if (aDescription.IsEmpty()) {
        nsIAtom *descAtom = isXUL ? nsGkAtoms::tooltiptext :
                                    nsGkAtoms::title;
        if (mContent->GetAttr(kNameSpaceID_None, descAtom, aDescription)) {
          nsAutoString name;
          Name(name);
          if (name.IsEmpty() || aDescription == name)
            
            
            aDescription.Truncate();
        }
      }
    }
    aDescription.CompressWhitespace();
}

NS_IMETHODIMP
nsAccessible::GetKeyboardShortcut(nsAString& aAccessKey)
{
  aAccessKey.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  AccessKey().ToString(aAccessKey);
  return NS_OK;
}

KeyBinding
nsAccessible::AccessKey() const
{
  PRUint32 key = nsCoreUtils::GetAccessKeyFor(mContent);
  if (!key && mContent->IsElement()) {
    nsAccessible* label = nsnull;

    
    if (mContent->IsHTML()) {
      
      
      HTMLLabelIterator iter(Document(), this,
                             HTMLLabelIterator::eSkipAncestorLabel);
      label = iter.Next();

    } else if (mContent->IsXUL()) {
      XULLabelIterator iter(Document(), mContent);
      label = iter.Next();
    }

    if (label)
      key = nsCoreUtils::GetAccessKeyFor(label->GetContent());
  }

  if (!key)
    return KeyBinding();

  
  switch (Preferences::GetInt("ui.key.generalAccessKey", -1)) {
  case -1:
    break;
  case nsIDOMKeyEvent::DOM_VK_SHIFT:
    return KeyBinding(key, KeyBinding::kShift);
  case nsIDOMKeyEvent::DOM_VK_CONTROL:
    return KeyBinding(key, KeyBinding::kControl);
  case nsIDOMKeyEvent::DOM_VK_ALT:
    return KeyBinding(key, KeyBinding::kAlt);
  case nsIDOMKeyEvent::DOM_VK_META:
    return KeyBinding(key, KeyBinding::kMeta);
  default:
    return KeyBinding();
  }

  
  nsIDocument* document = mContent->GetCurrentDoc();
  if (!document)
    return KeyBinding();
  nsCOMPtr<nsISupports> container = document->GetContainer();
  if (!container)
    return KeyBinding();
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(container));
  if (!treeItem)
    return KeyBinding();

  nsresult rv = NS_ERROR_FAILURE;
  PRInt32 itemType = 0, modifierMask = 0;
  treeItem->GetItemType(&itemType);
  switch (itemType) {
    case nsIDocShellTreeItem::typeChrome:
      rv = Preferences::GetInt("ui.key.chromeAccess", &modifierMask);
      break;
    case nsIDocShellTreeItem::typeContent:
      rv = Preferences::GetInt("ui.key.contentAccess", &modifierMask);
      break;
  }

  return NS_SUCCEEDED(rv) ? KeyBinding(key, modifierMask) : KeyBinding();
}

KeyBinding
nsAccessible::KeyboardShortcut() const
{
  return KeyBinding();
}

NS_IMETHODIMP
nsAccessible::GetParent(nsIAccessible **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aParent = Parent());
  return *aParent ? NS_OK : NS_ERROR_FAILURE;
}

  
NS_IMETHODIMP
nsAccessible::GetNextSibling(nsIAccessible **aNextSibling) 
{
  NS_ENSURE_ARG_POINTER(aNextSibling);
  *aNextSibling = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  NS_IF_ADDREF(*aNextSibling = GetSiblingAtOffset(1, &rv));
  return rv;
}

  
NS_IMETHODIMP
nsAccessible::GetPreviousSibling(nsIAccessible * *aPreviousSibling) 
{
  NS_ENSURE_ARG_POINTER(aPreviousSibling);
  *aPreviousSibling = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  NS_IF_ADDREF(*aPreviousSibling = GetSiblingAtOffset(-1, &rv));
  return rv;
}

  
NS_IMETHODIMP
nsAccessible::GetFirstChild(nsIAccessible **aFirstChild) 
{
  NS_ENSURE_ARG_POINTER(aFirstChild);
  *aFirstChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  if (childCount > 0)
    NS_ADDREF(*aFirstChild = GetChildAt(0));

  return NS_OK;
}

  
NS_IMETHODIMP
nsAccessible::GetLastChild(nsIAccessible **aLastChild)
{
  NS_ENSURE_ARG_POINTER(aLastChild);
  *aLastChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  NS_IF_ADDREF(*aLastChild = GetChildAt(childCount - 1));
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetChildAt(PRInt32 aChildIndex, nsIAccessible **aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);
  *aChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  
  
  if (aChildIndex < 0)
    aChildIndex = childCount - 1;

  nsAccessible* child = GetChildAt(aChildIndex);
  if (!child)
    return NS_ERROR_INVALID_ARG;

  NS_ADDREF(*aChild = child);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetChildren(nsIArray **aOutChildren)
{
  NS_ENSURE_ARG_POINTER(aOutChildren);
  *aOutChildren = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 childCount = GetChildCount();
  NS_ENSURE_TRUE(childCount != -1, NS_ERROR_FAILURE);

  nsresult rv = NS_OK;
  nsCOMPtr<nsIMutableArray> children =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsIAccessible* child = GetChildAt(childIdx);
    children->AppendElement(child, false);
  }

  NS_ADDREF(*aOutChildren = children);
  return NS_OK;
}

bool
nsAccessible::CanHaveAnonChildren()
{
  return true;
}


NS_IMETHODIMP
nsAccessible::GetChildCount(PRInt32 *aChildCount) 
{
  NS_ENSURE_ARG_POINTER(aChildCount);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aChildCount = GetChildCount();
  return *aChildCount != -1 ? NS_OK : NS_ERROR_FAILURE;  
}


NS_IMETHODIMP
nsAccessible::GetIndexInParent(PRInt32 *aIndexInParent)
{
  NS_ENSURE_ARG_POINTER(aIndexInParent);

  *aIndexInParent = IndexInParent();
  return *aIndexInParent != -1 ? NS_OK : NS_ERROR_FAILURE;
}

void 
nsAccessible::TranslateString(const nsString& aKey, nsAString& aStringOut)
{
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    services::GetStringBundleService();
  if (!stringBundleService)
    return;

  nsCOMPtr<nsIStringBundle> stringBundle;
  stringBundleService->CreateBundle(
    "chrome://global-platform/locale/accessible.properties", 
    getter_AddRefs(stringBundle));
  if (!stringBundle)
    return;

  nsXPIDLString xsValue;
  nsresult rv = stringBundle->GetStringFromName(aKey.get(), getter_Copies(xsValue));
  if (NS_SUCCEEDED(rv))
    aStringOut.Assign(xsValue);
}

PRUint64
nsAccessible::VisibilityState()
{
  PRUint64 vstates = states::INVISIBLE | states::OFFSCREEN;

  nsIFrame* frame = GetFrame();
  if (!frame)
    return vstates;

  nsIPresShell* shell(mDoc->PresShell());
  if (!shell)
    return vstates;

  
  
  const PRUint16 kMinPixels  = 12;
  const nsSize frameSize = frame->GetSize();
  const nsRectVisibility rectVisibility =
    shell->GetRectVisibility(frame, nsRect(nsPoint(0,0), frameSize),
                             nsPresContext::CSSPixelsToAppUnits(kMinPixels));

  if (rectVisibility == nsRectVisibility_kVisible)
    vstates &= ~states::OFFSCREEN;

  
  
  
  
  
  if (frame->GetType() == nsGkAtoms::textFrame &&
      !(frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
      frame->GetRect().IsEmpty()) {
    nsAutoString renderedText;
    frame->GetRenderedText(&renderedText, nsnull, nsnull, 0, 1);
    if (renderedText.IsEmpty())
      return vstates;

  }

  
  if (!frame->IsVisibleConsideringAncestors(nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY))
    return vstates;

  
  return vstates &= ~states::INVISIBLE;
}

PRUint64
nsAccessible::NativeState()
{
  PRUint64 state = 0;

  nsDocAccessible* document = Document();
  if (!document || !document->IsInDocument(this))
    state |= states::STALE;

  bool disabled = false;
  if (mContent->IsElement()) {
    nsEventStates elementState = mContent->AsElement()->State();

    if (elementState.HasState(NS_EVENT_STATE_INVALID))
      state |= states::INVALID;

    if (elementState.HasState(NS_EVENT_STATE_REQUIRED))
      state |= states::REQUIRED;

    disabled = mContent->IsHTML() ? 
      (elementState.HasState(NS_EVENT_STATE_DISABLED)) :
      (mContent->AttrValueIs(kNameSpaceID_None,
                             nsGkAtoms::disabled,
                             nsGkAtoms::_true,
                             eCaseMatters));
  }

  
  if (disabled) {
    state |= states::UNAVAILABLE;
  }
  else if (mContent->IsElement()) {
    nsIFrame* frame = GetFrame();
    if (frame && frame->IsFocusable())
      state |= states::FOCUSABLE;

    if (FocusMgr()->IsFocused(this))
      state |= states::FOCUSED;
  }

  
  state |= VisibilityState();

  nsIFrame *frame = GetFrame();
  if (frame && (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW))
    state |= states::FLOATING;

  
  if (mContent->IsXUL())
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::popup))
      state |= states::HASPOPUP;

  
  if (!mRoleMapEntry || mRoleMapEntry->roleRule == kUseNativeRole ||
      mRoleMapEntry->role == roles::LINK)
    state |= NativeLinkState();

  return state;
}

PRUint64
nsAccessible::NativeLinkState() const
{
  
  return nsCoreUtils::IsXLink(mContent) ? states::LINKED : 0;
}

  
NS_IMETHODIMP
nsAccessible::GetFocusedChild(nsIAccessible** aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);
  *aChild = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aChild = FocusedChild());
  return NS_OK;
}

nsAccessible*
nsAccessible::FocusedChild()
{
  nsAccessible* focus = FocusMgr()->FocusedAccessible();
  if (focus && (focus == this || focus->Parent() == this))
    return focus;

  return nsnull;
}


nsAccessible*
nsAccessible::ChildAtPoint(PRInt32 aX, PRInt32 aY,
                           EWhichChildAtPoint aWhichChild)
{
  
  
  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = GetBounds(&x, &y, &width, &height);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsAccessible* fallbackAnswer = nsnull;
  if (aX >= x && aX < x + width && aY >= y && aY < y + height)
    fallbackAnswer = this;

  if (nsAccUtils::MustPrune(this))  
    return fallbackAnswer;

  
  
  
  
  
  
  nsDocAccessible* accDocument = Document();
  NS_ENSURE_TRUE(accDocument, nsnull);

  nsIFrame *frame = accDocument->GetFrame();
  NS_ENSURE_TRUE(frame, nsnull);

  nsPresContext *presContext = frame->PresContext();

  nsRect screenRect = frame->GetScreenRectInAppUnits();
  nsPoint offset(presContext->DevPixelsToAppUnits(aX) - screenRect.x,
                 presContext->DevPixelsToAppUnits(aY) - screenRect.y);

  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();
  nsIFrame *foundFrame = presShell->GetFrameForPoint(frame, offset);

  nsIContent* content = nsnull;
  if (!foundFrame || !(content = foundFrame->GetContent()))
    return fallbackAnswer;

  
  
  nsDocAccessible* contentDocAcc = GetAccService()->
    GetDocAccessible(content->OwnerDoc());

  
  NS_ASSERTION(contentDocAcc, "could not get the document accessible");
  if (!contentDocAcc)
    return fallbackAnswer;

  nsAccessible* accessible = contentDocAcc->GetAccessibleOrContainer(content);
  if (!accessible)
    return fallbackAnswer;

  if (accessible == this) {
    
    
    
    
    
    PRInt32 childCount = GetChildCount();
    for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
      nsAccessible *child = GetChildAt(childIdx);

      PRInt32 childX, childY, childWidth, childHeight;
      child->GetBounds(&childX, &childY, &childWidth, &childHeight);
      if (aX >= childX && aX < childX + childWidth &&
          aY >= childY && aY < childY + childHeight &&
          (child->State() & states::INVISIBLE) == 0) {

        if (aWhichChild == eDeepestChild)
          return child->ChildAtPoint(aX, aY, eDeepestChild);

        return child;
      }
    }

    
    
    return accessible;
  }

  
  
  nsAccessible* child = accessible;
  while (true) {
    nsAccessible* parent = child->Parent();
    if (!parent) {
      
      
      return fallbackAnswer;
    }

    if (parent == this)
      return aWhichChild == eDeepestChild ? accessible : child;

    child = parent;
  }

  return nsnull;
}


NS_IMETHODIMP
nsAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                              nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aAccessible = ChildAtPoint(aX, aY, eDirectChild));
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetDeepestChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  NS_IF_ADDREF(*aAccessible = ChildAtPoint(aX, aY, eDeepestChild));
  return NS_OK;
}

void nsAccessible::GetBoundsRect(nsRect& aTotalBounds, nsIFrame** aBoundingFrame)
{











  
  *aBoundingFrame = nsnull;
  nsIFrame* firstFrame = GetFrame();
  if (!firstFrame)
    return;

  
  
  
  nsIFrame *ancestorFrame = firstFrame;

  while (ancestorFrame) {  
    *aBoundingFrame = ancestorFrame;
    
    
    if (ancestorFrame->GetType() != nsGkAtoms::inlineFrame &&
        ancestorFrame->GetType() != nsGkAtoms::textFrame)
      break;
    ancestorFrame = ancestorFrame->GetParent();
  }

  nsIFrame *iterFrame = firstFrame;
  nsCOMPtr<nsIContent> firstContent(mContent);
  nsIContent* iterContent = firstContent;
  PRInt32 depth = 0;

  
  while (iterContent == firstContent || depth > 0) {
    
    nsRect currFrameBounds = iterFrame->GetRect();
    
    
    currFrameBounds +=
      iterFrame->GetParent()->GetOffsetToExternal(*aBoundingFrame);

    
    aTotalBounds.UnionRect(aTotalBounds, currFrameBounds);

    nsIFrame *iterNextFrame = nsnull;

    if (iterFrame->GetType() == nsGkAtoms::inlineFrame) {
      
      
      iterNextFrame = iterFrame->GetFirstPrincipalChild();
    }

    if (iterNextFrame) 
      ++depth;  
    else {  
      
      
      while (iterFrame) {
        iterNextFrame = iterFrame->GetNextContinuation();
        if (!iterNextFrame)
          iterNextFrame = iterFrame->GetNextSibling();
        if (iterNextFrame || --depth < 0) 
          break;
        iterFrame = iterFrame->GetParent();
      }
    }

    
    iterFrame = iterNextFrame;
    if (iterFrame == nsnull)
      break;
    iterContent = nsnull;
    if (depth == 0)
      iterContent = iterFrame->GetContent();
  }
}



NS_IMETHODIMP
nsAccessible::GetBounds(PRInt32* aX, PRInt32* aY,
                        PRInt32* aWidth, PRInt32* aHeight)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = 0;
  NS_ENSURE_ARG_POINTER(aY);
  *aY = 0;
  NS_ENSURE_ARG_POINTER(aWidth);
  *aWidth = 0;
  NS_ENSURE_ARG_POINTER(aHeight);
  *aHeight = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIPresShell* presShell = mDoc->PresShell();

  
  
  
  
  

  nsRect unionRectTwips;
  nsIFrame* boundingFrame = nsnull;
  GetBoundsRect(unionRectTwips, &boundingFrame);   
  NS_ENSURE_STATE(boundingFrame);

  nsPresContext* presContext = presShell->GetPresContext();
  *aX = presContext->AppUnitsToDevPixels(unionRectTwips.x);
  *aY = presContext->AppUnitsToDevPixels(unionRectTwips.y);
  *aWidth = presContext->AppUnitsToDevPixels(unionRectTwips.width);
  *aHeight = presContext->AppUnitsToDevPixels(unionRectTwips.height);

  
  nsIntRect orgRectPixels = boundingFrame->GetScreenRectInAppUnits().
    ToNearestPixels(presContext->AppUnitsPerDevPixel());
  *aX += orgRectPixels.x;
  *aY += orgRectPixels.y;

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::SetSelected(bool aSelect)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible* select = nsAccUtils::GetSelectableContainer(this, State());
  if (select) {
    if (select->State() & states::MULTISELECTABLE) {
      if (mRoleMapEntry) {
        if (aSelect) {
          return mContent->SetAttr(kNameSpaceID_None,
                                   nsGkAtoms::aria_selected,
                                   NS_LITERAL_STRING("true"), true);
        }
        return mContent->UnsetAttr(kNameSpaceID_None,
                                   nsGkAtoms::aria_selected, true);
      }

      return NS_OK;
    }

    return aSelect ? TakeFocus() : NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsAccessible::TakeSelection()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible* select = nsAccUtils::GetSelectableContainer(this, State());
  if (select) {
    if (select->State() & states::MULTISELECTABLE)
      select->ClearSelection();
    return SetSelected(true);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsAccessible::TakeFocus()
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIFrame *frame = GetFrame();
  NS_ENSURE_STATE(frame);

  nsIContent* focusContent = mContent;

  
  
  if (!frame->IsFocusable()) {
    nsAccessible* widget = ContainerWidget();
    if (widget && widget->AreItemsOperable()) {
      nsIContent* widgetElm = widget->GetContent();
      nsIFrame* widgetFrame = widgetElm->GetPrimaryFrame();
      if (widgetFrame && widgetFrame->IsFocusable()) {
        focusContent = widgetElm;
        widget->SetCurrentItem(this);
      }
    }
  }

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(focusContent));
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->SetFocus(element, 0);

  return NS_OK;
}

nsresult
nsAccessible::GetHTMLName(nsAString& aLabel)
{
  nsAutoString label;

  nsAccessible* labelAcc = nsnull;
  HTMLLabelIterator iter(Document(), this);
  while ((labelAcc = iter.Next())) {
    nsresult rv = nsTextEquivUtils::
      AppendTextEquivFromContent(this, labelAcc->GetContent(), &label);
    NS_ENSURE_SUCCESS(rv, rv);

    label.CompressWhitespace();
  }

  if (label.IsEmpty())
    return nsTextEquivUtils::GetNameFromSubtree(this, aLabel);

  aLabel = label;
  return NS_OK;
}













nsresult
nsAccessible::GetXULName(nsAString& aLabel)
{
  
  nsresult rv = NS_OK;

  nsAutoString label;
  nsCOMPtr<nsIDOMXULLabeledControlElement> labeledEl(do_QueryInterface(mContent));
  if (labeledEl) {
    rv = labeledEl->GetLabel(label);
  }
  else {
    nsCOMPtr<nsIDOMXULSelectControlItemElement> itemEl(do_QueryInterface(mContent));
    if (itemEl) {
      rv = itemEl->GetLabel(label);
    }
    else {
      nsCOMPtr<nsIDOMXULSelectControlElement> select(do_QueryInterface(mContent));
      
      
      if (!select) {
        nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(mContent));
        if (xulEl) {
          rv = xulEl->GetAttribute(NS_LITERAL_STRING("label"), label);
        }
      }
    }
  }

  
  if (NS_FAILED(rv) || label.IsEmpty()) {
    label.Truncate();

    nsAccessible* labelAcc = nsnull;
    XULLabelIterator iter(Document(), mContent);
    while ((labelAcc = iter.Next())) {
      nsCOMPtr<nsIDOMXULLabelElement> xulLabel =
        do_QueryInterface(labelAcc->GetContent());
      
      if (xulLabel && NS_SUCCEEDED(xulLabel->GetValue(label)) && label.IsEmpty()) {
        
        
        nsTextEquivUtils::
          AppendTextEquivFromContent(this, labelAcc->GetContent(), &label);
      }
    }
  }

  
  label.CompressWhitespace();
  if (!label.IsEmpty()) {
    aLabel = label;
    return NS_OK;
  }

  
  nsIContent *bindingParent = mContent->GetBindingParent();
  nsIContent *parent = bindingParent? bindingParent->GetParent() :
                                      mContent->GetParent();
  while (parent) {
    if (parent->Tag() == nsGkAtoms::toolbaritem &&
        parent->GetAttr(kNameSpaceID_None, nsGkAtoms::title, label)) {
      label.CompressWhitespace();
      aLabel = label;
      return NS_OK;
    }
    parent = parent->GetParent();
  }

  return nsTextEquivUtils::GetNameFromSubtree(this, aLabel);
}

nsresult
nsAccessible::HandleAccEvent(AccEvent* aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(obsService, NS_ERROR_FAILURE);

  nsCOMPtr<nsISimpleEnumerator> observers;
  obsService->EnumerateObservers(NS_ACCESSIBLE_EVENT_TOPIC,
                                 getter_AddRefs(observers));

  NS_ENSURE_STATE(observers);

  bool hasObservers = false;
  observers->HasMoreElements(&hasObservers);
  if (hasObservers) {
    nsRefPtr<nsAccEvent> evnt(aEvent->CreateXPCOMObject());
    return obsService->NotifyObservers(evnt, NS_ACCESSIBLE_EVENT_TOPIC, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);
  *aRole = nsIAccessibleRole::ROLE_NOTHING;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aRole = Role();
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);  
  
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPersistentProperties> attributes = *aAttributes;
  if (!attributes) {
    
    attributes = do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
    NS_ENSURE_TRUE(attributes, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(*aAttributes = attributes);
  }
 
  nsresult rv = GetAttributesInternal(attributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString id;
  nsAutoString oldValueUnused;
  if (nsCoreUtils::GetID(mContent, id)) {
    
    
    attributes->SetStringProperty(NS_LITERAL_CSTRING("id"), id, oldValueUnused);
  }
  
  nsAutoString xmlRoles;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::role, xmlRoles)) {
    attributes->SetStringProperty(NS_LITERAL_CSTRING("xml-roles"),  xmlRoles, oldValueUnused);          
  }

  nsCOMPtr<nsIAccessibleValue> supportsValue = do_QueryInterface(static_cast<nsIAccessible*>(this));
  if (supportsValue) {
    
    
    
    nsAutoString valuetext;
    GetValue(valuetext);
    attributes->SetStringProperty(NS_LITERAL_CSTRING("valuetext"), valuetext, oldValueUnused);
  }

  
  if (State() & states::CHECKABLE)
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::checkable, NS_LITERAL_STRING("true"));

  
  PRInt32 level = 0, posInSet = 0, setSize = 0;
  rv = GroupPosition(&level, &setSize, &posInSet);
  if (NS_SUCCEEDED(rv))
    nsAccUtils::SetAccGroupAttrs(attributes, level, setSize, posInSet);

  
  PRUint32 numAttrs = mContent->GetAttrCount();
  for (PRUint32 count = 0; count < numAttrs; count ++) {
    const nsAttrName *attr = mContent->GetAttrNameAt(count);
    if (attr && attr->NamespaceEquals(kNameSpaceID_None)) {
      nsIAtom *attrAtom = attr->Atom();
      nsDependentAtomString attrStr(attrAtom);
      if (!StringBeginsWith(attrStr, NS_LITERAL_STRING("aria-"))) 
        continue; 
      PRUint8 attrFlags = nsAccUtils::GetAttributeCharacteristics(attrAtom);
      if (attrFlags & ATTR_BYPASSOBJ)
        continue; 
      if ((attrFlags & ATTR_VALTOKEN) &&
          !nsAccUtils::HasDefinedARIAToken(mContent, attrAtom))
        continue; 
      nsAutoString value;
      if (mContent->GetAttr(kNameSpaceID_None, attrAtom, value)) {
        attributes->SetStringProperty(NS_ConvertUTF16toUTF8(Substring(attrStr, 5)), value, oldValueUnused);
      }
    }
  }

  
  
  if (mRoleMapEntry) {
    nsAutoString live;
    nsAccUtils::GetAccAttr(attributes, nsGkAtoms::live, live);
    if (live.IsEmpty()) {
      if (nsAccUtils::GetLiveAttrValue(mRoleMapEntry->liveAttRule, live))
        nsAccUtils::SetAccAttr(attributes, nsGkAtoms::live, live);
    }
  }

  return NS_OK;
}

nsresult
nsAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  
  
  if (!IsPrimaryForNode())
    return NS_OK;

  
  

  nsEventShell::GetEventAttributes(GetNode(), aAttributes);
 
  
  
  nsAutoString _class;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::_class, _class))
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::_class, _class);

  
  
  
  
  
  
  nsIContent* startContent = mContent;
  while (startContent) {
    nsIDocument* doc = startContent->GetDocument();
    nsIContent* rootContent = nsCoreUtils::GetRoleContent(doc);
    if (!rootContent)
      return NS_OK;

    nsAccUtils::SetLiveContainerAttributes(aAttributes, startContent,
                                           rootContent);

    
    nsCOMPtr<nsISupports> container = doc->GetContainer(); 
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
      do_QueryInterface(container);
    if (!docShellTreeItem)
      break;

    nsCOMPtr<nsIDocShellTreeItem> sameTypeParent;
    docShellTreeItem->GetSameTypeParent(getter_AddRefs(sameTypeParent));
    if (!sameTypeParent || sameTypeParent == docShellTreeItem)
      break;

    nsIDocument *parentDoc = doc->GetParentDocument();
    if (!parentDoc)
      break;

    startContent = parentDoc->FindContentForSubDocument(doc);      
  }

  if (!mContent->IsElement())
    return NS_OK;

  
  nsAutoString tagName;
  mContent->NodeInfo()->GetName(tagName);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::tag, tagName);

  
  nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(mContent);
  if (htmlElement) {
    bool draggable = false;
    htmlElement->GetDraggable(&draggable);
    if (draggable) {
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::draggable,
                             NS_LITERAL_STRING("true"));
    }
  }

  
  
  if (!mContent->GetPrimaryFrame())
    return NS_OK;

  
  nsAutoString value;
  StyleInfo styleInfo(mContent->AsElement(), mDoc->PresShell());

  
  styleInfo.Display(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::display, value);

  
  styleInfo.TextAlign(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textAlign, value);

  
  styleInfo.TextIndent(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textIndent, value);

  
  styleInfo.MarginLeft(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::marginLeft, value);

  
  styleInfo.MarginRight(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::marginRight, value);

  
  styleInfo.MarginTop(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::marginTop, value);

  
  styleInfo.MarginBottom(value);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::marginBottom, value);

  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GroupPosition(PRInt32 *aGroupLevel,
                            PRInt32 *aSimilarItemsInGroup,
                            PRInt32 *aPositionInGroup)
{
  NS_ENSURE_ARG_POINTER(aGroupLevel);
  *aGroupLevel = 0;

  NS_ENSURE_ARG_POINTER(aSimilarItemsInGroup);
  *aSimilarItemsInGroup = 0;

  NS_ENSURE_ARG_POINTER(aPositionInGroup);
  *aPositionInGroup = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsCoreUtils::GetUIntAttr(mContent, nsGkAtoms::aria_level,
                           aGroupLevel);
  nsCoreUtils::GetUIntAttr(mContent, nsGkAtoms::aria_posinset,
                           aPositionInGroup);
  nsCoreUtils::GetUIntAttr(mContent, nsGkAtoms::aria_setsize,
                           aSimilarItemsInGroup);

  
  
  if (State() & states::INVISIBLE)
    return NS_OK;

  
  if (*aGroupLevel == 0) {
    PRInt32 level = GetLevelInternal();
    if (level != 0)
      *aGroupLevel = level;
  }

  
  if (*aSimilarItemsInGroup == 0 || *aPositionInGroup == 0) {
    PRInt32 posInSet = 0, setSize = 0;
    GetPositionAndSizeInternal(&posInSet, &setSize);
    if (posInSet != 0 && setSize != 0) {
      if (*aPositionInGroup == 0)
        *aPositionInGroup = posInSet;

      if (*aSimilarItemsInGroup == 0)
        *aSimilarItemsInGroup = setSize;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetState(PRUint32* aState, PRUint32* aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsAccUtils::To32States(State(), aState, aExtraState);
  return NS_OK;
}

PRUint64
nsAccessible::State()
{
  if (IsDefunct())
    return states::DEFUNCT;

  PRUint64 state = NativeState();
  
  ApplyARIAState(&state);

  
  
  
  if (mRoleMapEntry && !(state & states::SELECTED) &&
      !mContent->AttrValueIs(kNameSpaceID_None,
                             nsGkAtoms::aria_selected,
                             nsGkAtoms::_false, eCaseMatters)) {
    
    
    if (mRoleMapEntry->role == roles::PAGETAB) {
      if (state & states::FOCUSED) {
        state |= states::SELECTED;
      } else {
        
        Relation rel = RelationByType(nsIAccessibleRelation::RELATION_LABEL_FOR);
        nsAccessible* relTarget = nsnull;
        while ((relTarget = rel.Next())) {
          if (relTarget->Role() == roles::PROPERTYPAGE &&
              FocusMgr()->IsFocusWithin(relTarget))
            state |= states::SELECTED;
        }
      }
    } else if (state & states::FOCUSED) {
      nsAccessible* container = nsAccUtils::GetSelectableContainer(this, state);
      if (container &&
          !nsAccUtils::HasDefinedARIAToken(container->GetContent(),
                                           nsGkAtoms::aria_multiselectable)) {
        state |= states::SELECTED;
      }
    }
  }

  const PRUint32 kExpandCollapseStates = states::COLLAPSED | states::EXPANDED;
  if ((state & kExpandCollapseStates) == kExpandCollapseStates) {
    
    
    
    
    
    state &= ~states::COLLAPSED;
  }

  if (!(state & states::UNAVAILABLE)) {
    state |= states::ENABLED | states::SENSITIVE;

    
    
    
    
    nsAccessible* widget = ContainerWidget();
    if (widget && widget->CurrentItem() == this)
      state |= states::ACTIVE;
  }

  if ((state & states::COLLAPSED) || (state & states::EXPANDED))
    state |= states::EXPANDABLE;

  
  
  nsIFrame *frame = GetFrame();
  if (!frame)
    return state;

  const nsStyleDisplay* display = frame->GetStyleDisplay();
  if (display && display->mOpacity == 1.0f &&
      !(state & states::INVISIBLE)) {
    state |= states::OPAQUE1;
  }

  const nsStyleXUL *xulStyle = frame->GetStyleXUL();
  if (xulStyle) {
    
    if (xulStyle->mBoxOrient == NS_STYLE_BOX_ORIENT_VERTICAL) {
      state |= states::VERTICAL;
    }
    else {
      state |= states::HORIZONTAL;
    }
  }

  return state;
}

void
nsAccessible::ApplyARIAState(PRUint64* aState) const
{
  if (!mContent->IsElement())
    return;

  dom::Element* element = mContent->AsElement();

  
  *aState |= nsARIAMap::UniversalStatesFor(element);

  if (mRoleMapEntry) {

    
    
    
    if (mRoleMapEntry->role != roles::NOTHING)
      *aState &= ~states::READONLY;

    if (mContent->HasAttr(kNameSpaceID_None, mContent->GetIDAttributeName())) {
      
      nsIContent *ancestorContent = mContent;
      while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
        if (ancestorContent->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_activedescendant)) {
            
          *aState |= states::FOCUSABLE;
          break;
        }
      }
    }
  }

  if (*aState & states::FOCUSABLE) {
    
    nsIContent *ancestorContent = mContent;
    while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
      if (ancestorContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::aria_disabled,
                                       nsGkAtoms::_true, eCaseMatters)) {
          
        *aState |= states::UNAVAILABLE;
        break;
      }
    }    
  }

  if (!mRoleMapEntry)
    return;

  *aState |= mRoleMapEntry->state;

  if (aria::MapToState(mRoleMapEntry->attributeMap1, element, aState) &&
      aria::MapToState(mRoleMapEntry->attributeMap2, element, aState))
    aria::MapToState(mRoleMapEntry->attributeMap3, element, aState);
}

NS_IMETHODIMP
nsAccessible::GetValue(nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAutoString value;
  Value(value);
  aValue.Assign(value);

  return NS_OK;
}

void
nsAccessible::Value(nsString& aValue)
{
  if (mRoleMapEntry) {
    if (mRoleMapEntry->valueRule == eNoValue)
      return;

    
    
    if (!mContent->GetAttr(kNameSpaceID_None,
                           nsGkAtoms::aria_valuetext, aValue)) {
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_valuenow,
                        aValue);
    }
  }

  if (!aValue.IsEmpty())
    return;

  
  if (nsCoreUtils::IsXLink(mContent)) {
    nsIPresShell* presShell = mDoc->PresShell();
    if (presShell) {
      nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
      presShell->GetLinkLocation(DOMNode, aValue);
    }
  }
}


NS_IMETHODIMP
nsAccessible::GetMaximumValue(double *aMaximumValue)
{
  return GetAttrValue(nsGkAtoms::aria_valuemax, aMaximumValue);
}

NS_IMETHODIMP
nsAccessible::GetMinimumValue(double *aMinimumValue)
{
  return GetAttrValue(nsGkAtoms::aria_valuemin, aMinimumValue);
}

NS_IMETHODIMP
nsAccessible::GetMinimumIncrement(double *aMinIncrement)
{
  NS_ENSURE_ARG_POINTER(aMinIncrement);
  *aMinIncrement = 0;

  
  return NS_OK_NO_ARIA_VALUE;
}

NS_IMETHODIMP
nsAccessible::GetCurrentValue(double *aValue)
{
  return GetAttrValue(nsGkAtoms::aria_valuenow, aValue);
}

NS_IMETHODIMP
nsAccessible::SetCurrentValue(double aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (!mRoleMapEntry || mRoleMapEntry->valueRule == eNoValue)
    return NS_OK_NO_ARIA_VALUE;

  const PRUint32 kValueCannotChange = states::READONLY | states::UNAVAILABLE;

  if (State() & kValueCannotChange)
    return NS_ERROR_FAILURE;

  double minValue = 0;
  if (NS_SUCCEEDED(GetMinimumValue(&minValue)) && aValue < minValue)
    return NS_ERROR_INVALID_ARG;

  double maxValue = 0;
  if (NS_SUCCEEDED(GetMaximumValue(&maxValue)) && aValue > maxValue)
    return NS_ERROR_INVALID_ARG;

  nsAutoString newValue;
  newValue.AppendFloat(aValue);
  return mContent->SetAttr(kNameSpaceID_None,
                           nsGkAtoms::aria_valuenow, newValue, true);
}


NS_IMETHODIMP nsAccessible::SetName(const nsAString& name)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAccessible::GetDefaultKeyBinding(nsAString& aKeyBinding)
{
  aKeyBinding.Truncate();
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  KeyboardShortcut().ToString(aKeyBinding);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetKeyBindings(PRUint8 aActionIndex,
                             nsIDOMDOMStringList **aKeyBindings)
{
  
  NS_ENSURE_TRUE(aActionIndex == 0, NS_ERROR_INVALID_ARG);

  nsAccessibleDOMStringList *keyBindings = new nsAccessibleDOMStringList();
  NS_ENSURE_TRUE(keyBindings, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString defaultKey;
  nsresult rv = GetDefaultKeyBinding(defaultKey);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!defaultKey.IsEmpty())
    keyBindings->Add(defaultKey);

  NS_ADDREF(*aKeyBindings = keyBindings);
  return NS_OK;
}

role
nsAccessible::ARIATransformRole(role aRole)
{
  
  
  if (aRole == roles::PUSHBUTTON) {
    if (nsAccUtils::HasDefinedARIAToken(mContent, nsGkAtoms::aria_pressed)) {
      
      
      return roles::TOGGLE_BUTTON;
    }

    if (mContent->AttrValueIs(kNameSpaceID_None,
                              nsGkAtoms::aria_haspopup,
                              nsGkAtoms::_true,
                              eCaseMatters)) {
      
      return roles::BUTTONMENU;
    }

  } else if (aRole == roles::LISTBOX) {
    
    
    if (mParent && mParent->Role() == roles::COMBOBOX) {
      return roles::COMBOBOX_LIST;

      Relation rel = RelationByType(nsIAccessibleRelation::RELATION_NODE_CHILD_OF);
      nsAccessible* targetAcc = nsnull;
      while ((targetAcc = rel.Next()))
        if (targetAcc->Role() == roles::COMBOBOX)
          return roles::COMBOBOX_LIST;
    }

  } else if (aRole == roles::OPTION) {
    if (mParent && mParent->Role() == roles::COMBOBOX_LIST)
      return roles::COMBOBOX_OPTION;
  }

  return aRole;
}

role
nsAccessible::NativeRole()
{
  return nsCoreUtils::IsXLink(mContent) ? roles::LINK : roles::NOTHING;
}


NS_IMETHODIMP
nsAccessible::GetNumActions(PRUint8* aActionCount)
{
  NS_ENSURE_ARG_POINTER(aActionCount);
  *aActionCount = 0;
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aActionCount = ActionCount();
  return NS_OK;
}

PRUint8
nsAccessible::ActionCount()
{
  return GetActionRule(State()) == eNoAction ? 0 : 1;
}


NS_IMETHODIMP
nsAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRUint64 states = State();
  PRUint32 actionRule = GetActionRule(states);

 switch (actionRule) {
   case eActivateAction:
     aName.AssignLiteral("activate");
     return NS_OK;

   case eClickAction:
     aName.AssignLiteral("click");
     return NS_OK;

   case ePressAction:
     aName.AssignLiteral("press");
     return NS_OK;

   case eCheckUncheckAction:
     if (states & states::CHECKED)
       aName.AssignLiteral("uncheck");
     else if (states & states::MIXED)
       aName.AssignLiteral("cycle");
     else
       aName.AssignLiteral("check");
     return NS_OK;

   case eJumpAction:
     aName.AssignLiteral("jump");
     return NS_OK;

   case eOpenCloseAction:
     if (states & states::COLLAPSED)
       aName.AssignLiteral("open");
     else
       aName.AssignLiteral("close");
     return NS_OK;

   case eSelectAction:
     aName.AssignLiteral("select");
     return NS_OK;

   case eSwitchAction:
     aName.AssignLiteral("switch");
     return NS_OK;
     
   case eSortAction:
     aName.AssignLiteral("sort");
     return NS_OK;
   
   case eExpandAction:
     if (states & states::COLLAPSED)
       aName.AssignLiteral("expand");
     else
       aName.AssignLiteral("collapse");
     return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
nsAccessible::GetActionDescription(PRUint8 aIndex, nsAString& aDescription)
{
  
  nsAutoString name;
  nsresult rv = GetActionName(aIndex, name);
  NS_ENSURE_SUCCESS(rv, rv);

  TranslateString(name, aDescription);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (GetActionRule(State()) != eNoAction) {
    DoCommand();
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsAccessible::GetHelp(nsAString& _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsIContent*
nsAccessible::GetAtomicRegion() const
{
  nsIContent *loopContent = mContent;
  nsAutoString atomic;
  while (loopContent && !loopContent->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_atomic, atomic))
    loopContent = loopContent->GetParent();

  return atomic.EqualsLiteral("true") ? loopContent : nsnull;
}


NS_IMETHODIMP
nsAccessible::GetRelationByType(PRUint32 aType,
                                nsIAccessibleRelation** aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  Relation rel = RelationByType(aType);
  NS_ADDREF(*aRelation = new nsAccessibleRelation(aType, &rel));
  return *aRelation ? NS_OK : NS_ERROR_FAILURE;
}

Relation
nsAccessible::RelationByType(PRUint32 aType)
{
  
  
  switch (aType) {
    case nsIAccessibleRelation::RELATION_LABEL_FOR: {
      Relation rel(new RelatedAccIterator(Document(), mContent,
                                          nsGkAtoms::aria_labelledby));
      if (mContent->Tag() == nsGkAtoms::label)
        rel.AppendIter(new IDRefsIterator(mDoc, mContent, mContent->IsHTML() ?
                                          nsGkAtoms::_for :
                                          nsGkAtoms::control));

      return rel;
    }
    case nsIAccessibleRelation::RELATION_LABELLED_BY: {
      Relation rel(new IDRefsIterator(mDoc, mContent,
                                      nsGkAtoms::aria_labelledby));
      if (mContent->IsHTML()) {
        rel.AppendIter(new HTMLLabelIterator(Document(), this));
      } else if (mContent->IsXUL()) {
        rel.AppendIter(new XULLabelIterator(Document(), mContent));
      }

      return rel;
    }
    case nsIAccessibleRelation::RELATION_DESCRIBED_BY: {
      Relation rel(new IDRefsIterator(mDoc, mContent,
                                      nsGkAtoms::aria_describedby));
      if (mContent->IsXUL())
        rel.AppendIter(new XULDescriptionIterator(Document(), mContent));

      return rel;
    }
    case nsIAccessibleRelation::RELATION_DESCRIPTION_FOR: {
      Relation rel(new RelatedAccIterator(Document(), mContent,
                                          nsGkAtoms::aria_describedby));

      
      
      
      if (mContent->Tag() == nsGkAtoms::description &&
          mContent->IsXUL())
        rel.AppendIter(new IDRefsIterator(mDoc, mContent,
                                          nsGkAtoms::control));

      return rel;
    }
    case nsIAccessibleRelation::RELATION_NODE_CHILD_OF: {
      Relation rel(new RelatedAccIterator(Document(), mContent,
                                          nsGkAtoms::aria_owns));
      
      
      
      if (mRoleMapEntry && (mRoleMapEntry->role == roles::OUTLINEITEM || 
                            mRoleMapEntry->role == roles::ROW)) {
        AccGroupInfo* groupInfo = GetGroupInfo();
        if (!groupInfo)
          return rel;

        rel.AppendTarget(groupInfo->ConceptualParent());
      }

      
      
      
      
      
      
      nsIFrame *frame = GetFrame();
      if (frame) {
        nsIView *view = frame->GetViewExternal();
        if (view) {
          nsIScrollableFrame *scrollFrame = do_QueryFrame(frame);
          if (scrollFrame || view->GetWidget() || !frame->GetParent())
            rel.AppendTarget(Parent());
        }
      }

      return rel;
    }
    case nsIAccessibleRelation::RELATION_CONTROLLED_BY:
      return Relation(new RelatedAccIterator(Document(), mContent,
                                             nsGkAtoms::aria_controls));
    case nsIAccessibleRelation::RELATION_CONTROLLER_FOR: {
      Relation rel(new IDRefsIterator(mDoc, mContent,
                                      nsGkAtoms::aria_controls));
      rel.AppendIter(new HTMLOutputIterator(Document(), mContent));
      return rel;
    }
    case nsIAccessibleRelation::RELATION_FLOWS_TO:
      return Relation(new IDRefsIterator(mDoc, mContent,
                                         nsGkAtoms::aria_flowto));
    case nsIAccessibleRelation::RELATION_FLOWS_FROM:
      return Relation(new RelatedAccIterator(Document(), mContent,
                                             nsGkAtoms::aria_flowto));
    case nsIAccessibleRelation::RELATION_DEFAULT_BUTTON: {
      if (mContent->IsHTML()) {
        
        nsCOMPtr<nsIFormControl> control(do_QueryInterface(mContent));
        if (control) {
          nsCOMPtr<nsIForm> form(do_QueryInterface(control->GetFormElement()));
          if (form) {
            nsCOMPtr<nsIContent> formContent =
              do_QueryInterface(form->GetDefaultSubmitElement());
            return Relation(mDoc, formContent);
          }
        }
      } else {
        
        nsCOMPtr<nsIDOMXULDocument> xulDoc =
          do_QueryInterface(mContent->OwnerDoc());
        nsCOMPtr<nsIDOMXULButtonElement> buttonEl;
        if (xulDoc) {
          nsCOMPtr<nsIDOMNodeList> possibleDefaultButtons;
          xulDoc->GetElementsByAttribute(NS_LITERAL_STRING("default"),
                                         NS_LITERAL_STRING("true"),
                                         getter_AddRefs(possibleDefaultButtons));
          if (possibleDefaultButtons) {
            PRUint32 length;
            possibleDefaultButtons->GetLength(&length);
            nsCOMPtr<nsIDOMNode> possibleButton;
            
            for (PRUint32 count = 0; count < length && !buttonEl; count ++) {
              possibleDefaultButtons->Item(count, getter_AddRefs(possibleButton));
              buttonEl = do_QueryInterface(possibleButton);
            }
          }
          if (!buttonEl) { 
            nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(xulDoc));
            if (xblDoc) {
              nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(xulDoc);
              NS_ASSERTION(domDoc, "No DOM document");
              nsCOMPtr<nsIDOMElement> rootEl;
              domDoc->GetDocumentElement(getter_AddRefs(rootEl));
              if (rootEl) {
                nsCOMPtr<nsIDOMElement> possibleButtonEl;
                xblDoc->GetAnonymousElementByAttribute(rootEl,
                                                      NS_LITERAL_STRING("default"),
                                                      NS_LITERAL_STRING("true"),
                                                      getter_AddRefs(possibleButtonEl));
                buttonEl = do_QueryInterface(possibleButtonEl);
              }
            }
          }
          nsCOMPtr<nsIContent> relatedContent(do_QueryInterface(buttonEl));
          return Relation(mDoc, relatedContent);
        }
      }
      return Relation();
    }
    case nsIAccessibleRelation::RELATION_MEMBER_OF:
      return Relation(mDoc, GetAtomicRegion());
    case nsIAccessibleRelation::RELATION_SUBWINDOW_OF:
    case nsIAccessibleRelation::RELATION_EMBEDS:
    case nsIAccessibleRelation::RELATION_EMBEDDED_BY:
    case nsIAccessibleRelation::RELATION_POPUP_FOR:
    case nsIAccessibleRelation::RELATION_PARENT_WINDOW_OF:
    default:
    return Relation();
  }
}

NS_IMETHODIMP
nsAccessible::GetRelations(nsIArray **aRelations)
{
  NS_ENSURE_ARG_POINTER(aRelations);
  *aRelations = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIMutableArray> relations = do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_TRUE(relations, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 relType = nsIAccessibleRelation::RELATION_FIRST;
       relType < nsIAccessibleRelation::RELATION_LAST;
       ++relType) {

    nsCOMPtr<nsIAccessibleRelation> relation;
    nsresult rv = GetRelationByType(relType, getter_AddRefs(relation));

    if (NS_SUCCEEDED(rv) && relation) {
      PRUint32 targets = 0;
      relation->GetTargetsCount(&targets);
      if (targets)
        relations->AppendElement(relation, false);
    }
  }

  NS_ADDREF(*aRelations = relations);
  return NS_OK;
}


NS_IMETHODIMP nsAccessible::ExtendSelection()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsAccessible::GetNativeInterface(void **aOutAccessible)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsAccessible::DoCommand(nsIContent *aContent, PRUint32 aActionIndex)
{
  nsIContent* content = aContent ? aContent : mContent.get();
  NS_DISPATCH_RUNNABLEMETHOD_ARG2(DispatchClickEvent, this, content,
                                  aActionIndex);
}

void
nsAccessible::DispatchClickEvent(nsIContent *aContent, PRUint32 aActionIndex)
{
  if (IsDefunct())
    return;

  nsIPresShell* presShell = mDoc->PresShell();

  
  presShell->ScrollContentIntoView(aContent,
                                   nsIPresShell::ScrollAxis(),
                                   nsIPresShell::ScrollAxis(),
                                   nsIPresShell::SCROLL_OVERFLOW_HIDDEN);

  
  bool res = nsCoreUtils::DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, presShell,
                                               aContent);
  if (!res)
    return;

  nsCoreUtils::DispatchMouseEvent(NS_MOUSE_BUTTON_UP, presShell, aContent);
}

NS_IMETHODIMP
nsAccessible::ScrollTo(PRUint32 aHow)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCoreUtils::ScrollTo(mDoc->PresShell(), mContent, aHow);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::ScrollToPoint(PRUint32 aCoordinateType, PRInt32 aX, PRInt32 aY)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIntPoint coords;
  nsresult rv = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordinateType,
                                                  this, &coords);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame *parentFrame = frame;
  while ((parentFrame = parentFrame->GetParent()))
    nsCoreUtils::ScrollFrameToPoint(parentFrame, frame, coords);

  return NS_OK;
}


NS_IMETHODIMP nsAccessible::GetSelectedChildren(nsIArray **aSelectedAccessibles)
{
  NS_ENSURE_ARG_POINTER(aSelectedAccessibles);
  *aSelectedAccessibles = nsnull;

  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIArray> items = SelectedItems();
  if (items) {
    PRUint32 length = 0;
    items->GetLength(&length);
    if (length)
      items.swap(*aSelectedAccessibles);
  }

  return NS_OK;
}


NS_IMETHODIMP nsAccessible::RefSelection(PRInt32 aIndex, nsIAccessible **aSelected)
{
  NS_ENSURE_ARG_POINTER(aSelected);
  *aSelected = nsnull;

  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  *aSelected = GetSelectedItem(aIndex);
  if (*aSelected) {
    NS_ADDREF(*aSelected);
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  NS_ENSURE_ARG_POINTER(aSelectionCount);
  *aSelectionCount = 0;

  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  *aSelectionCount = SelectedItemCount();
  return NS_OK;
}

NS_IMETHODIMP nsAccessible::AddChildToSelection(PRInt32 aIndex)
{
  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  return aIndex >= 0 && AddItemToSelection(aIndex) ?
    NS_OK : NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsAccessible::RemoveChildFromSelection(PRInt32 aIndex)
{
  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  return aIndex >=0 && RemoveItemFromSelection(aIndex) ?
    NS_OK : NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP nsAccessible::IsChildSelected(PRInt32 aIndex, bool *aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aIsSelected);
  *aIsSelected = false;

  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(aIndex >= 0, NS_ERROR_FAILURE);

  *aIsSelected = IsItemSelected(aIndex);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::ClearSelection()
{
  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  UnselectAll();
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::SelectAllSelection(bool* aIsMultiSelect)
{
  NS_ENSURE_ARG_POINTER(aIsMultiSelect);
  *aIsMultiSelect = false;

  if (IsDefunct() || !IsSelect())
    return NS_ERROR_FAILURE;

  *aIsMultiSelect = SelectAll();
  return NS_OK;
}







NS_IMETHODIMP
nsAccessible::GetAnchorCount(PRInt32 *aAnchorCount)
{
  NS_ENSURE_ARG_POINTER(aAnchorCount);
  *aAnchorCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aAnchorCount = AnchorCount();
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetStartIndex(PRInt32 *aStartIndex)
{
  NS_ENSURE_ARG_POINTER(aStartIndex);
  *aStartIndex = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aStartIndex = StartOffset();
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetEndIndex(PRInt32 *aEndIndex)
{
  NS_ENSURE_ARG_POINTER(aEndIndex);
  *aEndIndex = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aEndIndex = EndOffset();
  return NS_OK;
}

NS_IMETHODIMP
nsAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aIndex < 0 || aIndex >= static_cast<PRInt32>(AnchorCount()))
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<nsIURI>(AnchorURIAt(aIndex)).forget(aURI);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetAnchor(PRInt32 aIndex, nsIAccessible** aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aIndex < 0 || aIndex >= static_cast<PRInt32>(AnchorCount()))
    return NS_ERROR_INVALID_ARG;

  NS_IF_ADDREF(*aAccessible = AnchorAt(aIndex));
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetValid(bool *aValid)
{
  NS_ENSURE_ARG_POINTER(aValid);
  *aValid = false;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aValid = IsLinkValid();
  return NS_OK;
}


NS_IMETHODIMP
nsAccessible::GetSelected(bool *aSelected)
{
  NS_ENSURE_ARG_POINTER(aSelected);
  *aSelected = false;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aSelected = IsLinkSelected();
  return NS_OK;

}

void
nsAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                           PRUint32 aLength)
{
  
  
  if (aStartOffset != 0 || aLength == 0)
    return;

  nsIFrame *frame = GetFrame();
  if (!frame)
    return;

  if (frame->GetType() == nsGkAtoms::brFrame) {
    aText += kForcedNewLineChar;
  } else if (nsAccUtils::MustPrune(Parent())) {
    
    
    
    aText += kImaginaryEmbeddedObjectChar;
  } else {
    aText += kEmbeddedObjectChar;
  }
}




void
nsAccessible::Shutdown()
{
  
  
  mFlags |= eIsDefunct;

  InvalidateChildren();
  if (mParent)
    mParent->RemoveChild(this);

  nsAccessNodeWrap::Shutdown();
}




nsresult
nsAccessible::GetARIAName(nsAString& aName)
{
  nsAutoString label;

  
  nsresult rv = nsTextEquivUtils::
    GetTextEquivFromIDRefs(this, nsGkAtoms::aria_labelledby, label);
  if (NS_SUCCEEDED(rv)) {
    label.CompressWhitespace();
    aName = label;
  }

  if (label.IsEmpty() &&
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::aria_label,
                        label)) {
    label.CompressWhitespace();
    aName = label;
  }
  
  return NS_OK;
}

nsresult
nsAccessible::GetNameInternal(nsAString& aName)
{
  if (mContent->IsHTML())
    return GetHTMLName(aName);

  if (mContent->IsXUL())
    return GetXULName(aName);

  return NS_OK;
}


void
nsAccessible::BindToParent(nsAccessible* aParent, PRUint32 aIndexInParent)
{
  NS_PRECONDITION(aParent, "This method isn't used to set null parent!");

  if (mParent) {
    if (mParent != aParent) {
      NS_ERROR("Adopting child!");
      mParent->RemoveChild(this);
    } else {
      NS_ERROR("Binding to the same parent!");
      return;
    }
  }

  mParent = aParent;
  mIndexInParent = aIndexInParent;
}

void
nsAccessible::UnbindFromParent()
{
  mParent = nsnull;
  mIndexInParent = -1;
  mIndexOfEmbeddedChild = -1;
  mGroupInfo = nsnull;
}

void
nsAccessible::InvalidateChildren()
{
  PRInt32 childCount = mChildren.Length();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible* child = mChildren.ElementAt(childIdx);
    child->UnbindFromParent();
  }

  mEmbeddedObjCollector = nsnull;
  mChildren.Clear();
  SetChildrenFlag(eChildrenUninitialized);
}

bool
nsAccessible::AppendChild(nsAccessible* aChild)
{
  if (!aChild)
    return false;

  if (!mChildren.AppendElement(aChild))
    return false;

  if (!nsAccUtils::IsEmbeddedObject(aChild))
    SetChildrenFlag(eMixedChildren);

  aChild->BindToParent(this, mChildren.Length() - 1);
  return true;
}

bool
nsAccessible::InsertChildAt(PRUint32 aIndex, nsAccessible* aChild)
{
  if (!aChild)
    return false;

  if (!mChildren.InsertElementAt(aIndex, aChild))
    return false;

  for (PRUint32 idx = aIndex + 1; idx < mChildren.Length(); idx++) {
    NS_ASSERTION(mChildren[idx]->mIndexInParent == idx - 1, "Accessible child index doesn't match");
    mChildren[idx]->mIndexInParent = idx;
  }

  if (nsAccUtils::IsText(aChild))
    SetChildrenFlag(eMixedChildren);

  mEmbeddedObjCollector = nsnull;

  aChild->BindToParent(this, aIndex);
  return true;
}

bool
nsAccessible::RemoveChild(nsAccessible* aChild)
{
  if (!aChild)
    return false;

  if (aChild->mParent != this || aChild->mIndexInParent == -1)
    return false;

  PRUint32 index = static_cast<PRUint32>(aChild->mIndexInParent);
  if (index >= mChildren.Length() || mChildren[index] != aChild) {
    NS_ERROR("Child is bound to parent but parent hasn't this child at its index!");
    aChild->UnbindFromParent();
    return false;
  }

  for (PRUint32 idx = index + 1; idx < mChildren.Length(); idx++) {
    NS_ASSERTION(mChildren[idx]->mIndexInParent == idx, "Accessible child index doesn't match");
    mChildren[idx]->mIndexInParent = idx - 1;
  }

  aChild->UnbindFromParent();
  mChildren.RemoveElementAt(index);
  mEmbeddedObjCollector = nsnull;

  return true;
}

nsAccessible*
nsAccessible::GetChildAt(PRUint32 aIndex)
{
  nsAccessible *child = mChildren.SafeElementAt(aIndex, nsnull);
  if (!child)
    return nsnull;

#ifdef DEBUG
  nsAccessible* realParent = child->mParent;
  NS_ASSERTION(!realParent || realParent == this,
               "Two accessibles have the same first child accessible!");
#endif

  return child;
}

PRInt32
nsAccessible::GetChildCount()
{
  return mChildren.Length();
}

PRInt32
nsAccessible::GetIndexOf(nsAccessible* aChild)
{
  return (aChild->mParent != this) ? -1 : aChild->IndexInParent();
}

PRInt32
nsAccessible::IndexInParent() const
{
  return mIndexInParent;
}

PRInt32
nsAccessible::GetEmbeddedChildCount()
{
  if (IsChildrenFlag(eMixedChildren)) {
    if (!mEmbeddedObjCollector)
      mEmbeddedObjCollector = new EmbeddedObjCollector(this);
    return mEmbeddedObjCollector ? mEmbeddedObjCollector->Count() : -1;
  }

  return GetChildCount();
}

nsAccessible*
nsAccessible::GetEmbeddedChildAt(PRUint32 aIndex)
{
  if (IsChildrenFlag(eMixedChildren)) {
    if (!mEmbeddedObjCollector)
      mEmbeddedObjCollector = new EmbeddedObjCollector(this);
    return mEmbeddedObjCollector ?
      mEmbeddedObjCollector->GetAccessibleAt(aIndex) : nsnull;
  }

  return GetChildAt(aIndex);
}

PRInt32
nsAccessible::GetIndexOfEmbeddedChild(nsAccessible* aChild)
{
  if (IsChildrenFlag(eMixedChildren)) {
    if (!mEmbeddedObjCollector)
      mEmbeddedObjCollector = new EmbeddedObjCollector(this);
    return mEmbeddedObjCollector ?
      mEmbeddedObjCollector->GetIndexAt(aChild) : -1;
  }

  return GetIndexOf(aChild);
}




bool
nsAccessible::IsLink()
{
  
  
  return mParent && mParent->IsHyperText() && nsAccUtils::IsEmbeddedObject(this);
}

PRUint32
nsAccessible::StartOffset()
{
  NS_PRECONDITION(IsLink(), "StartOffset is called not on hyper link!");

  nsHyperTextAccessible* hyperText = mParent ? mParent->AsHyperText() : nsnull;
  return hyperText ? hyperText->GetChildOffset(this) : 0;
}

PRUint32
nsAccessible::EndOffset()
{
  NS_PRECONDITION(IsLink(), "EndOffset is called on not hyper link!");

  nsHyperTextAccessible* hyperText = mParent ? mParent->AsHyperText() : nsnull;
  return hyperText ? (hyperText->GetChildOffset(this) + 1) : 0;
}

bool
nsAccessible::IsLinkSelected()
{
  NS_PRECONDITION(IsLink(),
                  "IsLinkSelected() called on something that is not a hyper link!");
  return FocusMgr()->IsFocused(this);
}

PRUint32
nsAccessible::AnchorCount()
{
  NS_PRECONDITION(IsLink(), "AnchorCount is called on not hyper link!");
  return 1;
}

nsAccessible*
nsAccessible::AnchorAt(PRUint32 aAnchorIndex)
{
  NS_PRECONDITION(IsLink(), "GetAnchor is called on not hyper link!");
  return aAnchorIndex == 0 ? this : nsnull;
}

already_AddRefed<nsIURI>
nsAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  NS_PRECONDITION(IsLink(), "AnchorURIAt is called on not hyper link!");

  if (aAnchorIndex != 0)
    return nsnull;

  
  if (nsCoreUtils::IsXLink(mContent)) {
    nsAutoString href;
    mContent->GetAttr(kNameSpaceID_XLink, nsGkAtoms::href, href);

    nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
    nsCOMPtr<nsIDocument> document = mContent->OwnerDoc();
    nsIURI* anchorURI = nsnull;
    NS_NewURI(&anchorURI, href,
              document ? document->GetDocumentCharacterSet().get() : nsnull,
              baseURI);
    return anchorURI;
  }

  return nsnull;
}





bool
nsAccessible::IsSelect()
{
  
  
  
  

  return mRoleMapEntry &&
    (mRoleMapEntry->attributeMap1 == aria::eARIAMultiSelectable ||
     mRoleMapEntry->attributeMap2 == aria::eARIAMultiSelectable ||
     mRoleMapEntry->attributeMap3 == aria::eARIAMultiSelectable);
}

already_AddRefed<nsIArray>
nsAccessible::SelectedItems()
{
  nsCOMPtr<nsIMutableArray> selectedItems = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!selectedItems)
    return nsnull;

  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsIAccessible* selected = nsnull;
  while ((selected = iter.Next()))
    selectedItems->AppendElement(selected, false);

  nsIMutableArray* items = nsnull;
  selectedItems.forget(&items);
  return items;
}

PRUint32
nsAccessible::SelectedItemCount()
{
  PRUint32 count = 0;
  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsAccessible* selected = nsnull;
  while ((selected = iter.Next()))
    ++count;

  return count;
}

nsAccessible*
nsAccessible::GetSelectedItem(PRUint32 aIndex)
{
  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  nsAccessible* selected = nsnull;

  PRUint32 index = 0;
  while ((selected = iter.Next()) && index < aIndex)
    index++;

  return selected;
}

bool
nsAccessible::IsItemSelected(PRUint32 aIndex)
{
  PRUint32 index = 0;
  AccIterator iter(this, filters::GetSelectable, AccIterator::eTreeNav);
  nsAccessible* selected = nsnull;
  while ((selected = iter.Next()) && index < aIndex)
    index++;

  return selected &&
    selected->State() & states::SELECTED;
}

bool
nsAccessible::AddItemToSelection(PRUint32 aIndex)
{
  PRUint32 index = 0;
  AccIterator iter(this, filters::GetSelectable, AccIterator::eTreeNav);
  nsAccessible* selected = nsnull;
  while ((selected = iter.Next()) && index < aIndex)
    index++;

  if (selected)
    selected->SetSelected(true);

  return static_cast<bool>(selected);
}

bool
nsAccessible::RemoveItemFromSelection(PRUint32 aIndex)
{
  PRUint32 index = 0;
  AccIterator iter(this, filters::GetSelectable, AccIterator::eTreeNav);
  nsAccessible* selected = nsnull;
  while ((selected = iter.Next()) && index < aIndex)
    index++;

  if (selected)
    selected->SetSelected(false);

  return static_cast<bool>(selected);
}

bool
nsAccessible::SelectAll()
{
  bool success = false;
  nsAccessible* selectable = nsnull;

  AccIterator iter(this, filters::GetSelectable, AccIterator::eTreeNav);
  while((selectable = iter.Next())) {
    success = true;
    selectable->SetSelected(true);
  }
  return success;
}

bool
nsAccessible::UnselectAll()
{
  bool success = false;
  nsAccessible* selected = nsnull;

  AccIterator iter(this, filters::GetSelected, AccIterator::eTreeNav);
  while ((selected = iter.Next())) {
    success = true;
    selected->SetSelected(false);
  }
  return success;
}




bool
nsAccessible::IsWidget() const
{
  return false;
}

bool
nsAccessible::IsActiveWidget() const
{
  return FocusMgr()->IsFocused(this);
}

bool
nsAccessible::AreItemsOperable() const
{
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::aria_activedescendant);
}

nsAccessible*
nsAccessible::CurrentItem()
{
  
  
  
  
  nsAutoString id;
  if (mContent->GetAttr(kNameSpaceID_None,
                        nsGkAtoms::aria_activedescendant, id)) {
    nsIDocument* DOMDoc = mContent->OwnerDoc();
    dom::Element* activeDescendantElm = DOMDoc->GetElementById(id);
    if (activeDescendantElm) {
      nsDocAccessible* document = Document();
      if (document)
        return document->GetAccessible(activeDescendantElm);
    }
  }
  return nsnull;
}

void
nsAccessible::SetCurrentItem(nsAccessible* aItem)
{
  nsIAtom* id = aItem->GetContent()->GetID();
  if (id) {
    nsAutoString idStr;
    id->ToString(idStr);
    mContent->SetAttr(kNameSpaceID_None,
                      nsGkAtoms::aria_activedescendant, idStr, true);
  }
}

nsAccessible*
nsAccessible::ContainerWidget() const
{
  if (HasARIARole() && mContent->HasID()) {
    for (nsAccessible* parent = Parent(); parent; parent = parent->Parent()) {
      nsIContent* parentContent = parent->GetContent();
      if (parentContent &&
        parentContent->HasAttr(kNameSpaceID_None,
                               nsGkAtoms::aria_activedescendant)) {
        return parent;
      }

      
      if (parent->IsDocumentNode())
        break;
    }
  }
  return nsnull;
}




void
nsAccessible::CacheChildren()
{
  nsDocAccessible* doc = Document();
  NS_ENSURE_TRUE(doc,);

  nsAccTreeWalker walker(doc, mContent, CanHaveAnonChildren());

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild()) && AppendChild(child));
}

void
nsAccessible::TestChildCache(nsAccessible* aCachedChild) const
{
#ifdef DEBUG
  PRInt32 childCount = mChildren.Length();
  if (childCount == 0) {
    NS_ASSERTION(IsChildrenFlag(eChildrenUninitialized),
                 "No children but initialized!");
    return;
  }

  nsAccessible *child = nsnull;
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    child = mChildren[childIdx];
    if (child == aCachedChild)
      break;
  }

  NS_ASSERTION(child == aCachedChild,
               "[TestChildCache] cached accessible wasn't found. Wrong accessible tree!");  
#endif
}


bool
nsAccessible::EnsureChildren()
{
  if (IsDefunct()) {
    SetChildrenFlag(eChildrenUninitialized);
    return true;
  }

  if (!IsChildrenFlag(eChildrenUninitialized))
    return false;

  
  SetChildrenFlag(eEmbeddedChildren); 

  CacheChildren();
  return false;
}

nsAccessible*
nsAccessible::GetSiblingAtOffset(PRInt32 aOffset, nsresult* aError) const
{
  if (!mParent || mIndexInParent == -1) {
    if (aError)
      *aError = NS_ERROR_UNEXPECTED;

    return nsnull;
  }

  if (aError && mIndexInParent + aOffset >= mParent->GetChildCount()) {
    *aError = NS_OK; 
    return nsnull;
  }

  nsAccessible* child = mParent->GetChildAt(mIndexInParent + aOffset);
  if (aError && !child)
    *aError = NS_ERROR_UNEXPECTED;

  return child;
}

nsAccessible *
nsAccessible::GetFirstAvailableAccessible(nsINode *aStartNode) const
{
  nsAccessible* accessible = mDoc->GetAccessible(aStartNode);
  if (accessible)
    return accessible;

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aStartNode->OwnerDoc());
  NS_ENSURE_TRUE(domDoc, nsnull);

  nsCOMPtr<nsIDOMNode> currentNode = do_QueryInterface(aStartNode);
  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(GetNode());
  nsCOMPtr<nsIDOMTreeWalker> walker;
  domDoc->CreateTreeWalker(rootNode,
                           nsIDOMNodeFilter::SHOW_ELEMENT | nsIDOMNodeFilter::SHOW_TEXT,
                           nsnull, false, getter_AddRefs(walker));
  NS_ENSURE_TRUE(walker, nsnull);

  walker->SetCurrentNode(currentNode);
  while (true) {
    walker->NextNode(getter_AddRefs(currentNode));
    if (!currentNode)
      return nsnull;

    nsCOMPtr<nsINode> node(do_QueryInterface(currentNode));
    nsAccessible* accessible = mDoc->GetAccessible(node);
    if (accessible)
      return accessible;
  }

  return nsnull;
}

nsresult
nsAccessible::GetAttrValue(nsIAtom *aProperty, double *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;  

 if (!mRoleMapEntry || mRoleMapEntry->valueRule == eNoValue)
    return NS_OK_NO_ARIA_VALUE;

  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, aProperty, attrValue);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  PRInt32 error = NS_OK;
  double value = attrValue.ToDouble(&error);
  if (NS_SUCCEEDED(error))
    *aValue = value;

  return NS_OK;
}

PRUint32
nsAccessible::GetActionRule(PRUint64 aStates)
{
  if (aStates & states::UNAVAILABLE)
    return eNoAction;
  
  
  if (nsCoreUtils::IsXLink(mContent))
    return eJumpAction;

  
  if (mContent->IsXUL())
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::popup))
      return eClickAction;

  
  bool isOnclick = nsCoreUtils::HasClickListener(mContent);

  if (isOnclick)
    return eClickAction;
  
  
  if (mRoleMapEntry &&
      mRoleMapEntry->actionRule != eNoAction)
    return mRoleMapEntry->actionRule;

  
  if (nsAccUtils::HasDefinedARIAToken(mContent,
                                      nsGkAtoms::aria_expanded))
    return eExpandAction;

  return eNoAction;
}

AccGroupInfo*
nsAccessible::GetGroupInfo()
{
  if (mGroupInfo)
    return mGroupInfo;

  mGroupInfo = AccGroupInfo::CreateGroupInfo(this);
  return mGroupInfo;
}

void
nsAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet, PRInt32 *aSetSize)
{
  AccGroupInfo* groupInfo = GetGroupInfo();
  if (groupInfo) {
    *aPosInSet = groupInfo->PosInSet();
    *aSetSize = groupInfo->SetSize();
  }
}

PRInt32
nsAccessible::GetLevelInternal()
{
  PRInt32 level = nsAccUtils::GetDefaultLevel(this);

  if (!IsBoundToParent())
    return level;

  roles::Role role = Role();
  if (role == roles::OUTLINEITEM) {
    
    
    
    level = 1;

    nsAccessible* parent = this;
    while ((parent = parent->Parent())) {
      roles::Role parentRole = parent->Role();

      if (parentRole == roles::OUTLINE)
        break;
      if (parentRole == roles::GROUPING)
        ++ level;

    }

  } else if (role == roles::LISTITEM) {
    
    
    
    

    
    level = 0;
    nsAccessible* parent = this;
    while ((parent = parent->Parent())) {
      roles::Role parentRole = parent->Role();

      if (parentRole == roles::LISTITEM)
        ++ level;
      else if (parentRole != roles::LIST)
        break;

    }

    if (level == 0) {
      
      
      parent = Parent();
      PRInt32 siblingCount = parent->GetChildCount();
      for (PRInt32 siblingIdx = 0; siblingIdx < siblingCount; siblingIdx++) {
        nsAccessible* sibling = parent->GetChildAt(siblingIdx);

        nsAccessible* siblingChild = sibling->LastChild();
        if (siblingChild && siblingChild->Role() == roles::LIST)
          return 1;
      }
    } else {
      ++ level; 
    }
  }

  return level;
}





void
KeyBinding::ToPlatformFormat(nsAString& aValue) const
{
  nsCOMPtr<nsIStringBundle> keyStringBundle;
  nsCOMPtr<nsIStringBundleService> stringBundleService =
      mozilla::services::GetStringBundleService();
  if (stringBundleService)
    stringBundleService->CreateBundle(
      "chrome://global-platform/locale/platformKeys.properties",
      getter_AddRefs(keyStringBundle));

  if (!keyStringBundle)
    return;

  nsAutoString separator;
  keyStringBundle->GetStringFromName(NS_LITERAL_STRING("MODIFIER_SEPARATOR").get(),
                                     getter_Copies(separator));

  nsAutoString modifierName;
  if (mModifierMask & kControl) {
    keyStringBundle->GetStringFromName(NS_LITERAL_STRING("VK_CONTROL").get(),
                                       getter_Copies(modifierName));

    aValue.Append(modifierName);
    aValue.Append(separator);
  }

  if (mModifierMask & kAlt) {
    keyStringBundle->GetStringFromName(NS_LITERAL_STRING("VK_ALT").get(),
                                       getter_Copies(modifierName));

    aValue.Append(modifierName);
    aValue.Append(separator);
  }

  if (mModifierMask & kShift) {
    keyStringBundle->GetStringFromName(NS_LITERAL_STRING("VK_SHIFT").get(),
                                       getter_Copies(modifierName));

    aValue.Append(modifierName);
    aValue.Append(separator);
  }

  if (mModifierMask & kMeta) {
    keyStringBundle->GetStringFromName(NS_LITERAL_STRING("VK_META").get(),
                                       getter_Copies(modifierName));

    aValue.Append(modifierName);
    aValue.Append(separator);
  }

  aValue.Append(mKey);
}

void
KeyBinding::ToAtkFormat(nsAString& aValue) const
{
  nsAutoString modifierName;
  if (mModifierMask & kControl)
    aValue.Append(NS_LITERAL_STRING("<Control>"));

  if (mModifierMask & kAlt)
    aValue.Append(NS_LITERAL_STRING("<Alt>"));

  if (mModifierMask & kShift)
    aValue.Append(NS_LITERAL_STRING("<Shift>"));

  if (mModifierMask & kMeta)
      aValue.Append(NS_LITERAL_STRING("<Meta>"));

  aValue.Append(mKey);
}
