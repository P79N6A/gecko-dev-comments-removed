






































#include "nsEditorEventListener.h"
#include "nsEditor.h"

#include "nsIDOMDOMStringList.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMEventTarget.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsIEditorMailSupport.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsILookAndFeel.h"
#include "nsFocusManager.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMEventGroup.h"


#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsIContent.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMNSRange.h"
#include "nsEditorUtils.h"
#include "nsIDOMEventTarget.h"
#include "nsIEventStateManager.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMDragEvent.h"
#include "nsIFocusManager.h"
#include "nsIDOMWindow.h"
#include "nsContentUtils.h"

nsEditorEventListener::nsEditorEventListener() :
  mEditor(nsnull), mCaretDrawn(PR_FALSE), mCommitText(PR_FALSE),
  mInTransaction(PR_FALSE)
{
}

nsEditorEventListener::~nsEditorEventListener() 
{
  if (mEditor) {
    NS_WARNING("We're not uninstalled");
    Disconnect();
  }
}

nsresult
nsEditorEventListener::Connect(nsEditor* aEditor)
{
  NS_ENSURE_ARG(aEditor);
  NS_ENSURE_TRUE(!mEditor, NS_ERROR_NOT_AVAILABLE);

  mEditor = aEditor;

  nsresult rv = InstallToEditor();
  if (NS_FAILED(rv)) {
    Disconnect();
  }
  return rv;
}

nsresult
nsEditorEventListener::InstallToEditor()
{
  NS_PRECONDITION(mEditor, "The caller must set mEditor");

  nsCOMPtr<nsPIDOMEventTarget> piTarget = mEditor->GetPIDOMEventTarget();
  NS_ENSURE_TRUE(piTarget, NS_ERROR_FAILURE);

  nsresult rv;

  
  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  piTarget->GetSystemEventGroup(getter_AddRefs(sysGroup));
  NS_ENSURE_STATE(sysGroup);
  nsIEventListenerManager* elmP = piTarget->GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elmP);

  rv = elmP->AddEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                    NS_LITERAL_STRING("keypress"),
                                    NS_EVENT_FLAG_BUBBLE |
                                    NS_PRIV_EVENT_UNTRUSTED_PERMITTED,
                                    sysGroup);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = elmP->AddEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                    NS_LITERAL_STRING("draggesture"),
                                    NS_EVENT_FLAG_BUBBLE, sysGroup);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = elmP->AddEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                    NS_LITERAL_STRING("dragenter"),
                                    NS_EVENT_FLAG_BUBBLE, sysGroup);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = elmP->AddEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                    NS_LITERAL_STRING("dragover"),
                                    NS_EVENT_FLAG_BUBBLE, sysGroup);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = elmP->AddEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                    NS_LITERAL_STRING("dragleave"),
                                    NS_EVENT_FLAG_BUBBLE, sysGroup);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = elmP->AddEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                    NS_LITERAL_STRING("drop"),
                                    NS_EVENT_FLAG_BUBBLE, sysGroup);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = piTarget->AddEventListenerByIID(static_cast<nsIDOMMouseListener*>(this),
                                       NS_GET_IID(nsIDOMMouseListener));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = elmP->AddEventListenerByIID(static_cast<nsIDOMFocusListener*>(this),
                                   NS_GET_IID(nsIDOMFocusListener),
                                   NS_EVENT_FLAG_CAPTURE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = piTarget->AddEventListenerByIID(static_cast<nsIDOMTextListener*>(this),
                                       NS_GET_IID(nsIDOMTextListener));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = piTarget->AddEventListenerByIID(
    static_cast<nsIDOMCompositionListener*>(this),
    NS_GET_IID(nsIDOMCompositionListener));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsEditorEventListener::Disconnect()
{
  if (!mEditor) {
    return;
  }
  UninstallFromEditor();
  mEditor = nsnull;
}

void
nsEditorEventListener::UninstallFromEditor()
{
  nsCOMPtr<nsPIDOMEventTarget> piTarget = mEditor->GetPIDOMEventTarget();
  if (!piTarget) {
    return;
  }

  nsCOMPtr<nsIEventListenerManager> elmP =
    piTarget->GetListenerManager(PR_TRUE);
  if (!elmP) {
    return;
  }
  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  piTarget->GetSystemEventGroup(getter_AddRefs(sysGroup));
  if (!sysGroup) {
    return;
  }

  elmP->RemoveEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                  NS_LITERAL_STRING("keypress"),
                                  NS_EVENT_FLAG_BUBBLE, sysGroup);
  elmP->RemoveEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                  NS_LITERAL_STRING("draggesture"),
                                  NS_EVENT_FLAG_BUBBLE, sysGroup);
  elmP->RemoveEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                  NS_LITERAL_STRING("dragenter"),
                                  NS_EVENT_FLAG_BUBBLE, sysGroup);
  elmP->RemoveEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                  NS_LITERAL_STRING("dragover"),
                                  NS_EVENT_FLAG_BUBBLE, sysGroup);
  elmP->RemoveEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                  NS_LITERAL_STRING("dragleave"),
                                  NS_EVENT_FLAG_BUBBLE, sysGroup);
  elmP->RemoveEventListenerByType(static_cast<nsIDOMKeyListener*>(this),
                                  NS_LITERAL_STRING("drop"),
                                  NS_EVENT_FLAG_BUBBLE, sysGroup);

  piTarget->RemoveEventListenerByIID(static_cast<nsIDOMMouseListener*>(this),
                                     NS_GET_IID(nsIDOMMouseListener));

  elmP->RemoveEventListenerByIID(static_cast<nsIDOMFocusListener*>(this),
                                 NS_GET_IID(nsIDOMFocusListener),
                                 NS_EVENT_FLAG_CAPTURE);

  piTarget->RemoveEventListenerByIID(static_cast<nsIDOMTextListener*>(this),
                                     NS_GET_IID(nsIDOMTextListener));

  piTarget->RemoveEventListenerByIID(
    static_cast<nsIDOMCompositionListener*>(this),
    NS_GET_IID(nsIDOMCompositionListener));
}

already_AddRefed<nsIPresShell>
nsEditorEventListener::GetPresShell()
{
  NS_PRECONDITION(mEditor,
    "The caller must check whether this is connected to an editor");
  nsCOMPtr<nsIPresShell> ps;
  mEditor->GetPresShell(getter_AddRefs(ps));
  return ps.forget();
}





NS_IMPL_ADDREF(nsEditorEventListener)
NS_IMPL_RELEASE(nsEditorEventListener)

NS_INTERFACE_MAP_BEGIN(nsEditorEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTextListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCompositionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsEditorEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
  if (dragEvent) {
    nsAutoString eventType;
    aEvent->GetType(eventType);
    if (eventType.EqualsLiteral("draggesture"))
      return DragGesture(dragEvent);
    if (eventType.EqualsLiteral("dragenter"))
      return DragEnter(dragEvent);
    if (eventType.EqualsLiteral("dragover"))
      return DragOver(dragEvent);
    if (eventType.EqualsLiteral("dragleave"))
      return DragLeave(dragEvent);
    if (eventType.EqualsLiteral("drop"))
      return Drop(dragEvent);
  }
  return NS_OK;
}





NS_IMETHODIMP
nsEditorEventListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  
  
  
  
  

  nsCOMPtr<nsIDOMNSUIEvent> UIEvent = do_QueryInterface(aKeyEvent);
  if(UIEvent) {
    PRBool defaultPrevented;
    UIEvent->GetPreventDefault(&defaultPrevented);
    if(defaultPrevented) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMKeyEvent>keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) {
    
    return NS_OK;
  }

  return mEditor->HandleKeyPressEvent(keyEvent);
}





NS_IMETHODIMP
nsEditorEventListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  nsCOMPtr<nsIDOMNSEvent> nsevent = do_QueryInterface(aMouseEvent);
  PRBool isTrusted = PR_FALSE;
  if (!mouseEvent || !nsevent ||
      NS_FAILED(nsevent->GetIsTrusted(&isTrusted)) || !isTrusted) {
    
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aMouseEvent);
  if (!nsuiEvent)
    return NS_ERROR_NULL_POINTER;

  PRBool preventDefault;
  rv = nsuiEvent->GetPreventDefault(&preventDefault);
  if (NS_FAILED(rv) || preventDefault)
  {
    
    return rv;
  }

  
  
  mEditor->ForceCompositionEnd();

  PRUint16 button = (PRUint16)-1;
  mouseEvent->GetButton(&button);
  
  if (button == 1)
  {
    nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && prefBranch)
    {
      PRBool doMiddleMousePaste = PR_FALSE;;
      rv = prefBranch->GetBoolPref("middlemouse.paste", &doMiddleMousePaste);
      if (NS_SUCCEEDED(rv) && doMiddleMousePaste)
      {
        
        nsCOMPtr<nsIDOMNode> parent;
        if (NS_FAILED(nsuiEvent->GetRangeParent(getter_AddRefs(parent))))
          return NS_ERROR_NULL_POINTER;
        PRInt32 offset = 0;
        if (NS_FAILED(nsuiEvent->GetRangeOffset(&offset)))
          return NS_ERROR_NULL_POINTER;

        nsCOMPtr<nsISelection> selection;
        if (NS_SUCCEEDED(mEditor->GetSelection(getter_AddRefs(selection))))
          (void)selection->Collapse(parent, offset);

        
        
        PRBool ctrlKey = PR_FALSE;
        mouseEvent->GetCtrlKey(&ctrlKey);

        nsCOMPtr<nsIEditorMailSupport> mailEditor;
        if (ctrlKey)
          mailEditor = do_QueryInterface(static_cast<nsIEditor*>(mEditor));

        PRInt32 clipboard;

#if defined(XP_OS2) || defined(XP_WIN32)
        clipboard = nsIClipboard::kGlobalClipboard;
#else
        clipboard = nsIClipboard::kSelectionClipboard;
#endif

        if (mailEditor)
          mailEditor->PasteAsQuotation(clipboard);
        else
          mEditor->Paste(clipboard);

        
        
        mouseEvent->StopPropagation();
        mouseEvent->PreventDefault();

        
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  mEditor->ForceCompositionEnd();
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  
  
  return NS_OK;
}





NS_IMETHODIMP
nsEditorEventListener::HandleText(nsIDOMEvent* aTextEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIPrivateTextEvent> textEvent = do_QueryInterface(aTextEvent);
  if (!textEvent) {
     
     return NS_OK;
  }

  nsAutoString                      composedText;
  nsCOMPtr<nsIPrivateTextRangeList> textRangeList;

  textEvent->GetText(composedText);
  textRangeList = textEvent->GetInputRange();

  
  if (mEditor->IsReadonly() || mEditor->IsDisabled()) {
    return NS_OK;
  }

  return mEditor->SetCompositionString(composedText, textRangeList);
}





nsresult
nsEditorEventListener::DragGesture(nsIDOMDragEvent* aDragEvent)
{
  
  PRBool canDrag;
  nsresult rv = mEditor->CanDrag(aDragEvent, &canDrag);
  if ( NS_SUCCEEDED(rv) && canDrag )
    rv = mEditor->DoDrag(aDragEvent);

  return rv;
}

nsresult
nsEditorEventListener::DragEnter(nsIDOMDragEvent* aDragEvent)
{
  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!presShell)
    return NS_OK;

  if (!mCaret)
  {
    NS_NewCaret(getter_AddRefs(mCaret));
    if (mCaret)
    {
      mCaret->Init(presShell);
      mCaret->SetCaretReadOnly(PR_TRUE);
    }
    mCaretDrawn = PR_FALSE;
  }

  presShell->SetCaret(mCaret);

  return DragOver(aDragEvent);
}

nsresult
nsEditorEventListener::DragOver(nsIDOMDragEvent* aDragEvent)
{
  nsCOMPtr<nsIDOMNode> parent;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aDragEvent);
  if (nsuiEvent) {
    PRBool defaultPrevented;
    nsuiEvent->GetPreventDefault(&defaultPrevented);
    if (defaultPrevented)
      return NS_OK;

    nsuiEvent->GetRangeParent(getter_AddRefs(parent));
    nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
    if (!dropParent)
      return NS_ERROR_FAILURE;

    if (!dropParent->IsEditable())
      return NS_OK;
  }

  PRBool canDrop = CanDrop(aDragEvent);
  if (canDrop)
  {
    aDragEvent->PreventDefault(); 

    if (mCaret && nsuiEvent)
    {
      PRInt32 offset = 0;
      nsresult rv = nsuiEvent->GetRangeOffset(&offset);
      if (NS_FAILED(rv)) return rv;

      
      if (mCaretDrawn)
        mCaret->EraseCaret();
      
      
      mCaret->DrawAtPosition(parent, offset);
      mCaretDrawn = PR_TRUE;
    }
  }
  else
  {
    if (mCaret && mCaretDrawn)
    {
      mCaret->EraseCaret();
      mCaretDrawn = PR_FALSE;
    } 
  }

  return NS_OK;
}

nsresult
nsEditorEventListener::DragLeave(nsIDOMDragEvent* aDragEvent)
{
  if (mCaret && mCaretDrawn)
  {
    mCaret->EraseCaret();
    mCaretDrawn = PR_FALSE;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (presShell)
    presShell->RestoreCaret();

  return NS_OK;
}

nsresult
nsEditorEventListener::Drop(nsIDOMDragEvent* aMouseEvent)
{
  if (mCaret)
  {
    if (mCaretDrawn)
    {
      mCaret->EraseCaret();
      mCaretDrawn = PR_FALSE;
    }
    mCaret->SetCaretVisible(PR_FALSE);    

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell)
    {
      presShell->RestoreCaret();
    }
  }

  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aMouseEvent);
  if (nsuiEvent) {
    PRBool defaultPrevented;
    nsuiEvent->GetPreventDefault(&defaultPrevented);
    if (defaultPrevented)
      return NS_OK;

    nsCOMPtr<nsIDOMNode> parent;
    nsuiEvent->GetRangeParent(getter_AddRefs(parent));
    nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
    if (!dropParent)
      return NS_ERROR_FAILURE;

    if (!dropParent->IsEditable())
      return NS_OK;
  }

  PRBool canDrop = CanDrop(aMouseEvent);
  if (!canDrop)
  {
    
    if (mEditor->IsReadonly() || mEditor->IsDisabled())
    {
      
      
      
      
      return aMouseEvent->StopPropagation();
    }
    return NS_OK;
  }

  aMouseEvent->StopPropagation();
  aMouseEvent->PreventDefault();
  
  
  return mEditor->InsertFromDrop(aMouseEvent);
}

PRBool
nsEditorEventListener::CanDrop(nsIDOMDragEvent* aEvent)
{
  
  if (mEditor->IsReadonly() || mEditor->IsDisabled()) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  aEvent->GetDataTransfer(getter_AddRefs(dataTransfer));
  if (!dataTransfer)
    return PR_FALSE;

  nsCOMPtr<nsIDOMDOMStringList> types;
  dataTransfer->GetTypes(getter_AddRefs(types));
  if (!types)
    return PR_FALSE;

  
  
  PRBool typeSupported;
  types->Contains(NS_LITERAL_STRING(kTextMime), &typeSupported);
  if (!typeSupported) {
    types->Contains(NS_LITERAL_STRING(kMozTextInternal), &typeSupported);
    if (!typeSupported && !mEditor->IsPlaintextEditor()) {
      types->Contains(NS_LITERAL_STRING(kHTMLMime), &typeSupported);
      if (!typeSupported) {
        types->Contains(NS_LITERAL_STRING(kFileMime), &typeSupported);
      }
    }
  }

  if (!typeSupported)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNSDataTransfer> dataTransferNS(do_QueryInterface(dataTransfer));
  if (!dataTransferNS)
    return PR_FALSE;

  
  
  
  nsCOMPtr<nsIDOMNode> sourceNode;
  dataTransferNS->GetMozSourceNode(getter_AddRefs(sourceNode));
  if (!sourceNode)
    return PR_TRUE;

  
  

  nsCOMPtr<nsIDOMDocument> domdoc;
  nsresult rv = mEditor->GetDocument(getter_AddRefs(domdoc));
  if (NS_FAILED(rv)) return PR_FALSE;

  nsCOMPtr<nsIDOMDocument> sourceDoc;
  rv = sourceNode->GetOwnerDocument(getter_AddRefs(sourceDoc));
  if (NS_FAILED(rv)) return PR_FALSE;
  if (domdoc == sourceDoc)      
  {
    nsCOMPtr<nsISelection> selection;
    rv = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(rv) || !selection)
      return PR_FALSE;
    
    PRBool isCollapsed;
    rv = selection->GetIsCollapsed(&isCollapsed);
    if (NS_FAILED(rv)) return PR_FALSE;
  
    
    if (!isCollapsed)
    {
      nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aEvent));
      if (!nsuiEvent) return PR_FALSE;

      nsCOMPtr<nsIDOMNode> parent;
      rv = nsuiEvent->GetRangeParent(getter_AddRefs(parent));
      if (NS_FAILED(rv) || !parent) return PR_FALSE;

      PRInt32 offset = 0;
      rv = nsuiEvent->GetRangeOffset(&offset);
      if (NS_FAILED(rv)) return PR_FALSE;

      PRInt32 rangeCount;
      rv = selection->GetRangeCount(&rangeCount);
      if (NS_FAILED(rv)) return PR_FALSE;

      for (PRInt32 i = 0; i < rangeCount; i++)
      {
        nsCOMPtr<nsIDOMRange> range;
        rv = selection->GetRangeAt(i, getter_AddRefs(range));
        nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
        if (NS_FAILED(rv) || !nsrange) 
          continue; 

        PRBool inRange = PR_TRUE;
        (void)nsrange->IsPointInRange(parent, offset, &inRange);
        if (inRange)
          return PR_FALSE;  
      }
    }
  }
  
  return PR_TRUE;
}





NS_IMETHODIMP
nsEditorEventListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  return mEditor->BeginComposition();
}

NS_IMETHODIMP
nsEditorEventListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  return mEditor->EndComposition();
}





static already_AddRefed<nsIContent>
FindSelectionRoot(nsEditor *aEditor, nsIContent *aContent)
{
  NS_PRECONDITION(aEditor, "aEditor must not be null");
  nsIDocument *document = aContent->GetCurrentDoc();
  if (!document) {
    return nsnull;
  }

  nsIContent *root;
  if (document->HasFlag(NODE_IS_EDITABLE)) {
    NS_IF_ADDREF(root = document->GetRootElement());

    return root;
  }

  
  
  
  if (aEditor->IsReadonly()) {
    
    nsCOMPtr<nsIDOMElement> rootElement;
    aEditor->GetRootElement(getter_AddRefs(rootElement));
    if (!rootElement) {
      return nsnull;
    }

    CallQueryInterface(rootElement, &root);

    return root;
  }

  if (!aContent->HasFlag(NODE_IS_EDITABLE)) {
    return nsnull;
  }

  
  
  
  
  
  
  
  nsIContent *parent, *content = aContent;
  while ((parent = content->GetParent()) && parent->HasFlag(NODE_IS_EDITABLE)) {
    content = parent;
  }

  NS_IF_ADDREF(content);

  return content;
}

NS_IMETHODIMP
nsEditorEventListener::Focus(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_ARG(aEvent);

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  
  if (mEditor->IsDisabled()) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(target);

  PRBool targetIsEditableDoc = PR_FALSE;
  nsCOMPtr<nsIContent> editableRoot;
  if (content) {
    
    
    
    
    
    
    editableRoot = FindSelectionRoot(mEditor, content);

    
    
    if (editableRoot) {
      nsIFocusManager* fm = nsFocusManager::GetFocusManager();
      NS_ENSURE_TRUE(fm, NS_OK);

      nsCOMPtr<nsIDOMElement> element;
      fm->GetFocusedElement(getter_AddRefs(element));
      if (!SameCOMIdentity(element, target))
        return NS_OK;
    }
  }
  else {
    nsCOMPtr<nsIDocument> document = do_QueryInterface(target);
    targetIsEditableDoc = document && document->HasFlag(NODE_IS_EDITABLE);
  }

  nsCOMPtr<nsISelectionController> selCon;
  mEditor->GetSelectionController(getter_AddRefs(selCon));
  if (selCon && (targetIsEditableDoc || editableRoot))
  {
    nsCOMPtr<nsISelection> selection;
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                         getter_AddRefs(selection));

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell) {
      nsRefPtr<nsCaret> caret = presShell->GetCaret();
      if (caret) {
        caret->SetIgnoreUserModify(PR_FALSE);
        if (selection) {
          caret->SetCaretDOMSelection(selection);
        }
      }
    }

    selCon->SetCaretReadOnly(mEditor->IsReadonly());
    selCon->SetCaretEnabled(PR_TRUE);
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
    selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);

    nsCOMPtr<nsISelectionPrivate> selectionPrivate =
      do_QueryInterface(selection);
    if (selectionPrivate)
    {
      selectionPrivate->SetAncestorLimiter(editableRoot);
    }

    if (selection && !editableRoot) {
      PRInt32 rangeCount;
      selection->GetRangeCount(&rangeCount);
      if (rangeCount == 0) {
        mEditor->BeginningOfDocument();
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::Blur(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_ARG(aEvent);

  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, NS_OK);

  nsCOMPtr<nsIDOMElement> element;
  fm->GetFocusedElement(getter_AddRefs(element));
  if (element)
    return NS_OK;

  
  nsCOMPtr<nsISelectionController>selCon;
  mEditor->GetSelectionController(getter_AddRefs(selCon));
  if (selCon)
  {
    nsCOMPtr<nsISelection> selection;
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                         getter_AddRefs(selection));

    nsCOMPtr<nsISelectionPrivate> selectionPrivate =
      do_QueryInterface(selection);
    if (selectionPrivate) {
      selectionPrivate->SetAncestorLimiter(nsnull);
    }

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell) {
      nsRefPtr<nsCaret> caret = presShell->GetCaret();
      if (caret) {
        caret->SetIgnoreUserModify(PR_TRUE);
      }
    }

    selCon->SetCaretEnabled(PR_FALSE);

    if(mEditor->IsFormWidget() || mEditor->IsPasswordEditor() ||
       mEditor->IsReadonly() || mEditor->IsDisabled() ||
       mEditor->IsInputFiltered())
    {
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);
    }
    else
    {
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
    }

    selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  }

  return NS_OK;
}

