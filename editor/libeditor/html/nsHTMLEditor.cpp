




#include "mozilla/DebugOnly.h"
#include "mozilla/EventStates.h"
#include "mozilla/TextEvents.h"

#include "nsCRT.h"

#include "nsUnicharUtils.h"

#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"

#include "nsHTMLEditorEventListener.h"
#include "TypeInState.h"

#include "nsHTMLURIRefObject.h"

#include "nsIDOMText.h"
#include "nsIDOMMozNamedAttrMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDocumentInlines.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsISelectionController.h"
#include "nsIDOMHTMLDocument.h"
#include "nsILinkHandler.h"
#include "nsIInlineSpellChecker.h"

#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"
#include "nsIDOMStyleSheet.h"

#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMRange.h"
#include "nsISupportsArray.h"
#include "nsContentUtils.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "SetDocTitleTxn.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"


#include "nsIURI.h"
#include "nsNetUtil.h"


#include "nsStyleSheetTxns.h"


#include "TextEditorTest.h"
#include "nsEditorUtils.h"
#include "nsWSRunObject.h"
#include "nsGkAtoms.h"
#include "nsIWidget.h"

#include "nsIFrame.h"
#include "nsIParserService.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/HTMLBodyElement.h"
#include "nsTextFragment.h"
#include "nsContentList.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::widget;


static char hrefText[] = "href";
static char anchorTxt[] = "anchor";
static char namedanchorText[] = "namedanchor";

#define IsLinkTag(s) (s.EqualsIgnoreCase(hrefText))
#define IsNamedAnchorTag(s) (s.EqualsIgnoreCase(anchorTxt) || s.EqualsIgnoreCase(namedanchorText))

nsHTMLEditor::nsHTMLEditor()
: nsPlaintextEditor()
, mCRInParagraphCreatesParagraph(false)
, mSelectedCellIndex(0)
, mIsObjectResizingEnabled(true)
, mIsResizing(false)
, mIsAbsolutelyPositioningEnabled(true)
, mResizedObjectIsAbsolutelyPositioned(false)
, mGrabberClicked(false)
, mIsMoving(false)
, mSnapToGridEnabled(false)
, mIsInlineTableEditingEnabled(true)
, mInfoXIncrement(20)
, mInfoYIncrement(20)
, mGridSize(0)
{
} 

nsHTMLEditor::~nsHTMLEditor()
{
  
  
  
  nsCOMPtr<nsIEditActionListener> mListener = do_QueryInterface(mRules);
  RemoveEditActionListener(mListener);

  
  
  nsCOMPtr<nsISelection>selection;
  nsresult result = GetSelection(getter_AddRefs(selection));
  
  if (NS_SUCCEEDED(result) && selection) 
  {
    nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
    nsCOMPtr<nsISelectionListener>listener;
    listener = do_QueryInterface(mTypeInState);
    if (listener)
    {
      selPriv->RemoveSelectionListener(listener); 
    }
    listener = do_QueryInterface(mSelectionListenerP);
    if (listener)
    {
      selPriv->RemoveSelectionListener(listener); 
    }
  }

  mTypeInState = nullptr;
  mSelectionListenerP = nullptr;

  
  RemoveAllDefaultProperties();

  if (mLinkHandler && mDocWeak)
  {
    nsCOMPtr<nsIPresShell> ps = GetPresShell();

    if (ps && ps->GetPresContext())
    {
      ps->GetPresContext()->SetLinkHandler(mLinkHandler);
    }
  }

  RemoveEventListeners();
}

void
nsHTMLEditor::HideAnonymousEditingUIs()
{
  if (mAbsolutelyPositionedObject)
    HideGrabber();
  if (mInlineEditedCell)
    HideInlineTableEditingUI();
  if (mResizedObject)
    HideResizers();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLEditor)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLEditor, nsPlaintextEditor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mTypeInState)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mStyleSheets)

  tmp->HideAnonymousEditingUIs();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLEditor, nsPlaintextEditor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTypeInState)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStyleSheets)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTopLeftHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTopHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTopRightHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLeftHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRightHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBottomLeftHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBottomHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBottomRightHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mActivatedHandle)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResizingShadow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResizingInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResizedObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMouseMotionListenerP)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSelectionListenerP)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResizeEventListenerP)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(objectResizeEventListeners)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAbsolutelyPositionedObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGrabber)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPositioningShadow)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mInlineEditedCell)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAddColumnBeforeButton)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRemoveColumnButton)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAddColumnAfterButton)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAddRowBeforeButton)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRemoveRowButton)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAddRowAfterButton)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLEditor, nsEditor)
NS_IMPL_RELEASE_INHERITED(nsHTMLEditor, nsEditor)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsHTMLEditor)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLEditor)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLObjectResizer)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLAbsPosEditor)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLInlineTableEditor)
  NS_INTERFACE_MAP_ENTRY(nsITableEditor)
  NS_INTERFACE_MAP_ENTRY(nsIEditorStyleSheets)
  NS_INTERFACE_MAP_ENTRY(nsICSSLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
NS_INTERFACE_MAP_END_INHERITING(nsPlaintextEditor)


NS_IMETHODIMP
nsHTMLEditor::Init(nsIDOMDocument *aDoc,
                   nsIContent *aRoot,
                   nsISelectionController *aSelCon,
                   uint32_t aFlags,
                   const nsAString& aInitialValue)
{
  NS_PRECONDITION(aDoc && !aSelCon, "bad arg");
  NS_ENSURE_TRUE(aDoc, NS_ERROR_NULL_POINTER);
  MOZ_ASSERT(aInitialValue.IsEmpty(), "Non-empty initial values not supported");

  nsresult result = NS_OK, rulesRes = NS_OK;
   
  if (1)
  {
    
    nsAutoEditInitRulesTrigger rulesTrigger(static_cast<nsPlaintextEditor*>(this), rulesRes);

    
    result = nsPlaintextEditor::Init(aDoc, aRoot, nullptr, aFlags, aInitialValue);
    if (NS_FAILED(result)) { return result; }

    
    nsCOMPtr<nsINode> document = do_QueryInterface(aDoc);
    document->AddMutationObserverUnlessExists(this);

    
    if (IsMailEditor())
    {
      SetAbsolutePositioningEnabled(false);
      SetSnapToGridEnabled(false);
    }

    
    mHTMLCSSUtils = new nsHTMLCSSUtils(this);

    
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    nsPresContext *context = presShell->GetPresContext();
    NS_ENSURE_TRUE(context, NS_ERROR_NULL_POINTER);
    if (!IsPlaintextEditor() && !IsInteractionAllowed()) {
      mLinkHandler = context->GetLinkHandler();

      context->SetLinkHandler(nullptr);
    }

    
    mTypeInState = new TypeInState();

    
    mSelectionListenerP = new ResizerSelectionListener(this);

    if (!IsInteractionAllowed()) {
      
      AddOverrideStyleSheet(NS_LITERAL_STRING("resource://gre/res/EditorOverride.css"));
    }

    nsCOMPtr<nsISelection>selection;
    result = GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) { return result; }
    if (selection) 
    {
      nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
      nsCOMPtr<nsISelectionListener>listener;
      listener = do_QueryInterface(mTypeInState);
      if (listener) {
        selPriv->AddSelectionListener(listener); 
      }
      listener = do_QueryInterface(mSelectionListenerP);
      if (listener) {
        selPriv->AddSelectionListener(listener); 
      }
    }
  }

  NS_ENSURE_SUCCESS(rulesRes, rulesRes);
  return result;
}

NS_IMETHODIMP
nsHTMLEditor::PreDestroy(bool aDestroyingFrames)
{
  if (mDidPreDestroy) {
    return NS_OK;
  }

  nsCOMPtr<nsINode> document = do_QueryReferent(mDocWeak);
  if (document) {
    document->RemoveMutationObserver(this);
  }

  while (mStyleSheetURLs.Length())
  {
    RemoveOverrideStyleSheet(mStyleSheetURLs[0]);
  }

  
  
  HideAnonymousEditingUIs();

  return nsPlaintextEditor::PreDestroy(aDestroyingFrames);
}

NS_IMETHODIMP
nsHTMLEditor::GetRootElement(nsIDOMElement **aRootElement)
{
  NS_ENSURE_ARG_POINTER(aRootElement);

  if (mRootElement) {
    return nsEditor::GetRootElement(aRootElement);
  }

  *aRootElement = nullptr;

  
  

  nsCOMPtr<nsIDOMElement> rootElement; 
  nsCOMPtr<nsIDOMHTMLElement> bodyElement; 
  nsresult rv = GetBodyElement(getter_AddRefs(bodyElement));
  NS_ENSURE_SUCCESS(rv, rv);

  if (bodyElement) {
    rootElement = bodyElement;
  } else {
    
    
    nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
    NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

    rv = doc->GetDocumentElement(getter_AddRefs(rootElement));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!rootElement) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  mRootElement = do_QueryInterface(rootElement);
  rootElement.forget(aRootElement);

  return NS_OK;
}

already_AddRefed<nsIContent>
nsHTMLEditor::FindSelectionRoot(nsINode *aNode)
{
  NS_PRECONDITION(aNode->IsNodeOfType(nsINode::eDOCUMENT) ||
                  aNode->IsNodeOfType(nsINode::eCONTENT),
                  "aNode must be content or document node");

  nsCOMPtr<nsIDocument> doc = aNode->GetCurrentDoc();
  if (!doc) {
    return nullptr;
  }

  nsCOMPtr<nsIContent> content;
  if (doc->HasFlag(NODE_IS_EDITABLE) || !aNode->IsContent()) {
    content = doc->GetRootElement();
    return content.forget();
  }
  content = aNode->AsContent();

  
  
  
  if (IsReadonly()) {
    
    content = do_QueryInterface(GetRoot());
    return content.forget();
  }

  if (!content->HasFlag(NODE_IS_EDITABLE)) {
    
    
    if (content->IsElement() &&
        content->AsElement()->State().HasState(NS_EVENT_STATE_MOZ_READWRITE)) {
      return content.forget();
    }
    return nullptr;
  }

  
  
  content = content->GetEditingHost();
  return content.forget();
}


void
nsHTMLEditor::CreateEventListeners()
{
  
  if (!mEventListener) {
    mEventListener = new nsHTMLEditorEventListener();
  }
}

nsresult
nsHTMLEditor::InstallEventListeners()
{
  NS_ENSURE_TRUE(mDocWeak && mEventListener,
                 NS_ERROR_NOT_INITIALIZED);

  
  

  nsHTMLEditorEventListener* listener =
    reinterpret_cast<nsHTMLEditorEventListener*>(mEventListener.get());
  return listener->Connect(this);
}

void
nsHTMLEditor::RemoveEventListeners()
{
  if (!mDocWeak)
  {
    return;
  }

  nsCOMPtr<nsIDOMEventTarget> target = GetDOMEventTarget();

  if (target)
  {
    
    
    
    
    
    

    if (mMouseMotionListenerP)
    {
      
      
      target->RemoveEventListener(NS_LITERAL_STRING("mousemove"),
                                  mMouseMotionListenerP, false);
      target->RemoveEventListener(NS_LITERAL_STRING("mousemove"),
                                  mMouseMotionListenerP, true);
    }

    if (mResizeEventListenerP)
    {
      target->RemoveEventListener(NS_LITERAL_STRING("resize"),
                                  mResizeEventListenerP, false);
    }
  }

  mMouseMotionListenerP = nullptr;
  mResizeEventListenerP = nullptr;

  nsPlaintextEditor::RemoveEventListeners();
}

NS_IMETHODIMP 
nsHTMLEditor::SetFlags(uint32_t aFlags)
{
  nsresult rv = nsPlaintextEditor::SetFlags(aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  mCSSAware = !NoCSS() && !IsMailEditor();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::InitRules()
{
  MOZ_ASSERT(!mRules);
  
  mRules = new nsHTMLEditRules();
  return mRules->Init(static_cast<nsPlaintextEditor*>(this));
}

NS_IMETHODIMP
nsHTMLEditor::BeginningOfDocument()
{
  if (!mDocWeak) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(GetRoot());
  if (!rootElement) {
    NS_WARNING("GetRoot() returned a null pointer (mRootElement is null)");
    return NS_OK;
  }

  
  bool done = false;
  nsCOMPtr<nsIDOMNode> curNode(rootElement), selNode;
  int32_t curOffset = 0, selOffset;
  while (!done)
  {
    nsWSRunObject wsObj(this, curNode, curOffset);
    nsCOMPtr<nsIDOMNode> visNode;
    int32_t visOffset=0;
    WSType visType;
    wsObj.NextVisibleNode(curNode, curOffset, address_of(visNode), &visOffset, &visType);
    if (visType == WSType::normalWS || visType == WSType::text) {
      selNode = visNode;
      selOffset = visOffset;
      done = true;
    } else if (visType == WSType::br || visType == WSType::special) {
      selNode = GetNodeLocation(visNode, &selOffset);
      done = true;
    } else if (visType == WSType::otherBlock) {
      
      
      
      
      
      

      if (!IsContainer(visNode))
      {
        
        
        
        
        

        selNode = GetNodeLocation(visNode, &selOffset);
        done = true;
      }
      else
      {
        bool isEmptyBlock;
        if (NS_SUCCEEDED(IsEmptyNode(visNode, &isEmptyBlock)) &&
            isEmptyBlock)
        {
          
          curNode = GetNodeLocation(visNode, &curOffset);
          ++curOffset;
        }
        else
        {
          curNode = visNode;
          curOffset = 0;
        }
        
      }
    }
    else
    {
      
      selNode = curNode;
      selOffset = curOffset;
      done = true;
    }
  }
  return selection->Collapse(selNode, selOffset);
}

nsresult
nsHTMLEditor::HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent)
{
  
  

  if (IsReadonly() || IsDisabled()) {
    
    
    return nsEditor::HandleKeyPressEvent(aKeyEvent);
  }

  WidgetKeyboardEvent* nativeKeyEvent =
    aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
  NS_ENSURE_TRUE(nativeKeyEvent, NS_ERROR_UNEXPECTED);
  NS_ASSERTION(nativeKeyEvent->message == NS_KEY_PRESS,
               "HandleKeyPressEvent gets non-keypress event");

  switch (nativeKeyEvent->keyCode) {
    case nsIDOMKeyEvent::DOM_VK_META:
    case nsIDOMKeyEvent::DOM_VK_WIN:
    case nsIDOMKeyEvent::DOM_VK_SHIFT:
    case nsIDOMKeyEvent::DOM_VK_CONTROL:
    case nsIDOMKeyEvent::DOM_VK_ALT:
    case nsIDOMKeyEvent::DOM_VK_BACK_SPACE:
    case nsIDOMKeyEvent::DOM_VK_DELETE:
      
      
      return nsEditor::HandleKeyPressEvent(aKeyEvent);
    case nsIDOMKeyEvent::DOM_VK_TAB: {
      if (IsPlaintextEditor()) {
        
        
        return nsPlaintextEditor::HandleKeyPressEvent(aKeyEvent);
      }

      if (IsTabbable()) {
        return NS_OK; 
      }

      if (nativeKeyEvent->IsControl() || nativeKeyEvent->IsAlt() ||
          nativeKeyEvent->IsMeta() || nativeKeyEvent->IsOS()) {
        return NS_OK;
      }

      nsRefPtr<Selection> selection = GetSelection();
      NS_ENSURE_TRUE(selection && selection->RangeCount(), NS_ERROR_FAILURE);

      nsCOMPtr<nsINode> node = selection->GetRangeAt(0)->GetStartParent();
      MOZ_ASSERT(node);

      nsCOMPtr<nsINode> blockParent;
      if (IsBlockNode(node)) {
        blockParent = node;
      } else {
        blockParent = GetBlockNodeParent(node);
      }

      if (!blockParent) {
        break;
      }

      bool handled = false;
      nsresult rv;
      if (nsHTMLEditUtils::IsTableElement(blockParent)) {
        rv = TabInTable(nativeKeyEvent->IsShift(), &handled);
        if (handled) {
          ScrollSelectionIntoView(false);
        }
      } else if (nsHTMLEditUtils::IsListItem(blockParent)) {
        rv = Indent(nativeKeyEvent->IsShift()
                    ? NS_LITERAL_STRING("outdent")
                    : NS_LITERAL_STRING("indent"));
        handled = true;
      }
      NS_ENSURE_SUCCESS(rv, rv);
      if (handled) {
        return aKeyEvent->PreventDefault(); 
      }
      if (nativeKeyEvent->IsShift()) {
        return NS_OK; 
      }
      aKeyEvent->PreventDefault();
      return TypedText(NS_LITERAL_STRING("\t"), eTypedText);
    }
    case nsIDOMKeyEvent::DOM_VK_RETURN:
      if (nativeKeyEvent->IsControl() || nativeKeyEvent->IsAlt() ||
          nativeKeyEvent->IsMeta() || nativeKeyEvent->IsOS()) {
        return NS_OK;
      }
      aKeyEvent->PreventDefault(); 
      if (nativeKeyEvent->IsShift() && !IsPlaintextEditor()) {
        
        return TypedText(EmptyString(), eTypedBR);
      }
      
      return TypedText(EmptyString(), eTypedBreak);
  }

  
  
  if (nativeKeyEvent->charCode == 0 || nativeKeyEvent->IsControl() ||
      nativeKeyEvent->IsAlt() || nativeKeyEvent->IsMeta() ||
      nativeKeyEvent->IsOS()) {
    
    return NS_OK;
  }
  aKeyEvent->PreventDefault();
  nsAutoString str(nativeKeyEvent->charCode);
  return TypedText(str, eTypedText);
}

static void
AssertParserServiceIsCorrect(nsIAtom* aTag, bool aIsBlock)
{
#ifdef DEBUG
  
  if (aTag==nsEditProperty::p          ||
      aTag==nsEditProperty::div        ||
      aTag==nsEditProperty::blockquote ||
      aTag==nsEditProperty::h1         ||
      aTag==nsEditProperty::h2         ||
      aTag==nsEditProperty::h3         ||
      aTag==nsEditProperty::h4         ||
      aTag==nsEditProperty::h5         ||
      aTag==nsEditProperty::h6         ||
      aTag==nsEditProperty::ul         ||
      aTag==nsEditProperty::ol         ||
      aTag==nsEditProperty::dl         ||
      aTag==nsEditProperty::noscript   ||
      aTag==nsEditProperty::form       ||
      aTag==nsEditProperty::hr         ||
      aTag==nsEditProperty::table      ||
      aTag==nsEditProperty::fieldset   ||
      aTag==nsEditProperty::address    ||
      aTag==nsEditProperty::col        ||
      aTag==nsEditProperty::colgroup   ||
      aTag==nsEditProperty::li         ||
      aTag==nsEditProperty::dt         ||
      aTag==nsEditProperty::dd         ||
      aTag==nsEditProperty::legend     )
  {
    if (!aIsBlock) {
      nsAutoString assertmsg (NS_LITERAL_STRING("Parser and editor disagree on blockness: "));

      nsAutoString tagName;
      aTag->ToString(tagName);
      assertmsg.Append(tagName);
      char* assertstr = ToNewCString(assertmsg);
      NS_ASSERTION(aIsBlock, assertstr);
      NS_Free(assertstr);
    }
  }
#endif 
}





bool
nsHTMLEditor::NodeIsBlockStatic(const dom::Element* aElement)
{
  MOZ_ASSERT(aElement);

  nsIAtom* tagAtom = aElement->Tag();
  MOZ_ASSERT(tagAtom);

  
  
  if (tagAtom==nsEditProperty::body       ||
      tagAtom==nsEditProperty::head       ||
      tagAtom==nsEditProperty::tbody      ||
      tagAtom==nsEditProperty::thead      ||
      tagAtom==nsEditProperty::tfoot      ||
      tagAtom==nsEditProperty::tr         ||
      tagAtom==nsEditProperty::th         ||
      tagAtom==nsEditProperty::td         ||
      tagAtom==nsEditProperty::li         ||
      tagAtom==nsEditProperty::dt         ||
      tagAtom==nsEditProperty::dd         ||
      tagAtom==nsEditProperty::pre)
  {
    return true;
  }

  bool isBlock;
#ifdef DEBUG
  
  nsresult rv =
#endif
    nsContentUtils::GetParserService()->
    IsBlock(nsContentUtils::GetParserService()->HTMLAtomTagToId(tagAtom),
            isBlock);
  MOZ_ASSERT(rv == NS_OK);

  AssertParserServiceIsCorrect(tagAtom, isBlock);

  return isBlock;
}

nsresult
nsHTMLEditor::NodeIsBlockStatic(nsIDOMNode *aNode, bool *aIsBlock)
{
  if (!aNode || !aIsBlock) { return NS_ERROR_NULL_POINTER; }

  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  *aIsBlock = element && NodeIsBlockStatic(element);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::NodeIsBlock(nsIDOMNode *aNode, bool *aIsBlock)
{
  return NodeIsBlockStatic(aNode, aIsBlock);
}

bool
nsHTMLEditor::IsBlockNode(nsINode *aNode)
{
  return aNode && aNode->IsElement() && NodeIsBlockStatic(aNode->AsElement());
}


NS_IMETHODIMP 
nsHTMLEditor::SetDocumentTitle(const nsAString &aTitle)
{
  nsRefPtr<SetDocTitleTxn> txn = new SetDocTitleTxn();
  NS_ENSURE_TRUE(txn, NS_ERROR_OUT_OF_MEMORY);

  nsresult result = txn->Init(this, &aTitle);
  NS_ENSURE_SUCCESS(result, result);

  
  nsAutoTxnsConserveSelection dontChangeSelection(this);
  return nsEditor::DoTransaction(txn);  
}





already_AddRefed<Element>
nsHTMLEditor::GetBlockNodeParent(nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  nsCOMPtr<nsINode> p = aNode->GetParentNode();

  while (p) {
    if (p->IsElement() && NodeIsBlockStatic(p->AsElement())) {
      return p.forget().downcast<Element>();
    }
    p = p->GetParentNode();
  }

  return nullptr;
}

already_AddRefed<nsIDOMNode>
nsHTMLEditor::GetBlockNodeParent(nsIDOMNode *aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);

  if (!node) {
    NS_NOTREACHED("null node passed to GetBlockNodeParent()");
    return nullptr;
  }

  nsCOMPtr<nsIDOMNode> ret =
    dont_AddRef(GetAsDOMNode(GetBlockNodeParent(node).take()));
  return ret.forget();
}

static const char16_t nbsp = 160;




void
nsHTMLEditor::IsNextCharInNodeWhitespace(nsIContent* aContent,
                                         int32_t aOffset,
                                         bool* outIsSpace,
                                         bool* outIsNBSP,
                                         nsIContent** outNode,
                                         int32_t* outOffset)
{
  MOZ_ASSERT(aContent && outIsSpace && outIsNBSP);
  MOZ_ASSERT((outNode && outOffset) || (!outNode && !outOffset));
  *outIsSpace = false;
  *outIsNBSP = false;
  if (outNode && outOffset) {
    *outNode = nullptr;
    *outOffset = -1;
  }

  if (aContent->IsNodeOfType(nsINode::eTEXT) &&
      (uint32_t)aOffset < aContent->Length()) {
    char16_t ch = aContent->GetText()->CharAt(aOffset);
    *outIsSpace = nsCRT::IsAsciiSpace(ch);
    *outIsNBSP = (ch == nbsp);
    if (outNode && outOffset) {
      NS_IF_ADDREF(*outNode = aContent);
      
      *outOffset = aOffset + 1;
    }
  }
}





void
nsHTMLEditor::IsPrevCharInNodeWhitespace(nsIContent* aContent,
                                         int32_t aOffset,
                                         bool* outIsSpace,
                                         bool* outIsNBSP,
                                         nsIContent** outNode,
                                         int32_t* outOffset)
{
  MOZ_ASSERT(aContent && outIsSpace && outIsNBSP);
  MOZ_ASSERT((outNode && outOffset) || (!outNode && !outOffset));
  *outIsSpace = false;
  *outIsNBSP = false;
  if (outNode && outOffset) {
    *outNode = nullptr;
    *outOffset = -1;
  }

  if (aContent->IsNodeOfType(nsINode::eTEXT) && aOffset > 0) {
    char16_t ch = aContent->GetText()->CharAt(aOffset - 1);
    *outIsSpace = nsCRT::IsAsciiSpace(ch);
    *outIsNBSP = (ch == nbsp);
    if (outNode && outOffset) {
      NS_IF_ADDREF(*outNode = aContent);
      *outOffset = aOffset - 1;
    }
  }
}






bool
nsHTMLEditor::IsVisBreak(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  if (!nsTextEditUtils::IsBreak(aNode)) {
    return false;
  }
  
  nsCOMPtr<nsINode> priorNode = GetPriorHTMLNode(aNode, true);
  if (priorNode && nsTextEditUtils::IsBreak(priorNode)) {
    return true;
  }
  nsCOMPtr<nsINode> nextNode = GetNextHTMLNode(aNode, true);
  if (nextNode && nsTextEditUtils::IsBreak(nextNode)) {
    return true;
  }
  
  
  if (!nextNode) {
    
    return false;
  }
  if (IsBlockNode(nextNode)) {
    
    return false;
  }
    
  
  
  int32_t selOffset;
  nsCOMPtr<nsINode> selNode = GetNodeLocation(aNode, &selOffset);
  
  selOffset++;
  nsWSRunObject wsObj(this, selNode->AsDOMNode(), selOffset);
  nsCOMPtr<nsIDOMNode> visNode;
  int32_t visOffset = 0;
  WSType visType;
  wsObj.NextVisibleNode(selNode->AsDOMNode(), selOffset, address_of(visNode),
                        &visOffset, &visType);
  if (visType & WSType::block) {
    return false;
  }
  
  return true;
}


bool
nsHTMLEditor::IsVisBreak(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, false);
  return IsVisBreak(node);
}

NS_IMETHODIMP
nsHTMLEditor::BreakIsVisible(nsIDOMNode *aNode, bool *aIsVisible)
{
  NS_ENSURE_ARG_POINTER(aNode && aIsVisible);

  *aIsVisible = IsVisBreak(aNode);

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::GetIsDocumentEditable(bool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);

  nsCOMPtr<nsIDOMDocument> doc = GetDOMDocument();
  *aIsDocumentEditable = doc && IsModifiable();

  return NS_OK;
}

bool nsHTMLEditor::IsModifiable()
{
  return !IsReadonly();
}

NS_IMETHODIMP
nsHTMLEditor::UpdateBaseURL()
{
  nsCOMPtr<nsIDocument> doc = GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  
  nsRefPtr<nsContentList> nodeList =
    doc->GetElementsByTagName(NS_LITERAL_STRING("base"));

  
  
  if (!nodeList || !nodeList->Item(0)) {
    return doc->SetBaseURI(doc->GetDocumentURI());
  }
  return NS_OK;
}







NS_IMETHODIMP
nsHTMLEditor::TypedText(const nsAString& aString, ETypingAction aAction)
{
  nsAutoPlaceHolderBatch batch(this, nsGkAtoms::TypingTxnName);

  if (aAction == eTypedBR) {
    
    nsCOMPtr<nsIDOMNode> brNode;
    return InsertBR(address_of(brNode));
  }

  return nsPlaintextEditor::TypedText(aString, aAction);
}

NS_IMETHODIMP
nsHTMLEditor::TabInTable(bool inIsShift, bool* outHandled)
{
  NS_ENSURE_TRUE(outHandled, NS_ERROR_NULL_POINTER);
  *outHandled = false;

  
  nsCOMPtr<nsIDOMElement> cellElement;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nullptr, getter_AddRefs(cellElement));
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(cellElement, NS_OK);

  
  nsCOMPtr<nsIDOMNode> tbl = GetEnclosingTable(cellElement);
  NS_ENSURE_TRUE(tbl, res);

  
  
  nsCOMPtr<nsIContentIterator> iter =
      do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &res);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIContent> cTbl = do_QueryInterface(tbl);
  nsCOMPtr<nsIContent> cBlock = do_QueryInterface(cellElement);
  res = iter->Init(cTbl);
  NS_ENSURE_SUCCESS(res, res);
  
  res = iter->PositionAt(cBlock);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> node;
  do
  {
    if (inIsShift)
      iter->Prev();
    else
      iter->Next();

    node = do_QueryInterface(iter->GetCurrentNode());

    if (node && nsHTMLEditUtils::IsTableCell(node) &&
        GetEnclosingTable(node) == tbl)
    {
      res = CollapseSelectionToDeepestNonTableFirstChild(nullptr, node);
      NS_ENSURE_SUCCESS(res, res);
      *outHandled = true;
      return NS_OK;
    }
  } while (!iter->IsDone());
  
  if (!(*outHandled) && !inIsShift)
  {
    
    
    res = InsertTableRow(1, true);
    NS_ENSURE_SUCCESS(res, res);
    *outHandled = true;
    
    
    nsCOMPtr<nsISelection>selection;
    nsCOMPtr<nsIDOMElement> tblElement;
    nsCOMPtr<nsIDOMElement> cell;
    int32_t row;
    res = GetCellContext(getter_AddRefs(selection), 
                         getter_AddRefs(tblElement),
                         getter_AddRefs(cell), 
                         nullptr, nullptr,
                         &row, nullptr);
    NS_ENSURE_SUCCESS(res, res);
    
    res = GetCellAt(tblElement, row, 0, getter_AddRefs(cell));
    NS_ENSURE_SUCCESS(res, res);
    
    
    
    node = do_QueryInterface(cell);
    if (node) selection->Collapse(node,0);
    return NS_OK;
  }
  
  return res;
}

NS_IMETHODIMP nsHTMLEditor::CreateBR(nsIDOMNode *aNode, int32_t aOffset, nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect)
{
  nsCOMPtr<nsIDOMNode> parent = aNode;
  int32_t offset = aOffset;
  return CreateBRImpl(address_of(parent), &offset, outBRNode, aSelect);
}

nsresult 
nsHTMLEditor::CollapseSelectionToDeepestNonTableFirstChild(nsISelection *aSelection, nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  nsresult res;

  nsCOMPtr<nsISelection> selection;
  if (aSelection)
  {
    selection = aSelection;
  } else {
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  }
  nsCOMPtr<nsIDOMNode> node = aNode;
  nsCOMPtr<nsIDOMNode> child;
  
  do {
    node->GetFirstChild(getter_AddRefs(child));
    
    if (child)
    {
      
      
      if (nsHTMLEditUtils::IsTable(child)) break;
      
      if (!IsContainer(child)) break;
      node = child;
    }
  }
  while (child);

  selection->Collapse(node,0);
  return NS_OK;
}





NS_IMETHODIMP
nsHTMLEditor::ReplaceHeadContentsWithHTML(const nsAString& aSourceToInsert)
{
  nsAutoRules beginRulesSniffing(this, EditAction::ignore, nsIEditor::eNone); 
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  ForceCompositionEnd();

  
  
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIDOMNodeList>nodeList; 
  res = doc->GetElementsByTagName(NS_LITERAL_STRING("head"), getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(nodeList, NS_ERROR_NULL_POINTER);

  uint32_t count; 
  nodeList->GetLength(&count);
  if (count < 1) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> headNode;
  res = nodeList->Item(0, getter_AddRefs(headNode)); 
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(headNode, NS_ERROR_NULL_POINTER);

  
  
  
  nsAutoString inputString (aSourceToInsert);  
 
  
  inputString.ReplaceSubstring(MOZ_UTF16("\r\n"),
                               MOZ_UTF16("\n"));
 
  
  inputString.ReplaceSubstring(MOZ_UTF16("\r"),
                               MOZ_UTF16("\n"));

  nsAutoEditBatch beginBatching(this);

  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  res = range->CreateContextualFragment(inputString,
                                        getter_AddRefs(docfrag));

  
  

  if (NS_FAILED(res))
  {
#ifdef DEBUG
    printf("Couldn't create contextual fragment: error was %X\n",
           static_cast<uint32_t>(res));
#endif
    return res;
  }
  NS_ENSURE_TRUE(docfrag, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> child;

  
  do {
    res = headNode->GetFirstChild(getter_AddRefs(child));
    NS_ENSURE_SUCCESS(res, res);
    if (child)
    {
      res = DeleteNode(child);
      NS_ENSURE_SUCCESS(res, res);
    }
  } while (child);

  
  int32_t offsetOfNewNode = 0;
  nsCOMPtr<nsIDOMNode> fragmentAsNode (do_QueryInterface(docfrag));

  
  do {
    res = fragmentAsNode->GetFirstChild(getter_AddRefs(child));
    NS_ENSURE_SUCCESS(res, res);
    if (child)
    {
      res = InsertNode(child, headNode, offsetOfNewNode++);
      NS_ENSURE_SUCCESS(res, res);
    }
  } while (child);

  return res;
}

NS_IMETHODIMP
nsHTMLEditor::RebuildDocumentFromSource(const nsAString& aSourceString)
{
  ForceCompositionEnd();

  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> bodyElement = do_QueryInterface(GetRoot());
  NS_ENSURE_TRUE(bodyElement, NS_ERROR_NULL_POINTER);

  
  nsReadingIterator<char16_t> beginbody;
  nsReadingIterator<char16_t> endbody;
  aSourceString.BeginReading(beginbody);
  aSourceString.EndReading(endbody);
  bool foundbody = CaseInsensitiveFindInReadable(NS_LITERAL_STRING("<body"),
                                                   beginbody, endbody);

  nsReadingIterator<char16_t> beginhead;
  nsReadingIterator<char16_t> endhead;
  aSourceString.BeginReading(beginhead);
  aSourceString.EndReading(endhead);
  bool foundhead = CaseInsensitiveFindInReadable(NS_LITERAL_STRING("<head"),
                                                   beginhead, endhead);
  
  if (foundbody && beginhead.get() > beginbody.get())
    foundhead = false;

  nsReadingIterator<char16_t> beginclosehead;
  nsReadingIterator<char16_t> endclosehead;
  aSourceString.BeginReading(beginclosehead);
  aSourceString.EndReading(endclosehead);

  
  bool foundclosehead = CaseInsensitiveFindInReadable(
           NS_LITERAL_STRING("</head>"), beginclosehead, endclosehead);
  
  if (foundhead && beginhead.get() > beginclosehead.get())
    foundclosehead = false;
  
  if (foundbody && beginclosehead.get() > beginbody.get())
    foundclosehead = false;
  
  
  nsAutoEditBatch beginBatching(this);

  nsReadingIterator<char16_t> endtotal;
  aSourceString.EndReading(endtotal);

  if (foundhead) {
    if (foundclosehead)
      res = ReplaceHeadContentsWithHTML(Substring(beginhead, beginclosehead));
    else if (foundbody)
      res = ReplaceHeadContentsWithHTML(Substring(beginhead, beginbody));
    else
      
      
      
      res = ReplaceHeadContentsWithHTML(Substring(beginhead, endtotal));
  } else {
    nsReadingIterator<char16_t> begintotal;
    aSourceString.BeginReading(begintotal);
    NS_NAMED_LITERAL_STRING(head, "<head>");
    if (foundclosehead)
      res = ReplaceHeadContentsWithHTML(head + Substring(begintotal, beginclosehead));
    else if (foundbody)
      res = ReplaceHeadContentsWithHTML(head + Substring(begintotal, beginbody));
    else
      
      
      
      res = ReplaceHeadContentsWithHTML(head);
  }
  NS_ENSURE_SUCCESS(res, res);

  res = SelectAll();
  NS_ENSURE_SUCCESS(res, res);

  if (!foundbody) {
    NS_NAMED_LITERAL_STRING(body, "<body>");
    
    
    if (foundclosehead) 
      res = LoadHTML(body + Substring(endclosehead, endtotal));
    else if (foundhead) 
      res = LoadHTML(body);
    else 
      res = LoadHTML(body + aSourceString);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMElement> divElement;
    res = CreateElementWithDefaults(NS_LITERAL_STRING("div"), getter_AddRefs(divElement));
    NS_ENSURE_SUCCESS(res, res);

    res = CloneAttributes(bodyElement, divElement);
    NS_ENSURE_SUCCESS(res, res);

    return BeginningOfDocument();
  }

  res = LoadHTML(Substring(beginbody, endtotal));
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  
  
  nsReadingIterator<char16_t> beginclosebody = beginbody;
  nsReadingIterator<char16_t> endclosebody;
  aSourceString.EndReading(endclosebody);
  if (!FindInReadable(NS_LITERAL_STRING(">"),beginclosebody,endclosebody))
    return NS_ERROR_FAILURE;

  
  
  nsAutoString bodyTag;
  bodyTag.AssignLiteral("<div ");
  bodyTag.Append(Substring(endbody, endclosebody));

  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  res = range->CreateContextualFragment(bodyTag, getter_AddRefs(docfrag));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> fragmentAsNode (do_QueryInterface(docfrag));
  NS_ENSURE_TRUE(fragmentAsNode, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIDOMNode> child;
  res = fragmentAsNode->GetFirstChild(getter_AddRefs(child));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(child, NS_ERROR_NULL_POINTER);
  
  
  res = CloneAttributes(bodyElement, child);
  NS_ENSURE_SUCCESS(res, res);
  
  
  return BeginningOfDocument();
}

void
nsHTMLEditor::NormalizeEOLInsertPosition(nsIDOMNode *firstNodeToInsert,
                                     nsCOMPtr<nsIDOMNode> *insertParentNode,
                                     int32_t *insertOffset)
{
  





























  if (!IsBlockNode(firstNodeToInsert))
    return;

  nsWSRunObject wsObj(this, *insertParentNode, *insertOffset);
  nsCOMPtr<nsIDOMNode> nextVisNode;
  nsCOMPtr<nsIDOMNode> prevVisNode;
  int32_t nextVisOffset=0;
  WSType nextVisType;
  int32_t prevVisOffset=0;
  WSType prevVisType;

  wsObj.NextVisibleNode(*insertParentNode, *insertOffset, address_of(nextVisNode), &nextVisOffset, &nextVisType);
  if (!nextVisNode)
    return;

  if (!(nextVisType & WSType::br)) {
    return;
  }

  wsObj.PriorVisibleNode(*insertParentNode, *insertOffset, address_of(prevVisNode), &prevVisOffset, &prevVisType);
  if (!prevVisNode)
    return;

  if (prevVisType & WSType::br) {
    return;
  }

  if (prevVisType & WSType::thisBlock) {
    return;
  }

  int32_t brOffset=0;
  nsCOMPtr<nsIDOMNode> brNode = GetNodeLocation(nextVisNode, &brOffset);

  *insertParentNode = brNode;
  *insertOffset = brOffset + 1;
}

NS_IMETHODIMP
nsHTMLEditor::InsertElementAtSelection(nsIDOMElement* aElement, bool aDeleteSelection)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsresult res = NS_ERROR_NOT_INITIALIZED;
  
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertElement, nsIEditor::eNext);

  nsRefPtr<Selection> selection = GetSelection();
  if (!selection) {
    return NS_ERROR_FAILURE;
  }

  
  bool cancel, handled;
  nsTextRulesInfo ruleInfo(EditAction::insertElement);
  ruleInfo.insertElement = aElement;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    if (aDeleteSelection)
    {
      if (!IsBlockNode(aElement)) {
        
        
        
        
        res = DeleteSelection(nsIEditor::eNone, nsIEditor::eNoStrip);
        NS_ENSURE_SUCCESS(res, res);
      }

      nsresult result = DeleteSelectionAndPrepareToCreateNode();
      NS_ENSURE_SUCCESS(result, result);
    }

    
    
    if (!aDeleteSelection)
    {
      
      
      
      if (nsHTMLEditUtils::IsNamedAnchor(node))
      {
        selection->CollapseToStart();
      } else {
        selection->CollapseToEnd();
      }
    }

    nsCOMPtr<nsIDOMNode> parentSelectedNode;
    int32_t offsetForInsert;
    res = selection->GetAnchorNode(getter_AddRefs(parentSelectedNode));
    
    if (NS_SUCCEEDED(res) && NS_SUCCEEDED(selection->GetAnchorOffset(&offsetForInsert)) && parentSelectedNode)
    {
#ifdef DEBUG_cmanske
      {
      nsAutoString name;
      parentSelectedNode->GetNodeName(name);
      printf("InsertElement: Anchor node of selection: ");
      wprintf(name.get());
      printf(" Offset: %d\n", offsetForInsert);
      }
#endif

      
      NormalizeEOLInsertPosition(node, address_of(parentSelectedNode), &offsetForInsert);

      res = InsertNodeAtPoint(node, address_of(parentSelectedNode), &offsetForInsert, false);
      NS_ENSURE_SUCCESS(res, res);
      
      
      if (!SetCaretInTableCell(aElement))
      {
        res = SetCaretAfterElement(aElement);
        NS_ENSURE_SUCCESS(res, res);
      }
      
      if (nsHTMLEditUtils::IsTable(node))
      {
        bool isLast;
        res = IsLastEditableChild(node, &isLast);
        NS_ENSURE_SUCCESS(res, res);
        if (isLast)
        {
          nsCOMPtr<nsIDOMNode> brNode;
          res = CreateBR(parentSelectedNode, offsetForInsert+1, address_of(brNode));
          NS_ENSURE_SUCCESS(res, res);
          selection->Collapse(parentSelectedNode, offsetForInsert+1);
        }
      }
    }
  }
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}















nsresult
nsHTMLEditor::InsertNodeAtPoint(nsIDOMNode *aNode, 
                                nsCOMPtr<nsIDOMNode> *ioParent, 
                                int32_t *ioOffset, 
                                bool aNoEmptyNodes)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(ioParent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(*ioParent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(ioOffset, NS_ERROR_NULL_POINTER);
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> parent = *ioParent;
  nsCOMPtr<nsIDOMNode> topChild = *ioParent;
  nsCOMPtr<nsIDOMNode> tmp;
  int32_t offsetOfInsert = *ioOffset;
   
  
  while (!CanContain(parent, aNode)) {
    
    
    if (nsTextEditUtils::IsBody(parent) || nsHTMLEditUtils::IsTableElement(parent))
      return NS_ERROR_FAILURE;
    
    parent->GetParentNode(getter_AddRefs(tmp));
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    topChild = parent;
    parent = tmp;
  }
  if (parent != topChild)
  {
    
    res = SplitNodeDeep(topChild, *ioParent, *ioOffset, &offsetOfInsert, aNoEmptyNodes);
    NS_ENSURE_SUCCESS(res, res);
    *ioParent = parent;
    *ioOffset = offsetOfInsert;
  }
  
  res = InsertNode(aNode, parent, offsetOfInsert);
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SelectElement(nsIDOMElement* aElement)
{
  nsresult res = NS_ERROR_NULL_POINTER;

  
  if (IsDescendantOfEditorRoot(aElement)) {
    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMNode>parent;
    res = aElement->GetParentNode(getter_AddRefs(parent));
    if (NS_SUCCEEDED(res) && parent)
    {
      int32_t offsetInParent = GetChildOffset(aElement, parent);

      
      res = selection->Collapse(parent, offsetInParent);
      if (NS_SUCCEEDED(res)) {
        
        res = selection->Extend(parent, offsetInParent + 1);
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SetCaretAfterElement(nsIDOMElement* aElement)
{
  nsresult res = NS_ERROR_NULL_POINTER;

  
  if (aElement && IsDescendantOfEditorRoot(aElement)) {
    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMNode>parent;
    res = aElement->GetParentNode(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);
    int32_t offsetInParent = GetChildOffset(aElement, parent);
    
    res = selection->Collapse(parent, offsetInParent + 1);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SetParagraphFormat(const nsAString& aParagraphFormat)
{
  nsAutoString tag; tag.Assign(aParagraphFormat);
  ToLowerCase(tag);
  if (tag.EqualsLiteral("dd") || tag.EqualsLiteral("dt"))
    return MakeDefinitionItem(tag);
  else
    return InsertBasicBlock(tag);
}

NS_IMETHODIMP 
nsHTMLEditor::GetParagraphState(bool *aMixed, nsAString &outFormat)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  nsRefPtr<nsHTMLEditRules> htmlRules = static_cast<nsHTMLEditRules*>(mRules.get());
  
  return htmlRules->GetParagraphState(aMixed, outFormat);
}

NS_IMETHODIMP
nsHTMLEditor::GetBackgroundColorState(bool *aMixed, nsAString &aOutColor)
{
  nsresult res;
  if (IsCSSEnabled()) {
    
    
    res = GetCSSBackgroundColorState(aMixed, aOutColor, true);
  }
  else {
    
    res = GetHTMLBackgroundColorState(aMixed, aOutColor);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::GetHighlightColorState(bool *aMixed, nsAString &aOutColor)
{
  nsresult res = NS_OK;
  *aMixed = false;
  aOutColor.AssignLiteral("transparent");
  if (IsCSSEnabled()) {
    
    
    
    res = GetCSSBackgroundColorState(aMixed, aOutColor, false);
  }
  return res;
}

nsresult
nsHTMLEditor::GetCSSBackgroundColorState(bool *aMixed, nsAString &aOutColor, bool aBlockLevel)
{
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = false;
  
  aOutColor.AssignLiteral("transparent");
  
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMNode> parent;
  int32_t offset;
  res = GetStartNodeAndOffset(selection, getter_AddRefs(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIDOMNode> nodeToExamine;
  if (selection->Collapsed() || IsTextNode(parent)) {
    
    nodeToExamine = parent;
  } else {
    
    
    nodeToExamine = GetChildAt(parent, offset);
    
  }
  
  NS_ENSURE_TRUE(nodeToExamine, NS_ERROR_NULL_POINTER);

  
  bool isBlock;
  res = NodeIsBlockStatic(nodeToExamine, &isBlock);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> tmp;

  if (aBlockLevel) {
    
    
    nsCOMPtr<nsIDOMNode> blockParent = nodeToExamine;
    if (!isBlock) {
      blockParent = GetBlockNodeParent(nodeToExamine);
      NS_ENSURE_TRUE(blockParent, NS_OK);
    }

    
    nsCOMPtr<nsIDOMElement> element;
    do {
      
      mHTMLCSSUtils->GetComputedProperty(blockParent,
                                         nsEditProperty::cssBackgroundColor,
                                         aOutColor);
      tmp.swap(blockParent);
      res = tmp->GetParentNode(getter_AddRefs(blockParent));
      element = do_QueryInterface(blockParent);
      
      
    } while (aOutColor.EqualsLiteral("transparent") && element);
    if (aOutColor.EqualsLiteral("transparent")) {
      
      
      
      mHTMLCSSUtils->GetDefaultBackgroundColor(aOutColor);
    }
  }
  else {
    
    if (IsTextNode(nodeToExamine)) {
      
      res = nodeToExamine->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(res, res);
      nodeToExamine = parent;
    }
    do {
      
      res = NodeIsBlockStatic(nodeToExamine, &isBlock);
      NS_ENSURE_SUCCESS(res, res);
      if (isBlock) {
        
        aOutColor.AssignLiteral("transparent");
        break;
      }
      else {
        
        
        mHTMLCSSUtils->GetComputedProperty(nodeToExamine, nsEditProperty::cssBackgroundColor,
                            aOutColor);
        if (!aOutColor.EqualsLiteral("transparent")) {
          break;
        }
      }
      tmp.swap(nodeToExamine);
      res = tmp->GetParentNode(getter_AddRefs(nodeToExamine));
      NS_ENSURE_SUCCESS(res, res);
    } while ( aOutColor.EqualsLiteral("transparent") && nodeToExamine );
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLEditor::GetHTMLBackgroundColorState(bool *aMixed, nsAString &aOutColor)
{
  
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = false;
  aOutColor.Truncate();
  
  nsCOMPtr<nsIDOMElement> domElement;
  int32_t selectedCount;
  nsAutoString tagName;
  nsresult res = GetSelectedOrParentTableElement(tagName,
                                                 &selectedCount,
                                                 getter_AddRefs(domElement));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<dom::Element> element = do_QueryInterface(domElement);

  while (element) {
    
    element->GetAttr(kNameSpaceID_None, nsGkAtoms::bgcolor, aOutColor);

    
    if (!aOutColor.IsEmpty()) {
      return NS_OK;
    }

    
    if (element->IsHTML(nsGkAtoms::body)) {
      return NS_OK;
    }

    
    
    element = element->GetParentElement();
  }

  
  dom::Element* bodyElement = GetRoot();
  NS_ENSURE_TRUE(bodyElement, NS_ERROR_NULL_POINTER);

  bodyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::bgcolor, aOutColor);
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLEditor::GetListState(bool *aMixed, bool *aOL, bool *aUL, bool *aDL)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed && aOL && aUL && aDL, NS_ERROR_NULL_POINTER);
  nsRefPtr<nsHTMLEditRules> htmlRules = static_cast<nsHTMLEditRules*>(mRules.get());
  
  return htmlRules->GetListState(aMixed, aOL, aUL, aDL);
}

NS_IMETHODIMP 
nsHTMLEditor::GetListItemState(bool *aMixed, bool *aLI, bool *aDT, bool *aDD)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed && aLI && aDT && aDD, NS_ERROR_NULL_POINTER);

  nsRefPtr<nsHTMLEditRules> htmlRules = static_cast<nsHTMLEditRules*>(mRules.get());
  
  return htmlRules->GetListItemState(aMixed, aLI, aDT, aDD);
}

NS_IMETHODIMP
nsHTMLEditor::GetAlignment(bool *aMixed, nsIHTMLEditor::EAlignment *aAlign)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed && aAlign, NS_ERROR_NULL_POINTER);
  nsRefPtr<nsHTMLEditRules> htmlRules = static_cast<nsHTMLEditRules*>(mRules.get());
  
  return htmlRules->GetAlignment(aMixed, aAlign);
}


NS_IMETHODIMP 
nsHTMLEditor::GetIndentState(bool *aCanIndent, bool *aCanOutdent)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aCanIndent && aCanOutdent, NS_ERROR_NULL_POINTER);

  nsRefPtr<nsHTMLEditRules> htmlRules = static_cast<nsHTMLEditRules*>(mRules.get());
  
  return htmlRules->GetIndentState(aCanIndent, aCanOutdent);
}

NS_IMETHODIMP
nsHTMLEditor::MakeOrChangeList(const nsAString& aListType, bool entireList, const nsAString& aBulletType)
{
  nsresult res;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  bool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::makeList, nsIEditor::eNext);
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(EditAction::makeList);
  ruleInfo.blockType = &aListType;
  ruleInfo.entireList = entireList;
  ruleInfo.bulletType = &aBulletType;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    
    bool isCollapsed = selection->Collapsed();

    nsCOMPtr<nsIDOMNode> node;
    int32_t offset;
    res = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
    if (!node) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);
  
    if (isCollapsed)
    {
      
      nsCOMPtr<nsIDOMNode> parent = node;
      nsCOMPtr<nsIDOMNode> topChild = node;
      nsCOMPtr<nsIDOMNode> tmp;
    
      nsCOMPtr<nsIAtom> listAtom = do_GetAtom(aListType);
      while (!CanContainTag(parent, listAtom)) {
        parent->GetParentNode(getter_AddRefs(tmp));
        NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
        topChild = parent;
        parent = tmp;
      }
    
      if (parent != node)
      {
        
        res = SplitNodeDeep(topChild, node, offset, &offset);
        NS_ENSURE_SUCCESS(res, res);
      }

      
      nsCOMPtr<nsIDOMNode> newList;
      res = CreateNode(aListType, parent, offset, getter_AddRefs(newList));
      NS_ENSURE_SUCCESS(res, res);
      
      nsCOMPtr<nsIDOMNode> newItem;
      res = CreateNode(NS_LITERAL_STRING("li"), newList, 0, getter_AddRefs(newItem));
      NS_ENSURE_SUCCESS(res, res);
      res = selection->Collapse(newItem,0);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}


NS_IMETHODIMP
nsHTMLEditor::RemoveList(const nsAString& aListType)
{
  nsresult res;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  bool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::removeList, nsIEditor::eNext);
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(EditAction::removeList);
  if (aListType.LowerCaseEqualsLiteral("ol"))
    ruleInfo.bOrdered = true;
  else  ruleInfo.bOrdered = false;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  

  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}

nsresult
nsHTMLEditor::MakeDefinitionItem(const nsAString& aItemType)
{
  nsresult res;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  bool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::makeDefListItem, nsIEditor::eNext);
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo(EditAction::makeDefListItem);
  ruleInfo.blockType = &aItemType;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    
  }

  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}

nsresult
nsHTMLEditor::InsertBasicBlock(const nsAString& aBlockType)
{
  nsresult res;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  bool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::makeBasicBlock, nsIEditor::eNext);
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo(EditAction::makeBasicBlock);
  ruleInfo.blockType = &aBlockType;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    
    bool isCollapsed = selection->Collapsed();

    nsCOMPtr<nsIDOMNode> node;
    int32_t offset;
    res = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
    if (!node) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);
  
    if (isCollapsed)
    {
      
      nsCOMPtr<nsIDOMNode> parent = node;
      nsCOMPtr<nsIDOMNode> topChild = node;
      nsCOMPtr<nsIDOMNode> tmp;
    
      nsCOMPtr<nsIAtom> blockAtom = do_GetAtom(aBlockType);
      while (!CanContainTag(parent, blockAtom)) {
        parent->GetParentNode(getter_AddRefs(tmp));
        NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
        topChild = parent;
        parent = tmp;
      }
    
      if (parent != node)
      {
        
        res = SplitNodeDeep(topChild, node, offset, &offset);
        NS_ENSURE_SUCCESS(res, res);
      }

      
      nsCOMPtr<nsIDOMNode> newBlock;
      res = CreateNode(aBlockType, parent, offset, getter_AddRefs(newBlock));
      NS_ENSURE_SUCCESS(res, res);
    
      
      res = selection->Collapse(newBlock,0);
      NS_ENSURE_SUCCESS(res, res);  
    }
  }

  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::Indent(const nsAString& aIndent)
{
  nsresult res;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  bool cancel, handled;
  EditAction opID = EditAction::indent;
  if (aIndent.LowerCaseEqualsLiteral("outdent"))
  {
    opID = EditAction::outdent;
  }
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, opID, nsIEditor::eNext);
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(opID);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;
  
  if (!handled)
  {
    
    nsCOMPtr<nsIDOMNode> node;
    int32_t offset;
    bool isCollapsed = selection->Collapsed();

    res = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
    if (!node) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);
  
    if (aIndent.EqualsLiteral("indent"))
    {
      if (isCollapsed)
      {
        
        nsCOMPtr<nsIDOMNode> parent = node;
        nsCOMPtr<nsIDOMNode> topChild = node;
        nsCOMPtr<nsIDOMNode> tmp;
        while (!CanContainTag(parent, nsGkAtoms::blockquote)) {
          parent->GetParentNode(getter_AddRefs(tmp));
          NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
          topChild = parent;
          parent = tmp;
        }
    
        if (parent != node)
        {
          
          res = SplitNodeDeep(topChild, node, offset, &offset);
          NS_ENSURE_SUCCESS(res, res);
        }

        
        nsCOMPtr<nsIDOMNode> newBQ;
        res = CreateNode(NS_LITERAL_STRING("blockquote"), parent, offset,
                         getter_AddRefs(newBQ));
        NS_ENSURE_SUCCESS(res, res);
        
        res = selection->Collapse(newBQ,0);
        NS_ENSURE_SUCCESS(res, res);
        res = InsertText(NS_LITERAL_STRING(" "));
        NS_ENSURE_SUCCESS(res, res);
        
        res = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
        NS_ENSURE_SUCCESS(res, res);
        res = selection->Collapse(node,0);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}



NS_IMETHODIMP
nsHTMLEditor::Align(const nsAString& aAlignType)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::align, nsIEditor::eNext);

  nsCOMPtr<nsIDOMNode> node;
  bool cancel, handled;
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo(EditAction::align);
  ruleInfo.alignType = &aAlignType;
  nsresult res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || NS_FAILED(res))
    return res;
  
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}

already_AddRefed<Element>
nsHTMLEditor::GetElementOrParentByTagName(const nsAString& aTagName,
                                          nsINode* aNode)
{
  MOZ_ASSERT(!aTagName.IsEmpty());

  nsCOMPtr<nsINode> node = aNode;
  if (!node) {
    
    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_TRUE(selection, nullptr);

    nsCOMPtr<nsINode> anchorNode = selection->GetAnchorNode();
    NS_ENSURE_TRUE(anchorNode, nullptr);

    
    if (anchorNode->HasChildNodes() && anchorNode->IsContent()) {
      node = anchorNode->GetChildAt(selection->AnchorOffset());
    }
    
    if (!node) {
      node = anchorNode;
    }
  }

  nsCOMPtr<Element> current;
  if (node->IsElement()) {
    current = node->AsElement();
  } else if (node->GetParentElement()) {
    current = node->GetParentElement();
  } else {
    
    MOZ_ASSERT(!node->GetParentNode() ||
               !node->GetParentNode()->GetParentNode());
    return nullptr;
  }

  nsAutoString tagName(aTagName);
  ToLowerCase(tagName);
  bool getLink = IsLinkTag(tagName);
  bool getNamedAnchor = IsNamedAnchorTag(tagName);
  if (getLink || getNamedAnchor) {
    tagName.AssignLiteral("a");
  }
  bool findTableCell = tagName.EqualsLiteral("td");
  bool findList = tagName.EqualsLiteral("list");

  for (; current; current = current->GetParentElement()) {
    
    if ((getLink && nsHTMLEditUtils::IsLink(current)) ||
        (getNamedAnchor && nsHTMLEditUtils::IsNamedAnchor(current))) {
      return current.forget();
    }
    if (findList) {
      
      if (nsHTMLEditUtils::IsList(current)) {
        return current.forget();
      }
    } else if (findTableCell) {
      
      if (nsHTMLEditUtils::IsTableCell(current)) {
        return current.forget();
      }
    } else if (current->NodeName().Equals(tagName,
                   nsCaseInsensitiveStringComparator())) {
      return current.forget();
    }

    
    
    
    if (current->GetParentElement() &&
        current->GetParentElement()->Tag() == nsGkAtoms::body) {
      break;
    }
  }

  return nullptr;
}

NS_IMETHODIMP
nsHTMLEditor::GetElementOrParentByTagName(const nsAString& aTagName,
                                          nsIDOMNode* aNode,
                                          nsIDOMElement** aReturn)
{
  NS_ENSURE_TRUE(!aTagName.IsEmpty(), NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aReturn, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  nsCOMPtr<Element> parent =
    GetElementOrParentByTagName(aTagName, node);
  nsCOMPtr<nsIDOMElement> ret = do_QueryInterface(parent);

  if (!ret) {
    return NS_EDITOR_ELEMENT_NOT_FOUND;
  }

  ret.forget(aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetSelectedElement(const nsAString& aTagName, nsIDOMElement** aReturn)
{
  NS_ENSURE_TRUE(aReturn , NS_ERROR_NULL_POINTER);
  
  
  *aReturn = nullptr;
  
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  Selection* sel = static_cast<Selection*>(selection.get());

  bool bNodeFound = false;
  bool isCollapsed = selection->Collapsed();

  nsAutoString domTagName;
  nsAutoString TagName(aTagName);
  ToLowerCase(TagName);
  
  bool anyTag = (TagName.IsEmpty());
  bool isLinkTag = IsLinkTag(TagName);
  bool isNamedAnchorTag = IsNamedAnchorTag(TagName);
  
  nsCOMPtr<nsIDOMElement> selectedElement;
  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> startParent;
  int32_t startOffset, endOffset;
  res = range->GetStartContainer(getter_AddRefs(startParent));
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> endParent;
  res = range->GetEndContainer(getter_AddRefs(endParent));
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  if (startParent && startParent == endParent && (endOffset-startOffset) == 1)
  {
    nsCOMPtr<nsIDOMNode> selectedNode = GetChildAt(startParent, startOffset);
    NS_ENSURE_SUCCESS(res, NS_OK);
    if (selectedNode)
    {
      selectedNode->GetNodeName(domTagName);
      ToLowerCase(domTagName);

      
      if (anyTag || (TagName == domTagName) ||
          (isLinkTag && nsHTMLEditUtils::IsLink(selectedNode)) ||
          (isNamedAnchorTag && nsHTMLEditUtils::IsNamedAnchor(selectedNode)))
      {
        bNodeFound = true;
        selectedElement = do_QueryInterface(selectedNode);
      }
    }
  }

  if (!bNodeFound)
  {
    if (isLinkTag)
    {
      
      
      
      nsCOMPtr<nsIDOMNode> anchorNode;
      res = selection->GetAnchorNode(getter_AddRefs(anchorNode));
      NS_ENSURE_SUCCESS(res, res);
      int32_t anchorOffset = -1;
      if (anchorNode)
        selection->GetAnchorOffset(&anchorOffset);

      nsCOMPtr<nsIDOMNode> focusNode;
      res = selection->GetFocusNode(getter_AddRefs(focusNode));
      NS_ENSURE_SUCCESS(res, res);
      int32_t focusOffset = -1;
      if (focusNode)
        selection->GetFocusOffset(&focusOffset);

      
      if (NS_SUCCEEDED(res) && anchorNode)
      {
        nsCOMPtr<nsIDOMElement> parentLinkOfAnchor;
        res = GetElementOrParentByTagName(NS_LITERAL_STRING("href"), anchorNode, getter_AddRefs(parentLinkOfAnchor));
        
        if (NS_SUCCEEDED(res) && parentLinkOfAnchor)
        {
          if (isCollapsed)
          {
            
            bNodeFound = true;
          } else if(focusNode) 
          {  
            nsCOMPtr<nsIDOMElement> parentLinkOfFocus;
            res = GetElementOrParentByTagName(NS_LITERAL_STRING("href"), focusNode, getter_AddRefs(parentLinkOfFocus));
            if (NS_SUCCEEDED(res) && parentLinkOfFocus == parentLinkOfAnchor)
              bNodeFound = true;
          }

          
          if (bNodeFound) {
            
            *aReturn = parentLinkOfAnchor;
            NS_IF_ADDREF(*aReturn);
            return NS_OK;
          }
        }
        else if (anchorOffset >= 0)  
        {
          nsCOMPtr<nsIDOMNode> anchorChild;
          anchorChild = GetChildAt(anchorNode,anchorOffset);
          if (anchorChild && nsHTMLEditUtils::IsLink(anchorChild) && 
              (anchorNode == focusNode) && focusOffset == (anchorOffset+1))
          {
            selectedElement = do_QueryInterface(anchorChild);
            bNodeFound = true;
          }
        }
      }
    } 

    if (!isCollapsed)   
    {
      nsRefPtr<nsRange> currange = sel->GetRangeAt(0);
      if (currange) {
        nsCOMPtr<nsIContentIterator> iter =
          do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &res);
        NS_ENSURE_SUCCESS(res, res);

        iter->Init(currange);
        
        while (!iter->IsDone())
        {
          
          
          
          selectedElement = do_QueryInterface(iter->GetCurrentNode());
          if (selectedElement)
          {
            
            
            if (bNodeFound)
            {
              bNodeFound = false;
              break;
            }

            selectedElement->GetNodeName(domTagName);
            ToLowerCase(domTagName);

            if (anyTag)
            {
              
              selectedElement->GetTagName(TagName);
              ToLowerCase(TagName);
              anyTag = false;
            }

            
            
            nsCOMPtr<nsIDOMNode> selectedNode = do_QueryInterface(selectedElement);
            if ( (isLinkTag && nsHTMLEditUtils::IsLink(selectedNode)) ||
                (isNamedAnchorTag && nsHTMLEditUtils::IsNamedAnchor(selectedNode)) )
            {
              bNodeFound = true;
            } else if (TagName == domTagName) { 
              bNodeFound = true;
            }
            if (!bNodeFound)
            {
              
              break;
            }
          }
          iter->Next();
        }
      } else {
        
        isCollapsed = true;
        NS_WARNING("isCollapsed was FALSE, but no elements found in selection\n");
      }
    }
  }
  if (bNodeFound)
  {
    
    *aReturn = selectedElement;
    if (selectedElement)
    {  
      
      NS_ADDREF(*aReturn);
    }
  } 
  else res = NS_EDITOR_ELEMENT_NOT_FOUND;

  return res;
}

NS_IMETHODIMP
nsHTMLEditor::CreateElementWithDefaults(const nsAString& aTagName, nsIDOMElement** aReturn)
{
  nsresult res=NS_ERROR_NOT_INITIALIZED;
  if (aReturn)
    *aReturn = nullptr;


  NS_ENSURE_TRUE(!aTagName.IsEmpty() && aReturn, NS_ERROR_NULL_POINTER);
    
  nsAutoString TagName(aTagName);
  ToLowerCase(TagName);
  nsAutoString realTagName;

  if (IsLinkTag(TagName) || IsNamedAnchorTag(TagName))
  {
    realTagName.AssignLiteral("a");
  } else {
    realTagName = TagName;
  }
  
  

  nsCOMPtr<nsIDOMElement>newElement;
  nsCOMPtr<dom::Element> newContent;
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

  
  res = CreateHTMLContent(realTagName, getter_AddRefs(newContent));
  newElement = do_QueryInterface(newContent);
  if (NS_FAILED(res) || !newElement)
    return NS_ERROR_FAILURE;

  
  newElement->SetAttribute(NS_LITERAL_STRING("_moz_dirty"), EmptyString());

  
  if (TagName.EqualsLiteral("table")) {
    res = newElement->SetAttribute(NS_LITERAL_STRING("cellpadding"),NS_LITERAL_STRING("2"));
    NS_ENSURE_SUCCESS(res, res);
    res = newElement->SetAttribute(NS_LITERAL_STRING("cellspacing"),NS_LITERAL_STRING("2"));
    NS_ENSURE_SUCCESS(res, res);
    res = newElement->SetAttribute(NS_LITERAL_STRING("border"),NS_LITERAL_STRING("1"));
  } else if (TagName.EqualsLiteral("td"))
  {
    res = SetAttributeOrEquivalent(newElement, NS_LITERAL_STRING("valign"),
                                   NS_LITERAL_STRING("top"), true);
  }
  

  if (NS_SUCCEEDED(res))
  {
    *aReturn = newElement;
    
    NS_ADDREF(*aReturn);
  }

  return res;
}

NS_IMETHODIMP
nsHTMLEditor::InsertLinkAroundSelection(nsIDOMElement* aAnchorElement)
{
  NS_ENSURE_TRUE(aAnchorElement, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  if (!selection)
  {
    res = NS_ERROR_NULL_POINTER;
  }
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  if (selection->Collapsed()) {
    NS_WARNING("InsertLinkAroundSelection called but there is no selection!!!");
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor = do_QueryInterface(aAnchorElement);
  if (!anchor) {
    return NS_OK;
  }

  nsAutoString href;
  res = anchor->GetHref(href);
  NS_ENSURE_SUCCESS(res, res);
  if (href.IsEmpty()) {
    return NS_OK;
  }

  nsAutoEditBatch beginBatching(this);

  
  nsCOMPtr<nsIDOMMozNamedAttrMap> attrMap;
  aAnchorElement->GetAttributes(getter_AddRefs(attrMap));
  NS_ENSURE_TRUE(attrMap, NS_ERROR_FAILURE);

  uint32_t count;
  attrMap->GetLength(&count);
  nsAutoString name, value;

  for (uint32_t i = 0; i < count; ++i) {
    nsCOMPtr<nsIDOMAttr> attribute;
    res = attrMap->Item(i, getter_AddRefs(attribute));
    NS_ENSURE_SUCCESS(res, res);

    if (attribute) {
      
      
      name.Truncate();
      value.Truncate();

      res = attribute->GetName(name);
      NS_ENSURE_SUCCESS(res, res);

      res = attribute->GetValue(value);
      NS_ENSURE_SUCCESS(res, res);

      res = SetInlineProperty(nsEditProperty::a, name, value);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::SetHTMLBackgroundColor(const nsAString& aColor)
{
  NS_PRECONDITION(mDocWeak, "Missing Editor DOM Document");
  
  
  nsCOMPtr<nsIDOMElement> element;
  int32_t selectedCount;
  nsAutoString tagName;
  nsresult res = GetSelectedOrParentTableElement(tagName, &selectedCount,
                                                 getter_AddRefs(element));
  NS_ENSURE_SUCCESS(res, res);

  bool setColor = !aColor.IsEmpty();

  NS_NAMED_LITERAL_STRING(bgcolor, "bgcolor");
  if (element)
  {
    if (selectedCount > 0)
    {
      
      nsCOMPtr<nsIDOMElement> cell;
      res = GetFirstSelectedCell(nullptr, getter_AddRefs(cell));
      if (NS_SUCCEEDED(res) && cell)
      {
        while(cell)
        {
          if (setColor)
            res = SetAttribute(cell, bgcolor, aColor);
          else
            res = RemoveAttribute(cell, bgcolor);
          if (NS_FAILED(res)) break;

          GetNextSelectedCell(nullptr, getter_AddRefs(cell));
        };
        return res;
      }
    }
    
  } else {
    
    element = do_QueryInterface(GetRoot());
    NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);
  }
  
  if (setColor)
    res = SetAttribute(element, bgcolor, aColor);
  else
    res = RemoveAttribute(element, bgcolor);

  return res;
}

NS_IMETHODIMP nsHTMLEditor::SetBodyAttribute(const nsAString& aAttribute, const nsAString& aValue)
{
  

  NS_ASSERTION(mDocWeak, "Missing Editor DOM Document");
  
  
  nsCOMPtr<nsIDOMElement> bodyElement = do_QueryInterface(GetRoot());
  NS_ENSURE_TRUE(bodyElement, NS_ERROR_NULL_POINTER);

  
  return SetAttribute(bodyElement, aAttribute, aValue);
}

NS_IMETHODIMP
nsHTMLEditor::GetLinkedObjects(nsISupportsArray** aNodeList)
{
  NS_ENSURE_TRUE(aNodeList, NS_ERROR_NULL_POINTER);

  nsresult res;

  res = NS_NewISupportsArray(aNodeList);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(*aNodeList, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContentIterator> iter =
       do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &res);
  NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);
  if ((NS_SUCCEEDED(res)))
  {
    nsCOMPtr<nsIDocument> doc = GetDocument();
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    iter->Init(doc->GetRootElement());

    
    while (!iter->IsDone())
    {
      nsCOMPtr<nsIDOMNode> node (do_QueryInterface(iter->GetCurrentNode()));
      if (node)
      {
        
        nsCOMPtr<nsIURIRefObject> refObject;
        res = NS_NewHTMLURIRefObject(getter_AddRefs(refObject), node);
        if (NS_SUCCEEDED(res))
        {
          nsCOMPtr<nsISupports> isupp (do_QueryInterface(refObject));

          (*aNodeList)->AppendElement(isupp);
        }
      }
      iter->Next();
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::AddStyleSheet(const nsAString &aURL)
{
  
  if (EnableExistingStyleSheet(aURL))
    return NS_OK;

  
  
  
  
  mLastStyleSheetURL.Truncate();
  return ReplaceStyleSheet(aURL);
}

NS_IMETHODIMP
nsHTMLEditor::ReplaceStyleSheet(const nsAString& aURL)
{
  
  if (EnableExistingStyleSheet(aURL))
  {
    
    if (!mLastStyleSheetURL.IsEmpty() && !mLastStyleSheetURL.Equals(aURL))
      return EnableStyleSheet(mLastStyleSheetURL, false);

    return NS_OK;
  }

  
  NS_ENSURE_TRUE(mDocWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uaURI;
  nsresult rv = NS_NewURI(getter_AddRefs(uaURI), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  return ps->GetDocument()->CSSLoader()->
    LoadSheet(uaURI, nullptr, EmptyCString(), this);
}

NS_IMETHODIMP
nsHTMLEditor::RemoveStyleSheet(const nsAString &aURL)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  nsresult rv = GetStyleSheetForURL(aURL, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(sheet, NS_ERROR_UNEXPECTED);

  nsRefPtr<RemoveStyleSheetTxn> txn;
  rv = CreateTxnForRemoveStyleSheet(sheet, getter_AddRefs(txn));
  if (!txn) rv = NS_ERROR_NULL_POINTER;
  if (NS_SUCCEEDED(rv))
  {
    rv = DoTransaction(txn);
    if (NS_SUCCEEDED(rv))
      mLastStyleSheetURL.Truncate();        

    
    rv = RemoveStyleSheetFromList(aURL);
  }
  
  return rv;
}


NS_IMETHODIMP
nsHTMLEditor::AddOverrideStyleSheet(const nsAString& aURL)
{
  
  if (EnableExistingStyleSheet(aURL))
    return NS_OK;

  
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uaURI;
  nsresult rv = NS_NewURI(getter_AddRefs(uaURI), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsRefPtr<nsCSSStyleSheet> sheet;
  
  rv = ps->GetDocument()->CSSLoader()->
    LoadSheetSync(uaURI, true, true, getter_AddRefs(sheet));

  
  NS_ENSURE_TRUE(sheet, NS_ERROR_NULL_POINTER);

  
  
  ps->AddOverrideStyleSheet(sheet);

  ps->ReconstructStyleData();

  
  mLastOverrideStyleSheetURL = aURL;

  
  return AddNewStyleSheetToList(aURL, sheet);
}

NS_IMETHODIMP
nsHTMLEditor::ReplaceOverrideStyleSheet(const nsAString& aURL)
{
  
  if (EnableExistingStyleSheet(aURL))
  {
    
    if (!mLastOverrideStyleSheetURL.IsEmpty() && !mLastOverrideStyleSheetURL.Equals(aURL))
      return EnableStyleSheet(mLastOverrideStyleSheetURL, false);

    return NS_OK;
  }
  
  if (!mLastOverrideStyleSheetURL.IsEmpty())
    RemoveOverrideStyleSheet(mLastOverrideStyleSheetURL);

  return AddOverrideStyleSheet(aURL);
}


NS_IMETHODIMP
nsHTMLEditor::RemoveOverrideStyleSheet(const nsAString &aURL)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  GetStyleSheetForURL(aURL, getter_AddRefs(sheet));

  
  
  nsresult rv = RemoveStyleSheetFromList(aURL);

  NS_ENSURE_TRUE(sheet, NS_OK); 

  NS_ENSURE_TRUE(mDocWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  ps->RemoveOverrideStyleSheet(sheet);
  ps->ReconstructStyleData();

  
  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::EnableStyleSheet(const nsAString &aURL, bool aEnable)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  nsresult rv = GetStyleSheetForURL(aURL, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(sheet, NS_OK); 

  
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  sheet->SetOwningDocument(doc);

  return sheet->SetDisabled(!aEnable);
}

bool
nsHTMLEditor::EnableExistingStyleSheet(const nsAString &aURL)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  nsresult rv = GetStyleSheetForURL(aURL, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, false);

  
  if (sheet)
  {
    
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
    sheet->SetOwningDocument(doc);

    sheet->SetDisabled(false);
    return true;
  }
  return false;
}

nsresult
nsHTMLEditor::AddNewStyleSheetToList(const nsAString &aURL,
                                     nsCSSStyleSheet *aStyleSheet)
{
  uint32_t countSS = mStyleSheets.Length();
  uint32_t countU = mStyleSheetURLs.Length();

  if (countSS != countU)
    return NS_ERROR_UNEXPECTED;

  if (!mStyleSheetURLs.AppendElement(aURL))
    return NS_ERROR_UNEXPECTED;

  return mStyleSheets.AppendElement(aStyleSheet) ? NS_OK : NS_ERROR_UNEXPECTED;
}

nsresult
nsHTMLEditor::RemoveStyleSheetFromList(const nsAString &aURL)
{
  
  uint32_t foundIndex;
  foundIndex = mStyleSheetURLs.IndexOf(aURL);
  if (foundIndex == mStyleSheetURLs.NoIndex)
    return NS_ERROR_FAILURE;

  
  mStyleSheets.RemoveElementAt(foundIndex);
  mStyleSheetURLs.RemoveElementAt(foundIndex);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetStyleSheetForURL(const nsAString &aURL,
                                  nsCSSStyleSheet **aStyleSheet)
{
  NS_ENSURE_ARG_POINTER(aStyleSheet);
  *aStyleSheet = 0;

  
  uint32_t foundIndex;
  foundIndex = mStyleSheetURLs.IndexOf(aURL);
  if (foundIndex == mStyleSheetURLs.NoIndex)
    return NS_OK; 

  *aStyleSheet = mStyleSheets[foundIndex];
  NS_ENSURE_TRUE(*aStyleSheet, NS_ERROR_FAILURE);

  NS_ADDREF(*aStyleSheet);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetURLForStyleSheet(nsCSSStyleSheet *aStyleSheet,
                                  nsAString &aURL)
{
  
  int32_t foundIndex = mStyleSheets.IndexOf(aStyleSheet);

  
  
  
  if (foundIndex == -1)
    return NS_OK;

  
  aURL = mStyleSheetURLs[foundIndex];
  return NS_OK;
}





NS_IMETHODIMP
nsHTMLEditor::GetEmbeddedObjects(nsISupportsArray** aNodeList)
{
  NS_ENSURE_TRUE(aNodeList, NS_ERROR_NULL_POINTER);

  nsresult rv = NS_NewISupportsArray(aNodeList);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(*aNodeList, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContentIterator> iter =
      do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &rv);
  NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc = GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

  iter->Init(doc->GetRootElement());

  
  while (!iter->IsDone()) {
    nsINode* node = iter->GetCurrentNode();
    if (node->IsElement()) {
      dom::Element* element = node->AsElement();

      
      
      if (element->IsHTML(nsGkAtoms::img) ||
          element->IsHTML(nsGkAtoms::embed) ||
          element->IsHTML(nsGkAtoms::a) ||
          (element->IsHTML(nsGkAtoms::body) &&
           element->HasAttr(kNameSpaceID_None, nsGkAtoms::background))) {
        nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(node);
        (*aNodeList)->AppendElement(domNode);
      }
    }
    iter->Next();
  }

  return rv;
}


NS_IMETHODIMP
nsHTMLEditor::DeleteSelectionImpl(EDirection aAction,
                                  EStripWrappers aStripWrappers)
{
  MOZ_ASSERT(aStripWrappers == eStrip || aStripWrappers == eNoStrip);

  nsresult res = nsEditor::DeleteSelectionImpl(aAction, aStripWrappers);
  NS_ENSURE_SUCCESS(res, res);

  
  if (aStripWrappers == eNoStrip) {
    return NS_OK;
  }

  nsRefPtr<Selection> selection = GetSelection();
  
  
  NS_ENSURE_STATE(selection);
  NS_ENSURE_STATE(selection->GetAnchorFocusRange());
  NS_ENSURE_STATE(selection->GetAnchorFocusRange()->Collapsed());

  NS_ENSURE_STATE(selection->GetAnchorNode()->IsContent());
  nsCOMPtr<nsIContent> content = selection->GetAnchorNode()->AsContent();

  
  
  nsCOMPtr<nsIContent> blockParent = content;
  while (blockParent && !IsBlockNode(blockParent)) {
    blockParent = blockParent->GetParent();
  }
  if (!blockParent) {
    return NS_OK;
  }
  bool emptyBlockParent;
  res = IsEmptyNode(blockParent, &emptyBlockParent);
  NS_ENSURE_SUCCESS(res, res);
  if (emptyBlockParent) {
    return NS_OK;
  }

  if (content && !IsBlockNode(content) && !content->Length() &&
      content->IsEditable() && content != content->GetEditingHost()) {
    while (content->GetParent() && !IsBlockNode(content->GetParent()) &&
           content->GetParent()->Length() == 1 &&
           content->GetParent()->IsEditable() &&
           content->GetParent() != content->GetEditingHost()) {
      content = content->GetParent();
    }
    res = DeleteNode(content);
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}


nsresult
nsHTMLEditor::DeleteNode(nsINode* aNode)
{
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode);
  return DeleteNode(node);
}

NS_IMETHODIMP
nsHTMLEditor::DeleteNode(nsIDOMNode* aNode)
{
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (!IsModifiableNode(aNode) && !IsMozEditorBogusNode(content)) {
    return NS_ERROR_FAILURE;
  }

  return nsEditor::DeleteNode(aNode);
}

NS_IMETHODIMP nsHTMLEditor::DeleteText(nsIDOMCharacterData *aTextNode,
                                       uint32_t             aOffset,
                                       uint32_t             aLength)
{
  
  if (!IsModifiableNode(aTextNode)) {
    return NS_ERROR_FAILURE;
  }

  return nsEditor::DeleteText(aTextNode, aOffset, aLength);
}

NS_IMETHODIMP nsHTMLEditor::InsertTextImpl(const nsAString& aStringToInsert, 
                                           nsCOMPtr<nsIDOMNode> *aInOutNode, 
                                           int32_t *aInOutOffset,
                                           nsIDOMDocument *aDoc)
{
  
  if (!IsModifiableNode(*aInOutNode)) {
    return NS_ERROR_FAILURE;
  }

  return nsEditor::InsertTextImpl(aStringToInsert, aInOutNode, aInOutOffset, aDoc);
}

void
nsHTMLEditor::ContentAppended(nsIDocument *aDocument, nsIContent* aContainer,
                              nsIContent* aFirstNewContent,
                              int32_t aIndexInContainer)
{
  DoContentInserted(aDocument, aContainer, aFirstNewContent, aIndexInContainer,
                    eAppended);
}

void
nsHTMLEditor::ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                              nsIContent* aChild, int32_t aIndexInContainer)
{
  DoContentInserted(aDocument, aContainer, aChild, aIndexInContainer,
                    eInserted);
}

void
nsHTMLEditor::DoContentInserted(nsIDocument* aDocument, nsIContent* aContainer,
                                nsIContent* aChild, int32_t aIndexInContainer,
                                InsertedOrAppended aInsertedOrAppended)
{
  if (!aChild) {
    return;
  }

  nsCOMPtr<nsIHTMLEditor> kungFuDeathGrip(this);

  if (ShouldReplaceRootElement()) {
    nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(
      this, &nsHTMLEditor::ResetRootElementAndEventTarget));
  }
  
  else if (!mAction && (aContainer ? aContainer->IsEditable() : aDocument->IsEditable())) {
    if (IsMozEditorBogusNode(aChild)) {
      
      return;
    }
    
    nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
    mRules->DocumentModified();

    
    if (mInlineSpellChecker) {
      nsRefPtr<nsRange> range = new nsRange(aChild);
      int32_t endIndex = aIndexInContainer + 1;
      if (aInsertedOrAppended == eAppended) {
        
        nsIContent* sibling = aChild->GetNextSibling();
        while (sibling) {
          endIndex++;
          sibling = sibling->GetNextSibling();
        }
      }
      nsresult res = range->Set(aContainer, aIndexInContainer,
                                aContainer, endIndex);
      if (NS_SUCCEEDED(res)) {
        mInlineSpellChecker->SpellCheckRange(range);
      }
    }
  }
}

void
nsHTMLEditor::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                             nsIContent* aChild, int32_t aIndexInContainer,
                             nsIContent* aPreviousSibling)
{
  nsCOMPtr<nsIHTMLEditor> kungFuDeathGrip(this);

  if (SameCOMIdentity(aChild, mRootElement)) {
    nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(
      this, &nsHTMLEditor::ResetRootElementAndEventTarget));
  }
  
  else if (!mAction && (aContainer ? aContainer->IsEditable() : aDocument->IsEditable())) {
    if (aChild && IsMozEditorBogusNode(aChild)) {
      
      return;
    }
    
    nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
    mRules->DocumentModified();
  }
}

NS_IMETHODIMP_(bool)
nsHTMLEditor::IsModifiableNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return IsModifiableNode(node);
}

bool
nsHTMLEditor::IsModifiableNode(nsINode *aNode)
{
  return !aNode || aNode->IsEditable();
}

NS_IMETHODIMP
nsHTMLEditor::GetIsSelectionEditable(bool* aIsSelectionEditable)
{
  MOZ_ASSERT(aIsSelectionEditable);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  
  
  *aIsSelectionEditable = selection->GetRangeCount() &&
                          selection->GetAnchorNode()->IsEditable() &&
                          selection->GetFocusNode()->IsEditable();

  if (*aIsSelectionEditable) {
    nsINode* commonAncestor =
      selection->GetAnchorFocusRange()->GetCommonAncestor();
    while (commonAncestor && !commonAncestor->IsEditable()) {
      commonAncestor = commonAncestor->GetParentNode();
    }
    if (!commonAncestor) {
      
      *aIsSelectionEditable = false;
    }
  }

  return NS_OK;
}

static nsresult
SetSelectionAroundHeadChildren(nsISelection* aSelection,
                               nsIWeakReference* aDocWeak)
{
  
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(aDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

  dom::Element* headNode = doc->GetHeadElement();
  NS_ENSURE_STATE(headNode);

  
  nsresult rv = aSelection->CollapseNative(headNode, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  uint32_t childCount = headNode->GetChildCount();
  return aSelection->ExtendNative(headNode, childCount + 1);
}

NS_IMETHODIMP
nsHTMLEditor::GetHeadContentsAsHTML(nsAString& aOutputString)
{
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsAutoSelectionReset selectionResetter(selection, this);

  nsresult res = SetSelectionAroundHeadChildren(selection, mDocWeak);
  NS_ENSURE_SUCCESS(res, res);

  res = OutputToString(NS_LITERAL_STRING("text/html"),
                       nsIDocumentEncoder::OutputSelectionOnly,
                       aOutputString);
  if (NS_SUCCEEDED(res))
  {
    
    
    nsReadingIterator<char16_t> findIter,endFindIter;
    aOutputString.BeginReading(findIter);
    aOutputString.EndReading(endFindIter);
    
    if (CaseInsensitiveFindInReadable(NS_LITERAL_STRING("<body"),
                                      findIter, endFindIter))
    {
      nsReadingIterator<char16_t> beginIter;
      aOutputString.BeginReading(beginIter);
      int32_t offset = Distance(beginIter, findIter);

      nsWritingIterator<char16_t> writeIter;
      aOutputString.BeginWriting(writeIter);
      
      char16_t newline ('\n');
      findIter.advance(-1);
      if (offset ==0 || (offset >0 &&  (*findIter) != newline)) 
      {
        writeIter.advance(offset);
        *writeIter = newline;
        aOutputString.Truncate(offset+1);
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::DebugUnitTests(int32_t *outNumTests, int32_t *outNumTestsFailed)
{
#ifdef DEBUG
  NS_ENSURE_TRUE(outNumTests && outNumTestsFailed, NS_ERROR_NULL_POINTER);

  TextEditorTest *tester = new TextEditorTest();
  NS_ENSURE_TRUE(tester, NS_ERROR_OUT_OF_MEMORY);
   
  tester->Run(this, outNumTests, outNumTestsFailed);
  delete tester;
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}


NS_IMETHODIMP 
nsHTMLEditor::StyleSheetLoaded(nsCSSStyleSheet* aSheet, bool aWasAlternate,
                               nsresult aStatus)
{
  nsresult rv = NS_OK;
  nsAutoEditBatch batchIt(this);

  if (!mLastStyleSheetURL.IsEmpty())
    RemoveStyleSheet(mLastStyleSheetURL);

  nsRefPtr<AddStyleSheetTxn> txn;
  rv = CreateTxnForAddStyleSheet(aSheet, getter_AddRefs(txn));
  if (!txn) rv = NS_ERROR_NULL_POINTER;
  if (NS_SUCCEEDED(rv))
  {
    rv = DoTransaction(txn);
    if (NS_SUCCEEDED(rv))
    {
      
      nsAutoCString spec;
      rv = aSheet->GetSheetURI()->GetSpec(spec);

      if (NS_SUCCEEDED(rv))
      {
        
        mLastStyleSheetURL.AssignWithConversion(spec.get());

        
        AddNewStyleSheetToList(mLastStyleSheetURL, aSheet);
      }
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsHTMLEditor::StartOperation(EditAction opID,
                             nsIEditor::EDirection aDirection)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsEditor::StartOperation(opID, aDirection);  
  if (mRules) return mRules->BeforeEdit(mAction, mDirection);
  return NS_OK;
}




NS_IMETHODIMP
nsHTMLEditor::EndOperation()
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  nsresult res = NS_OK;
  if (mRules) res = mRules->AfterEdit(mAction, mDirection);
  nsEditor::EndOperation();  
  return res;
}  

bool 
nsHTMLEditor::TagCanContainTag(nsIAtom* aParentTag, nsIAtom* aChildTag)
{
  MOZ_ASSERT(aParentTag && aChildTag);

  nsIParserService* parserService = nsContentUtils::GetParserService();

  int32_t childTagEnum;
  
  if (aChildTag == nsGkAtoms::textTagName) {
    childTagEnum = eHTMLTag_text;
  } else {
    childTagEnum = parserService->HTMLAtomTagToId(aChildTag);
  }

  int32_t parentTagEnum = parserService->HTMLAtomTagToId(aParentTag);
  return nsHTMLEditUtils::CanContain(parentTagEnum, childTagEnum);
}

bool
nsHTMLEditor::IsContainer(nsIDOMNode *aNode)
{
  if (!aNode) {
    return false;
  }

  nsAutoString stringTag;

  nsresult rv = aNode->GetNodeName(stringTag);
  NS_ENSURE_SUCCESS(rv, false);

  int32_t tagEnum;
  
  if (stringTag.EqualsLiteral("#text")) {
    tagEnum = eHTMLTag_text;
  }
  else {
    tagEnum = nsContentUtils::GetParserService()->HTMLStringTagToId(stringTag);
  }

  return nsHTMLEditUtils::IsContainer(tagEnum);
}


NS_IMETHODIMP 
nsHTMLEditor::SelectEntireDocument(nsISelection *aSelection)
{
  if (!aSelection || !mRules) { return NS_ERROR_NULL_POINTER; }
  
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(GetRoot());
  
  
  bool bDocIsEmpty;
  nsresult res = mRules->DocumentIsEmpty(&bDocIsEmpty);
  NS_ENSURE_SUCCESS(res, res);
    
  if (bDocIsEmpty)
  {
    
    return aSelection->Collapse(rootElement, 0);
  }

  return nsEditor::SelectEntireDocument(aSelection);
}

NS_IMETHODIMP
nsHTMLEditor::SelectAll()
{
  ForceCompositionEnd();

  nsresult rv;
  nsCOMPtr<nsISelectionController> selCon;
  rv = GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelection> selection;
  rv = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                            getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> anchorNode;
  rv = selection->GetAnchorNode(getter_AddRefs(anchorNode));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> anchorContent = do_QueryInterface(anchorNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  if (anchorContent->HasIndependentSelection()) {
    nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection);
    NS_ENSURE_TRUE(selPriv, NS_ERROR_UNEXPECTED);
    rv = selPriv->SetAncestorLimiter(nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMNode> rootElement = do_QueryInterface(mRootElement, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    return selection->SelectAllChildren(rootElement);
  }

  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  nsIContent *rootContent = anchorContent->GetSelectionRootContent(ps);
  NS_ENSURE_TRUE(rootContent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNode> rootElement = do_QueryInterface(rootContent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return selection->SelectAllChildren(rootElement);
}




bool nsHTMLEditor::IsTextPropertySetByContent(nsIContent*      aContent,
                                              nsIAtom*         aProperty,
                                              const nsAString* aAttribute,
                                              const nsAString* aValue,
                                              nsAString*       outValue)
{
  MOZ_ASSERT(aContent && aProperty);
  MOZ_ASSERT_IF(aAttribute, aValue);
  bool isSet;
  IsTextPropertySetByContent(aContent->AsDOMNode(), aProperty, aAttribute,
                             aValue, isSet, outValue);
  return isSet;
}

void nsHTMLEditor::IsTextPropertySetByContent(nsIDOMNode        *aNode,
                                              nsIAtom           *aProperty, 
                                              const nsAString   *aAttribute, 
                                              const nsAString   *aValue, 
                                              bool              &aIsSet,
                                              nsAString *outValue)
{
  nsresult result;
  aIsSet = false;  
  nsAutoString propName;
  aProperty->ToString(propName);
  nsCOMPtr<nsIDOMNode>node = aNode;

  while (node)
  {
    nsCOMPtr<nsIDOMElement>element;
    element = do_QueryInterface(node);
    if (element)
    {
      nsAutoString tag, value;
      element->GetTagName(tag);
      if (propName.Equals(tag, nsCaseInsensitiveStringComparator()))
      {
        bool found = false;
        if (aAttribute && 0!=aAttribute->Length())
        {
          element->GetAttribute(*aAttribute, value);
          if (outValue) *outValue = value;
          if (!value.IsEmpty())
          {
            if (!aValue) {
              found = true;
            }
            else
            {
              nsString tString(*aValue);
              if (tString.Equals(value, nsCaseInsensitiveStringComparator())) {
                found = true;
              }
              else {  
                break;
              }
            }
          }
        }
        else { 
          found = true;
        }
        if (found)
        {
          aIsSet = true;
          break;
        }
      }
    }
    nsCOMPtr<nsIDOMNode>temp;
    result = node->GetParentNode(getter_AddRefs(temp));
    if (NS_SUCCEEDED(result) && temp) {
      node = temp;
    }
    else {
      node = nullptr;
    }
  }
}









bool
nsHTMLEditor::SetCaretInTableCell(nsIDOMElement* aElement)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aElement);
  if (!element || !element->IsHTML() ||
      !nsHTMLEditUtils::IsTableElement(element) ||
      !IsDescendantOfEditorRoot(element)) {
    return false;
  }

  nsIContent* node = element;
  while (node->HasChildren()) {
    node = node->GetFirstChild();
  }

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(selection, false);

  return NS_SUCCEEDED(selection->CollapseNative(node, 0));
}            




nsCOMPtr<nsIDOMNode> 
nsHTMLEditor::GetEnclosingTable(nsIDOMNode *aNode)
{
  NS_PRECONDITION(aNode, "null node passed to nsHTMLEditor::GetEnclosingTable");
  nsCOMPtr<nsIDOMNode> tbl, tmp, node = aNode;

  while (!tbl)
  {
    tmp = GetBlockNodeParent(node);
    if (!tmp) break;
    if (nsHTMLEditUtils::IsTable(tmp)) tbl = tmp;
    node = tmp;
  }
  return tbl;
}








NS_IMETHODIMP
nsHTMLEditor::CollapseAdjacentTextNodes(nsIDOMRange *aInRange)
{
  NS_ENSURE_TRUE(aInRange, NS_ERROR_NULL_POINTER);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  nsTArray<nsCOMPtr<nsIDOMNode> > textNodes;
  
  
  


  
  nsresult result;
  nsCOMPtr<nsIContentIterator> iter =
    do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &result);
  NS_ENSURE_SUCCESS(result, result);

  iter->Init(aInRange);

  while (!iter->IsDone())
  {
    nsINode* node = iter->GetCurrentNode();
    if (node->NodeType() == nsIDOMNode::TEXT_NODE &&
        IsEditable(static_cast<nsIContent*>(node))) {
      nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(node);
      textNodes.AppendElement(domNode);
    }

    iter->Next();
  }

  
  
  while (textNodes.Length() > 1)
  {
    
    nsIDOMNode *leftTextNode = textNodes[0];
    nsIDOMNode *rightTextNode = textNodes[1];
    NS_ASSERTION(leftTextNode && rightTextNode,"left or rightTextNode null in CollapseAdjacentTextNodes");

    
    nsCOMPtr<nsIDOMNode> prevSibOfRightNode;
    result =
      rightTextNode->GetPreviousSibling(getter_AddRefs(prevSibOfRightNode));
    NS_ENSURE_SUCCESS(result, result);
    if (prevSibOfRightNode && (prevSibOfRightNode == leftTextNode))
    {
      nsCOMPtr<nsIDOMNode> parent;
      result = rightTextNode->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(result, result);
      NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);
      result = JoinNodes(leftTextNode, rightTextNode, parent);
      NS_ENSURE_SUCCESS(result, result);
    }

    textNodes.RemoveElementAt(0); 
  }

  return result;
}

NS_IMETHODIMP 
nsHTMLEditor::SetSelectionAtDocumentStart(nsISelection *aSelection)
{
  dom::Element* rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER);

  return aSelection->CollapseNative(rootElement, 0);
}







nsresult
nsHTMLEditor::RemoveBlockContainer(nsIDOMNode *inNode)
{
  NS_ENSURE_TRUE(inNode, NS_ERROR_NULL_POINTER);
  nsresult res;
  nsCOMPtr<nsIDOMNode> sibling, child, unused;
  
  
  
  
  
  
  
  
  
  res = GetFirstEditableChild(inNode, address_of(child));
  NS_ENSURE_SUCCESS(res, res);
  
  if (child)  
  {
    
    
    
    
    
    
    res = GetPriorHTMLSibling(inNode, address_of(sibling));
    NS_ENSURE_SUCCESS(res, res);
    if (sibling && !IsBlockNode(sibling) && !nsTextEditUtils::IsBreak(sibling))
    {
      res = GetFirstEditableChild(inNode, address_of(child));
      NS_ENSURE_SUCCESS(res, res);
      if (child && !IsBlockNode(child))
      {
        
        res = CreateBR(inNode, 0, address_of(unused));
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    
    
    
    
    
    

    res = GetNextHTMLSibling(inNode, address_of(sibling));
    NS_ENSURE_SUCCESS(res, res);
    if (sibling && !IsBlockNode(sibling))
    {
      res = GetLastEditableChild(inNode, address_of(child));
      NS_ENSURE_SUCCESS(res, res);
      if (child && !IsBlockNode(child) && !nsTextEditUtils::IsBreak(child))
      {
        
        uint32_t len;
        res = GetLengthOfDOMNode(inNode, len);
        NS_ENSURE_SUCCESS(res, res);
        res = CreateBR(inNode, (int32_t)len, address_of(unused));
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  else  
  {
    
    
    
    
    
    
    res = GetPriorHTMLSibling(inNode, address_of(sibling));
    NS_ENSURE_SUCCESS(res, res);
    if (sibling && !IsBlockNode(sibling) && !nsTextEditUtils::IsBreak(sibling))
    {
      res = GetNextHTMLSibling(inNode, address_of(sibling));
      NS_ENSURE_SUCCESS(res, res);
      if (sibling && !IsBlockNode(sibling) && !nsTextEditUtils::IsBreak(sibling))
      {
        
        res = CreateBR(inNode, 0, address_of(unused));
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
    
  
  return RemoveContainer(inNode);
}






nsIContent*
nsHTMLEditor::GetPriorHTMLSibling(nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  nsIContent* node = aNode->GetPreviousSibling();
  while (node && !IsEditable(node)) {
    node = node->GetPreviousSibling();
  }

  return node;
}

nsresult
nsHTMLEditor::GetPriorHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  *outNode = nullptr;

  nsCOMPtr<nsINode> node = do_QueryInterface(inNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  *outNode = do_QueryInterface(GetPriorHTMLSibling(node));
  return NS_OK;
}








nsIContent*
nsHTMLEditor::GetPriorHTMLSibling(nsINode* aParent, int32_t aOffset)
{
  MOZ_ASSERT(aParent);

  nsIContent* node = aParent->GetChildAt(aOffset - 1);
  if (!node || IsEditable(node)) {
    return node;
  }

  return GetPriorHTMLSibling(node);
}

nsresult
nsHTMLEditor::GetPriorHTMLSibling(nsIDOMNode *inParent, int32_t inOffset, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  *outNode = nullptr;

  nsCOMPtr<nsINode> parent = do_QueryInterface(inParent);
  NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);

  *outNode = do_QueryInterface(GetPriorHTMLSibling(parent, inOffset));
  return NS_OK;
}







nsIContent*
nsHTMLEditor::GetNextHTMLSibling(nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  nsIContent* node = aNode->GetNextSibling();
  while (node && !IsEditable(node)) {
    node = node->GetNextSibling();
  }

  return node;
}

nsresult
nsHTMLEditor::GetNextHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  *outNode = nullptr;

  nsCOMPtr<nsINode> node = do_QueryInterface(inNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  
  *outNode = do_QueryInterface(GetNextHTMLSibling(node));
  return NS_OK;
}







nsIContent*
nsHTMLEditor::GetNextHTMLSibling(nsINode* aParent, int32_t aOffset)
{
  MOZ_ASSERT(aParent);

  nsIContent* node = aParent->GetChildAt(aOffset + 1);
  if (!node || IsEditable(node)) {
    return node;
  }

  return GetNextHTMLSibling(node);
}

nsresult
nsHTMLEditor::GetNextHTMLSibling(nsIDOMNode *inParent, int32_t inOffset, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  *outNode = nullptr;

  nsCOMPtr<nsINode> parent = do_QueryInterface(inParent);
  NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);

  *outNode = do_QueryInterface(GetNextHTMLSibling(parent, inOffset));
  return NS_OK;
}







nsIContent*
nsHTMLEditor::GetPriorHTMLNode(nsINode* aNode, bool aNoBlockCrossing)
{
  MOZ_ASSERT(aNode);

  if (!GetActiveEditingHost()) {
    return nullptr;
  }

  return GetPriorNode(aNode, true, aNoBlockCrossing);
}

nsresult
nsHTMLEditor::GetPriorHTMLNode(nsIDOMNode* aNode,
                               nsCOMPtr<nsIDOMNode>* aResultNode,
                               bool aNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetPriorHTMLNode(node, aNoBlockCrossing));
  return NS_OK;
}





nsIContent*
nsHTMLEditor::GetPriorHTMLNode(nsINode* aParent, int32_t aOffset,
                               bool aNoBlockCrossing)
{
  MOZ_ASSERT(aParent);

  if (!GetActiveEditingHost()) {
    return nullptr;
  }

  return GetPriorNode(aParent, aOffset, true, aNoBlockCrossing);
}

nsresult
nsHTMLEditor::GetPriorHTMLNode(nsIDOMNode* aNode, int32_t aOffset,
                               nsCOMPtr<nsIDOMNode>* aResultNode,
                               bool aNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetPriorHTMLNode(node, aOffset,
                                                    aNoBlockCrossing));
  return NS_OK;
}






nsIContent*
nsHTMLEditor::GetNextHTMLNode(nsINode* aNode, bool aNoBlockCrossing)
{
  MOZ_ASSERT(aNode);

  nsIContent* result = GetNextNode(aNode, true, aNoBlockCrossing);

  if (result && !IsDescendantOfEditorRoot(result)) {
    return nullptr;
  }

  return result;
}

nsresult
nsHTMLEditor::GetNextHTMLNode(nsIDOMNode* aNode,
                              nsCOMPtr<nsIDOMNode>* aResultNode,
                              bool aNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetNextHTMLNode(node, aNoBlockCrossing));
  return NS_OK;
}





nsIContent*
nsHTMLEditor::GetNextHTMLNode(nsINode* aParent, int32_t aOffset,
                              bool aNoBlockCrossing)
{
  nsIContent* content = GetNextNode(aParent, aOffset, true, aNoBlockCrossing);
  if (content && !IsDescendantOfEditorRoot(content)) {
    return nullptr;
  }
  return content;
}

nsresult
nsHTMLEditor::GetNextHTMLNode(nsIDOMNode* aNode, int32_t aOffset,
                              nsCOMPtr<nsIDOMNode>* aResultNode,
                              bool aNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetNextHTMLNode(node, aOffset,
                                                   aNoBlockCrossing));
  return NS_OK;
}


nsresult 
nsHTMLEditor::IsFirstEditableChild( nsIDOMNode *aNode, bool *aOutIsFirst)
{
  
  NS_ENSURE_TRUE(aOutIsFirst && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutIsFirst = false;
  
  
  nsCOMPtr<nsIDOMNode> parent, firstChild;
  nsresult res = aNode->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);
  res = GetFirstEditableChild(parent, address_of(firstChild));
  NS_ENSURE_SUCCESS(res, res);
  
  *aOutIsFirst = (firstChild.get() == aNode);
  return res;
}


nsresult 
nsHTMLEditor::IsLastEditableChild( nsIDOMNode *aNode, bool *aOutIsLast)
{
  
  NS_ENSURE_TRUE(aOutIsLast && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutIsLast = false;
  
  
  nsCOMPtr<nsIDOMNode> parent, lastChild;
  nsresult res = aNode->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);
  res = GetLastEditableChild(parent, address_of(lastChild));
  NS_ENSURE_SUCCESS(res, res);
  
  *aOutIsLast = (lastChild.get() == aNode);
  return res;
}


nsresult 
nsHTMLEditor::GetFirstEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstChild)
{
  
  NS_ENSURE_TRUE(aOutFirstChild && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutFirstChild = nullptr;
  
  
  nsCOMPtr<nsIDOMNode> child;
  nsresult res = aNode->GetFirstChild(getter_AddRefs(child));
  NS_ENSURE_SUCCESS(res, res);
  
  while (child && !IsEditable(child))
  {
    nsCOMPtr<nsIDOMNode> tmp;
    res = child->GetNextSibling(getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    child = tmp;
  }
  
  *aOutFirstChild = child;
  return res;
}


nsresult 
nsHTMLEditor::GetLastEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastChild)
{
  
  NS_ENSURE_TRUE(aOutLastChild && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutLastChild = aNode;
  
  
  nsCOMPtr<nsIDOMNode> child;
  nsresult res = aNode->GetLastChild(getter_AddRefs(child));
  NS_ENSURE_SUCCESS(res, res);
  
  while (child && !IsEditable(child))
  {
    nsCOMPtr<nsIDOMNode> tmp;
    res = child->GetPreviousSibling(getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    child = tmp;
  }
  
  *aOutLastChild = child;
  return res;
}

nsresult 
nsHTMLEditor::GetFirstEditableLeaf( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstLeaf)
{
  
  NS_ENSURE_TRUE(aOutFirstLeaf && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutFirstLeaf = aNode;
  
  
  nsCOMPtr<nsIDOMNode> child;
  nsresult res = NS_OK;
  child = GetLeftmostChild(aNode);  
  while (child && (!IsEditable(child) || !nsEditorUtils::IsLeafNode(child)))
  {
    nsCOMPtr<nsIDOMNode> tmp;
    res = GetNextHTMLNode(child, address_of(tmp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    
    
    if (nsEditorUtils::IsDescendantOf(tmp, aNode))
      child = tmp;
    else
    {
      child = nullptr;  
    }
  }
  
  *aOutFirstLeaf = child;
  return res;
}


nsresult 
nsHTMLEditor::GetLastEditableLeaf(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastLeaf)
{
  
  NS_ENSURE_TRUE(aOutLastLeaf && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutLastLeaf = nullptr;
  
  
  nsCOMPtr<nsIDOMNode> child = GetRightmostChild(aNode, false);
  nsresult res = NS_OK;
  while (child && (!IsEditable(child) || !nsEditorUtils::IsLeafNode(child)))
  {
    nsCOMPtr<nsIDOMNode> tmp;
    res = GetPriorHTMLNode(child, address_of(tmp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    
    
    if (nsEditorUtils::IsDescendantOf(tmp, aNode))
      child = tmp;
    else
    {
      child = nullptr;
    }
  }
  
  *aOutLastLeaf = child;
  return res;
}





nsresult
nsHTMLEditor::IsVisTextNode(nsIContent* aNode,
                            bool* outIsEmptyNode,
                            bool aSafeToAskFrames)
{
  MOZ_ASSERT(aNode);
  MOZ_ASSERT(aNode->NodeType() == nsIDOMNode::TEXT_NODE);
  MOZ_ASSERT(outIsEmptyNode);

  *outIsEmptyNode = true;

  uint32_t length = aNode->TextLength();
  if (aSafeToAskFrames)
  {
    nsCOMPtr<nsISelectionController> selCon;
    nsresult res = GetSelectionController(getter_AddRefs(selCon));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
    bool isVisible = false;
    
    
    
    
    
    
    res = selCon->CheckVisibilityContent(aNode, 0, length, &isVisible);
    NS_ENSURE_SUCCESS(res, res);
    if (isVisible) 
    {
      *outIsEmptyNode = false;
    }
  }
  else if (length)
  {
    if (aNode->TextIsOnlyWhitespace())
    {
      nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode);
      nsWSRunObject wsRunObj(this, node, 0);
      nsCOMPtr<nsIDOMNode> visNode;
      int32_t outVisOffset=0;
      WSType visType;
      wsRunObj.NextVisibleNode(node, 0, address_of(visNode),
                               &outVisOffset, &visType);
      if (visType == WSType::normalWS || visType == WSType::text) {
        *outIsEmptyNode = (node != visNode);
      }
    }
    else
    {
      *outIsEmptyNode = false;
    }
  }
  return NS_OK;  
}
  






nsresult
nsHTMLEditor::IsEmptyNode( nsIDOMNode *aNode, 
                           bool *outIsEmptyNode, 
                           bool aSingleBRDoesntCount,
                           bool aListOrCellNotEmpty,
                           bool aSafeToAskFrames)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return IsEmptyNode(node, outIsEmptyNode, aSingleBRDoesntCount,
                     aListOrCellNotEmpty, aSafeToAskFrames);
}

nsresult
nsHTMLEditor::IsEmptyNode(nsINode* aNode,
                          bool* outIsEmptyNode,
                          bool aSingleBRDoesntCount,
                          bool aListOrCellNotEmpty,
                          bool aSafeToAskFrames)
{
  NS_ENSURE_TRUE(aNode && outIsEmptyNode, NS_ERROR_NULL_POINTER);
  *outIsEmptyNode = true;
  bool seenBR = false;
  return IsEmptyNodeImpl(aNode, outIsEmptyNode, aSingleBRDoesntCount,
                         aListOrCellNotEmpty, aSafeToAskFrames, &seenBR);
}




nsresult
nsHTMLEditor::IsEmptyNodeImpl(nsINode* aNode,
                              bool *outIsEmptyNode,
                              bool aSingleBRDoesntCount,
                              bool aListOrCellNotEmpty,
                              bool aSafeToAskFrames,
                              bool *aSeenBR)
{
  NS_ENSURE_TRUE(aNode && outIsEmptyNode && aSeenBR, NS_ERROR_NULL_POINTER);

  if (aNode->NodeType() == nsIDOMNode::TEXT_NODE) {
    return IsVisTextNode(static_cast<nsIContent*>(aNode), outIsEmptyNode, aSafeToAskFrames);
  }

  
  
  
  
  
  
  if (!IsContainer(aNode->AsDOMNode())                      ||
      (nsHTMLEditUtils::IsNamedAnchor(aNode) ||
       nsHTMLEditUtils::IsFormWidget(aNode) ||
       (aListOrCellNotEmpty &&
        (nsHTMLEditUtils::IsListItem(aNode) ||
         nsHTMLEditUtils::IsTableCell(aNode))))) {
    *outIsEmptyNode = false;
    return NS_OK;
  }
    
  
  bool isListItemOrCell = nsHTMLEditUtils::IsListItem(aNode) ||
                          nsHTMLEditUtils::IsTableCell(aNode);
       
  
  
  for (nsCOMPtr<nsIContent> child = aNode->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    
    if (nsEditor::IsEditable(child)) {
      if (child->NodeType() == nsIDOMNode::TEXT_NODE) {
        nsresult rv = IsVisTextNode(child, outIsEmptyNode, aSafeToAskFrames);
        NS_ENSURE_SUCCESS(rv, rv);
        
        if (!*outIsEmptyNode) {
          return NS_OK;
        }
      } else {
        
        
        if (child == aNode) {
          break;
        }

        if (aSingleBRDoesntCount && !*aSeenBR && child->IsHTML(nsGkAtoms::br)) {
          
          *aSeenBR = true;
        } else {
          
          
          
          if (child->IsElement()) {
            if (isListItemOrCell) {
              if (nsHTMLEditUtils::IsList(child) ||
                  child->IsHTML(nsGkAtoms::table)) {
                
                *outIsEmptyNode = false;
                return NS_OK;
              }
            } else if (nsHTMLEditUtils::IsFormWidget(child)) {
              
              
              *outIsEmptyNode = false;
              return NS_OK;
            }
          }

          bool isEmptyNode = true;
          nsresult rv = IsEmptyNodeImpl(child, &isEmptyNode,
                                        aSingleBRDoesntCount,
                                        aListOrCellNotEmpty, aSafeToAskFrames,
                                        aSeenBR);
          NS_ENSURE_SUCCESS(rv, rv);
          if (!isEmptyNode) {
            
            *outIsEmptyNode = false;
            return NS_OK;
          }
        }
      }
    }
  }
  
  return NS_OK;
}



nsresult
nsHTMLEditor::SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                       const nsAString & aAttribute,
                                       const nsAString & aValue,
                                       bool aSuppressTransaction)
{
  nsAutoScriptBlocker scriptBlocker;

  nsresult res = NS_OK;
  if (IsCSSEnabled() && mHTMLCSSUtils) {
    int32_t count;
    res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(aElement, nullptr, &aAttribute, &aValue, &count,
                                                     aSuppressTransaction);
    NS_ENSURE_SUCCESS(res, res);
    if (count) {
      
      nsAutoString existingValue;
      bool wasSet = false;
      res = GetAttributeValue(aElement, aAttribute, existingValue, &wasSet);
      NS_ENSURE_SUCCESS(res, res);
      if (wasSet) {
        if (aSuppressTransaction)
          res = aElement->RemoveAttribute(aAttribute);
        else
          res = RemoveAttribute(aElement, aAttribute);
      }
    }
    else {
      
      
      
      if (aAttribute.EqualsLiteral("style")) {
        
        
        nsAutoString existingValue;
        bool wasSet = false;
        res = GetAttributeValue(aElement, NS_LITERAL_STRING("style"), existingValue, &wasSet);
        NS_ENSURE_SUCCESS(res, res);
        existingValue.AppendLiteral(" ");
        existingValue.Append(aValue);
        if (aSuppressTransaction)
          res = aElement->SetAttribute(aAttribute, existingValue);
        else
          res = SetAttribute(aElement, aAttribute, existingValue);
      }
      else {
        
        
        if (aSuppressTransaction)
          res = aElement->SetAttribute(aAttribute, aValue);
        else
          res = SetAttribute(aElement, aAttribute, aValue);
      }
    }
  }
  else {
    
    if (aSuppressTransaction)
      res = aElement->SetAttribute(aAttribute, aValue);
    else
      res = SetAttribute(aElement, aAttribute, aValue);
  }  
  return res;
}

nsresult
nsHTMLEditor::RemoveAttributeOrEquivalent(nsIDOMElement* aElement,
                                          const nsAString& aAttribute,
                                          bool aSuppressTransaction)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(element, NS_OK);

  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttribute);
  MOZ_ASSERT(attribute);

  nsresult res = NS_OK;
  if (IsCSSEnabled() && mHTMLCSSUtils) {
    res = mHTMLCSSUtils->RemoveCSSEquivalentToHTMLStyle(
        element, nullptr, &aAttribute, nullptr, aSuppressTransaction);
    NS_ENSURE_SUCCESS(res, res);
  }

  if (element->HasAttr(kNameSpaceID_None, attribute)) {
    if (aSuppressTransaction) {
      res = element->UnsetAttr(kNameSpaceID_None, attribute,
                                true);
    } else {
      res = RemoveAttribute(aElement, aAttribute);
    }
  }
  return res;
}

nsresult
nsHTMLEditor::SetIsCSSEnabled(bool aIsCSSPrefChecked)
{
  if (!mHTMLCSSUtils) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  mHTMLCSSUtils->SetCSSEnabled(aIsCSSPrefChecked);

  
  uint32_t flags = mFlags;
  if (aIsCSSPrefChecked) {
    
    flags &= ~eEditorNoCSSMask;
  } else {
    
    flags |= eEditorNoCSSMask;
  }

  return SetFlags(flags);
}


NS_IMETHODIMP
nsHTMLEditor::SetCSSBackgroundColor(const nsAString& aColor)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  ForceCompositionEnd();

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsRefPtr<Selection> selection = GetSelection();

  bool isCollapsed = selection->Collapsed();

  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertElement, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  
  bool cancel, handled;
  nsTextRulesInfo ruleInfo(EditAction::setTextProperty);
  nsresult res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    nsAutoString bgcolor; bgcolor.AssignLiteral("bgcolor");
    uint32_t rangeCount = selection->GetRangeCount();
    for (uint32_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
      nsCOMPtr<nsIDOMNode> cachedBlockParent = nullptr;
      nsRefPtr<nsRange> range = selection->GetRangeAt(rangeIdx);
      NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);
      
      
      nsCOMPtr<nsIDOMNode> startNode, endNode;
      int32_t startOffset, endOffset;
      res = range->GetStartContainer(getter_AddRefs(startNode));
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetEndContainer(getter_AddRefs(endNode));
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetStartOffset(&startOffset);
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetEndOffset(&endOffset);
      NS_ENSURE_SUCCESS(res, res);
      if ((startNode == endNode) && IsTextNode(startNode))
      {
        
        nsCOMPtr<nsIDOMNode> blockParent;
        blockParent = GetBlockNodeParent(startNode);
        
        if (blockParent && cachedBlockParent != blockParent) {
          cachedBlockParent = blockParent;
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
          int32_t count;
          res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nullptr, &bgcolor, &aColor, &count, false);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
      else if ((startNode == endNode) && nsTextEditUtils::IsBody(startNode) && isCollapsed)
      {
        
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(startNode);
        int32_t count;
        res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nullptr, &bgcolor, &aColor, &count, false);
        NS_ENSURE_SUCCESS(res, res);
      }
      else if ((startNode == endNode) && (((endOffset-startOffset) == 1) || (!startOffset && !endOffset)))
      {
        
        
        nsCOMPtr<nsIDOMNode> selectedNode = GetChildAt(startNode, startOffset);
        bool isBlock =false;
        res = NodeIsBlockStatic(selectedNode, &isBlock);
        NS_ENSURE_SUCCESS(res, res);
        nsCOMPtr<nsIDOMNode> blockParent = selectedNode;
        if (!isBlock) {
          blockParent = GetBlockNodeParent(selectedNode);
        }
        if (blockParent && cachedBlockParent != blockParent) {
          cachedBlockParent = blockParent;
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
          int32_t count;
          res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nullptr, &bgcolor, &aColor, &count, false);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
      else
      {
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsIContentIterator> iter =
          do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_TRUE(iter, NS_ERROR_FAILURE);

        nsCOMArray<nsIDOMNode> arrayOfNodes;
        nsCOMPtr<nsIDOMNode> node;
                
        
        res = iter->Init(range);
        
        
        
        
        if (NS_SUCCEEDED(res))
        {
          while (!iter->IsDone())
          {
            node = do_QueryInterface(iter->GetCurrentNode());
            NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

            if (IsEditable(node))
            {
              arrayOfNodes.AppendObject(node);
            }

            iter->Next();
          }
        }
        
        
        
        if (IsTextNode(startNode) && IsEditable(startNode))
        {
          nsCOMPtr<nsIDOMNode> blockParent;
          blockParent = GetBlockNodeParent(startNode);
          if (blockParent && cachedBlockParent != blockParent) {
            cachedBlockParent = blockParent;
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
            int32_t count;
            res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nullptr, &bgcolor, &aColor, &count, false);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
        
        
        int32_t listCount = arrayOfNodes.Count();
        int32_t j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          
          bool isBlock =false;
          res = NodeIsBlockStatic(node, &isBlock);
          NS_ENSURE_SUCCESS(res, res);
          nsCOMPtr<nsIDOMNode> blockParent = node;
          if (!isBlock) {
            
            blockParent = GetBlockNodeParent(node);
          }
          if (blockParent && cachedBlockParent != blockParent) {
            cachedBlockParent = blockParent;
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
            int32_t count;
            
            res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nullptr, &bgcolor, &aColor, &count, false);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
        arrayOfNodes.Clear();
        
        
        
        
        if (IsTextNode(endNode) && IsEditable(endNode))
        {
          nsCOMPtr<nsIDOMNode> blockParent;
          blockParent = GetBlockNodeParent(endNode);
          if (blockParent && cachedBlockParent != blockParent) {
            cachedBlockParent = blockParent;
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
            int32_t count;
            res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nullptr, &bgcolor, &aColor, &count, false);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
      }
    }
  }
  if (!cancel)
  {
    
    res = mRules->DidDoAction(selection, &ruleInfo, res);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SetBackgroundColor(const nsAString& aColor)
{
  nsresult res;
  if (IsCSSEnabled()) {
    
    
    
    res = SetCSSBackgroundColor(aColor);
  }
  else {
    
    res = SetHTMLBackgroundColor(aColor);
  }
  return res;
}





bool
nsHTMLEditor::AreNodesSameType(nsIContent* aNode1, nsIContent* aNode2)
{
  MOZ_ASSERT(aNode1);
  MOZ_ASSERT(aNode2);

  if (aNode1->Tag() != aNode2->Tag()) {
    return false;
  }

  if (!IsCSSEnabled() || !aNode1->IsHTML(nsGkAtoms::span)) {
    return true;
  }

  
  return mHTMLCSSUtils->ElementsSameStyle(aNode1->AsDOMNode(),
                                          aNode2->AsDOMNode());
}

NS_IMETHODIMP
nsHTMLEditor::CopyLastEditableChildStyles(nsIDOMNode * aPreviousBlock, nsIDOMNode * aNewBlock,
                                          nsIDOMNode **aOutBrNode)
{
  *aOutBrNode = nullptr;
  nsCOMPtr<nsIDOMNode> child, tmp;
  nsresult res;
  
  res = aNewBlock->GetFirstChild(getter_AddRefs(child));
  while (NS_SUCCEEDED(res) && child)
  {
    res = DeleteNode(child);
    NS_ENSURE_SUCCESS(res, res);
    res = aNewBlock->GetFirstChild(getter_AddRefs(child));
  }
  
  child = aPreviousBlock;
  tmp = aPreviousBlock;
  while (tmp) {
    child = tmp;
    res = GetLastEditableChild(child, address_of(tmp));
    NS_ENSURE_SUCCESS(res, res);
  }
  while (child && nsTextEditUtils::IsBreak(child)) {
    nsCOMPtr<nsIDOMNode> priorNode;
    res = GetPriorHTMLNode(child, address_of(priorNode));
    NS_ENSURE_SUCCESS(res, res);
    child = priorNode;
  }
  nsCOMPtr<nsIDOMNode> newStyles = nullptr, deepestStyle = nullptr;
  while (child && (child != aPreviousBlock)) {
    if (nsHTMLEditUtils::IsInlineStyle(child) ||
        nsEditor::NodeIsType(child, nsEditProperty::span)) {
      nsAutoString domTagName;
      child->GetNodeName(domTagName);
      ToLowerCase(domTagName);
      if (newStyles) {
        nsCOMPtr<nsIDOMNode> newContainer;
        res = InsertContainerAbove(newStyles, address_of(newContainer), domTagName);
        NS_ENSURE_SUCCESS(res, res);
        newStyles = newContainer;
      }
      else {
        res = CreateNode(domTagName, aNewBlock, 0, getter_AddRefs(newStyles));
        NS_ENSURE_SUCCESS(res, res);
        deepestStyle = newStyles;
      }
      res = CloneAttributes(newStyles, child);
      NS_ENSURE_SUCCESS(res, res);
    }
    nsCOMPtr<nsIDOMNode> tmp;
    res = child->GetParentNode(getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
    child = tmp;
  }
  if (deepestStyle) {
    nsCOMPtr<nsIDOMNode> outBRNode;
    res = CreateBR(deepestStyle, 0, address_of(outBRNode));
    NS_ENSURE_SUCCESS(res, res);
    
    outBRNode.forget(aOutBrNode);
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::GetElementOrigin(nsIDOMElement * aElement, int32_t & aX, int32_t & aY)
{
  aX = 0;
  aY = 0;

  NS_ENSURE_TRUE(mDocWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  nsIFrame *frame = content->GetPrimaryFrame();
  NS_ENSURE_TRUE(frame, NS_OK);

  nsIFrame *container = ps->GetAbsoluteContainingBlock(frame);
  NS_ENSURE_TRUE(container, NS_OK);
  nsPoint off = frame->GetOffsetTo(container);
  aX = nsPresContext::AppUnitsToIntCSSPixels(off.x);
  aY = nsPresContext::AppUnitsToIntCSSPixels(off.y);

  return NS_OK;
}

nsresult
nsHTMLEditor::EndUpdateViewBatch()
{
  nsresult res = nsEditor::EndUpdateViewBatch();
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  
  
  
  if (mUpdateCount == 0) {
    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);
    res = CheckSelectionStateForAnonymousButtons(selection);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::GetSelectionContainer(nsIDOMElement ** aReturn)
{
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  
  if (NS_FAILED(res) || !selection) return res;

  nsCOMPtr<nsIDOMNode> focusNode;

  if (selection->Collapsed()) {
    res = selection->GetFocusNode(getter_AddRefs(focusNode));
    NS_ENSURE_SUCCESS(res, res);
  } else {

    int32_t rangeCount;
    res = selection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(res, res);

    if (rangeCount == 1) {

      nsCOMPtr<nsIDOMRange> range;
      res = selection->GetRangeAt(0, getter_AddRefs(range));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);

      nsCOMPtr<nsIDOMNode> startContainer, endContainer;
      res = range->GetStartContainer(getter_AddRefs(startContainer));
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetEndContainer(getter_AddRefs(endContainer));
      NS_ENSURE_SUCCESS(res, res);
      int32_t startOffset, endOffset;
      res = range->GetStartOffset(&startOffset);
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetEndOffset(&endOffset);
      NS_ENSURE_SUCCESS(res, res);

      nsCOMPtr<nsIDOMElement> focusElement;
      if (startContainer == endContainer && startOffset + 1 == endOffset) {
        res = GetSelectedElement(EmptyString(), getter_AddRefs(focusElement));
        NS_ENSURE_SUCCESS(res, res);
        if (focusElement)
          focusNode = do_QueryInterface(focusElement);
      }
      if (!focusNode) {
        res = range->GetCommonAncestorContainer(getter_AddRefs(focusNode));
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    else {
      int32_t i;
      nsCOMPtr<nsIDOMRange> range;
      for (i = 0; i < rangeCount; i++)
      {
        res = selection->GetRangeAt(i, getter_AddRefs(range));
        NS_ENSURE_SUCCESS(res, res);
        nsCOMPtr<nsIDOMNode> startContainer;
        res = range->GetStartContainer(getter_AddRefs(startContainer));
        if (NS_FAILED(res)) continue;
        if (!focusNode)
          focusNode = startContainer;
        else if (focusNode != startContainer) {
          res = startContainer->GetParentNode(getter_AddRefs(focusNode));
          NS_ENSURE_SUCCESS(res, res);
          break;
        }
      }
    }
  }

  if (focusNode) {
    uint16_t nodeType;
    focusNode->GetNodeType(&nodeType);
    if (nsIDOMNode::TEXT_NODE == nodeType) {
      nsCOMPtr<nsIDOMNode> parent;
      res = focusNode->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(res, res);
      focusNode = parent;
    }
  }

  nsCOMPtr<nsIDOMElement> focusElement = do_QueryInterface(focusNode);
  focusElement.forget(aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::IsAnonymousElement(nsIDOMElement * aElement, bool * aReturn)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  *aReturn = content->IsRootOfNativeAnonymousSubtree();
  return NS_OK;
}

nsresult
nsHTMLEditor::SetReturnInParagraphCreatesNewParagraph(bool aCreatesNewParagraph)
{
  mCRInParagraphCreatesParagraph = aCreatesNewParagraph;
  return NS_OK;
}

bool
nsHTMLEditor::GetReturnInParagraphCreatesNewParagraph()
{
  return mCRInParagraphCreatesParagraph;
}

nsresult
nsHTMLEditor::GetReturnInParagraphCreatesNewParagraph(bool *aCreatesNewParagraph)
{
  *aCreatesNewParagraph = mCRInParagraphCreatesParagraph;
  return NS_OK;
}

already_AddRefed<nsIContent>
nsHTMLEditor::GetFocusedContent()
{
  NS_ENSURE_TRUE(mDocWeak, nullptr);

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, nullptr);

  nsCOMPtr<nsIContent> focusedContent = fm->GetFocusedContent();

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  bool inDesignMode = doc->HasFlag(NODE_IS_EDITABLE);
  if (!focusedContent) {
    
    if (inDesignMode && OurWindowHasFocus()) {
      nsCOMPtr<nsIContent> docRoot = doc->GetRootElement();
      return docRoot.forget();
    }
    return nullptr;
  }

  if (inDesignMode) {
    return OurWindowHasFocus() &&
      nsContentUtils::ContentIsDescendantOf(focusedContent, doc) ?
      focusedContent.forget() : nullptr;
  }

  

  
  
  if (!focusedContent->HasFlag(NODE_IS_EDITABLE) ||
      focusedContent->HasIndependentSelection()) {
    return nullptr;
  }
  
  return OurWindowHasFocus() ? focusedContent.forget() : nullptr;
}

already_AddRefed<nsIContent>
nsHTMLEditor::GetFocusedContentForIME()
{
  nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
  if (!focusedContent) {
    return nullptr;
  }

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, nullptr);
  return doc->HasFlag(NODE_IS_EDITABLE) ? nullptr : focusedContent.forget();
}

bool
nsHTMLEditor::IsActiveInDOMWindow()
{
  NS_ENSURE_TRUE(mDocWeak, false);

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, false);

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  bool inDesignMode = doc->HasFlag(NODE_IS_EDITABLE);

  
  if (inDesignMode) {
    return true;
  }

  nsPIDOMWindow* ourWindow = doc->GetWindow();
  nsCOMPtr<nsPIDOMWindow> win;
  nsIContent* content =
    nsFocusManager::GetFocusedDescendant(ourWindow, false,
                                         getter_AddRefs(win));
  if (!content) {
    return false;
  }

  

  
  
  if (!content->HasFlag(NODE_IS_EDITABLE) ||
      content->HasIndependentSelection()) {
    return false;
  }
  return true;
}

dom::Element*
nsHTMLEditor::GetActiveEditingHost()
{
  NS_ENSURE_TRUE(mDocWeak, nullptr);

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, nullptr);
  if (doc->HasFlag(NODE_IS_EDITABLE)) {
    return doc->GetBodyElement();
  }

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, nullptr);
  nsCOMPtr<nsIDOMNode> focusNode;
  rv = selection->GetFocusNode(getter_AddRefs(focusNode));
  NS_ENSURE_SUCCESS(rv, nullptr);
  nsCOMPtr<nsIContent> content = do_QueryInterface(focusNode);
  if (!content) {
    return nullptr;
  }

  
  
  if (!content->HasFlag(NODE_IS_EDITABLE) ||
      content->HasIndependentSelection()) {
    return nullptr;
  }
  return content->GetEditingHost();
}

already_AddRefed<mozilla::dom::EventTarget>
nsHTMLEditor::GetDOMEventTarget()
{
  
  
  
  NS_PRECONDITION(mDocWeak, "This editor has not been initialized yet");
  nsCOMPtr<mozilla::dom::EventTarget> target = do_QueryReferent(mDocWeak);
  return target.forget();
}

bool
nsHTMLEditor::ShouldReplaceRootElement()
{
  if (!mRootElement) {
    
    return true;
  }

  
  
  nsCOMPtr<nsIDOMHTMLElement> docBody;
  GetBodyElement(getter_AddRefs(docBody));
  return !SameCOMIdentity(docBody, mRootElement);
}

void
nsHTMLEditor::ResetRootElementAndEventTarget()
{
  nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);

  
  
  
  RemoveEventListeners();
  mRootElement = nullptr;
  nsresult rv = InstallEventListeners();
  if (NS_FAILED(rv)) {
    return;
  }

  
  nsCOMPtr<nsIDOMElement> root;
  rv = GetRootElement(getter_AddRefs(root));
  if (NS_FAILED(rv) || !mRootElement) {
    return;
  }

  rv = BeginningOfDocument();
  if (NS_FAILED(rv)) {
    return;
  }

  
  
  nsCOMPtr<nsINode> node = GetFocusedNode();
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(node);
  if (target) {
    InitializeSelection(target);
  }

  SyncRealTimeSpell();
}

nsresult
nsHTMLEditor::GetBodyElement(nsIDOMHTMLElement** aBody)
{
  NS_PRECONDITION(mDocWeak, "bad state, null mDocWeak");
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryReferent(mDocWeak);
  if (!htmlDoc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  return htmlDoc->GetBody(aBody);
}

already_AddRefed<nsINode>
nsHTMLEditor::GetFocusedNode()
{
  nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
  if (!focusedContent) {
    return nullptr;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ASSERTION(fm, "Focus manager is null");
  nsCOMPtr<nsIDOMElement> focusedElement;
  fm->GetFocusedElement(getter_AddRefs(focusedElement));
  if (focusedElement) {
    nsCOMPtr<nsINode> node = do_QueryInterface(focusedElement);
    return node.forget();
  }

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  return doc.forget();
}

bool
nsHTMLEditor::OurWindowHasFocus()
{
  NS_ENSURE_TRUE(mDocWeak, false);
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, false);
  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
  if (!focusedWindow) {
    return false;
  }
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  nsCOMPtr<nsIDOMWindow> ourWindow = do_QueryInterface(doc->GetWindow());
  return ourWindow == focusedWindow;
}

bool
nsHTMLEditor::IsAcceptableInputEvent(nsIDOMEvent* aEvent)
{
  if (!nsEditor::IsAcceptableInputEvent(aEvent)) {
    return false;
  }

  NS_ENSURE_TRUE(mDocWeak, false);

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_TRUE(target, false);

  nsCOMPtr<nsIDocument> document = do_QueryReferent(mDocWeak);
  if (document->HasFlag(NODE_IS_EDITABLE)) {
    
    
    nsCOMPtr<nsIDocument> targetDocument = do_QueryInterface(target);
    if (targetDocument) {
      return targetDocument == document;
    }
    
    nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);
    NS_ENSURE_TRUE(targetContent, false);
    return document == targetContent->GetCurrentDoc();
  }

  
  
  nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);
  NS_ENSURE_TRUE(targetContent, false);

  
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  if (mouseEvent) {
    nsIContent* editingHost = GetActiveEditingHost();
    
    
    if (!editingHost) {
      return false;
    }
    
    
    if (targetContent == document->GetRootElement() &&
        !targetContent->HasFlag(NODE_IS_EDITABLE) &&
        editingHost == document->GetBodyElement()) {
      targetContent = editingHost;
    }
    
    
    if (!nsContentUtils::ContentIsDescendantOf(targetContent, editingHost)) {
      return false;
    }
    
    
    if (targetContent->HasIndependentSelection()) {
      return false;
    }
    
    return targetContent->HasFlag(NODE_IS_EDITABLE);
  }

  
  
  
  if (!targetContent->HasFlag(NODE_IS_EDITABLE) ||
      targetContent->HasIndependentSelection()) {
    return false;
  }

  
  
  
  
  return IsActiveInDOMWindow();
}

NS_IMETHODIMP
nsHTMLEditor::GetPreferredIMEState(IMEState *aState)
{
  
  aState->mOpen = IMEState::DONT_CHANGE_OPEN_STATE;
  if (IsReadonly() || IsDisabled()) {
    aState->mEnabled = IMEState::DISABLED;
  } else {
    aState->mEnabled = IMEState::ENABLED;
  }
  return NS_OK;
}

already_AddRefed<nsIContent>
nsHTMLEditor::GetInputEventTargetContent()
{
  nsCOMPtr<nsIContent> target = GetActiveEditingHost();
  return target.forget();
}

bool
nsHTMLEditor::IsEditable(nsIContent* aNode) {
  if (!nsPlaintextEditor::IsEditable(aNode)) {
    return false;
  }
  if (aNode->IsElement()) {
    
    return aNode->IsEditable();
  }
  
  
  return true;
}


dom::Element*
nsHTMLEditor::GetEditorRoot()
{
  return GetActiveEditingHost();
}
