





































#include "nsCRT.h"

#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"

#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"

#include "nsHTMLEditorEventListener.h"
#include "TypeInState.h"

#include "nsHTMLURIRefObject.h"

#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMAttr.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOM3EventTarget.h" 
#include "nsIDOMKeyEvent.h"
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsISelectionController.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLHtmlElement.h"
#include "nsGUIEvent.h"
#include "nsIDOMEventGroup.h"
#include "nsILinkHandler.h"

#include "nsCSSLoader.h"
#include "nsCSSStyleSheet.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDocumentObserver.h"
#include "nsIDocumentStateListener.h"

#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsIRangeUtils.h"
#include "nsISupportsArray.h"
#include "nsContentUtils.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIDocumentEncoder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "SetDocTitleTxn.h"
#include "nsGUIEvent.h"
#include "nsTextFragment.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"


#include "nsIURI.h"
#include "nsNetUtil.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIDragService.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIContentFilter.h"


#include "nsStyleSheetTxns.h"


#include "TextEditorTest.h"
#include "nsEditorUtils.h"
#include "nsWSRunObject.h"
#include "nsHTMLObjectResizer.h"
#include "nsGkAtoms.h"

#include "nsIFrame.h"
#include "nsIView.h"
#include "nsIParserService.h"
#include "nsIEventStateManager.h"


static char hrefText[] = "href";
static char anchorTxt[] = "anchor";
static char namedanchorText[] = "namedanchor";

nsIRangeUtils* nsHTMLEditor::sRangeHelper;


nsresult NS_NewTextEditRules(nsIEditRules** aInstancePtrResult);
nsresult NS_NewHTMLEditRules(nsIEditRules** aInstancePtrResult);

#define IsLinkTag(s) (s.EqualsIgnoreCase(hrefText))
#define IsNamedAnchorTag(s) (s.EqualsIgnoreCase(anchorTxt) || s.EqualsIgnoreCase(namedanchorText))

nsHTMLEditor::nsHTMLEditor()
: nsPlaintextEditor()
, mIgnoreSpuriousDragEvent(PR_FALSE)
, mTypeInState(nsnull)
, mCRInParagraphCreatesParagraph(PR_FALSE)
, mHTMLCSSUtils(nsnull)
, mSelectedCellIndex(0)
, mIsObjectResizingEnabled(PR_TRUE)
, mIsResizing(PR_FALSE)
, mIsAbsolutelyPositioningEnabled(PR_TRUE)
, mResizedObjectIsAbsolutelyPositioned(PR_FALSE)
, mGrabberClicked(PR_FALSE)
, mIsMoving(PR_FALSE)
, mSnapToGridEnabled(PR_FALSE)
, mIsInlineTableEditingEnabled(PR_TRUE)
, mInfoXIncrement(20)
, mInfoYIncrement(20)
, mGridSize(0)
{
} 

nsHTMLEditor::~nsHTMLEditor()
{
  
  
  
  nsCOMPtr<nsIEditActionListener> mListener = do_QueryInterface(mRules);
  RemoveEditActionListener(mListener);
  
  
  

  if (mAbsolutelyPositionedObject)
    HideGrabber();
  if (mInlineEditedCell)
    HideInlineTableEditingUI();
  if (mResizedObject)
    HideResizers();

  
  
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

  NS_IF_RELEASE(mTypeInState);
  mSelectionListenerP = nsnull;

  delete mHTMLCSSUtils;

  
  RemoveAllDefaultProperties();

  while (mStyleSheetURLs.Length())
  {
    RemoveOverrideStyleSheet(mStyleSheetURLs[0]);
  }

  if (mLinkHandler && mPresShellWeak)
  {
    nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);

    if (ps && ps->GetPresContext())
    {
      ps->GetPresContext()->SetLinkHandler(mLinkHandler);
    }
  }

  RemoveEventListeners();
}


void
nsHTMLEditor::Shutdown()
{
  NS_IF_RELEASE(sRangeHelper);
}

NS_IMPL_ADDREF_INHERITED(nsHTMLEditor, nsEditor)
NS_IMPL_RELEASE_INHERITED(nsHTMLEditor, nsEditor)

NS_INTERFACE_MAP_BEGIN(nsHTMLEditor)
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
nsHTMLEditor::Init(nsIDOMDocument *aDoc, nsIPresShell *aPresShell,
                   nsIContent *aRoot, nsISelectionController *aSelCon,
                   PRUint32 aFlags)
{
  NS_PRECONDITION(aDoc && aPresShell, "bad arg");
  NS_ENSURE_TRUE(aDoc && aPresShell, NS_ERROR_NULL_POINTER);

  nsresult result = NS_OK, rulesRes = NS_OK;

  
  if (!sRangeHelper) {
    result = CallGetService("@mozilla.org/content/range-utils;1",
                            &sRangeHelper);
    NS_ENSURE_TRUE(sRangeHelper, result);
  }
   
  if (1)
  {
    
    nsAutoEditInitRulesTrigger rulesTrigger(static_cast<nsPlaintextEditor*>(this), rulesRes);

    
    result = nsPlaintextEditor::Init(aDoc, aPresShell, aRoot, aSelCon, aFlags);
    if (NS_FAILED(result)) { return result; }

    
    nsCOMPtr<nsINode> document = do_QueryInterface(aDoc);
    document->AddMutationObserver(this);

    
    if (IsMailEditor())
    {
      SetAbsolutePositioningEnabled(PR_FALSE);
      SetSnapToGridEnabled(PR_FALSE);
    }

    
    if (mHTMLCSSUtils)
      delete mHTMLCSSUtils;
    result = NS_NewHTMLCSSUtils(&mHTMLCSSUtils);
    if (NS_FAILED(result)) { return result; }
    mHTMLCSSUtils->Init(this);

    
    nsPresContext *context = aPresShell->GetPresContext();
    NS_ENSURE_TRUE(context, NS_ERROR_NULL_POINTER);
    if (!IsPlaintextEditor() && !IsInteractionAllowed()) {
      mLinkHandler = context->GetLinkHandler();

      context->SetLinkHandler(nsnull);
    }

    
    mTypeInState = new TypeInState();
    if (!mTypeInState) {return NS_ERROR_NULL_POINTER;}
    NS_ADDREF(mTypeInState);

    
    mSelectionListenerP = new ResizerSelectionListener(this);
    if (!mSelectionListenerP) {return NS_ERROR_NULL_POINTER;}

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
nsHTMLEditor::PreDestroy(PRBool aDestroyingFrames)
{
  if (mDidPreDestroy) {
    return NS_OK;
  }

  nsCOMPtr<nsINode> document = do_QueryReferent(mDocWeak);
  if (document) {
    document->RemoveMutationObserver(this);
  }

  return nsPlaintextEditor::PreDestroy(aDestroyingFrames);
}

NS_IMETHODIMP
nsHTMLEditor::GetRootElement(nsIDOMElement **aRootElement)
{
  NS_ENSURE_ARG_POINTER(aRootElement);

  if (mRootElement) {
    return nsEditor::GetRootElement(aRootElement);
  }

  *aRootElement = nsnull;

  
  

  nsCOMPtr<nsIDOMHTMLElement> bodyElement; 
  nsresult rv = GetBodyElement(getter_AddRefs(bodyElement));
  NS_ENSURE_SUCCESS(rv, rv);

  if (bodyElement) {
    mRootElement = bodyElement;
  } else {
    
    
    nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
    NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

    rv = doc->GetDocumentElement(getter_AddRefs(mRootElement));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!mRootElement) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  *aRootElement = mRootElement;
  NS_ADDREF(*aRootElement);

  return NS_OK;
}

already_AddRefed<nsIContent>
nsHTMLEditor::FindSelectionRoot(nsINode *aNode)
{
  NS_PRECONDITION(aNode->IsNodeOfType(nsINode::eDOCUMENT) ||
                  aNode->IsNodeOfType(nsINode::eCONTENT),
                  "aNode must be content or document node");

  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  nsCOMPtr<nsIDocument> doc = aNode->GetCurrentDoc();
  if (!doc) {
    return nsnull;
  }

  if (doc->HasFlag(NODE_IS_EDITABLE) || !content) {
    content = doc->GetRootElement();
    return content.forget();
  }

  
  
  
  if (IsReadonly()) {
    
    content = do_QueryInterface(GetRoot());
    return content.forget();
  }

  if (!content->HasFlag(NODE_IS_EDITABLE)) {
    return nsnull;
  }

  
  
  nsIContent *parent;
  while ((parent = content->GetParent()) && parent->HasFlag(NODE_IS_EDITABLE)) {
    content = parent;
  }
  return content.forget();
}

nsresult
nsHTMLEditor::CreateEventListeners()
{
  NS_ENSURE_TRUE(!mEventListener, NS_ERROR_ALREADY_INITIALIZED);
  mEventListener = do_QueryInterface(
    static_cast<nsIDOMKeyListener*>(new nsHTMLEditorEventListener()));
  NS_ENSURE_TRUE(mEventListener, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

nsresult
nsHTMLEditor::InstallEventListeners()
{
  NS_ENSURE_TRUE(mDocWeak && mPresShellWeak && mEventListener,
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

  nsCOMPtr<nsPIDOMEventTarget> piTarget = GetPIDOMEventTarget();
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(piTarget);

  if (piTarget && target)
  {
    
    
    
    
    
    

    if (mMouseMotionListenerP)
    {
      
      
      piTarget->RemoveEventListenerByIID(mMouseMotionListenerP,
                                         NS_GET_IID(nsIDOMMouseMotionListener));

      target->RemoveEventListener(NS_LITERAL_STRING("mousemove"),
                                  mMouseMotionListenerP, PR_TRUE);
    }

    if (mResizeEventListenerP)
    {
      target->RemoveEventListener(NS_LITERAL_STRING("resize"),
                                  mResizeEventListenerP, PR_FALSE);
    }
  }

  mMouseMotionListenerP = nsnull;
  mResizeEventListenerP = nsnull;

  nsPlaintextEditor::RemoveEventListeners();
}

NS_IMETHODIMP 
nsHTMLEditor::SetFlags(PRUint32 aFlags)
{
  nsresult rv = nsPlaintextEditor::SetFlags(aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  mCSSAware = !NoCSS() && !IsMailEditor();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::InitRules()
{
  
  nsresult res = NS_NewHTMLEditRules(getter_AddRefs(mRules));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(mRules, NS_ERROR_UNEXPECTED);
  res = mRules->Init(static_cast<nsPlaintextEditor*>(this));
  
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::BeginningOfDocument()
{
  if (!mDocWeak || !mPresShellWeak) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);
    
  
  nsIDOMElement *rootElement = GetRoot(); 
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER); 
  
  
  PRBool done = PR_FALSE;
  nsCOMPtr<nsIDOMNode> curNode(rootElement), selNode;
  PRInt32 curOffset = 0, selOffset;
  while (!done)
  {
    nsWSRunObject wsObj(this, curNode, curOffset);
    nsCOMPtr<nsIDOMNode> visNode;
    PRInt32 visOffset=0;
    PRInt16 visType=0;
    wsObj.NextVisibleNode(curNode, curOffset, address_of(visNode), &visOffset, &visType);
    if ((visType==nsWSRunObject::eNormalWS) || 
        (visType==nsWSRunObject::eText))
    {
      selNode = visNode;
      selOffset = visOffset;
      done = PR_TRUE;
    }
    else if ((visType==nsWSRunObject::eBreak)    ||
             (visType==nsWSRunObject::eSpecial))
    {
      res = GetNodeLocation(visNode, address_of(selNode), &selOffset);
      NS_ENSURE_SUCCESS(res, res); 
      done = PR_TRUE;
    }
    else if (visType==nsWSRunObject::eOtherBlock)
    {
      
      
      
      
      
      

      if (!IsContainer(visNode))
      {
        
        
        
        
        

        res = GetNodeLocation(visNode, address_of(selNode), &selOffset);
        NS_ENSURE_SUCCESS(res, res); 
        done = PR_TRUE;
      }
      else
      {
        PRBool isEmptyBlock;
        if (NS_SUCCEEDED(IsEmptyNode(visNode, &isEmptyBlock)) &&
            isEmptyBlock)
        {
          
          res = GetNodeLocation(visNode, address_of(curNode), &curOffset);
          NS_ENSURE_SUCCESS(res, res); 
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
      done = PR_TRUE;
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

  
  
  
  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = aKeyEvent->GetTarget(getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(target);
  if (!IsModifiableNode(targetNode)) {
    return NS_OK;
  }

  nsKeyEvent* nativeKeyEvent = GetNativeKeyEvent(aKeyEvent);
  NS_ENSURE_TRUE(nativeKeyEvent, NS_ERROR_UNEXPECTED);
  NS_ASSERTION(nativeKeyEvent->message == NS_KEY_PRESS,
               "HandleKeyPressEvent gets non-keypress event");

  switch (nativeKeyEvent->keyCode) {
    case nsIDOMKeyEvent::DOM_VK_META:
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

      if (nativeKeyEvent->isControl || nativeKeyEvent->isAlt ||
          nativeKeyEvent->isMeta) {
        return NS_OK;
      }

      nsCOMPtr<nsISelection> selection;
      nsresult rv = GetSelection(getter_AddRefs(selection));
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt32 offset;
      nsCOMPtr<nsIDOMNode> node, blockParent;
      rv = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

      PRBool isBlock = PR_FALSE;
      NodeIsBlock(node, &isBlock);
      if (isBlock) {
        blockParent = node;
      } else {
        blockParent = GetBlockNodeParent(node);
      }

      if (!blockParent) {
        break;
      }

      PRBool handled = PR_FALSE;
      if (nsHTMLEditUtils::IsTableElement(blockParent)) {
        rv = TabInTable(nativeKeyEvent->isShift, &handled);
        if (handled) {
          ScrollSelectionIntoView(PR_FALSE);
        }
      } else if (nsHTMLEditUtils::IsListItem(blockParent)) {
        rv = Indent(nativeKeyEvent->isShift ?
                      NS_LITERAL_STRING("outdent") :
                      NS_LITERAL_STRING("indent"));
        handled = PR_TRUE;
      }
      NS_ENSURE_SUCCESS(rv, rv);
      if (handled) {
        return aKeyEvent->PreventDefault(); 
      }
      if (nativeKeyEvent->isShift) {
        return NS_OK; 
      }
      aKeyEvent->PreventDefault();
      return TypedText(NS_LITERAL_STRING("\t"), eTypedText);
    }
    case nsIDOMKeyEvent::DOM_VK_RETURN:
    case nsIDOMKeyEvent::DOM_VK_ENTER:
      if (nativeKeyEvent->isControl || nativeKeyEvent->isAlt ||
          nativeKeyEvent->isMeta) {
        return NS_OK;
      }
      aKeyEvent->PreventDefault(); 
      if (nativeKeyEvent->isShift && !IsPlaintextEditor()) {
        
        return TypedText(EmptyString(), eTypedBR);
      }
      
      return TypedText(EmptyString(), eTypedBreak);
  }

  
  
  if (nativeKeyEvent->charCode == 0 || nativeKeyEvent->isControl ||
      nativeKeyEvent->isAlt || nativeKeyEvent->isMeta) {
    
    return NS_OK;
  }
  aKeyEvent->PreventDefault();
  nsAutoString str(nativeKeyEvent->charCode);
  return TypedText(str, eTypedText);
}





nsresult
nsHTMLEditor::NodeIsBlockStatic(nsIDOMNode *aNode, PRBool *aIsBlock)
{
  if (!aNode || !aIsBlock) { return NS_ERROR_NULL_POINTER; }

  *aIsBlock = PR_FALSE;

#define USE_PARSER_FOR_BLOCKNESS 1
#ifdef USE_PARSER_FOR_BLOCKNESS
  nsresult rv;

  nsCOMPtr<nsIDOMElement>element = do_QueryInterface(aNode);
  if (!element)
  {
    
    return NS_OK;
  }

  nsIAtom *tagAtom = GetTag(aNode);
  NS_ENSURE_TRUE(tagAtom, NS_ERROR_NULL_POINTER);

  
  
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
    *aIsBlock = PR_TRUE;
    return NS_OK;
  }

  rv = nsContentUtils::GetParserService()->
    IsBlock(nsContentUtils::GetParserService()->HTMLAtomTagToId(tagAtom),
            *aIsBlock);

#ifdef DEBUG
  
  if (tagAtom==nsEditProperty::p          ||
      tagAtom==nsEditProperty::div        ||
      tagAtom==nsEditProperty::blockquote ||
      tagAtom==nsEditProperty::h1         ||
      tagAtom==nsEditProperty::h2         ||
      tagAtom==nsEditProperty::h3         ||
      tagAtom==nsEditProperty::h4         ||
      tagAtom==nsEditProperty::h5         ||
      tagAtom==nsEditProperty::h6         ||
      tagAtom==nsEditProperty::ul         ||
      tagAtom==nsEditProperty::ol         ||
      tagAtom==nsEditProperty::dl         ||
      tagAtom==nsEditProperty::noscript   ||
      tagAtom==nsEditProperty::form       ||
      tagAtom==nsEditProperty::hr         ||
      tagAtom==nsEditProperty::table      ||
      tagAtom==nsEditProperty::fieldset   ||
      tagAtom==nsEditProperty::address    ||
      tagAtom==nsEditProperty::caption    ||
      tagAtom==nsEditProperty::col        ||
      tagAtom==nsEditProperty::colgroup   ||
      tagAtom==nsEditProperty::li         ||
      tagAtom==nsEditProperty::dt         ||
      tagAtom==nsEditProperty::dd         ||
      tagAtom==nsEditProperty::legend     )
  {
    if (!(*aIsBlock))
    {
      nsAutoString assertmsg (NS_LITERAL_STRING("Parser and editor disagree on blockness: "));

      nsAutoString tagName;
      rv = element->GetTagName(tagName);
      NS_ENSURE_SUCCESS(rv, rv);

      assertmsg.Append(tagName);
      char* assertstr = ToNewCString(assertmsg);
      NS_ASSERTION(*aIsBlock, assertstr);
      NS_Free(assertstr);
    }
  }
#endif 

  return rv;
#else 
  nsresult result = NS_ERROR_FAILURE;
  *aIsBlock = PR_FALSE;
  nsCOMPtr<nsIDOMElement>element;
  element = do_QueryInterface(aNode);
  if (element)
  {
    nsAutoString tagName;
    result = element->GetTagName(tagName);
    if (NS_SUCCEEDED(result))
    {
      ToLowerCase(tagName);
      nsCOMPtr<nsIAtom> tagAtom = do_GetAtom(tagName);
      if (!tagAtom) { return NS_ERROR_NULL_POINTER; }

      if (tagAtom==nsEditProperty::p          ||
          tagAtom==nsEditProperty::div        ||
          tagAtom==nsEditProperty::blockquote ||
          tagAtom==nsEditProperty::h1         ||
          tagAtom==nsEditProperty::h2         ||
          tagAtom==nsEditProperty::h3         ||
          tagAtom==nsEditProperty::h4         ||
          tagAtom==nsEditProperty::h5         ||
          tagAtom==nsEditProperty::h6         ||
          tagAtom==nsEditProperty::ul         ||
          tagAtom==nsEditProperty::ol         ||
          tagAtom==nsEditProperty::dl         ||
          tagAtom==nsEditProperty::pre        ||
          tagAtom==nsEditProperty::noscript   ||
          tagAtom==nsEditProperty::form       ||
          tagAtom==nsEditProperty::hr         ||
          tagAtom==nsEditProperty::fieldset   ||
          tagAtom==nsEditProperty::address    ||
          tagAtom==nsEditProperty::body       ||
          tagAtom==nsEditProperty::caption    ||
          tagAtom==nsEditProperty::table      ||
          tagAtom==nsEditProperty::tbody      ||
          tagAtom==nsEditProperty::thead      ||
          tagAtom==nsEditProperty::tfoot      ||
          tagAtom==nsEditProperty::tr         ||
          tagAtom==nsEditProperty::td         ||
          tagAtom==nsEditProperty::th         ||
          tagAtom==nsEditProperty::col        ||
          tagAtom==nsEditProperty::colgroup   ||
          tagAtom==nsEditProperty::li         ||
          tagAtom==nsEditProperty::dt         ||
          tagAtom==nsEditProperty::dd         ||
          tagAtom==nsEditProperty::legend     )
      {
        *aIsBlock = PR_TRUE;
      }
      result = NS_OK;
    }
  } else {
    
    nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(aNode);
    if (nodeAsText)
    {
      *aIsBlock = PR_FALSE;
      result = NS_OK;
    }
  }
  return result;

#endif 
}

NS_IMETHODIMP
nsHTMLEditor::NodeIsBlock(nsIDOMNode *aNode, PRBool *aIsBlock)
{
  return NodeIsBlockStatic(aNode, aIsBlock);
}

PRBool
nsHTMLEditor::IsBlockNode(nsIDOMNode *aNode)
{
  PRBool isBlock;
  NodeIsBlockStatic(aNode, &isBlock);
  return isBlock;
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





nsCOMPtr<nsIDOMNode>
nsHTMLEditor::GetBlockNodeParent(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> tmp;
  nsCOMPtr<nsIDOMNode> p;

  if (!aNode)
  {
    NS_NOTREACHED("null node passed to GetBlockNodeParent()");
    return PR_FALSE;
  }

  if (NS_FAILED(aNode->GetParentNode(getter_AddRefs(p))))  
    return tmp;

  while (p)
  {
    PRBool isBlock;
    if (NS_FAILED(NodeIsBlockStatic(p, &isBlock)) || isBlock)
      break;
    if ( NS_FAILED(p->GetParentNode(getter_AddRefs(tmp))) || !tmp) 
      return p;

    p = tmp;
  }
  return p;
}




nsresult
nsHTMLEditor::GetBlockSection(nsIDOMNode *aChild,
                              nsIDOMNode **aLeftNode, 
                              nsIDOMNode **aRightNode) 
{
  nsresult result = NS_OK;
  if (!aChild || !aLeftNode || !aRightNode) {return NS_ERROR_NULL_POINTER;}
  *aLeftNode = aChild;
  *aRightNode = aChild;

  nsCOMPtr<nsIDOMNode>sibling;
  result = aChild->GetPreviousSibling(getter_AddRefs(sibling));
  while ((NS_SUCCEEDED(result)) && sibling)
  {
    PRBool isBlock;
    NodeIsBlockStatic(sibling, &isBlock);
    if (isBlock)
    {
      nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(sibling);
      if (!nodeAsText) {
        break;
      }
      
    }
    *aLeftNode = sibling;
    result = (*aLeftNode)->GetPreviousSibling(getter_AddRefs(sibling)); 
  }
  NS_ADDREF((*aLeftNode));
  
  result = aChild->GetNextSibling(getter_AddRefs(sibling));
  while ((NS_SUCCEEDED(result)) && sibling)
  {
    PRBool isBlock;
    NodeIsBlockStatic(sibling, &isBlock);
    if (isBlock) 
    {
      nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(sibling);
      if (!nodeAsText) {
        break;
      }
    }
    *aRightNode = sibling;
    result = (*aRightNode)->GetNextSibling(getter_AddRefs(sibling)); 
  }
  NS_ADDREF((*aRightNode));

  return result;
}





nsresult
nsHTMLEditor::GetBlockSectionsForRange(nsIDOMRange *aRange,
                                       nsCOMArray<nsIDOMRange>& aSections) 
{
  if (!aRange) {return NS_ERROR_NULL_POINTER;}

  nsresult result;
  nsCOMPtr<nsIContentIterator>iter =
    do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &result);
  if ((NS_SUCCEEDED(result)) && iter)
  {
    nsCOMPtr<nsIDOMRange> lastRange;
    iter->Init(aRange);
    while (iter->IsDone())
    {
      nsCOMPtr<nsIContent> currentContent =
        do_QueryInterface(iter->GetCurrentNode());

      nsCOMPtr<nsIDOMNode>currentNode = do_QueryInterface(currentContent);
      if (currentNode)
      {
        
        if (currentContent->Tag() == nsEditProperty::br)
        {
          lastRange = nsnull;
        }
        else
        {
          PRBool isNotInlineOrText;
          result = NodeIsBlockStatic(currentNode, &isNotInlineOrText);
          if (isNotInlineOrText)
          {
            PRUint16 nodeType;
            currentNode->GetNodeType(&nodeType);
            if (nsIDOMNode::TEXT_NODE == nodeType) {
              isNotInlineOrText = PR_TRUE;
            }
          }
          if (PR_FALSE==isNotInlineOrText)
          {
            nsCOMPtr<nsIDOMNode>leftNode;
            nsCOMPtr<nsIDOMNode>rightNode;
            result = GetBlockSection(currentNode,
                                     getter_AddRefs(leftNode),
                                     getter_AddRefs(rightNode));
            if ((NS_SUCCEEDED(result)) && leftNode && rightNode)
            {
              
              PRBool addRange=PR_TRUE;
              if (lastRange)
              {
                nsCOMPtr<nsIDOMNode> lastStartNode;
                nsCOMPtr<nsIDOMElement> blockParentOfLastStartNode;
                lastRange->GetStartContainer(getter_AddRefs(lastStartNode));
                blockParentOfLastStartNode = do_QueryInterface(GetBlockNodeParent(lastStartNode));
                if (blockParentOfLastStartNode)
                {
                  nsCOMPtr<nsIDOMElement> blockParentOfLeftNode;
                  blockParentOfLeftNode = do_QueryInterface(GetBlockNodeParent(leftNode));
                  if (blockParentOfLeftNode)
                  {
                    if (blockParentOfLastStartNode==blockParentOfLeftNode) {
                      addRange = PR_FALSE;
                    }
                  }
                }
              }
              if (PR_TRUE==addRange) 
              {
                nsCOMPtr<nsIDOMRange> range =
                     do_CreateInstance("@mozilla.org/content/range;1", &result);
                if ((NS_SUCCEEDED(result)) && range)
                { 
                  range->SetStart(leftNode, 0);
                  range->SetEnd(rightNode, 0);
                  aSections.AppendObject(range);
                  lastRange = do_QueryInterface(range);
                }
              }        
            }
          }
        }
      }
      


      iter->Next();
    }
  }
  return result;
}





nsCOMPtr<nsIDOMNode>
nsHTMLEditor::NextNodeInBlock(nsIDOMNode *aNode, IterDirection aDir)
{
  nsCOMPtr<nsIDOMNode> nullNode;
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIContent> blockContent;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMNode> blockParent;
  
  NS_ENSURE_TRUE(aNode, nullNode);

  nsresult rv;
  nsCOMPtr<nsIContentIterator> iter =
       do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &rv);
  NS_ENSURE_SUCCESS(rv, nullNode);

  
  content = do_QueryInterface(aNode);
  PRBool isBlock;
  if (NS_SUCCEEDED(NodeIsBlockStatic(aNode, &isBlock)) && isBlock)
  {
    blockParent = aNode;
  }
  else
  {
    blockParent = GetBlockNodeParent(aNode);
  }
  NS_ENSURE_TRUE(blockParent, nullNode);
  blockContent = do_QueryInterface(blockParent);
  NS_ENSURE_TRUE(blockContent, nullNode);
  
  if (NS_FAILED(iter->Init(blockContent)))  return nullNode;
  if (NS_FAILED(iter->PositionAt(content)))  return nullNode;
  
  while (!iter->IsDone())
  {
    
    
    node = do_QueryInterface(iter->GetCurrentNode());
    if (node && IsTextOrElementNode(node) && node != blockParent &&
        node != aNode)
      return node;

    if (aDir == kIterForward)
      iter->Next();
    else
      iter->Prev();
  }
  
  return nullNode;
}

static const PRUnichar nbsp = 160;




nsresult 
nsHTMLEditor::IsNextCharWhitespace(nsIDOMNode *aParentNode, 
                                   PRInt32 aOffset,
                                   PRBool *outIsSpace,
                                   PRBool *outIsNBSP,
                                   nsCOMPtr<nsIDOMNode> *outNode,
                                   PRInt32 *outOffset)
{
  NS_ENSURE_TRUE(outIsSpace && outIsNBSP, NS_ERROR_NULL_POINTER);
  *outIsSpace = PR_FALSE;
  *outIsNBSP = PR_FALSE;
  if (outNode) *outNode = nsnull;
  if (outOffset) *outOffset = -1;
  
  nsAutoString tempString;
  PRUint32 strLength;
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(aParentNode);
  if (textNode)
  {
    textNode->GetLength(&strLength);
    if ((PRUint32)aOffset < strLength)
    {
      
      textNode->SubstringData(aOffset,aOffset+1,tempString);
      *outIsSpace = nsCRT::IsAsciiSpace(tempString.First());
      *outIsNBSP = (tempString.First() == nbsp);
      if (outNode) *outNode = do_QueryInterface(aParentNode);
      if (outOffset) *outOffset = aOffset+1;  
      return NS_OK;
    }
  }
  
  
  nsCOMPtr<nsIDOMNode> node = NextNodeInBlock(aParentNode, kIterForward);
  nsCOMPtr<nsIDOMNode> tmp;
  while (node) 
  {
    PRBool isBlock (PR_FALSE);
    NodeIsBlock(node, &isBlock);
    if (isBlock)  
    {
      if (IsTextNode(node) && IsEditable(node))
      {
        textNode = do_QueryInterface(node);
        textNode->GetLength(&strLength);
        if (strLength)
        {
          textNode->SubstringData(0,1,tempString);
          *outIsSpace = nsCRT::IsAsciiSpace(tempString.First());
          *outIsNBSP = (tempString.First() == nbsp);
          if (outNode) *outNode = do_QueryInterface(node);
          if (outOffset) *outOffset = 1;  
          return NS_OK;
        }
        
      }
      else  
      {
        break;
      }
    }
    tmp = node;
    node = NextNodeInBlock(tmp, kIterForward);
  }
  
  return NS_OK;
}





nsresult 
nsHTMLEditor::IsPrevCharWhitespace(nsIDOMNode *aParentNode, 
                                   PRInt32 aOffset,
                                   PRBool *outIsSpace,
                                   PRBool *outIsNBSP,
                                   nsCOMPtr<nsIDOMNode> *outNode,
                                   PRInt32 *outOffset)
{
  NS_ENSURE_TRUE(outIsSpace && outIsNBSP, NS_ERROR_NULL_POINTER);
  *outIsSpace = PR_FALSE;
  *outIsNBSP = PR_FALSE;
  if (outNode) *outNode = nsnull;
  if (outOffset) *outOffset = -1;
  
  nsAutoString tempString;
  PRUint32 strLength;
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(aParentNode);
  if (textNode)
  {
    if (aOffset > 0)
    {
      
      textNode->SubstringData(aOffset-1,aOffset,tempString);
      *outIsSpace = nsCRT::IsAsciiSpace(tempString.First());
      *outIsNBSP = (tempString.First() == nbsp);
      if (outNode) *outNode = do_QueryInterface(aParentNode);
      if (outOffset) *outOffset = aOffset-1;  
      return NS_OK;
    }
  }
  
  
  nsCOMPtr<nsIDOMNode> node = NextNodeInBlock(aParentNode, kIterBackward);
  nsCOMPtr<nsIDOMNode> tmp;
  while (node) 
  {
    PRBool isBlock (PR_FALSE);
    NodeIsBlock(node, &isBlock);
    if (isBlock)  
    {
      if (IsTextNode(node) && IsEditable(node))
      {
        textNode = do_QueryInterface(node);
        textNode->GetLength(&strLength);
        if (strLength)
        {
          
          textNode->SubstringData(strLength-1,strLength,tempString);
          *outIsSpace = nsCRT::IsAsciiSpace(tempString.First());
          *outIsNBSP = (tempString.First() == nbsp);
          if (outNode) *outNode = do_QueryInterface(aParentNode);
          if (outOffset) *outOffset = strLength-1;  
          return NS_OK;
        }
        
      }
      else  
      {
        break;
      }
    }
    
    tmp = node;
    node = NextNodeInBlock(tmp, kIterBackward);
  }
  
  return NS_OK;
  
}






PRBool nsHTMLEditor::IsVisBreak(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);
  if (!nsTextEditUtils::IsBreak(aNode)) 
    return PR_FALSE;
  
  nsCOMPtr<nsIDOMNode> priorNode, nextNode;
  GetPriorHTMLNode(aNode, address_of(priorNode), PR_TRUE); 
  GetNextHTMLNode(aNode, address_of(nextNode), PR_TRUE); 
  
  if (priorNode && nsTextEditUtils::IsBreak(priorNode))
    return PR_TRUE;
  if (nextNode && nsTextEditUtils::IsBreak(nextNode))
    return PR_TRUE;
  
  
  NS_ENSURE_TRUE(nextNode, PR_FALSE);  
  if (IsBlockNode(nextNode))
    return PR_FALSE; 
    
  
  
  nsCOMPtr<nsIDOMNode> selNode, tmp;
  PRInt32 selOffset;
  GetNodeLocation(aNode, address_of(selNode), &selOffset);
  selOffset++; 
  nsWSRunObject wsObj(this, selNode, selOffset);
  nsCOMPtr<nsIDOMNode> visNode;
  PRInt32 visOffset=0;
  PRInt16 visType=0;
  wsObj.NextVisibleNode(selNode, selOffset, address_of(visNode), &visOffset, &visType);
  if (visType & nsWSRunObject::eBlock)
    return PR_FALSE;
  
  return PR_TRUE;
}


NS_IMETHODIMP
nsHTMLEditor::GetIsDocumentEditable(PRBool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);

  nsCOMPtr<nsIDOMDocument> doc;
  GetDocument(getter_AddRefs(doc));
  *aIsDocumentEditable = doc ? IsModifiable() : PR_FALSE;

  return NS_OK;
}

PRBool nsHTMLEditor::IsModifiable()
{
  return !IsReadonly();
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsIHTMLEditor methods 
#pragma mark -
#endif

NS_IMETHODIMP
nsHTMLEditor::UpdateBaseURL()
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMNodeList> nodeList;
  nsresult rv = domDoc->GetElementsByTagName(NS_LITERAL_STRING("base"), getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> baseNode;
  if (nodeList)
  {
    PRUint32 count;
    nodeList->GetLength(&count);
    if (count >= 1)
    {
      rv = nodeList->Item(0, getter_AddRefs(baseNode));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  
  if (!baseNode)
  {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

    return doc->SetBaseURI(doc->GetDocumentURI());
  }
  return NS_OK;
}







NS_IMETHODIMP nsHTMLEditor::TypedText(const nsAString& aString,
                                      PRInt32 aAction)
{
  nsAutoPlaceHolderBatch batch(this, nsGkAtoms::TypingTxnName);

  switch (aAction)
  {
    case eTypedText:
    case eTypedBreak:
      {
        return nsPlaintextEditor::TypedText(aString, aAction);
      }
    case eTypedBR:
      {
        nsCOMPtr<nsIDOMNode> brNode;
        return InsertBR(address_of(brNode));  
      }
  } 
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP nsHTMLEditor::TabInTable(PRBool inIsShift, PRBool *outHandled)
{
  NS_ENSURE_TRUE(outHandled, NS_ERROR_NULL_POINTER);
  *outHandled = PR_FALSE;

  
  nsCOMPtr<nsIDOMElement> cellElement;
    
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nsnull, getter_AddRefs(cellElement));
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
      res = CollapseSelectionToDeepestNonTableFirstChild(nsnull, node);
      NS_ENSURE_SUCCESS(res, res);
      *outHandled = PR_TRUE;
      return NS_OK;
    }
  } while (!iter->IsDone());
  
  if (!(*outHandled) && !inIsShift)
  {
    
    
    res = InsertTableRow(1, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
    *outHandled = PR_TRUE;
    
    
    nsCOMPtr<nsISelection>selection;
    nsCOMPtr<nsIDOMElement> tblElement;
    nsCOMPtr<nsIDOMElement> cell;
    PRInt32 row;
    res = GetCellContext(getter_AddRefs(selection), 
                         getter_AddRefs(tblElement),
                         getter_AddRefs(cell), 
                         nsnull, nsnull,
                         &row, nsnull);
    NS_ENSURE_SUCCESS(res, res);
    
    res = GetCellAt(tblElement, row, 0, getter_AddRefs(cell));
    NS_ENSURE_SUCCESS(res, res);
    
    
    
    node = do_QueryInterface(cell);
    if (node) selection->Collapse(node,0);
    return NS_OK;
  }
  
  return res;
}

NS_IMETHODIMP nsHTMLEditor::CreateBRImpl(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                                         PRInt32 *aInOutOffset, 
                                         nsCOMPtr<nsIDOMNode> *outBRNode, 
                                         EDirection aSelect)
{
  NS_ENSURE_TRUE(aInOutParent && *aInOutParent && aInOutOffset && outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nsnull;
  nsresult res;
  
  
  nsCOMPtr<nsIDOMNode> node = *aInOutParent;
  PRInt32 theOffset = *aInOutOffset;
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(node);
  NS_NAMED_LITERAL_STRING(brType, "br");
  nsCOMPtr<nsIDOMNode> brNode;
  if (nodeAsText)  
  {
    nsCOMPtr<nsIDOMNode> tmp;
    PRInt32 offset;
    PRUint32 len;
    nodeAsText->GetLength(&len);
    GetNodeLocation(node, address_of(tmp), &offset);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    if (!theOffset)
    {
      
    }
    else if (theOffset == (PRInt32)len)
    {
      
      offset++;
    }
    else
    {
      
      res = SplitNode(node, theOffset, getter_AddRefs(tmp));
      NS_ENSURE_SUCCESS(res, res);
      res = GetNodeLocation(node, address_of(tmp), &offset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    res = CreateNode(brType, tmp, offset, getter_AddRefs(brNode));
    NS_ENSURE_SUCCESS(res, res);
    *aInOutParent = tmp;
    *aInOutOffset = offset+1;
  }
  else
  {
    res = CreateNode(brType, node, theOffset, getter_AddRefs(brNode));
    NS_ENSURE_SUCCESS(res, res);
    (*aInOutOffset)++;
  }

  *outBRNode = brNode;
  if (*outBRNode && (aSelect != eNone))
  {
    nsCOMPtr<nsISelection> selection;
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
    res = GetNodeLocation(*outBRNode, address_of(parent), &offset);
    NS_ENSURE_SUCCESS(res, res);
    if (aSelect == eNext)
    {
      
      selPriv->SetInterlinePosition(PR_TRUE);
      res = selection->Collapse(parent, offset+1);
    }
    else if (aSelect == ePrevious)
    {
      
      selPriv->SetInterlinePosition(PR_TRUE);
      res = selection->Collapse(parent, offset);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsHTMLEditor::CreateBR(nsIDOMNode *aNode, PRInt32 aOffset, nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect)
{
  nsCOMPtr<nsIDOMNode> parent = aNode;
  PRInt32 offset = aOffset;
  return CreateBRImpl(address_of(parent), &offset, outBRNode, aSelect);
}

NS_IMETHODIMP nsHTMLEditor::InsertBR(nsCOMPtr<nsIDOMNode> *outBRNode)
{
  PRBool bCollapsed;
  nsCOMPtr<nsISelection> selection;

  NS_ENSURE_TRUE(outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nsnull;

  
  nsAutoRules beginRulesSniffing(this, kOpInsertText, nsIEditor::eNext);

  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
  res = selection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(res, res);
  if (!bCollapsed)
  {
    res = DeleteSelection(nsIEditor::eNone);
    NS_ENSURE_SUCCESS(res, res);
  }
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;
  res = GetStartNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  res = CreateBR(selNode, selOffset, outBRNode);
  NS_ENSURE_SUCCESS(res, res);
    
  
  res = GetNodeLocation(*outBRNode, address_of(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  selPriv->SetInterlinePosition(PR_TRUE);
  res = selection->Collapse(selNode, selOffset+1);
  
  return res;
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
  nsAutoRules beginRulesSniffing(this, kOpIgnore, nsIEditor::eNone); 
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

  PRUint32 count; 
  nodeList->GetLength(&count);
  if (count < 1) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> headNode;
  res = nodeList->Item(0, getter_AddRefs(headNode)); 
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(headNode, NS_ERROR_NULL_POINTER);

  
  
  
  nsAutoString inputString (aSourceToInsert);  
 
  
  inputString.ReplaceSubstring(NS_LITERAL_STRING("\r\n").get(),
                               NS_LITERAL_STRING("\n").get());
 
  
  inputString.ReplaceSubstring(NS_LITERAL_STRING("\r").get(),
                               NS_LITERAL_STRING("\n").get());

  nsAutoEditBatch beginBatching(this);

  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNSRange> nsrange (do_QueryInterface(range));
  NS_ENSURE_TRUE(nsrange, NS_ERROR_NO_INTERFACE);
  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  res = nsrange->CreateContextualFragment(inputString,
                                          getter_AddRefs(docfrag));

  
  

  if (NS_FAILED(res))
  {
#ifdef DEBUG
    printf("Couldn't create contextual fragment: error was %d\n", res);
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

  
  PRInt32 offsetOfNewNode = 0;
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

  nsIDOMElement *bodyElement = GetRoot();
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(bodyElement, NS_ERROR_NULL_POINTER);

  
  nsReadingIterator<PRUnichar> beginbody;
  nsReadingIterator<PRUnichar> endbody;
  aSourceString.BeginReading(beginbody);
  aSourceString.EndReading(endbody);
  PRBool foundbody = CaseInsensitiveFindInReadable(NS_LITERAL_STRING("<body"),
                                                   beginbody, endbody);

  nsReadingIterator<PRUnichar> beginhead;
  nsReadingIterator<PRUnichar> endhead;
  aSourceString.BeginReading(beginhead);
  aSourceString.EndReading(endhead);
  PRBool foundhead = CaseInsensitiveFindInReadable(NS_LITERAL_STRING("<head"),
                                                   beginhead, endhead);

  nsReadingIterator<PRUnichar> beginclosehead;
  nsReadingIterator<PRUnichar> endclosehead;
  aSourceString.BeginReading(beginclosehead);
  aSourceString.EndReading(endclosehead);

  
  PRBool foundclosehead = CaseInsensitiveFindInReadable(
           NS_LITERAL_STRING("</head>"), beginclosehead, endclosehead);
  
  
  nsAutoEditBatch beginBatching(this);

  nsReadingIterator<PRUnichar> endtotal;
  aSourceString.EndReading(endtotal);

  if (foundhead) {
    if (foundclosehead)
      res = ReplaceHeadContentsWithHTML(Substring(beginhead, beginclosehead));
    else if (foundbody)
      res = ReplaceHeadContentsWithHTML(Substring(beginhead, beginbody));
    else
      
      
      
      res = ReplaceHeadContentsWithHTML(Substring(beginhead, endtotal));
  } else {
    nsReadingIterator<PRUnichar> begintotal;
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

  
  
  
  
  
  nsReadingIterator<PRUnichar> beginclosebody = beginbody;
  nsReadingIterator<PRUnichar> endclosebody;
  aSourceString.EndReading(endclosebody);
  if (!FindInReadable(NS_LITERAL_STRING(">"),beginclosebody,endclosebody))
    return NS_ERROR_FAILURE;

  
  
  nsAutoString bodyTag;
  bodyTag.AssignLiteral("<div ");
  bodyTag.Append(Substring(endbody, endclosebody));

  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNSRange> nsrange (do_QueryInterface(range));
  NS_ENSURE_TRUE(nsrange, NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIDOMDocumentFragment> docfrag;
  res = nsrange->CreateContextualFragment(bodyTag, getter_AddRefs(docfrag));
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
                                     PRInt32 *insertOffset)
{
  





























  if (!IsBlockNode(firstNodeToInsert))
    return;

  nsWSRunObject wsObj(this, *insertParentNode, *insertOffset);
  nsCOMPtr<nsIDOMNode> nextVisNode;
  nsCOMPtr<nsIDOMNode> prevVisNode;
  PRInt32 nextVisOffset=0;
  PRInt16 nextVisType=0;
  PRInt32 prevVisOffset=0;
  PRInt16 prevVisType=0;

  wsObj.NextVisibleNode(*insertParentNode, *insertOffset, address_of(nextVisNode), &nextVisOffset, &nextVisType);
  if (!nextVisNode)
    return;

  if (! (nextVisType & nsWSRunObject::eBreak))
    return;

  wsObj.PriorVisibleNode(*insertParentNode, *insertOffset, address_of(prevVisNode), &prevVisOffset, &prevVisType);
  if (!prevVisNode)
    return;

  if (prevVisType & nsWSRunObject::eBreak)
    return;

  if (prevVisType & nsWSRunObject::eThisBlock)
    return;

  nsCOMPtr<nsIDOMNode> brNode;
  PRInt32 brOffset=0;

  GetNodeLocation(nextVisNode, address_of(brNode), &brOffset);

  *insertParentNode = brNode;
  *insertOffset = brOffset + 1;
}

NS_IMETHODIMP
nsHTMLEditor::InsertElementAtSelection(nsIDOMElement* aElement, PRBool aDeleteSelection)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsresult res = NS_ERROR_NOT_INITIALIZED;
  
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  
  ForceCompositionEnd();
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertElement, nsIEditor::eNext);

  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res) || !selection)
    return NS_ERROR_FAILURE;

  
  PRBool cancel, handled;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
  ruleInfo.insertElement = aElement;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    if (aDeleteSelection)
    {
      nsCOMPtr<nsIDOMNode> tempNode;
      PRInt32 tempOffset;
      nsresult result = DeleteSelectionAndPrepareToCreateNode(tempNode,tempOffset);
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
    PRInt32 offsetForInsert;
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

      res = InsertNodeAtPoint(node, address_of(parentSelectedNode), &offsetForInsert, PR_FALSE);
      NS_ENSURE_SUCCESS(res, res);
      
      
      if (!SetCaretInTableCell(aElement))
      {
        res = SetCaretAfterElement(aElement);
        NS_ENSURE_SUCCESS(res, res);
      }
      
      if (nsHTMLEditUtils::IsTable(node))
      {
        PRBool isLast;
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
                                PRInt32 *ioOffset, 
                                PRBool aNoEmptyNodes)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(ioParent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(*ioParent, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(ioOffset, NS_ERROR_NULL_POINTER);
  
  nsresult res = NS_OK;
  nsAutoString tagName;
  aNode->GetNodeName(tagName);
  ToLowerCase(tagName);
  nsCOMPtr<nsIDOMNode> parent = *ioParent;
  nsCOMPtr<nsIDOMNode> topChild = *ioParent;
  nsCOMPtr<nsIDOMNode> tmp;
  PRInt32 offsetOfInsert = *ioOffset;
   
  
  while (!CanContainTag(parent, tagName))
  {
    
    
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

  
  if (IsElementInBody(aElement))
  {
    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMNode>parent;
    res = aElement->GetParentNode(getter_AddRefs(parent));
    if (NS_SUCCEEDED(res) && parent)
    {
      PRInt32 offsetInParent;
      res = GetChildOffset(aElement, parent, offsetInParent);

      if (NS_SUCCEEDED(res))
      {
        
        res = selection->Collapse(parent, offsetInParent);
        if (NS_SUCCEEDED(res)) {
          
          res = selection->Extend(parent, offsetInParent+1);
        }
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SetCaretAfterElement(nsIDOMElement* aElement)
{
  nsresult res = NS_ERROR_NULL_POINTER;

  
  if (aElement && IsElementInBody(aElement))
  {
    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMNode>parent;
    res = aElement->GetParentNode(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);
    PRInt32 offsetInParent;
    res = GetChildOffset(aElement, parent, offsetInParent);
    if (NS_SUCCEEDED(res))
    {
      
      res = selection->Collapse(parent, offsetInParent+1);
#if 0 
      {
      nsAutoString name;
      parent->GetNodeName(name);
      printf("SetCaretAfterElement: Parent node: ");
      wprintf(name.get());
      printf(" Offset: %d\n\nHTML:\n", offsetInParent+1);
      nsAutoString Format("text/html");
      nsAutoString ContentsAs;
      OutputToString(Format, 2, ContentsAs);
      wprintf(ContentsAs.get());
      }
#endif
    }
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
nsHTMLEditor::GetParagraphState(PRBool *aMixed, nsAString &outFormat)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIHTMLEditRules> htmlRules = do_QueryInterface(mRules);
  NS_ENSURE_TRUE(htmlRules, NS_ERROR_FAILURE);
  
  return htmlRules->GetParagraphState(aMixed, outFormat);
}

NS_IMETHODIMP
nsHTMLEditor::GetBackgroundColorState(PRBool *aMixed, nsAString &aOutColor)
{
  nsresult res;
  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);
  if (useCSS) {
    
    
    res = GetCSSBackgroundColorState(aMixed, aOutColor, PR_TRUE);
  }
  else {
    
    res = GetHTMLBackgroundColorState(aMixed, aOutColor);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::GetHighlightColorState(PRBool *aMixed, nsAString &aOutColor)
{
  nsresult res = NS_OK;
  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);
  *aMixed = PR_FALSE;
  aOutColor.AssignLiteral("transparent");
  if (useCSS) {
    
    
    
    res = GetCSSBackgroundColorState(aMixed, aOutColor, PR_FALSE);
  }
  return res;
}

nsresult
nsHTMLEditor::GetCSSBackgroundColorState(PRBool *aMixed, nsAString &aOutColor, PRBool aBlockLevel)
{
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = PR_FALSE;
  
  aOutColor.AssignLiteral("transparent");
  
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  res = GetStartNodeAndOffset(selection, getter_AddRefs(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);

  
  PRBool bCollapsed;
  res = selection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNode> nodeToExamine;
  if (bCollapsed || IsTextNode(parent))
  {
    
    nodeToExamine = parent;
  }
  else
  {
    
    
    nodeToExamine = GetChildAt(parent, offset);
    
  }
  
  NS_ENSURE_TRUE(nodeToExamine, NS_ERROR_NULL_POINTER);

  
  PRBool isBlock;
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
nsHTMLEditor::GetHTMLBackgroundColorState(PRBool *aMixed, nsAString &aOutColor)
{
  
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = PR_FALSE;
  aOutColor.Truncate();
  
  nsCOMPtr<nsIDOMElement> element;
  PRInt32 selectedCount;
  nsAutoString tagName;
  nsresult res = GetSelectedOrParentTableElement(tagName,
                                                 &selectedCount,
                                                 getter_AddRefs(element));
  NS_ENSURE_SUCCESS(res, res);

  NS_NAMED_LITERAL_STRING(styleName, "bgcolor"); 

  while (element)
  {
    
    res = element->GetAttribute(styleName, aOutColor);
    NS_ENSURE_SUCCESS(res, res);

    
    if (!aOutColor.IsEmpty())
      return NS_OK;

    
    if(nsTextEditUtils::IsBody(element)) return NS_OK;

    
    
    nsCOMPtr<nsIDOMNode> parentNode;
    res = element->GetParentNode(getter_AddRefs(parentNode));
    NS_ENSURE_SUCCESS(res, res);
    element = do_QueryInterface(parentNode);
  }

  
  element = GetRoot();
  NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);

  return element->GetAttribute(styleName, aOutColor);
}

NS_IMETHODIMP 
nsHTMLEditor::GetListState(PRBool *aMixed, PRBool *aOL, PRBool *aUL, PRBool *aDL)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed && aOL && aUL && aDL, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIHTMLEditRules> htmlRules = do_QueryInterface(mRules);
  NS_ENSURE_TRUE(htmlRules, NS_ERROR_FAILURE);
  
  return htmlRules->GetListState(aMixed, aOL, aUL, aDL);
}

NS_IMETHODIMP 
nsHTMLEditor::GetListItemState(PRBool *aMixed, PRBool *aLI, PRBool *aDT, PRBool *aDD)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed && aLI && aDT && aDD, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIHTMLEditRules> htmlRules = do_QueryInterface(mRules);
  NS_ENSURE_TRUE(htmlRules, NS_ERROR_FAILURE);
  
  return htmlRules->GetListItemState(aMixed, aLI, aDT, aDD);
}

NS_IMETHODIMP
nsHTMLEditor::GetAlignment(PRBool *aMixed, nsIHTMLEditor::EAlignment *aAlign)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aMixed && aAlign, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIHTMLEditRules> htmlRules = do_QueryInterface(mRules);
  NS_ENSURE_TRUE(htmlRules, NS_ERROR_FAILURE);
  
  return htmlRules->GetAlignment(aMixed, aAlign);
}


NS_IMETHODIMP 
nsHTMLEditor::GetIndentState(PRBool *aCanIndent, PRBool *aCanOutdent)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  NS_ENSURE_TRUE(aCanIndent && aCanOutdent, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIHTMLEditRules> htmlRules = do_QueryInterface(mRules);
  NS_ENSURE_TRUE(htmlRules, NS_ERROR_FAILURE);
  
  return htmlRules->GetIndentState(aCanIndent, aCanOutdent);
}

NS_IMETHODIMP
nsHTMLEditor::MakeOrChangeList(const nsAString& aListType, PRBool entireList, const nsAString& aBulletType)
{
  nsresult res;
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsCOMPtr<nsISelection> selection;
  PRBool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpMakeList, nsIEditor::eNext);
  
  
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(nsTextEditRules::kMakeList);
  ruleInfo.blockType = &aListType;
  ruleInfo.entireList = entireList;
  ruleInfo.bulletType = &aBulletType;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    
    PRBool isCollapsed;
    res = selection->GetIsCollapsed(&isCollapsed);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMNode> node;
    PRInt32 offset;
  
    res = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
    if (!node) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);
  
    if (isCollapsed)
    {
      
      nsCOMPtr<nsIDOMNode> parent = node;
      nsCOMPtr<nsIDOMNode> topChild = node;
      nsCOMPtr<nsIDOMNode> tmp;
    
      while ( !CanContainTag(parent, aListType))
      {
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

  nsCOMPtr<nsISelection> selection;
  PRBool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpRemoveList, nsIEditor::eNext);
  
  
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(nsTextEditRules::kRemoveList);
  if (aListType.LowerCaseEqualsLiteral("ol"))
    ruleInfo.bOrdered = PR_TRUE;
  else  ruleInfo.bOrdered = PR_FALSE;
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

  nsCOMPtr<nsISelection> selection;
  PRBool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpMakeDefListItem, nsIEditor::eNext);
  
  
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo(nsTextEditRules::kMakeDefListItem);
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

  nsCOMPtr<nsISelection> selection;
  PRBool cancel, handled;

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpMakeBasicBlock, nsIEditor::eNext);
  
  
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo(nsTextEditRules::kMakeBasicBlock);
  ruleInfo.blockType = &aBlockType;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;

  if (!handled)
  {
    
    PRBool isCollapsed;
    res = selection->GetIsCollapsed(&isCollapsed);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMNode> node;
    PRInt32 offset;
  
    res = GetStartNodeAndOffset(selection, getter_AddRefs(node), &offset);
    if (!node) res = NS_ERROR_FAILURE;
    NS_ENSURE_SUCCESS(res, res);
  
    if (isCollapsed)
    {
      
      nsCOMPtr<nsIDOMNode> parent = node;
      nsCOMPtr<nsIDOMNode> topChild = node;
      nsCOMPtr<nsIDOMNode> tmp;
    
      while ( !CanContainTag(parent, aBlockType))
      {
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

  PRBool cancel, handled;
  PRInt32 theAction = nsTextEditRules::kIndent;
  PRInt32 opID = kOpIndent;
  if (aIndent.LowerCaseEqualsLiteral("outdent"))
  {
    theAction = nsTextEditRules::kOutdent;
    opID = kOpOutdent;
  }
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, opID, nsIEditor::eNext);
  
  
  nsCOMPtr<nsISelection> selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(theAction);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || (NS_FAILED(res))) return res;
  
  if (!handled)
  {
    
    nsCOMPtr<nsIDOMNode> node;
    PRInt32 offset;
    PRBool isCollapsed;
    res = selection->GetIsCollapsed(&isCollapsed);
    NS_ENSURE_SUCCESS(res, res);

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
        NS_NAMED_LITERAL_STRING(bq, "blockquote");
        while ( !CanContainTag(parent, bq))
        {
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
        res = CreateNode(bq, parent, offset, getter_AddRefs(newBQ));
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
  nsAutoRules beginRulesSniffing(this, kOpAlign, nsIEditor::eNext);

  nsCOMPtr<nsIDOMNode> node;
  PRBool cancel, handled;
  
  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo(nsTextEditRules::kAlign);
  ruleInfo.alignType = &aAlignType;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || NS_FAILED(res))
    return res;
  
  res = mRules->DidDoAction(selection, &ruleInfo, res);
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::GetElementOrParentByTagName(const nsAString& aTagName, nsIDOMNode *aNode, nsIDOMElement** aReturn)
{
  if (aTagName.IsEmpty() || !aReturn )
    return NS_ERROR_NULL_POINTER;
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> currentNode;

  if (aNode)
    currentNode = aNode;
  else
  {
    
    nsCOMPtr<nsISelection>selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

    nsCOMPtr<nsIDOMNode> anchorNode;
    res = selection->GetAnchorNode(getter_AddRefs(anchorNode));
    if(NS_FAILED(res)) return res;
    NS_ENSURE_TRUE(anchorNode, NS_ERROR_FAILURE);

    
    PRBool hasChildren = PR_FALSE;
    anchorNode->HasChildNodes(&hasChildren);
    if (hasChildren)
    {
      PRInt32 offset;
      res = selection->GetAnchorOffset(&offset);
      if(NS_FAILED(res)) return res;
      currentNode = nsEditor::GetChildAt(anchorNode, offset);
    }
    
    if (!currentNode)
      currentNode = anchorNode;
  }
   
  nsAutoString TagName(aTagName);
  ToLowerCase(TagName);
  PRBool getLink = IsLinkTag(TagName);
  PRBool getNamedAnchor = IsNamedAnchorTag(TagName);
  if ( getLink || getNamedAnchor)
  {
    TagName.AssignLiteral("a");  
  }
  PRBool findTableCell = TagName.EqualsLiteral("td");
  PRBool findList = TagName.EqualsLiteral("list");

  
  *aReturn = nsnull;
  
  nsCOMPtr<nsIDOMNode> parent;
  PRBool bNodeFound = PR_FALSE;

  while (PR_TRUE)
  {
    nsAutoString currentTagName; 
    
    if ( (getLink && nsHTMLEditUtils::IsLink(currentNode)) ||
         (getNamedAnchor && nsHTMLEditUtils::IsNamedAnchor(currentNode)) )
    {
      bNodeFound = PR_TRUE;
      break;
    } else {
      if (findList)
      {
        
        if (nsHTMLEditUtils::IsList(currentNode))
          goto NODE_FOUND;

      } else if (findTableCell)
      {
        
        
        if (nsHTMLEditUtils::IsTableCell(currentNode))
          goto NODE_FOUND;

      } else {
        currentNode->GetNodeName(currentTagName);
        if (currentTagName.Equals(TagName, nsCaseInsensitiveStringComparator()))
        {
NODE_FOUND:
          bNodeFound = PR_TRUE;
          break;
        } 
      }
    }
    
    
    
    if (NS_FAILED(currentNode->GetParentNode(getter_AddRefs(parent))) || !parent)
      break;

    
    nsAutoString parentTagName;
    parent->GetNodeName(parentTagName);
    
    
    
    
    if(parentTagName.LowerCaseEqualsLiteral("body"))
      break;

    currentNode = parent;
  }
  if (bNodeFound)
  {
    nsCOMPtr<nsIDOMElement> currentElement = do_QueryInterface(currentNode);
    if (currentElement)
    {
      *aReturn = currentElement;
      
      NS_ADDREF(*aReturn);
    }
  }
  else res = NS_EDITOR_ELEMENT_NOT_FOUND;

  return res;
}

NS_IMETHODIMP
nsHTMLEditor::GetSelectedElement(const nsAString& aTagName, nsIDOMElement** aReturn)
{
  NS_ENSURE_TRUE(aReturn , NS_ERROR_NULL_POINTER);
  
  
  *aReturn = nsnull;
  
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  PRBool bNodeFound = PR_FALSE;
  res=NS_ERROR_NOT_INITIALIZED;
  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);

  nsAutoString domTagName;
  nsAutoString TagName(aTagName);
  ToLowerCase(TagName);
  
  PRBool anyTag = (TagName.IsEmpty());
  PRBool isLinkTag = IsLinkTag(TagName);
  PRBool isNamedAnchorTag = IsNamedAnchorTag(TagName);
  
  nsCOMPtr<nsIDOMElement> selectedElement;
  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> startParent;
  PRInt32 startOffset, endOffset;
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
        bNodeFound = PR_TRUE;
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
      PRInt32 anchorOffset = -1;
      if (anchorNode)
        selection->GetAnchorOffset(&anchorOffset);
    
      nsCOMPtr<nsIDOMNode> focusNode;
      res = selection->GetFocusNode(getter_AddRefs(focusNode));
      NS_ENSURE_SUCCESS(res, res);
      PRInt32 focusOffset = -1;
      if (focusNode)
        selection->GetFocusOffset(&focusOffset);

      
      if (NS_SUCCEEDED(res) && anchorNode)
      {
  #ifdef DEBUG_cmanske
        {
        nsAutoString name;
        anchorNode->GetNodeName(name);
        printf("GetSelectedElement: Anchor node of selection: ");
        wprintf(name.get());
        printf(" Offset: %d\n", anchorOffset);
        focusNode->GetNodeName(name);
        printf("Focus node of selection: ");
        wprintf(name.get());
        printf(" Offset: %d\n", focusOffset);
        }
  #endif
        nsCOMPtr<nsIDOMElement> parentLinkOfAnchor;
        res = GetElementOrParentByTagName(NS_LITERAL_STRING("href"), anchorNode, getter_AddRefs(parentLinkOfAnchor));
        
        if (NS_SUCCEEDED(res) && parentLinkOfAnchor)
        {
          if (isCollapsed)
          {
            
            bNodeFound = PR_TRUE;
          } else if(focusNode) 
          {  
            nsCOMPtr<nsIDOMElement> parentLinkOfFocus;
            res = GetElementOrParentByTagName(NS_LITERAL_STRING("href"), focusNode, getter_AddRefs(parentLinkOfFocus));
            if (NS_SUCCEEDED(res) && parentLinkOfFocus == parentLinkOfAnchor)
              bNodeFound = PR_TRUE;
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
            bNodeFound = PR_TRUE;
          }
        }
      }
    } 

    if (!isCollapsed)   
    {
      nsCOMPtr<nsIEnumerator> enumerator;
      res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
      if (NS_SUCCEEDED(res))
      {
        if(!enumerator)
          return NS_ERROR_NULL_POINTER;

        enumerator->First(); 
        nsCOMPtr<nsISupports> currentItem;
        res = enumerator->CurrentItem(getter_AddRefs(currentItem));
        if ((NS_SUCCEEDED(res)) && currentItem)
        {
          nsCOMPtr<nsIDOMRange> currange( do_QueryInterface(currentItem) );
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
                bNodeFound = PR_FALSE;
                break;
              }

              selectedElement->GetNodeName(domTagName);
              ToLowerCase(domTagName);

              if (anyTag)
              {
                
                selectedElement->GetTagName(TagName);
                ToLowerCase(TagName);
                anyTag = PR_FALSE;
              }

              
              
              nsCOMPtr<nsIDOMNode> selectedNode = do_QueryInterface(selectedElement);
              if ( (isLinkTag && nsHTMLEditUtils::IsLink(selectedNode)) ||
                   (isNamedAnchorTag && nsHTMLEditUtils::IsNamedAnchor(selectedNode)) )
              {
                bNodeFound = PR_TRUE;
              } else if (TagName == domTagName) { 
                bNodeFound = PR_TRUE;
              }
              if (!bNodeFound)
              {
                
                break;
              }
            }
            iter->Next();
          }
        } else {
          
          isCollapsed = PR_TRUE;
          printf("isCollapsed was FALSE, but no elements found in selection\n");
        }
      } else {
        printf("Could not create enumerator for GetSelectionProperties\n");
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
    *aReturn = nsnull;


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
  nsCOMPtr<nsIContent> newContent;
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

  
  res = CreateHTMLContent(realTagName, getter_AddRefs(newContent));
  newElement = do_QueryInterface(newContent);
  if (NS_FAILED(res) || !newElement)
    return NS_ERROR_FAILURE;

  
  newElement->SetAttribute(NS_LITERAL_STRING("_moz_dirty"), EmptyString());

  
  if (TagName.EqualsLiteral("hr"))
  {
    
    res = SetAttributeOrEquivalent(newElement, NS_LITERAL_STRING("width"),
                                   NS_LITERAL_STRING("100%"), PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
    res = SetAttributeOrEquivalent(newElement, NS_LITERAL_STRING("size"),
                                   NS_LITERAL_STRING("2"), PR_TRUE);
  } else if (TagName.EqualsLiteral("table"))
  {
    res = newElement->SetAttribute(NS_LITERAL_STRING("cellpadding"),NS_LITERAL_STRING("2"));
    NS_ENSURE_SUCCESS(res, res);
    res = newElement->SetAttribute(NS_LITERAL_STRING("cellspacing"),NS_LITERAL_STRING("2"));
    NS_ENSURE_SUCCESS(res, res);
    res = newElement->SetAttribute(NS_LITERAL_STRING("border"),NS_LITERAL_STRING("1"));
  } else if (TagName.EqualsLiteral("td"))
  {
    res = SetAttributeOrEquivalent(newElement, NS_LITERAL_STRING("valign"),
                                   NS_LITERAL_STRING("top"), PR_TRUE);
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
  nsresult res=NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelection> selection;

  NS_ENSURE_TRUE(aAnchorElement, NS_ERROR_NULL_POINTER); 


  
  res = GetSelection(getter_AddRefs(selection));
  if (!selection)
  {
    res = NS_ERROR_NULL_POINTER;
  }
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  PRBool isCollapsed;
  res = selection->GetIsCollapsed(&isCollapsed);
  if (NS_FAILED(res))
    isCollapsed = PR_TRUE;
  
  if (isCollapsed)
  {
    printf("InsertLinkAroundSelection called but there is no selection!!!\n");     
    res = NS_OK;
  } else {
    
    nsCOMPtr<nsIDOMHTMLAnchorElement> anchor = do_QueryInterface(aAnchorElement);
    if (anchor)
    {
      nsAutoString href;
      res = anchor->GetHref(href);
      NS_ENSURE_SUCCESS(res, res);
      if (!href.IsEmpty())      
      {
        nsAutoEditBatch beginBatching(this);

        
        nsCOMPtr<nsIDOMNamedNodeMap> attrMap;
        aAnchorElement->GetAttributes(getter_AddRefs(attrMap));
        NS_ENSURE_TRUE(attrMap, NS_ERROR_FAILURE);

        PRUint32 count, i;
        attrMap->GetLength(&count);
        nsAutoString name, value;

        for (i = 0; i < count; i++)
        {
          nsCOMPtr<nsIDOMNode> attrNode;
          res = attrMap->Item(i, getter_AddRefs(attrNode));
          NS_ENSURE_SUCCESS(res, res);

          if (attrNode)
          {
            nsCOMPtr<nsIDOMAttr> attribute = do_QueryInterface(attrNode);
            if (attribute)
            {
              
              
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
        }
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SetHTMLBackgroundColor(const nsAString& aColor)
{
  NS_PRECONDITION(mDocWeak, "Missing Editor DOM Document");
  
  
  nsCOMPtr<nsIDOMElement> element;
  PRInt32 selectedCount;
  nsAutoString tagName;
  nsresult res = GetSelectedOrParentTableElement(tagName, &selectedCount,
                                                 getter_AddRefs(element));
  NS_ENSURE_SUCCESS(res, res);

  PRBool setColor = !aColor.IsEmpty();

  NS_NAMED_LITERAL_STRING(bgcolor, "bgcolor");
  if (element)
  {
    if (selectedCount > 0)
    {
      
      nsCOMPtr<nsIDOMElement> cell;
      res = GetFirstSelectedCell(nsnull, getter_AddRefs(cell));
      if (NS_SUCCEEDED(res) && cell)
      {
        while(cell)
        {
          if (setColor)
            res = SetAttribute(cell, bgcolor, aColor);
          else
            res = RemoveAttribute(cell, bgcolor);
          if (NS_FAILED(res)) break;

          GetNextSelectedCell(nsnull, getter_AddRefs(cell));
        };
        return res;
      }
    }
    
  } else {
    
    element = GetRoot();
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
  
  
  nsIDOMElement *bodyElement = GetRoot();

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
    nsCOMPtr<nsIDOMDocument> domdoc;
    nsEditor::GetDocument(getter_AddRefs(domdoc));
    NS_ENSURE_TRUE(domdoc, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDocument> doc (do_QueryInterface(domdoc));
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

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsIEditorStyleSheets methods 
#pragma mark -
#endif

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
      return EnableStyleSheet(mLastStyleSheetURL, PR_FALSE);

    return NS_OK;
  }

  
  NS_ENSURE_TRUE(mPresShellWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uaURI;
  nsresult rv = NS_NewURI(getter_AddRefs(uaURI), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  return ps->GetDocument()->CSSLoader()->
    LoadSheet(uaURI, nsnull, EmptyCString(), this);
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

  
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uaURI;
  nsresult rv = NS_NewURI(getter_AddRefs(uaURI), aURL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsRefPtr<nsCSSStyleSheet> sheet;
  
  rv = ps->GetDocument()->CSSLoader()->
    LoadSheetSync(uaURI, PR_TRUE, PR_TRUE, getter_AddRefs(sheet));

  
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
      return EnableStyleSheet(mLastOverrideStyleSheetURL, PR_FALSE);

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

  NS_ENSURE_TRUE(mPresShellWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  ps->RemoveOverrideStyleSheet(sheet);
  ps->ReconstructStyleData();

  
  return rv;
}

NS_IMETHODIMP
nsHTMLEditor::EnableStyleSheet(const nsAString &aURL, PRBool aEnable)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  nsresult rv = GetStyleSheetForURL(aURL, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(sheet, NS_OK); 

  
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  sheet->SetOwningDocument(doc);

  return sheet->SetDisabled(!aEnable);
}

PRBool
nsHTMLEditor::EnableExistingStyleSheet(const nsAString &aURL)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  nsresult rv = GetStyleSheetForURL(aURL, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  if (sheet)
  {
    
    nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
    sheet->SetOwningDocument(doc);

    sheet->SetDisabled(PR_FALSE);
    return PR_TRUE;
  }
  return PR_FALSE;
}

nsresult
nsHTMLEditor::AddNewStyleSheetToList(const nsAString &aURL,
                                     nsCSSStyleSheet *aStyleSheet)
{
  PRUint32 countSS = mStyleSheets.Length();
  PRUint32 countU = mStyleSheetURLs.Length();

  if (countU < 0 || countSS != countU)
    return NS_ERROR_UNEXPECTED;

  if (!mStyleSheetURLs.AppendElement(aURL))
    return NS_ERROR_UNEXPECTED;

  return mStyleSheets.AppendElement(aStyleSheet) ? NS_OK : NS_ERROR_UNEXPECTED;
}

nsresult
nsHTMLEditor::RemoveStyleSheetFromList(const nsAString &aURL)
{
  
  PRUint32 foundIndex;
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

  
  PRUint32 foundIndex;
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
  
  PRInt32 foundIndex = mStyleSheets.IndexOf(aStyleSheet);

  
  
  
  if (foundIndex == -1)
    return NS_OK;

  
  aURL = mStyleSheetURLs[foundIndex];
  return NS_OK;
}





NS_IMETHODIMP
nsHTMLEditor::GetEmbeddedObjects(nsISupportsArray** aNodeList)
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
    nsCOMPtr<nsIDOMDocument> domdoc;
    nsEditor::GetDocument(getter_AddRefs(domdoc));
    NS_ENSURE_TRUE(domdoc, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDocument> doc (do_QueryInterface(domdoc));
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    iter->Init(doc->GetRootElement());

    
    while (!iter->IsDone())
    {
      nsCOMPtr<nsIDOMNode> node (do_QueryInterface(iter->GetCurrentNode()));
      if (node)
      {
        nsAutoString tagName;
        node->GetNodeName(tagName);
        ToLowerCase(tagName);

        
        
        if (tagName.EqualsLiteral("img") || tagName.EqualsLiteral("embed") ||
            tagName.EqualsLiteral("a"))
          (*aNodeList)->AppendElement(node);
        else if (tagName.EqualsLiteral("body"))
        {
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
          if (element)
          {
            PRBool hasBackground = PR_FALSE;
            if (NS_SUCCEEDED(element->HasAttribute(NS_LITERAL_STRING("background"), &hasBackground)) && hasBackground)
              (*aNodeList)->AppendElement(node);
          }
        }
      }
      iter->Next();
    }
  }

  return res;
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  nsIEditor overrides 
#pragma mark -
#endif

NS_IMETHODIMP nsHTMLEditor::DeleteNode(nsIDOMNode * aNode)
{
  
  if (!IsModifiableNode(aNode)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> selectAllNode = FindUserSelectAllNode(aNode);
  
  if (selectAllNode)
  {
    return nsEditor::DeleteNode(selectAllNode);
  }
  return nsEditor::DeleteNode(aNode);
}

NS_IMETHODIMP nsHTMLEditor::DeleteText(nsIDOMCharacterData *aTextNode,
                                       PRUint32             aOffset,
                                       PRUint32             aLength)
{
  
  if (!IsModifiableNode(aTextNode)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> selectAllNode = FindUserSelectAllNode(aTextNode);
  
  if (selectAllNode)
  {
    return nsEditor::DeleteNode(selectAllNode);
  }
  return nsEditor::DeleteText(aTextNode, aOffset, aLength);
}

NS_IMETHODIMP nsHTMLEditor::InsertTextImpl(const nsAString& aStringToInsert, 
                                           nsCOMPtr<nsIDOMNode> *aInOutNode, 
                                           PRInt32 *aInOutOffset,
                                           nsIDOMDocument *aDoc)
{
  
  if (!IsModifiableNode(*aInOutNode)) {
    return NS_ERROR_FAILURE;
  }

  return nsEditor::InsertTextImpl(aStringToInsert, aInOutNode, aInOutOffset, aDoc);
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsStubMutationObserver overrides 
#pragma mark -
#endif

void
nsHTMLEditor::ContentAppended(nsIDocument *aDocument, nsIContent* aContainer,
                              nsIContent* aFirstNewContent,
                              PRInt32 )
{
  ContentInserted(aDocument, aContainer, aFirstNewContent, 0);
}

void
nsHTMLEditor::ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                              nsIContent* aChild, PRInt32 )
{
  if (!aChild || !aChild->IsElement()) {
    return;
  }

  if (ShouldReplaceRootElement()) {
    ResetRootElementAndEventTarget();
  }
}

void
nsHTMLEditor::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                             nsIContent* aChild, PRInt32 aIndexInContainer)
{
  if (SameCOMIdentity(aChild, mRootElement)) {
    ResetRootElementAndEventTarget();
  }
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  support utils
#pragma mark -
#endif



nsCOMPtr<nsIDOMNode> nsHTMLEditor::FindUserSelectAllNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> resultNode;  
  nsCOMPtr<nsIDOMNode> node = aNode;
  nsIDOMElement *root = GetRoot();
  if (!nsEditorUtils::IsDescendantOf(aNode, root))
    return nsnull;

  
  nsAutoString mozUserSelectValue;
  while (node)
  {
    mHTMLCSSUtils->GetComputedProperty(node, nsEditProperty::cssMozUserSelect, mozUserSelectValue);
    if (mozUserSelectValue.EqualsLiteral("all"))
    {
      resultNode = node;
    }
    if (node != root)
    {
      nsCOMPtr<nsIDOMNode> tmp;
      node->GetParentNode(getter_AddRefs(tmp));
      node = tmp;
    }
    else
    {
      node = nsnull;
    }
  } 

  return resultNode;
}

NS_IMETHODIMP_(PRBool)
nsHTMLEditor::IsModifiableNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);

  return !content || !(content->IntrinsicState() & NS_EVENT_STATE_MOZ_READONLY);
}

static nsresult SetSelectionAroundHeadChildren(nsCOMPtr<nsISelection> aSelection, nsWeakPtr aDocWeak)
{
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(aDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIDOMNodeList>nodeList; 
  res = doc->GetElementsByTagName(NS_LITERAL_STRING("head"), getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(nodeList, NS_ERROR_NULL_POINTER);

  PRUint32 count; 
  nodeList->GetLength(&count);
  if (count < 1) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> headNode;
  res = nodeList->Item(0, getter_AddRefs(headNode)); 
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(headNode, NS_ERROR_NULL_POINTER);

  
  res = aSelection->Collapse(headNode, 0);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMNodeList> childNodes;
  res = headNode->GetChildNodes(getter_AddRefs(childNodes));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(childNodes, NS_ERROR_NULL_POINTER);
  PRUint32 childCount;
  childNodes->GetLength(&childCount);

  return aSelection->Extend(headNode, childCount+1);
}

NS_IMETHODIMP
nsHTMLEditor::GetHeadContentsAsHTML(nsAString& aOutputString)
{
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  nsAutoSelectionReset selectionResetter(selection, this);

  res = SetSelectionAroundHeadChildren(selection, mDocWeak);
  NS_ENSURE_SUCCESS(res, res);

  res = OutputToString(NS_LITERAL_STRING("text/html"),
                       nsIDocumentEncoder::OutputSelectionOnly,
                       aOutputString);
  if (NS_SUCCEEDED(res))
  {
    
    
    nsReadingIterator<PRUnichar> findIter,endFindIter;
    aOutputString.BeginReading(findIter);
    aOutputString.EndReading(endFindIter);
    
    if (CaseInsensitiveFindInReadable(NS_LITERAL_STRING("<body"),
                                      findIter, endFindIter))
    {
      nsReadingIterator<PRUnichar> beginIter;
      aOutputString.BeginReading(beginIter);
      PRInt32 offset = Distance(beginIter, findIter);

      nsWritingIterator<PRUnichar> writeIter;
      aOutputString.BeginWriting(writeIter);
      
      PRUnichar newline ('\n');
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
nsHTMLEditor::DebugUnitTests(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed)
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

#ifdef XP_MAC
#pragma mark -
#pragma mark  StyleSheet utils 
#pragma mark -
#endif


NS_IMETHODIMP 
nsHTMLEditor::StyleSheetLoaded(nsCSSStyleSheet* aSheet, PRBool aWasAlternate,
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
      
      nsCAutoString spec;
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

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsEditor overrides 
#pragma mark -
#endif




NS_IMETHODIMP
nsHTMLEditor::StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection)
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

PRBool 
nsHTMLEditor::TagCanContainTag(const nsAString& aParentTag, const nsAString& aChildTag)  
{
  nsIParserService* parserService = nsContentUtils::GetParserService();

  PRInt32 childTagEnum;
  
  if (aChildTag.EqualsLiteral("#text")) {
    childTagEnum = eHTMLTag_text;
  }
  else {
    childTagEnum = parserService->HTMLStringTagToId(aChildTag);
  }

  PRInt32 parentTagEnum = parserService->HTMLStringTagToId(aParentTag);
  NS_ASSERTION(parentTagEnum < NS_HTML_TAG_MAX,
               "Fix the caller, this type of node can never contain children.");

  return nsHTMLEditUtils::CanContain(parentTagEnum, childTagEnum);
}

PRBool 
nsHTMLEditor::IsContainer(nsIDOMNode *aNode)
{
  if (!aNode) {
    return PR_FALSE;
  }

  nsAutoString stringTag;

  nsresult rv = aNode->GetNodeName(stringTag);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRInt32 tagEnum;
  
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

  
  nsIDOMElement *rootElement = GetRoot();
  
  
  PRBool bDocIsEmpty;
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
  nsCOMPtr<nsISelectionController> selCon = do_QueryReferent(mSelConWeak, &rv);
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
  
  
  
  nsIFrame* frame = anchorContent->GetPrimaryFrame();
  if (frame && frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION) {
    nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection);
    NS_ENSURE_TRUE(selPriv, NS_ERROR_UNEXPECTED);
    rv = selPriv->SetAncestorLimiter(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    return selection->SelectAllChildren(mRootElement);
  }

  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  nsIContent *rootContent = anchorContent->GetSelectionRootContent(ps);
  NS_ENSURE_TRUE(rootContent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNode> rootElement = do_QueryInterface(rootContent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return selection->SelectAllChildren(rootElement);
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  Random methods 
#pragma mark -
#endif



void nsHTMLEditor::IsTextPropertySetByContent(nsIDOMNode        *aNode,
                                              nsIAtom           *aProperty, 
                                              const nsAString   *aAttribute, 
                                              const nsAString   *aValue, 
                                              PRBool            &aIsSet,
                                              nsIDOMNode       **aStyleNode,
                                              nsAString *outValue)
{
  nsresult result;
  aIsSet = PR_FALSE;  
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
        PRBool found = PR_FALSE;
        if (aAttribute && 0!=aAttribute->Length())
        {
          element->GetAttribute(*aAttribute, value);
          if (outValue) *outValue = value;
          if (!value.IsEmpty())
          {
            if (!aValue) {
              found = PR_TRUE;
            }
            else
            {
              nsString tString(*aValue);
              if (tString.Equals(value, nsCaseInsensitiveStringComparator())) {
                found = PR_TRUE;
              }
              else {  
                break;
              }
            }
          }
        }
        else { 
          found = PR_TRUE;
        }
        if (found)
        {
          aIsSet = PR_TRUE;
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
      node = nsnull;
    }
  }
}

#ifdef XP_MAC
#pragma mark -
#endif








PRBool nsHTMLEditor::IsElementInBody(nsIDOMElement* aElement)
{
  return nsTextEditUtils::InBody(aElement, this);
}

PRBool
nsHTMLEditor::SetCaretInTableCell(nsIDOMElement* aElement)
{
  PRBool caretIsSet = PR_FALSE;

  if (aElement && IsElementInBody(aElement))
  {
    nsresult res = NS_OK;
    nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
    if (content)
    {
      nsIAtom *atom = content->Tag();
      if (atom == nsEditProperty::table ||
          atom == nsEditProperty::tbody ||
          atom == nsEditProperty::thead ||
          atom == nsEditProperty::tfoot ||
          atom == nsEditProperty::caption ||
          atom == nsEditProperty::tr ||
          atom == nsEditProperty::td )
      {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
        nsCOMPtr<nsIDOMNode> parent;
        
        node->GetParentNode(getter_AddRefs(parent));
        nsCOMPtr<nsIDOMNode>firstChild;
        
        PRBool hasChild;
        while (NS_SUCCEEDED(node->HasChildNodes(&hasChild)) && hasChild)
        {
          if (NS_SUCCEEDED(node->GetFirstChild(getter_AddRefs(firstChild))))
          {
            parent = node;
            node = firstChild;
          }
        }
        
        nsCOMPtr<nsISelection> selection;
        res = GetSelection(getter_AddRefs(selection));
        if (NS_SUCCEEDED(res) && selection && firstChild)
        {
          res = selection->Collapse(firstChild, 0);
          if (NS_SUCCEEDED(res))
            caretIsSet = PR_TRUE;
        }
      }
    }
  }
  return caretIsSet;
}            



NS_IMETHODIMP
nsHTMLEditor::IsRootTag(nsString &aTag, PRBool &aIsTag)
{
  static char bodyTag[] = "body";
  static char tdTag[] = "td";
  static char thTag[] = "th";
  static char captionTag[] = "caption";
  if (aTag.EqualsIgnoreCase(bodyTag) ||
      aTag.EqualsIgnoreCase(tdTag) ||
      aTag.EqualsIgnoreCase(thTag) ||
      aTag.EqualsIgnoreCase(captionTag) )
  {
    aIsTag = PR_TRUE;
  }
  else {
    aIsTag = PR_FALSE;
  }
  return NS_OK;
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

#ifdef XP_MAC
#pragma mark -
#endif

#ifdef PRE_NODE_IN_BODY
nsCOMPtr<nsIDOMElement> nsHTMLEditor::FindPreElement()
{
  nsCOMPtr<nsIDOMDocument> domdoc;
  nsEditor::GetDocument(getter_AddRefs(domdoc));
  NS_ENSURE_TRUE(domdoc, 0);

  nsCOMPtr<nsIDocument> doc (do_QueryInterface(domdoc));
  NS_ENSURE_TRUE(doc, 0);

  nsCOMPtr<nsIContent> rootContent = doc->GetRootElement();
  NS_ENSURE_TRUE(rootContent, 0);

  nsCOMPtr<nsIDOMNode> rootNode (do_QueryInterface(rootContent));
  NS_ENSURE_TRUE(rootNode, 0);

  nsString prestr ("PRE");  
  nsCOMPtr<nsIDOMNode> preNode;
  if (NS_FAILED(nsEditor::GetFirstNodeOfType(rootNode, prestr,
                                                 getter_AddRefs(preNode))))
    return 0;

  return do_QueryInterface(preNode);
}
#endif 







NS_IMETHODIMP
nsHTMLEditor::CollapseAdjacentTextNodes(nsIDOMRange *aInRange)
{
  NS_ENSURE_TRUE(aInRange, NS_ERROR_NULL_POINTER);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  nsTArray<nsIDOMNode*> textNodes;
  
  
  


  
  nsresult result;
  nsCOMPtr<nsIContentIterator> iter =
    do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &result);
  NS_ENSURE_SUCCESS(result, result);

  iter->Init(aInRange);

  while (!iter->IsDone())
  {
    nsCOMPtr<nsIDOMCharacterData> text = do_QueryInterface(iter->GetCurrentNode());
    if (text && IsEditable(text))
    {
      textNodes.AppendElement(text);
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
  nsIDOMElement *rootElement = GetRoot();  
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER);

  return aSelection->Collapse(rootElement,0);
}

#ifdef XP_MAC
#pragma mark -
#endif






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
        
        PRUint32 len;
        res = GetLengthOfDOMNode(inNode, len);
        NS_ENSURE_SUCCESS(res, res);
        res = CreateBR(inNode, (PRInt32)len, address_of(unused));
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






nsresult
nsHTMLEditor::GetPriorHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode && inNode, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  *outNode = nsnull;
  nsCOMPtr<nsIDOMNode> temp, node = do_QueryInterface(inNode);
  
  while (1)
  {
    res = node->GetPreviousSibling(getter_AddRefs(temp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(temp, NS_OK);  
    
    if (IsEditable(temp)) break;
    
    node = temp;
  }
  *outNode = temp;
  return res;
}








nsresult
nsHTMLEditor::GetPriorHTMLSibling(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode && inParent, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  *outNode = nsnull;
  NS_ENSURE_TRUE(inOffset, NS_OK);  
  nsCOMPtr<nsIDOMNode> node = nsEditor::GetChildAt(inParent,inOffset-1);
  if (IsEditable(node)) 
  {
    *outNode = node;
    return res;
  }
  
  return GetPriorHTMLSibling(node, outNode);
}







nsresult
nsHTMLEditor::GetNextHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  *outNode = nsnull;
  nsCOMPtr<nsIDOMNode> temp, node = do_QueryInterface(inNode);
  
  while (1)
  {
    res = node->GetNextSibling(getter_AddRefs(temp));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(temp, NS_OK);  
    
    if (IsEditable(temp)) break;
    
    node = temp;
  }
  *outNode = temp;
  return res;
}








nsresult
nsHTMLEditor::GetNextHTMLSibling(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode)
{
  NS_ENSURE_TRUE(outNode && inParent, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  *outNode = nsnull;
  nsCOMPtr<nsIDOMNode> node = nsEditor::GetChildAt(inParent,inOffset);
  NS_ENSURE_TRUE(node, NS_OK); 
  if (IsEditable(node)) 
  {
    *outNode = node;
    return res;
  }
  
  return GetPriorHTMLSibling(node, outNode);
}







nsresult
nsHTMLEditor::GetPriorHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  nsresult res = GetPriorNode(inNode, PR_TRUE, address_of(*outNode), bNoBlockCrossing);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (*outNode && !nsTextEditUtils::InBody(*outNode, this))
  {
    *outNode = nsnull;
  }
  return res;
}





nsresult
nsHTMLEditor::GetPriorHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  nsresult res = GetPriorNode(inParent, inOffset, PR_TRUE, address_of(*outNode), bNoBlockCrossing);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (*outNode && !nsTextEditUtils::InBody(*outNode, this))
  {
    *outNode = nsnull;
  }
  return res;
}






nsresult
nsHTMLEditor::GetNextHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  nsresult res = GetNextNode(inNode, PR_TRUE, address_of(*outNode), bNoBlockCrossing);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (*outNode && !nsTextEditUtils::InBody(*outNode, this))
  {
    *outNode = nsnull;
  }
  return res;
}





nsresult
nsHTMLEditor::GetNextHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing)
{
  NS_ENSURE_TRUE(outNode, NS_ERROR_NULL_POINTER);
  nsresult res = GetNextNode(inParent, inOffset, PR_TRUE, address_of(*outNode), bNoBlockCrossing);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (*outNode && !nsTextEditUtils::InBody(*outNode, this))
  {
    *outNode = nsnull;
  }
  return res;
}


nsresult 
nsHTMLEditor::IsFirstEditableChild( nsIDOMNode *aNode, PRBool *aOutIsFirst)
{
  
  NS_ENSURE_TRUE(aOutIsFirst && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutIsFirst = PR_FALSE;
  
  
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
nsHTMLEditor::IsLastEditableChild( nsIDOMNode *aNode, PRBool *aOutIsLast)
{
  
  NS_ENSURE_TRUE(aOutIsLast && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutIsLast = PR_FALSE;
  
  
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
  
  
  *aOutFirstChild = nsnull;
  
  
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
      child = nsnull;  
    }
  }
  
  *aOutFirstLeaf = child;
  return res;
}


nsresult 
nsHTMLEditor::GetLastEditableLeaf( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastLeaf)
{
  
  NS_ENSURE_TRUE(aOutLastLeaf && aNode, NS_ERROR_NULL_POINTER);
  
  
  *aOutLastLeaf = nsnull;
  
  
  nsCOMPtr<nsIDOMNode> child;
  nsresult res = NS_OK;
  child = GetRightmostChild(aNode, PR_FALSE);  
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
      child = nsnull;
    }
  }
  
  *aOutLastLeaf = child;
  return res;
}

PRBool
nsHTMLEditor::IsTextInDirtyFrameVisible(nsIDOMNode *aNode)
{
  PRBool isEmptyTextNode;
  nsresult res = IsVisTextNode(aNode, &isEmptyTextNode, PR_FALSE);
  if (NS_FAILED(res))
  {
    
    

    return PR_TRUE;
  }

  return !isEmptyTextNode;
}





nsresult
nsHTMLEditor::IsVisTextNode( nsIDOMNode *aNode, 
                             PRBool *outIsEmptyNode, 
                             PRBool aSafeToAskFrames)
{
  NS_ENSURE_TRUE(aNode && outIsEmptyNode, NS_ERROR_NULL_POINTER);
  *outIsEmptyNode = PR_TRUE;
  nsresult res = NS_OK;

  nsCOMPtr<nsIContent> textContent = do_QueryInterface(aNode);
  
  if (!textContent || !textContent->IsNodeOfType(nsINode::eTEXT)) 
    return NS_ERROR_NULL_POINTER;
  PRUint32 length = textContent->TextLength();
  if (aSafeToAskFrames)
  {
    nsCOMPtr<nsISelectionController> selCon;
    res = GetSelectionController(getter_AddRefs(selCon));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
    PRBool isVisible = PR_FALSE;
    
    
    
    
    
    
    res = selCon->CheckVisibility(aNode, 0, length, &isVisible);
    NS_ENSURE_SUCCESS(res, res);
    if (isVisible) 
    {
      *outIsEmptyNode = PR_FALSE;
    }
  }
  else if (length)
  {
    if (textContent->TextIsOnlyWhitespace())
    {
      nsWSRunObject wsRunObj(this, aNode, 0);
      nsCOMPtr<nsIDOMNode> visNode;
      PRInt32 outVisOffset=0;
      PRInt16 visType=0;
      res = wsRunObj.NextVisibleNode(aNode, 0, address_of(visNode), &outVisOffset, &visType);
      NS_ENSURE_SUCCESS(res, res);
      if ( (visType == nsWSRunObject::eNormalWS) ||
           (visType == nsWSRunObject::eText) )
      {
        *outIsEmptyNode = (aNode != visNode);
      }
    }
    else
    {
      *outIsEmptyNode = PR_FALSE;
    }
  }
  return NS_OK;  
}
  






nsresult
nsHTMLEditor::IsEmptyNode( nsIDOMNode *aNode, 
                           PRBool *outIsEmptyNode, 
                           PRBool aSingleBRDoesntCount,
                           PRBool aListOrCellNotEmpty,
                           PRBool aSafeToAskFrames)
{
  NS_ENSURE_TRUE(aNode && outIsEmptyNode, NS_ERROR_NULL_POINTER);
  *outIsEmptyNode = PR_TRUE;
  PRBool seenBR = PR_FALSE;
  return IsEmptyNodeImpl(aNode, outIsEmptyNode, aSingleBRDoesntCount,
                         aListOrCellNotEmpty, aSafeToAskFrames, &seenBR);
}




nsresult
nsHTMLEditor::IsEmptyNodeImpl( nsIDOMNode *aNode, 
                               PRBool *outIsEmptyNode, 
                               PRBool aSingleBRDoesntCount,
                               PRBool aListOrCellNotEmpty,
                               PRBool aSafeToAskFrames,
                               PRBool *aSeenBR)
{
  NS_ENSURE_TRUE(aNode && outIsEmptyNode && aSeenBR, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;

  if (nsEditor::IsTextNode(aNode))
  {
    res = IsVisTextNode(aNode, outIsEmptyNode, aSafeToAskFrames);
    return res;
  }

  
  
  
  
  
  
  if (!IsContainer(aNode) || nsHTMLEditUtils::IsNamedAnchor(aNode) ||
        nsHTMLEditUtils::IsFormWidget(aNode)                       ||
       (aListOrCellNotEmpty && nsHTMLEditUtils::IsListItem(aNode)) ||
       (aListOrCellNotEmpty && nsHTMLEditUtils::IsTableCell(aNode)) ) 
  {
    *outIsEmptyNode = PR_FALSE;
    return NS_OK;
  }
    
  
  PRBool isListItemOrCell = 
       nsHTMLEditUtils::IsListItem(aNode) || nsHTMLEditUtils::IsTableCell(aNode);
       
  
  
  nsCOMPtr<nsIDOMNode> child;
  aNode->GetFirstChild(getter_AddRefs(child));
   
  while (child)
  {
    nsCOMPtr<nsIDOMNode> node = child;
    
    if (nsEditor::IsEditable(node))
    {
      if (nsEditor::IsTextNode(node))
      {
        res = IsVisTextNode(node, outIsEmptyNode, aSafeToAskFrames);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_TRUE(*outIsEmptyNode, NS_OK);  
      }
      else  
      {
        
        if (node == aNode) break;
        else if (aSingleBRDoesntCount && !*aSeenBR && nsTextEditUtils::IsBreak(node))
        {
          
          *aSeenBR = PR_TRUE;
        }
        else
        {
          
          
          
          if (isListItemOrCell)
          {
            if (nsHTMLEditUtils::IsList(node) || nsHTMLEditUtils::IsTable(node))
            { 
              *outIsEmptyNode = PR_FALSE;
              return NS_OK;
            }
          }
          
          else if (nsHTMLEditUtils::IsFormWidget(aNode))
          { 
            *outIsEmptyNode = PR_FALSE;
            return NS_OK;
          }
          
          PRBool isEmptyNode = PR_TRUE;
          res = IsEmptyNodeImpl(node, &isEmptyNode, aSingleBRDoesntCount, 
                                aListOrCellNotEmpty, aSafeToAskFrames, aSeenBR);
          NS_ENSURE_SUCCESS(res, res);
          if (!isEmptyNode) 
          { 
            
            *outIsEmptyNode = PR_FALSE;
            return NS_OK;
          }
        }
      }
    }
    node->GetNextSibling(getter_AddRefs(child));
  }
  
  return NS_OK;
}



nsresult
nsHTMLEditor::SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                       const nsAString & aAttribute,
                                       const nsAString & aValue,
                                       PRBool aSuppressTransaction)
{
  PRBool useCSS;
  nsresult res = NS_OK;
  GetIsCSSEnabled(&useCSS);
  if (useCSS && mHTMLCSSUtils) {
    PRInt32 count;
    res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(aElement, nsnull, &aAttribute, &aValue, &count,
                                                     aSuppressTransaction);
    NS_ENSURE_SUCCESS(res, res);
    if (count) {
      
      nsAutoString existingValue;
      PRBool wasSet = PR_FALSE;
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
        PRBool wasSet = PR_FALSE;
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
nsHTMLEditor::RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                          const nsAString & aAttribute,
                                          PRBool aSuppressTransaction)
{
  PRBool useCSS;
  nsresult res = NS_OK;
  GetIsCSSEnabled(&useCSS);
  if (useCSS && mHTMLCSSUtils) {
    res = mHTMLCSSUtils->RemoveCSSEquivalentToHTMLStyle(aElement, nsnull, &aAttribute, nsnull,
                                                        aSuppressTransaction);
    NS_ENSURE_SUCCESS(res, res);
  }

  nsAutoString existingValue;
  PRBool wasSet = PR_FALSE;
  res = GetAttributeValue(aElement, aAttribute, existingValue, &wasSet);
  NS_ENSURE_SUCCESS(res, res);
  if (wasSet) {
    if (aSuppressTransaction)
      res = aElement->RemoveAttribute(aAttribute);
    else
      res = RemoveAttribute(aElement, aAttribute);
  }
  return res;
}

nsresult
nsHTMLEditor::SetIsCSSEnabled(PRBool aIsCSSPrefChecked)
{
  nsresult  err = NS_ERROR_NOT_INITIALIZED;
  if (mHTMLCSSUtils)
  {
    err = mHTMLCSSUtils->SetCSSEnabled(aIsCSSPrefChecked);
  }
  
  if (NS_SUCCEEDED(err)) {
    PRUint32 flags = mFlags;
    if (aIsCSSPrefChecked) {
      
      flags &= ~eEditorNoCSSMask;
    } else {
      
      flags |= eEditorNoCSSMask;
    }

    err = SetFlags(flags);
    NS_ENSURE_SUCCESS(err, err);
  }
  return err;
}


NS_IMETHODIMP
nsHTMLEditor::SetCSSBackgroundColor(const nsAString& aColor)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  ForceCompositionEnd();

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsresult res;
  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);

  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertElement, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  
  PRBool cancel, handled;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kSetTextProperty);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

    
    enumerator->First(); 
    nsCOMPtr<nsISupports> currentItem;
    nsAutoString bgcolor; bgcolor.AssignLiteral("bgcolor");
    nsCOMPtr<nsIDOMNode> cachedBlockParent = nsnull;
    while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
    {
      res = enumerator->CurrentItem(getter_AddRefs(currentItem));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(currentItem, NS_ERROR_FAILURE);
      
      nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
      
      
      nsCOMPtr<nsIDOMNode> startNode, endNode;
      PRInt32 startOffset, endOffset;
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
        
        if (cachedBlockParent != blockParent)
        {
          cachedBlockParent = blockParent;
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
          PRInt32 count;
          res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nsnull, &bgcolor, &aColor, &count, PR_FALSE);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
      else if ((startNode == endNode) && nsTextEditUtils::IsBody(startNode) && isCollapsed)
      {
        
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(startNode);
        PRInt32 count;
        res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nsnull, &bgcolor, &aColor, &count, PR_FALSE);
        NS_ENSURE_SUCCESS(res, res);
      }
      else if ((startNode == endNode) && (((endOffset-startOffset) == 1) || (!startOffset && !endOffset)))
      {
        
        
        nsCOMPtr<nsIDOMNode> selectedNode = GetChildAt(startNode, startOffset);
        PRBool isBlock =PR_FALSE;
        res = NodeIsBlockStatic(selectedNode, &isBlock);
        NS_ENSURE_SUCCESS(res, res);
        nsCOMPtr<nsIDOMNode> blockParent = selectedNode;
        if (!isBlock) {
          blockParent = GetBlockNodeParent(selectedNode);
        }
        if (cachedBlockParent != blockParent)
        {
          cachedBlockParent = blockParent;
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
          PRInt32 count;
          res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nsnull, &bgcolor, &aColor, &count, PR_FALSE);
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
          if (cachedBlockParent != blockParent)
          {
            cachedBlockParent = blockParent;
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
            PRInt32 count;
            res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nsnull, &bgcolor, &aColor, &count, PR_FALSE);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
        
        
        PRInt32 listCount = arrayOfNodes.Count();
        PRInt32 j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          
          PRBool isBlock =PR_FALSE;
          res = NodeIsBlockStatic(node, &isBlock);
          NS_ENSURE_SUCCESS(res, res);
          nsCOMPtr<nsIDOMNode> blockParent = node;
          if (!isBlock) {
            
            blockParent = GetBlockNodeParent(node);
          }
          if (cachedBlockParent != blockParent)
          {
            cachedBlockParent = blockParent;
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
            PRInt32 count;
            
            res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nsnull, &bgcolor, &aColor, &count, PR_FALSE);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
        arrayOfNodes.Clear();
        
        
        
        
        if (IsTextNode(endNode) && IsEditable(endNode))
        {
          nsCOMPtr<nsIDOMNode> blockParent;
          blockParent = GetBlockNodeParent(endNode);
          if (cachedBlockParent != blockParent)
          {
            cachedBlockParent = blockParent;
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(blockParent);
            PRInt32 count;
            res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, nsnull, &bgcolor, &aColor, &count, PR_FALSE);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
      }
      enumerator->Next();
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
  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);
  if (useCSS) {
    
    
    
    res = SetCSSBackgroundColor(aColor);
  }
  else {
    
    res = SetHTMLBackgroundColor(aColor);
  }
  return res;
}




PRBool 
nsHTMLEditor::NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2)
{
  if (!aNode1 || !aNode2) 
  {
    NS_NOTREACHED("null node passed to nsEditor::NodesSameType()");
    return PR_FALSE;
  }

  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);

  nsIAtom *tag1 = GetTag(aNode1);

  if (tag1 == GetTag(aNode2)) {
    if (useCSS && tag1 == nsEditProperty::span) {
      if (mHTMLCSSUtils->ElementsSameStyle(aNode1, aNode2)) {
        return PR_TRUE;
      }
    }
    else {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsHTMLEditor::CopyLastEditableChildStyles(nsIDOMNode * aPreviousBlock, nsIDOMNode * aNewBlock,
                                          nsIDOMNode **aOutBrNode)
{
  *aOutBrNode = nsnull;
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
  nsCOMPtr<nsIDOMNode> newStyles = nsnull, deepestStyle = nsnull;
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
    
    *aOutBrNode = outBRNode;
    NS_ADDREF(*aOutBrNode);
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::GetElementOrigin(nsIDOMElement * aElement, PRInt32 & aX, PRInt32 & aY)
{
  aX = 0;
  aY = 0;

  NS_ENSURE_TRUE(mPresShellWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  nsIFrame *frame = content->GetPrimaryFrame();

  nsIFrame *container = ps->GetAbsoluteContainingBlock(frame);
  NS_ENSURE_TRUE(frame, NS_OK);
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
nsHTMLEditor::IgnoreSpuriousDragEvent(PRBool aIgnoreSpuriousDragEvent)
{
  mIgnoreSpuriousDragEvent = aIgnoreSpuriousDragEvent;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetSelectionContainer(nsIDOMElement ** aReturn)
{
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  
  if (NS_FAILED(res) || !selection) return res;

  PRBool bCollapsed;
  res = selection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> focusNode;

  if (bCollapsed) {
    res = selection->GetFocusNode(getter_AddRefs(focusNode));
    NS_ENSURE_SUCCESS(res, res);
  }
  else {

    PRInt32 rangeCount;
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
      PRInt32 startOffset, endOffset;
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
      PRInt32 i;
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
    PRUint16 nodeType;
    focusNode->GetNodeType(&nodeType);
    if (nsIDOMNode::TEXT_NODE == nodeType) {
      nsCOMPtr<nsIDOMNode> parent;
      res = focusNode->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(res, res);
      focusNode = parent;
    }
  }

  nsCOMPtr<nsIDOMElement> focusElement = do_QueryInterface(focusNode);
  *aReturn = focusElement;
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::IsAnonymousElement(nsIDOMElement * aElement, PRBool * aReturn)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  *aReturn = content->IsRootOfNativeAnonymousSubtree();
  return NS_OK;
}

nsresult
nsHTMLEditor::SetReturnInParagraphCreatesNewParagraph(PRBool aCreatesNewParagraph)
{
  mCRInParagraphCreatesParagraph = aCreatesNewParagraph;
  return NS_OK;
}

nsresult
nsHTMLEditor::GetReturnInParagraphCreatesNewParagraph(PRBool *aCreatesNewParagraph)
{
  *aCreatesNewParagraph = mCRInParagraphCreatesParagraph;
  return NS_OK;
}

PRBool
nsHTMLEditor::HasFocus()
{
  NS_ENSURE_TRUE(mDocWeak, PR_FALSE);

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, PR_FALSE);

  nsCOMPtr<nsIContent> focusedContent = fm->GetFocusedContent();

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  PRBool inDesignMode = doc->HasFlag(NODE_IS_EDITABLE);
  if (!focusedContent) {
    
    return inDesignMode ? OurWindowHasFocus() : PR_FALSE;
  }

  if (inDesignMode) {
    return OurWindowHasFocus() ?
      nsContentUtils::ContentIsDescendantOf(focusedContent, doc) : PR_FALSE;
  }

  

  
  
  if (!focusedContent->HasFlag(NODE_IS_EDITABLE) ||
      IsIndependentSelectionContent(focusedContent)) {
    return PR_FALSE;
  }
  
  return OurWindowHasFocus();
}

already_AddRefed<nsPIDOMEventTarget>
nsHTMLEditor::GetPIDOMEventTarget()
{
  
  
  
  NS_PRECONDITION(mDocWeak, "This editor has not been initialized yet");
  nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryReferent(mDocWeak.get());
  return piTarget.forget();
}

PRBool
nsHTMLEditor::ShouldReplaceRootElement()
{
  if (!mRootElement) {
    
    return PR_TRUE;
  }

  
  
  nsCOMPtr<nsIDOMHTMLElement> docBody;
  GetBodyElement(getter_AddRefs(docBody));
  return !SameCOMIdentity(docBody, mRootElement);
}

void
nsHTMLEditor::ResetRootElementAndEventTarget()
{
  
  
  
  RemoveEventListeners();
  mRootElement = nsnull;
  nsresult rv = InstallEventListeners();
  NS_ENSURE_SUCCESS(rv, );

  
  nsCOMPtr<nsIDOMElement> root;
  rv = GetRootElement(getter_AddRefs(root));
  NS_ENSURE_SUCCESS(rv, );
  NS_ENSURE_TRUE(mRootElement, );

  rv = BeginningOfDocument();
  NS_ENSURE_SUCCESS(rv, );

  
  
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
  nsCOMPtr<nsIDOMHTMLElement> bodyElement; 
  return htmlDoc->GetBody(aBody);
}

already_AddRefed<nsINode>
nsHTMLEditor::GetFocusedNode()
{
  if (!HasFocus()) {
    return nsnull;
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
  nsCOMPtr<nsINode> node = do_QueryInterface(doc);
  return node.forget();
}

PRBool
nsHTMLEditor::OurWindowHasFocus()
{
  NS_ENSURE_TRUE(mDocWeak, PR_FALSE);
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, PR_FALSE);
  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
  if (!focusedWindow) {
    return PR_FALSE;
  }
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  nsCOMPtr<nsIDOMWindow> ourWindow = do_QueryInterface(doc->GetWindow());
  return ourWindow == focusedWindow;
}

PRBool
nsHTMLEditor::IsIndependentSelectionContent(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "aContent must not be null");
  nsIFrame* frame = aContent->GetPrimaryFrame();
  return (frame && (frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION));
}

NS_IMETHODIMP
nsHTMLEditor::GetPreferredIMEState(PRUint32 *aState)
{
  if (IsReadonly() || IsDisabled()) {
    *aState = nsIContent::IME_STATUS_DISABLE;
    return NS_OK;
  }

  
  *aState = nsIContent::IME_STATUS_ENABLE;
  return NS_OK;
}
