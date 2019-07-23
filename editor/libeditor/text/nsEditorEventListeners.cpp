





































#include "nsEditorEventListeners.h"
#include "nsEditor.h"

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsIPrivateCompositionEvent.h"
#include "nsIEditorMailSupport.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsILookAndFeel.h"
#include "nsPresContext.h"
#include "nsFocusManager.h"


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







NS_IMPL_ISUPPORTS2(nsTextEditorKeyListener, nsIDOMEventListener, nsIDOMKeyListener)


nsTextEditorKeyListener::nsTextEditorKeyListener()
{
}



nsTextEditorKeyListener::~nsTextEditorKeyListener() 
{
}


nsresult
nsTextEditorKeyListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}




nsresult
nsTextEditorKeyListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorKeyListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorKeyListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  
  
  
  
  

  nsCOMPtr<nsIDOMNSUIEvent> nsUIEvent = do_QueryInterface(aKeyEvent);
  if(nsUIEvent) 
  {
    PRBool defaultPrevented;
    nsUIEvent->GetPreventDefault(&defaultPrevented);
    if(defaultPrevented)
      return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent>keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) 
  {
    
    return NS_OK;
  }

  
  
  

  PRUint32 keyCode;
  keyEvent->GetKeyCode(&keyCode);

  
  PRUint32 flags;
  if (NS_SUCCEEDED(mEditor->GetFlags(&flags)))
  {
    if (flags & nsIPlaintextEditor::eEditorReadonlyMask || 
        flags & nsIPlaintextEditor::eEditorDisabledMask) 
    {
      
      
      if (keyCode == nsIDOMKeyEvent::DOM_VK_BACK_SPACE)
        aKeyEvent->PreventDefault();

      return NS_OK;
    }
  }
  else
    return NS_ERROR_FAILURE;  

  nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(mEditor));
  if (!textEditor) return NS_ERROR_NO_INTERFACE;

  
  
  if (0 != keyCode)
  {
    PRBool isAnyModifierKeyButShift;
    nsresult rv;
    rv = keyEvent->GetAltKey(&isAnyModifierKeyButShift);
    if (NS_FAILED(rv)) return rv;
    
    if (!isAnyModifierKeyButShift)
    {
      rv = keyEvent->GetMetaKey(&isAnyModifierKeyButShift);
      if (NS_FAILED(rv)) return rv;
      
      if (!isAnyModifierKeyButShift)
      {
        rv = keyEvent->GetCtrlKey(&isAnyModifierKeyButShift);
        if (NS_FAILED(rv)) return rv;
      }
    }

    switch (keyCode)
    {
      case nsIDOMKeyEvent::DOM_VK_META:
      case nsIDOMKeyEvent::DOM_VK_SHIFT:
      case nsIDOMKeyEvent::DOM_VK_CONTROL:
      case nsIDOMKeyEvent::DOM_VK_ALT:
        aKeyEvent->PreventDefault(); 
        return NS_OK;
        break;

      case nsIDOMKeyEvent::DOM_VK_BACK_SPACE: 
        if (isAnyModifierKeyButShift)
          return NS_OK;

        mEditor->DeleteSelection(nsIEditor::ePrevious);
        aKeyEvent->PreventDefault(); 
        return NS_OK;
        break;
 
      case nsIDOMKeyEvent::DOM_VK_DELETE:
        


        PRBool isShiftModifierKey;
        rv = keyEvent->GetShiftKey(&isShiftModifierKey);
        if (NS_FAILED(rv)) return rv;

        if (isAnyModifierKeyButShift || isShiftModifierKey)
           return NS_OK;
        mEditor->DeleteSelection(nsIEditor::eNext);
        aKeyEvent->PreventDefault(); 
        return NS_OK; 
        break;
 
      case nsIDOMKeyEvent::DOM_VK_TAB:
        if ((flags & nsIPlaintextEditor::eEditorSingleLineMask) ||
            (flags & nsIPlaintextEditor::eEditorPasswordMask)   ||
            (flags & nsIPlaintextEditor::eEditorWidgetMask)     ||
            (flags & nsIPlaintextEditor::eEditorAllowInteraction))
          return NS_OK; 

        if (isAnyModifierKeyButShift)
          return NS_OK;

        
        textEditor->HandleKeyPress(keyEvent);
        
        return NS_OK; 

      case nsIDOMKeyEvent::DOM_VK_RETURN:
      case nsIDOMKeyEvent::DOM_VK_ENTER:
        if (isAnyModifierKeyButShift)
          return NS_OK;

        if (!(flags & nsIPlaintextEditor::eEditorSingleLineMask))
        {
          textEditor->HandleKeyPress(keyEvent);
          aKeyEvent->PreventDefault(); 
        }
        return NS_OK;
    }
  }

  textEditor->HandleKeyPress(keyEvent);
  return NS_OK; 
}






NS_IMPL_ISUPPORTS2(nsTextEditorMouseListener, nsIDOMEventListener, nsIDOMMouseListener)


nsTextEditorMouseListener::nsTextEditorMouseListener() 
{
}



nsTextEditorMouseListener::~nsTextEditorMouseListener() 
{
}


nsresult
nsTextEditorMouseListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsTextEditorMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
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

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) { return NS_OK; }

  
  
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  if (imeEditor)
    imeEditor->ForceCompositionEnd();

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
        if (NS_SUCCEEDED(editor->GetSelection(getter_AddRefs(selection))))
          (void)selection->Collapse(parent, offset);

        
        
        PRBool ctrlKey = PR_FALSE;
        mouseEvent->GetCtrlKey(&ctrlKey);

        nsCOMPtr<nsIEditorMailSupport> mailEditor;
        if (ctrlKey)
          mailEditor = do_QueryInterface(mEditor);

        PRInt32 clipboard;

#if defined(XP_OS2) || defined(XP_WIN32)
        clipboard = nsIClipboard::kGlobalClipboard;
#else
        clipboard = nsIClipboard::kSelectionClipboard;
#endif

        if (mailEditor)
          mailEditor->PasteAsQuotation(clipboard);
        else
          editor->Paste(clipboard);

        
        
        mouseEvent->StopPropagation();
        mouseEvent->PreventDefault();

        
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

nsresult
nsTextEditorMouseListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  if (!imeEditor)
    return NS_OK;
  imeEditor->ForceCompositionEnd();
  return NS_OK;
}

nsresult
nsTextEditorMouseListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorMouseListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}






NS_IMPL_ISUPPORTS2(nsTextEditorTextListener, nsIDOMEventListener, nsIDOMTextListener)


nsTextEditorTextListener::nsTextEditorTextListener()
:   mCommitText(PR_FALSE),
   mInTransaction(PR_FALSE)
{
}


nsTextEditorTextListener::~nsTextEditorTextListener() 
{
}

nsresult
nsTextEditorTextListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorTextListener::HandleEvent\n");
#endif
  return NS_OK;
}



nsresult
nsTextEditorTextListener::HandleText(nsIDOMEvent* aTextEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorTextListener::HandleText\n");
#endif
   nsCOMPtr<nsIPrivateTextEvent> textEvent = do_QueryInterface(aTextEvent);
   if (!textEvent) {
      
      return NS_OK;
   }

   nsAutoString                      composedText;
   nsresult                          result;
   nsCOMPtr<nsIPrivateTextRangeList> textRangeList;
   nsTextEventReply*                 textEventReply;

   textEvent->GetText(composedText);
   textRangeList = textEvent->GetInputRange();
   textEventReply = textEvent->GetEventReply();
   nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor, &result);
   if (imeEditor) {
     PRUint32 flags;
     
     if (NS_SUCCEEDED(mEditor->GetFlags(&flags))) {
       if (flags & nsIPlaintextEditor::eEditorReadonlyMask || 
           flags & nsIPlaintextEditor::eEditorDisabledMask) {
#if DEBUG_IME
         printf("nsTextEditorTextListener::HandleText,  Readonly or Disabled\n");
#endif
         return NS_OK;
       }
     }
     result = imeEditor->SetCompositionString(composedText,textRangeList,textEventReply);
   }
   return result;
}





nsTextEditorDragListener::nsTextEditorDragListener() 
: mEditor(nsnull)
, mPresShell(nsnull)
, mCaretDrawn(PR_FALSE)
{
}

nsTextEditorDragListener::~nsTextEditorDragListener() 
{
}

NS_IMPL_ISUPPORTS1(nsTextEditorDragListener, nsIDOMEventListener)

nsresult
nsTextEditorDragListener::HandleEvent(nsIDOMEvent* aEvent)
{
  
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


nsresult
nsTextEditorDragListener::DragGesture(nsIDOMDragEvent* aDragEvent)
{
  if ( !mEditor )
    return NS_ERROR_NULL_POINTER;

  
  PRBool canDrag;
  nsresult rv = mEditor->CanDrag(aDragEvent, &canDrag);
  if ( NS_SUCCEEDED(rv) && canDrag )
    rv = mEditor->DoDrag(aDragEvent);

  return rv;
}


nsresult
nsTextEditorDragListener::DragEnter(nsIDOMDragEvent* aDragEvent)
{
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
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
nsTextEditorDragListener::DragOver(nsIDOMDragEvent* aDragEvent)
{
  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (!dragService) return rv;

  
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parent;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aDragEvent);
  if (nsuiEvent) {
    nsuiEvent->GetRangeParent(getter_AddRefs(parent));
    nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
    if (!dropParent)
      return NS_ERROR_FAILURE;

    if (!dropParent->IsEditable())
      return NS_OK;
  }

  PRBool canDrop = CanDrop(aDragEvent);
  dragSession->SetCanDrop(canDrop);

  if (canDrop)
  {
    
    
    aDragEvent->PreventDefault(); 

    if (mCaret && nsuiEvent)
    {
      PRInt32 offset = 0;
      rv = nsuiEvent->GetRangeOffset(&offset);
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
nsTextEditorDragListener::DragLeave(nsIDOMDragEvent* aDragEvent)
{
  if (mCaret && mCaretDrawn)
  {
    mCaret->EraseCaret();
    mCaretDrawn = PR_FALSE;
  }

  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
  if (presShell)
    presShell->RestoreCaret();

  return NS_OK;
}



nsresult
nsTextEditorDragListener::Drop(nsIDOMDragEvent* aMouseEvent)
{
  if (mCaret)
  {
    if (mCaretDrawn)
    {
      mCaret->EraseCaret();
      mCaretDrawn = PR_FALSE;
    }
    mCaret->SetCaretVisible(PR_FALSE);    

    nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
    if (presShell)
    {
      presShell->RestoreCaret();
    }
  }

  if (!mEditor)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aMouseEvent);
  if (nsuiEvent) {
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
    

    PRUint32 flags;
    if (NS_SUCCEEDED(mEditor->GetFlags(&flags))
        && ((flags & nsIPlaintextEditor::eEditorDisabledMask) ||
            (flags & nsIPlaintextEditor::eEditorReadonlyMask)) )
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
nsTextEditorDragListener::CanDrop(nsIDOMDragEvent* aEvent)
{
  
  PRUint32 flags;
  if (NS_FAILED(mEditor->GetFlags(&flags)))
    return PR_FALSE;

  if ((flags & nsIPlaintextEditor::eEditorDisabledMask) || 
      (flags & nsIPlaintextEditor::eEditorReadonlyMask)) {
    return PR_FALSE;
  }

  
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = do_GetService("@mozilla.org/widget/dragservice;1", &rv);

  
  nsCOMPtr<nsIDragSession> dragSession;
  if (dragService)
    dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return PR_FALSE;

  PRBool flavorSupported = PR_FALSE;
  dragSession->IsDataFlavorSupported(kUnicodeMime, &flavorSupported);

  if (!flavorSupported)
    dragSession->IsDataFlavorSupported(kMozTextInternal, &flavorSupported);

  
  if (!flavorSupported 
     && (flags & nsIPlaintextEditor::eEditorPlaintextMask) == 0)
  {
    dragSession->IsDataFlavorSupported(kHTMLMime, &flavorSupported);
    if (!flavorSupported)
      dragSession->IsDataFlavorSupported(kFileMime, &flavorSupported);
#if 0
    if (!flavorSupported)
      dragSession->IsDataFlavorSupported(kJPEGImageMime, &flavorSupported);
#endif
  }

  if (!flavorSupported)
    return PR_FALSE;     

  nsCOMPtr<nsIDOMDocument> domdoc;
  rv = mEditor->GetDocument(getter_AddRefs(domdoc));
  if (NS_FAILED(rv)) return PR_FALSE;

  nsCOMPtr<nsIDOMDocument> sourceDoc;
  rv = dragSession->GetSourceDocument(getter_AddRefs(sourceDoc));
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


nsTextEditorCompositionListener::nsTextEditorCompositionListener()
{
}

nsTextEditorCompositionListener::~nsTextEditorCompositionListener() 
{
}

NS_IMPL_ISUPPORTS2(nsTextEditorCompositionListener, nsIDOMEventListener, nsIDOMCompositionListener)

nsresult
nsTextEditorCompositionListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleEvent\n");
#endif
  return NS_OK;
}

void nsTextEditorCompositionListener::SetEditor(nsIEditor *aEditor)
{
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(aEditor);
  if (!imeEditor) return;      
  
  
  mEditor = imeEditor;
}

nsresult
nsTextEditorCompositionListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleStartComposition\n");
#endif
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aCompositionEvent);
  if (!pCompositionEvent) return NS_ERROR_FAILURE;
  
  nsTextEventReply* eventReply;
  nsresult rv = pCompositionEvent->GetCompositionReply(&eventReply);
  if (NS_FAILED(rv)) return rv;

  return mEditor->BeginComposition(eventReply);
}

nsresult
nsTextEditorCompositionListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleEndComposition\n");
#endif
   return mEditor->EndComposition();
}







nsresult 
NS_NewEditorKeyListener(nsIDOMEventListener ** aInstancePtrResult, 
                        nsIEditor *aEditor)
{
  nsTextEditorKeyListener* it = new nsTextEditorKeyListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}



nsresult
NS_NewEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor)
{
  nsTextEditorMouseListener* it = new nsTextEditorMouseListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}


nsresult
NS_NewEditorTextListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor* aEditor)
{
   nsTextEditorTextListener*   it = new nsTextEditorTextListener();
   if (nsnull==it) {
      return NS_ERROR_OUT_OF_MEMORY;
   }

   it->SetEditor(aEditor);

   return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}



nsresult
NS_NewEditorDragListener(nsIDOMEventListener ** aInstancePtrResult, nsIPresShell* aPresShell,
                          nsIEditor *aEditor)
{
  nsTextEditorDragListener* it = new nsTextEditorDragListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);
  it->SetPresShell(aPresShell);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}

nsresult
NS_NewEditorCompositionListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor* aEditor)
{
   nsTextEditorCompositionListener*   it = new nsTextEditorCompositionListener();
   if (nsnull==it) {
      return NS_ERROR_OUT_OF_MEMORY;
   }
   it->SetEditor(aEditor);
  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}

nsresult 
NS_NewEditorFocusListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor,
                          nsIPresShell *aPresShell)
{
  nsTextEditorFocusListener* it =
    new nsTextEditorFocusListener(aEditor, aPresShell);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}







NS_IMPL_ISUPPORTS2(nsTextEditorFocusListener, nsIDOMEventListener, nsIDOMFocusListener)


nsTextEditorFocusListener::nsTextEditorFocusListener(nsIEditor *aEditor,
                                                     nsIPresShell *aShell) 
  : mEditor(aEditor),
    mPresShell(do_GetWeakReference(aShell))
{
}

nsTextEditorFocusListener::~nsTextEditorFocusListener() 
{
}

nsresult
nsTextEditorFocusListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

static already_AddRefed<nsIContent>
FindSelectionRoot(nsIEditor *aEditor, nsIContent *aContent)
{
  PRUint32 flags;
  aEditor->GetFlags(&flags);

  nsIDocument *document = aContent->GetCurrentDoc();
  if (!document) {
    return nsnull;
  }

  nsIContent *root;
  if (document->HasFlag(NODE_IS_EDITABLE)) {
    NS_IF_ADDREF(root = document->GetRootContent());

    return root;
  }

  if (flags & nsIPlaintextEditor::eEditorReadonlyMask) {
    
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

nsresult
nsTextEditorFocusListener::Focus(nsIDOMEvent* aEvent)
{
  NS_ENSURE_ARG(aEvent);

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  
  if (mEditor)
  {
    PRUint32 flags;
    mEditor->GetFlags(&flags);
    if (! (flags & nsIPlaintextEditor::eEditorDisabledMask))
    { 
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

        nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
        if (presShell) {
          nsRefPtr<nsCaret> caret;
          presShell->GetCaret(getter_AddRefs(caret));
          if (caret) {
            caret->SetIgnoreUserModify(PR_FALSE);
            if (selection) {
              caret->SetCaretDOMSelection(selection);
            }
          }
        }

        const PRBool kIsReadonly = (flags & nsIPlaintextEditor::eEditorReadonlyMask) != 0;
        selCon->SetCaretReadOnly(kIsReadonly);
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
    }
  }
  return NS_OK;
}

nsresult
nsTextEditorFocusListener::Blur(nsIDOMEvent* aEvent)
{
  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, NS_OK);

  nsCOMPtr<nsIDOMElement> element;
  fm->GetFocusedElement(getter_AddRefs(element));
  if (element)
    return NS_OK;

  NS_ENSURE_ARG(aEvent);
  
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>editor = do_QueryInterface(mEditor);
    if (editor)
    {
      nsCOMPtr<nsISelectionController>selCon;
      editor->GetSelectionController(getter_AddRefs(selCon));
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

        nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShell);
        if (presShell) {
          nsRefPtr<nsCaret> caret;
          presShell->GetCaret(getter_AddRefs(caret));
          if (caret) {
            caret->SetIgnoreUserModify(PR_TRUE);
          }
        }

        selCon->SetCaretEnabled(PR_FALSE);

        PRUint32 flags;
        mEditor->GetFlags(&flags);
        if((flags & nsIPlaintextEditor::eEditorWidgetMask)  ||
          (flags & nsIPlaintextEditor::eEditorPasswordMask) ||
          (flags & nsIPlaintextEditor::eEditorReadonlyMask) ||
          (flags & nsIPlaintextEditor::eEditorDisabledMask) ||
          (flags & nsIPlaintextEditor::eEditorFilterInputMask))
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);
        }
        else
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
        }

        selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
      }
    }
  }

  return NS_OK;
}

