




#include "mozilla/Assertions.h"         
#include "mozilla/EventListenerManager.h" 
#include "mozilla/IMEStateManager.h"    
#include "mozilla/Preferences.h"        
#include "mozilla/TextEvents.h"         
#include "mozilla/dom/Element.h"        
#include "mozilla/dom/EventTarget.h"    
#include "nsAString.h"
#include "nsCaret.h"                    
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsEditorEventListener.h"
#include "nsFocusManager.h"             
#include "nsGkAtoms.h"                  
#include "nsIClipboard.h"               
#include "nsIContent.h"                 
#include "nsIController.h"              
#include "nsID.h"
#include "mozilla/dom/DOMStringList.h"
#include "mozilla/dom/DataTransfer.h"
#include "nsIDOMDocument.h"             
#include "nsIDOMDragEvent.h"            
#include "nsIDOMElement.h"              
#include "nsIDOMEvent.h"                
#include "nsIDOMEventTarget.h"          
#include "nsIDOMKeyEvent.h"             
#include "nsIDOMMouseEvent.h"           
#include "nsIDOMNode.h"                 
#include "nsIDOMRange.h"                
#include "nsIDocument.h"                
#include "nsIEditor.h"                  
#include "nsIEditorIMESupport.h"
#include "nsIEditorMailSupport.h"       
#include "nsIFocusManager.h"            
#include "nsIFormControl.h"             
#include "nsIHTMLEditor.h"              
#include "nsINode.h"                    
#include "nsIPlaintextEditor.h"         
#include "nsIPresShell.h"               
#include "nsISelection.h"               
#include "nsISelectionController.h"     
#include "nsISelectionPrivate.h"        
#include "nsITransferable.h"            
#include "nsIWidget.h"                  
#include "nsLiteralString.h"            
#include "nsPIWindowRoot.h"             
#include "nsServiceManagerUtils.h"      
#include "nsString.h"                   
#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
#include "nsContentUtils.h"             
#include "nsIBidiKeyboard.h"            
#endif

class nsPresContext;

using namespace mozilla;
using namespace mozilla::dom;

static void
DoCommandCallback(Command aCommand, void* aData)
{
  nsIDocument* doc = static_cast<nsIDocument*>(aData);
  nsPIDOMWindow* win = doc->GetWindow();
  if (!win) {
    return;
  }
  nsCOMPtr<nsPIWindowRoot> root = win->GetTopWindowRoot();
  if (!root) {
    return;
  }

  const char* commandStr = WidgetKeyboardEvent::GetCommandStr(aCommand);

  nsCOMPtr<nsIController> controller;
  root->GetControllerForCommand(commandStr, getter_AddRefs(controller));
  if (!controller) {
    return;
  }

  bool commandEnabled;
  nsresult rv = controller->IsCommandEnabled(commandStr, &commandEnabled);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (commandEnabled) {
    controller->DoCommand(commandStr);
  }
}

nsEditorEventListener::nsEditorEventListener() :
  mEditor(nullptr), mCommitText(false),
  mInTransaction(false)
#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  , mHaveBidiKeyboards(false)
  , mShouldSwitchTextDirection(false)
  , mSwitchToRTL(false)
#endif
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

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  nsIBidiKeyboard* bidiKeyboard = nsContentUtils::GetBidiKeyboard();
  if (bidiKeyboard) {
    bool haveBidiKeyboards = false;
    bidiKeyboard->GetHaveBidiKeyboards(&haveBidiKeyboards);
    mHaveBidiKeyboards = haveBidiKeyboards;
  }
#endif

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

  nsCOMPtr<EventTarget> piTarget = mEditor->GetDOMEventTarget();
  NS_ENSURE_TRUE(piTarget, NS_ERROR_FAILURE);

  
  EventListenerManager* elmP = piTarget->GetOrCreateListenerManager();
  NS_ENSURE_STATE(elmP);

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("keydown"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("keyup"),
                               TrustedEventsAtSystemGroupBubble());
#endif
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("keypress"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("dragenter"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("dragover"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("dragexit"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("drop"),
                               TrustedEventsAtSystemGroupBubble());
  
  
  
  
  
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("mousedown"),
                               TrustedEventsAtCapture());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("mouseup"),
                               TrustedEventsAtCapture());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("click"),
                               TrustedEventsAtCapture());


  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("blur"),
                               TrustedEventsAtCapture());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("focus"),
                               TrustedEventsAtCapture());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("text"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("compositionstart"),
                               TrustedEventsAtSystemGroupBubble());
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("compositionend"),
                               TrustedEventsAtSystemGroupBubble());

  return NS_OK;
}

void
nsEditorEventListener::Disconnect()
{
  if (!mEditor) {
    return;
  }
  UninstallFromEditor();

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    nsCOMPtr<nsIDOMElement> domFocus;
    fm->GetFocusedElement(getter_AddRefs(domFocus));
    nsCOMPtr<nsINode> focusedElement = do_QueryInterface(domFocus);
    mozilla::dom::Element* root = mEditor->GetRoot();
    if (focusedElement && root &&
        nsContentUtils::ContentIsDescendantOf(focusedElement, root)) {
      
      
      mEditor->FinalizeSelection();
    }
  }

  mEditor = nullptr;
}

void
nsEditorEventListener::UninstallFromEditor()
{
  nsCOMPtr<EventTarget> piTarget = mEditor->GetDOMEventTarget();
  if (!piTarget) {
    return;
  }

  EventListenerManager* elmP = piTarget->GetOrCreateListenerManager();
  if (!elmP) {
    return;
  }

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("keydown"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("keyup"),
                                  TrustedEventsAtSystemGroupBubble());
#endif
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("keypress"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("dragenter"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("dragover"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("dragexit"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("drop"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("mousedown"),
                                  TrustedEventsAtCapture());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("mouseup"),
                                  TrustedEventsAtCapture());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("click"),
                                  TrustedEventsAtCapture());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("blur"),
                                  TrustedEventsAtCapture());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("focus"),
                                  TrustedEventsAtCapture());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("text"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("compositionstart"),
                                  TrustedEventsAtSystemGroupBubble());
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("compositionend"),
                                  TrustedEventsAtSystemGroupBubble());
}

already_AddRefed<nsIPresShell>
nsEditorEventListener::GetPresShell()
{
  NS_PRECONDITION(mEditor,
    "The caller must check whether this is connected to an editor");
  return mEditor->GetPresShell();
}





NS_IMPL_ISUPPORTS(nsEditorEventListener, nsIDOMEventListener)





NS_IMETHODIMP
nsEditorEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIEditor> kungFuDeathGrip = mEditor;

  nsAutoString eventType;
  aEvent->GetType(eventType);

  nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
  if (dragEvent) {
    if (eventType.EqualsLiteral("dragenter"))
      return DragEnter(dragEvent);
    if (eventType.EqualsLiteral("dragover"))
      return DragOver(dragEvent);
    if (eventType.EqualsLiteral("dragexit"))
      return DragExit(dragEvent);
    if (eventType.EqualsLiteral("drop"))
      return Drop(dragEvent);
  }

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  if (eventType.EqualsLiteral("keydown"))
    return KeyDown(aEvent);
  if (eventType.EqualsLiteral("keyup"))
    return KeyUp(aEvent);
#endif
  if (eventType.EqualsLiteral("keypress"))
    return KeyPress(aEvent);
  if (eventType.EqualsLiteral("mousedown"))
    return MouseDown(aEvent);
  if (eventType.EqualsLiteral("mouseup"))
    return MouseUp(aEvent);
  if (eventType.EqualsLiteral("click"))
    return MouseClick(aEvent);
  if (eventType.EqualsLiteral("focus"))
    return Focus(aEvent);
  if (eventType.EqualsLiteral("blur"))
    return Blur(aEvent);
  if (eventType.EqualsLiteral("text"))
    return HandleText(aEvent);
  if (eventType.EqualsLiteral("compositionstart"))
    return HandleStartComposition(aEvent);
  if (eventType.EqualsLiteral("compositionend")) {
    HandleEndComposition(aEvent);
    return NS_OK;
  }

  return NS_OK;
}

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
#include <windows.h>

namespace {


bool IsCtrlShiftPressed(bool& isRTL)
{
  BYTE keystate[256];
  if (!::GetKeyboardState(keystate)) {
    return false;
  }

  
  
  
  
  
  
  
  
  const int kKeyDownMask = 0x80;
  if ((keystate[VK_CONTROL] & kKeyDownMask) == 0)
    return false;

  if (keystate[VK_RSHIFT] & kKeyDownMask) {
    keystate[VK_RSHIFT] = 0;
    isRTL = true;
  } else if (keystate[VK_LSHIFT] & kKeyDownMask) {
    keystate[VK_LSHIFT] = 0;
    isRTL = false;
  } else {
    return false;
  }

  
  
  
  
  
  
  keystate[VK_SHIFT] = 0;
  keystate[VK_CONTROL] = 0;
  keystate[VK_RCONTROL] = 0;
  keystate[VK_LCONTROL] = 0;
  for (int i = 0; i <= VK_PACKET; ++i) {
    if (keystate[i] & kKeyDownMask)
      return false;
  }
  return true;
}

}




NS_IMETHODIMP
nsEditorEventListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  if (mHaveBidiKeyboards) {
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    if (!keyEvent) {
      
      return NS_OK;
    }

    uint32_t keyCode = 0;
    keyEvent->GetKeyCode(&keyCode);
    if (keyCode == nsIDOMKeyEvent::DOM_VK_SHIFT ||
        keyCode == nsIDOMKeyEvent::DOM_VK_CONTROL) {
      if (mShouldSwitchTextDirection && mEditor->IsPlaintextEditor()) {
        mEditor->SwitchTextDirectionTo(mSwitchToRTL ?
          nsIPlaintextEditor::eEditorRightToLeft :
          nsIPlaintextEditor::eEditorLeftToRight);
        mShouldSwitchTextDirection = false;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  if (mHaveBidiKeyboards) {
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    if (!keyEvent) {
      
      return NS_OK;
    }

    uint32_t keyCode = 0;
    keyEvent->GetKeyCode(&keyCode);
    if (keyCode == nsIDOMKeyEvent::DOM_VK_SHIFT) {
      bool switchToRTL;
      if (IsCtrlShiftPressed(switchToRTL)) {
        mShouldSwitchTextDirection = true;
        mSwitchToRTL = switchToRTL;
      }
    } else if (keyCode != nsIDOMKeyEvent::DOM_VK_CONTROL) {
      
      mShouldSwitchTextDirection = false;
    }
  }

  return NS_OK;
}
#endif

NS_IMETHODIMP
nsEditorEventListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  if (!mEditor->IsAcceptableInputEvent(aKeyEvent)) {
    return NS_OK;
  }

  
  
  
  
  

  bool defaultPrevented;
  aKeyEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) {
    
    return NS_OK;
  }

  nsresult rv = mEditor->HandleKeyPressEvent(keyEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  aKeyEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  if (ShouldHandleNativeKeyBindings(aKeyEvent)) {
    
    WidgetKeyboardEvent* keyEvent =
      aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
    MOZ_ASSERT(keyEvent,
               "DOM key event's internal event must be WidgetKeyboardEvent");
    nsIWidget* widget = keyEvent->widget;
    
    if (!widget) {
      nsCOMPtr<nsIPresShell> ps = GetPresShell();
      nsPresContext* pc = ps ? ps->GetPresContext() : nullptr;
      widget = pc ? pc->GetNearestWidget() : nullptr;
      NS_ENSURE_TRUE(widget, NS_OK);
    }

    nsCOMPtr<nsIDocument> doc = mEditor->GetDocument();
    bool handled = widget->ExecuteNativeKeyBinding(
                             nsIWidget::NativeKeyBindingsForRichTextEditor,
                             *keyEvent, DoCommandCallback, doc);
    if (handled) {
      aKeyEvent->PreventDefault();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_OK);

  
  if (mEditor->IsReadonly() || mEditor->IsDisabled() ||
      !mEditor->IsAcceptableInputEvent(aMouseEvent)) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIContent> focusedContent = mEditor->GetFocusedContent();
  if (focusedContent) {
    nsIDocument* currentDoc = focusedContent->GetCurrentDoc();
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    nsPresContext* presContext =
      presShell ? presShell->GetPresContext() : nullptr;
    if (presContext && currentDoc) {
      IMEStateManager::OnClickInEditor(presContext,
        currentDoc->HasFlag(NODE_IS_EDITABLE) ? nullptr : focusedContent,
        mouseEvent);
    }
  }

  bool preventDefault;
  nsresult rv = aMouseEvent->GetDefaultPrevented(&preventDefault);
  if (NS_FAILED(rv) || preventDefault) {
    
    return rv;
  }

  
  
  mEditor->ForceCompositionEnd();

  int16_t button = -1;
  mouseEvent->GetButton(&button);
  
  if (button == 1)
  {
    if (Preferences::GetBool("middlemouse.paste", false))
    {
      
      nsCOMPtr<nsIDOMNode> parent;
      if (NS_FAILED(mouseEvent->GetRangeParent(getter_AddRefs(parent))))
        return NS_ERROR_NULL_POINTER;
      int32_t offset = 0;
      if (NS_FAILED(mouseEvent->GetRangeOffset(&offset)))
        return NS_ERROR_NULL_POINTER;

      nsCOMPtr<nsISelection> selection;
      if (NS_SUCCEEDED(mEditor->GetSelection(getter_AddRefs(selection))))
        (void)selection->Collapse(parent, offset);

      
      
      bool ctrlKey = false;
      mouseEvent->GetCtrlKey(&ctrlKey);

      nsCOMPtr<nsIEditorMailSupport> mailEditor;
      if (ctrlKey)
        mailEditor = do_QueryObject(mEditor);

      int32_t clipboard = nsIClipboard::kGlobalClipboard;
      nsCOMPtr<nsIClipboard> clipboardService =
        do_GetService("@mozilla.org/widget/clipboard;1", &rv);
      if (NS_SUCCEEDED(rv)) {
        bool selectionSupported;
        rv = clipboardService->SupportsSelectionClipboard(&selectionSupported);
        if (NS_SUCCEEDED(rv) && selectionSupported) {
          clipboard = nsIClipboard::kSelectionClipboard;
        }
      }

      if (mailEditor)
        mailEditor->PasteAsQuotation(clipboard);
      else
        mEditor->Paste(clipboard);

      
      
      mouseEvent->StopPropagation();
      mouseEvent->PreventDefault();

      
      return NS_OK;
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
nsEditorEventListener::HandleText(nsIDOMEvent* aTextEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

  if (!mEditor->IsAcceptableInputEvent(aTextEvent)) {
    return NS_OK;
  }

  
  if (mEditor->IsReadonly() || mEditor->IsDisabled()) {
    return NS_OK;
  }

  return mEditor->UpdateIMEComposition(aTextEvent);
}





nsresult
nsEditorEventListener::DragEnter(nsIDOMDragEvent* aDragEvent)
{
  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_OK);

  if (!mCaret) {
    mCaret = new nsCaret();
    mCaret->Init(presShell);
    mCaret->SetCaretReadOnly(true);
  }

  presShell->SetCaret(mCaret);

  return DragOver(aDragEvent);
}

nsresult
nsEditorEventListener::DragOver(nsIDOMDragEvent* aDragEvent)
{
  nsCOMPtr<nsIDOMNode> parent;
  bool defaultPrevented;
  aDragEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  aDragEvent->GetRangeParent(getter_AddRefs(parent));
  nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
  NS_ENSURE_TRUE(dropParent, NS_ERROR_FAILURE);

  if (dropParent->IsEditable() && CanDrop(aDragEvent)) {
    aDragEvent->PreventDefault(); 

    if (mCaret) {
      int32_t offset = 0;
      nsresult rv = aDragEvent->GetRangeOffset(&offset);
      NS_ENSURE_SUCCESS(rv, rv);

      mCaret->SetVisible(true);
      mCaret->SetCaretPosition(parent, offset);
    }
  }
  else
  {
    if (!IsFileControlTextBox()) {
      
      
      aDragEvent->StopPropagation();
    }

    if (mCaret)
    {
      mCaret->SetVisible(false);
    }
  }

  return NS_OK;
}

void
nsEditorEventListener::CleanupDragDropCaret()
{
  if (mCaret)
  {
    mCaret->SetVisible(false);    

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell)
    {
      nsCOMPtr<nsISelectionController> selCon(do_QueryInterface(presShell));
      if (selCon) {
        selCon->SetCaretEnabled(false);
      }
      presShell->RestoreCaret();
    }

    mCaret->Terminate();
    mCaret = nullptr;
  }
}

nsresult
nsEditorEventListener::DragExit(nsIDOMDragEvent* aDragEvent)
{
  CleanupDragDropCaret();

  return NS_OK;
}

nsresult
nsEditorEventListener::Drop(nsIDOMDragEvent* aMouseEvent)
{
  CleanupDragDropCaret();

  bool defaultPrevented;
  aMouseEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> parent;
  aMouseEvent->GetRangeParent(getter_AddRefs(parent));
  nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
  NS_ENSURE_TRUE(dropParent, NS_ERROR_FAILURE);

  if (!dropParent->IsEditable() || !CanDrop(aMouseEvent)) {
    
    if ((mEditor->IsReadonly() || mEditor->IsDisabled()) &&
        !IsFileControlTextBox()) {
      
      
      
      
      return aMouseEvent->StopPropagation();
    }
    return NS_OK;
  }

  aMouseEvent->StopPropagation();
  aMouseEvent->PreventDefault();
  return mEditor->InsertFromDrop(aMouseEvent);
}

bool
nsEditorEventListener::CanDrop(nsIDOMDragEvent* aEvent)
{
  
  if (mEditor->IsReadonly() || mEditor->IsDisabled()) {
    return false;
  }

  nsCOMPtr<nsIDOMDataTransfer> domDataTransfer;
  aEvent->GetDataTransfer(getter_AddRefs(domDataTransfer));
  nsCOMPtr<DataTransfer> dataTransfer = do_QueryInterface(domDataTransfer);
  NS_ENSURE_TRUE(dataTransfer, false);

  nsRefPtr<DOMStringList> types = dataTransfer->Types();

  
  
  if (!types->Contains(NS_LITERAL_STRING(kTextMime)) &&
      !types->Contains(NS_LITERAL_STRING(kMozTextInternal)) &&
      (mEditor->IsPlaintextEditor() ||
       (!types->Contains(NS_LITERAL_STRING(kHTMLMime)) &&
        !types->Contains(NS_LITERAL_STRING(kFileMime))))) {
    return false;
  }

  
  
  
  nsCOMPtr<nsIDOMNode> sourceNode;
  dataTransfer->GetMozSourceNode(getter_AddRefs(sourceNode));
  if (!sourceNode)
    return true;

  
  

  nsCOMPtr<nsIDOMDocument> domdoc = mEditor->GetDOMDocument();
  NS_ENSURE_TRUE(domdoc, false);

  nsCOMPtr<nsIDOMDocument> sourceDoc;
  nsresult rv = sourceNode->GetOwnerDocument(getter_AddRefs(sourceDoc));
  NS_ENSURE_SUCCESS(rv, false);
  if (domdoc == sourceDoc)      
  {
    nsCOMPtr<nsISelection> selection;
    rv = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(rv) || !selection)
      return false;
    
    
    if (!selection->Collapsed()) {
      nsCOMPtr<nsIDOMNode> parent;
      rv = aEvent->GetRangeParent(getter_AddRefs(parent));
      if (NS_FAILED(rv) || !parent) return false;

      int32_t offset = 0;
      rv = aEvent->GetRangeOffset(&offset);
      NS_ENSURE_SUCCESS(rv, false);

      int32_t rangeCount;
      rv = selection->GetRangeCount(&rangeCount);
      NS_ENSURE_SUCCESS(rv, false);

      for (int32_t i = 0; i < rangeCount; i++)
      {
        nsCOMPtr<nsIDOMRange> range;
        rv = selection->GetRangeAt(i, getter_AddRefs(range));
        if (NS_FAILED(rv) || !range) 
          continue; 

        bool inRange = true;
        (void)range->IsPointInRange(parent, offset, &inRange);
        if (inRange)
          return false;  
      }
    }
  }
  
  return true;
}

NS_IMETHODIMP
nsEditorEventListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  if (!mEditor->IsAcceptableInputEvent(aCompositionEvent)) {
    return NS_OK;
  }
  WidgetCompositionEvent* compositionStart =
    aCompositionEvent->GetInternalNSEvent()->AsCompositionEvent();
  return mEditor->BeginIMEComposition(compositionStart);
}

void
nsEditorEventListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
  MOZ_ASSERT(mEditor);
  if (!mEditor->IsAcceptableInputEvent(aCompositionEvent)) {
    return;
  }

  mEditor->EndIMEComposition();
}

NS_IMETHODIMP
nsEditorEventListener::Focus(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_ARG(aEvent);

  
  if (mEditor->IsDisabled()) {
    return NS_OK;
  }

  
  SpellCheckIfNeeded();

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));
  nsCOMPtr<nsINode> node = do_QueryInterface(target);
  NS_ENSURE_TRUE(node, NS_ERROR_UNEXPECTED);

  
  
  if (node->IsNodeOfType(nsINode::eDOCUMENT) &&
      !node->HasFlag(NODE_IS_EDITABLE)) {
    return NS_OK;
  }

  if (node->IsNodeOfType(nsINode::eCONTENT)) {
    
    
    
    
    
    
    nsCOMPtr<nsIContent> editableRoot = mEditor->FindSelectionRoot(node);

    
    
    if (editableRoot) {
      nsIFocusManager* fm = nsFocusManager::GetFocusManager();
      NS_ENSURE_TRUE(fm, NS_OK);

      nsCOMPtr<nsIDOMElement> element;
      fm->GetFocusedElement(getter_AddRefs(element));
      if (!SameCOMIdentity(element, target))
        return NS_OK;
    }
  }

  mEditor->OnFocus(target);

  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_OK);
  nsCOMPtr<nsIContent> focusedContent = mEditor->GetFocusedContentForIME();
  IMEStateManager::OnFocusInEditor(ps->GetPresContext(), focusedContent);

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

  mEditor->FinalizeSelection();
  return NS_OK;
}

void
nsEditorEventListener::SpellCheckIfNeeded() {
  
  
  uint32_t currentFlags = 0;
  mEditor->GetFlags(&currentFlags);
  if(currentFlags & nsIPlaintextEditor::eEditorSkipSpellCheck)
  {
    currentFlags ^= nsIPlaintextEditor::eEditorSkipSpellCheck;
    mEditor->SetFlags(currentFlags);
  }
}

bool
nsEditorEventListener::IsFileControlTextBox()
{
  dom::Element* root = mEditor->GetRoot();
  if (root && root->ChromeOnlyAccess()) {
    nsIContent* parent = root->FindFirstNonChromeOnlyAccessContent();
    if (parent && parent->IsHTML(nsGkAtoms::input)) {
      nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(parent);
      MOZ_ASSERT(formControl);
      return formControl->GetType() == NS_FORM_INPUT_FILE;
    }
  }
  return false;
}

bool
nsEditorEventListener::ShouldHandleNativeKeyBindings(nsIDOMEvent* aKeyEvent)
{
  
  
  
  
  
  
  

  nsCOMPtr<nsIDOMEventTarget> target;
  aKeyEvent->GetTarget(getter_AddRefs(target));
  nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);
  if (!targetContent) {
    return false;
  }

  nsCOMPtr<nsIHTMLEditor> htmlEditor =
    do_QueryInterface(static_cast<nsIEditor*>(mEditor));
  if (!htmlEditor) {
    return false;
  }

  nsCOMPtr<nsIDocument> doc = mEditor->GetDocument();
  if (doc->HasFlag(NODE_IS_EDITABLE)) {
    
    return true;
  }

  nsIContent* editingHost = htmlEditor->GetActiveEditingHost();
  if (!editingHost) {
    return false;
  }

  return nsContentUtils::ContentIsDescendantOf(targetContent, editingHost);
}

