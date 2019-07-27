





#include "nsEditorEventListener.h"

#include "mozilla/Assertions.h"         
#include "mozilla/EventListenerManager.h" 
#include "mozilla/IMEStateManager.h"    
#include "mozilla/Preferences.h"        
#include "mozilla/TextEvents.h"         
#include "mozilla/dom/Element.h"        
#include "mozilla/dom/EventTarget.h"    
#include "mozilla/dom/Selection.h"
#include "nsAString.h"
#include "nsCaret.h"                    
#include "nsDebug.h"                    
#include "nsEditor.h"                   
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
#include "nsISelectionController.h"     
#include "nsITransferable.h"            
#include "nsIWidget.h"                  
#include "nsLiteralString.h"            
#include "nsPIWindowRoot.h"             
#include "nsPrintfCString.h"            
#include "nsRange.h"
#include "nsServiceManagerUtils.h"      
#include "nsString.h"                   
#include "nsQueryObject.h"              
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

nsEditorEventListener::nsEditorEventListener()
  : mEditor(nullptr)
  , mCommitText(false)
  , mInTransaction(false)
  , mMouseDownOrUpConsumedByIME(false)
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

nsPresContext*
nsEditorEventListener::GetPresContext()
{
  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  return presShell ? presShell->GetPresContext() : nullptr;
}

nsIContent*
nsEditorEventListener::GetFocusedRootContent()
{
  NS_ENSURE_TRUE(mEditor, nullptr);

  nsCOMPtr<nsIContent> focusedContent = mEditor->GetFocusedContent();
  if (!focusedContent) {
    return nullptr;
  }

  nsIDocument* composedDoc = focusedContent->GetComposedDoc();
  NS_ENSURE_TRUE(composedDoc, nullptr);

  return composedDoc->HasFlag(NODE_IS_EDITABLE) ? nullptr : focusedContent;
}

bool
nsEditorEventListener::EditorHasFocus()
{
  NS_PRECONDITION(mEditor,
    "The caller must check whether this is connected to an editor");
  nsCOMPtr<nsIContent> focusedContent = mEditor->GetFocusedContent();
  if (!focusedContent) {
    return false;
  }
  nsIDocument* composedDoc = focusedContent->GetComposedDoc();
  return !!composedDoc;
}





NS_IMPL_ISUPPORTS(nsEditorEventListener, nsIDOMEventListener)





NS_IMETHODIMP
nsEditorEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_FAILURE);

  nsCOMPtr<nsIEditor> kungFuDeathGrip = mEditor;

  WidgetEvent* internalEvent = aEvent->GetInternalNSEvent();

  
  
  
  
  
  
  
  
  
  
  switch (internalEvent->message) {
    
    case NS_DRAGDROP_ENTER: {
      nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
      return DragEnter(dragEvent);
    }
    
    case NS_DRAGDROP_OVER_SYNTH: {
      nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
      return DragOver(dragEvent);
    }
    
    case NS_DRAGDROP_EXIT_SYNTH: {
      nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
      return DragExit(dragEvent);
    }
    
    case NS_DRAGDROP_DROP: {
      nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
      return Drop(dragEvent);
    }
#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
    
    case NS_KEY_DOWN: {
      nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
      return KeyDown(keyEvent);
    }
    
    case NS_KEY_UP: {
      nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
      return KeyUp(keyEvent);
    }
#endif 
    
    case NS_KEY_PRESS: {
      nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
      return KeyPress(keyEvent);
    }
    
    case NS_MOUSE_BUTTON_DOWN: {
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
      NS_ENSURE_TRUE(mouseEvent, NS_OK);
      
      
      
      
      
      
      
      mMouseDownOrUpConsumedByIME = NotifyIMEOfMouseButtonEvent(mouseEvent);
      return mMouseDownOrUpConsumedByIME ? NS_OK : MouseDown(mouseEvent);
    }
    
    case NS_MOUSE_BUTTON_UP: {
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
      NS_ENSURE_TRUE(mouseEvent, NS_OK);
      
      
      
      
      
      
      
      
      
      
      if (NotifyIMEOfMouseButtonEvent(mouseEvent)) {
        mMouseDownOrUpConsumedByIME = true;
      }
      return mMouseDownOrUpConsumedByIME ? NS_OK : MouseUp(mouseEvent);
    }
    
    case NS_MOUSE_CLICK: {
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
      NS_ENSURE_TRUE(mouseEvent, NS_OK);
      
      
      if (mMouseDownOrUpConsumedByIME) {
        mMouseDownOrUpConsumedByIME = false;
        mouseEvent->PreventDefault();
        return NS_OK;
      }
      return MouseClick(mouseEvent);
    }
    
    case NS_FOCUS_CONTENT:
      return Focus(aEvent);
    
    case NS_BLUR_CONTENT:
      return Blur(aEvent);
    
    case NS_COMPOSITION_CHANGE:
      return HandleText(aEvent);
    
    case NS_COMPOSITION_START:
      return HandleStartComposition(aEvent);
    
    case NS_COMPOSITION_END:
      HandleEndComposition(aEvent);
      return NS_OK;
  }

  nsAutoString eventType;
  aEvent->GetType(eventType);
  
  
  if (eventType.EqualsLiteral("focus")) {
    return Focus(aEvent);
  }
  if (eventType.EqualsLiteral("blur")) {
    return Blur(aEvent);
  }
#ifdef DEBUG
  nsPrintfCString assertMessage("Editor doesn't handle \"%s\" event "
    "because its internal event doesn't have proper message",
    NS_ConvertUTF16toUTF8(eventType).get());
  NS_ASSERTION(false, assertMessage.get());
#endif

  return NS_OK;
}

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
#include <windows.h>

#undef GetMessage
#undef CreateEvent
#undef GetClassName
#undef GetBinaryType
#undef RemoveDirectory
#undef SetProp

namespace {


bool IsCtrlShiftPressed(bool& isRTL)
{
  BYTE keystate[256];
  if (!::GetKeyboardState(keystate)) {
    return false;
  }

  
  
  
  
  
  
  
  
  const int kKeyDownMask = 0x80;
  if ((keystate[VK_CONTROL] & kKeyDownMask) == 0) {
    return false;
  }

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
    if (keystate[i] & kKeyDownMask) {
      return false;
    }
  }
  return true;
}

}




nsresult
nsEditorEventListener::KeyUp(nsIDOMKeyEvent* aKeyEvent)
{
  NS_ENSURE_TRUE(aKeyEvent, NS_OK);

  if (!mHaveBidiKeyboards) {
    return NS_OK;
  }

  uint32_t keyCode = 0;
  aKeyEvent->GetKeyCode(&keyCode);
  if ((keyCode == nsIDOMKeyEvent::DOM_VK_SHIFT ||
       keyCode == nsIDOMKeyEvent::DOM_VK_CONTROL) &&
      mShouldSwitchTextDirection && mEditor->IsPlaintextEditor()) {
    mEditor->SwitchTextDirectionTo(mSwitchToRTL ?
      nsIPlaintextEditor::eEditorRightToLeft :
      nsIPlaintextEditor::eEditorLeftToRight);
    mShouldSwitchTextDirection = false;
  }
  return NS_OK;
}

nsresult
nsEditorEventListener::KeyDown(nsIDOMKeyEvent* aKeyEvent)
{
  NS_ENSURE_TRUE(aKeyEvent, NS_OK);

  if (!mHaveBidiKeyboards) {
    return NS_OK;
  }

  uint32_t keyCode = 0;
  aKeyEvent->GetKeyCode(&keyCode);
  if (keyCode == nsIDOMKeyEvent::DOM_VK_SHIFT) {
    bool switchToRTL;
    if (IsCtrlShiftPressed(switchToRTL)) {
      mShouldSwitchTextDirection = true;
      mSwitchToRTL = switchToRTL;
    }
  } else if (keyCode != nsIDOMKeyEvent::DOM_VK_CONTROL) {
    
    mShouldSwitchTextDirection = false;
  }
  return NS_OK;
}
#endif

nsresult
nsEditorEventListener::KeyPress(nsIDOMKeyEvent* aKeyEvent)
{
  NS_ENSURE_TRUE(aKeyEvent, NS_OK);

  if (!mEditor->IsAcceptableInputEvent(aKeyEvent)) {
    return NS_OK;
  }

  
  
  
  
  

  bool defaultPrevented;
  aKeyEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  nsresult rv = mEditor->HandleKeyPressEvent(aKeyEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  aKeyEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  if (!ShouldHandleNativeKeyBindings(aKeyEvent)) {
    return NS_OK;
  }

  
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
  return NS_OK;
}

nsresult
nsEditorEventListener::MouseClick(nsIDOMMouseEvent* aMouseEvent)
{
  
  if (mEditor->IsReadonly() || mEditor->IsDisabled() ||
      !mEditor->IsAcceptableInputEvent(aMouseEvent)) {
    return NS_OK;
  }

  
  
  if (EditorHasFocus()) {
    nsPresContext* presContext = GetPresContext();
    if (presContext) {
      IMEStateManager::OnClickInEditor(presContext, GetFocusedRootContent(),
                                       aMouseEvent);
     }
  }

  bool preventDefault;
  nsresult rv = aMouseEvent->GetDefaultPrevented(&preventDefault);
  if (NS_FAILED(rv) || preventDefault) {
    
    return rv;
  }

  
  
  mEditor->ForceCompositionEnd();

  int16_t button = -1;
  aMouseEvent->GetButton(&button);
  if (button == 1) {
    return HandleMiddleClickPaste(aMouseEvent);
  }
  return NS_OK;
}

nsresult
nsEditorEventListener::HandleMiddleClickPaste(nsIDOMMouseEvent* aMouseEvent)
{
  if (!Preferences::GetBool("middlemouse.paste", false)) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> parent;
  if (NS_FAILED(aMouseEvent->GetRangeParent(getter_AddRefs(parent)))) {
    return NS_ERROR_NULL_POINTER;
  }
  int32_t offset = 0;
  if (NS_FAILED(aMouseEvent->GetRangeOffset(&offset))) {
    return NS_ERROR_NULL_POINTER;
  }

  nsRefPtr<Selection> selection = mEditor->GetSelection();
  if (selection) {
    selection->Collapse(parent, offset);
  }

  
  
  bool ctrlKey = false;
  aMouseEvent->GetCtrlKey(&ctrlKey);

  nsCOMPtr<nsIEditorMailSupport> mailEditor;
  if (ctrlKey) {
    mailEditor = do_QueryObject(mEditor);
  }

  nsresult rv;
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

  if (mailEditor) {
    mailEditor->PasteAsQuotation(clipboard);
  } else {
    mEditor->Paste(clipboard);
  }

  
  
  aMouseEvent->StopPropagation();
  aMouseEvent->PreventDefault();

  
  return NS_OK;
}

bool
nsEditorEventListener::NotifyIMEOfMouseButtonEvent(
                         nsIDOMMouseEvent* aMouseEvent)
{
  if (!EditorHasFocus()) {
    return false;
  }

  bool defaultPrevented;
  nsresult rv = aMouseEvent->GetDefaultPrevented(&defaultPrevented);
  NS_ENSURE_SUCCESS(rv, false);
  if (defaultPrevented) {
    return false;
  }
  nsPresContext* presContext = GetPresContext();
  NS_ENSURE_TRUE(presContext, false);
  return IMEStateManager::OnMouseButtonEventInEditor(presContext,
                                                     GetFocusedRootContent(),
                                                     aMouseEvent);
}

nsresult
nsEditorEventListener::MouseDown(nsIDOMMouseEvent* aMouseEvent)
{
  mEditor->ForceCompositionEnd();
  return NS_OK;
}

nsresult
nsEditorEventListener::HandleText(nsIDOMEvent* aTextEvent)
{
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
  NS_ENSURE_TRUE(aDragEvent, NS_OK);

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_OK);

  if (!mCaret) {
    mCaret = new nsCaret();
    mCaret->Init(presShell);
    mCaret->SetCaretReadOnly(true);
    
    
    
    mCaret->SetVisibilityDuringSelection(true);
  }

  presShell->SetCaret(mCaret);

  return DragOver(aDragEvent);
}

nsresult
nsEditorEventListener::DragOver(nsIDOMDragEvent* aDragEvent)
{
  NS_ENSURE_TRUE(aDragEvent, NS_OK);

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

    if (!mCaret) {
      return NS_OK;
    }

    int32_t offset = 0;
    nsresult rv = aDragEvent->GetRangeOffset(&offset);
    NS_ENSURE_SUCCESS(rv, rv);

    mCaret->SetVisible(true);
    mCaret->SetCaretPosition(parent, offset);

    return NS_OK;
  }

  if (!IsFileControlTextBox()) {
    
    
    aDragEvent->StopPropagation();
  }

  if (mCaret) {
    mCaret->SetVisible(false);
  }
  return NS_OK;
}

void
nsEditorEventListener::CleanupDragDropCaret()
{
  if (!mCaret) {
    return;
  }

  mCaret->SetVisible(false);    

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (presShell) {
    presShell->RestoreCaret();
  }

  mCaret->Terminate();
  mCaret = nullptr;
}

nsresult
nsEditorEventListener::DragExit(nsIDOMDragEvent* aDragEvent)
{
  NS_ENSURE_TRUE(aDragEvent, NS_OK);

  CleanupDragDropCaret();

  return NS_OK;
}

nsresult
nsEditorEventListener::Drop(nsIDOMDragEvent* aDragEvent)
{
  NS_ENSURE_TRUE(aDragEvent, NS_OK);

  CleanupDragDropCaret();

  bool defaultPrevented;
  aDragEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> parent;
  aDragEvent->GetRangeParent(getter_AddRefs(parent));
  nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
  NS_ENSURE_TRUE(dropParent, NS_ERROR_FAILURE);

  if (!dropParent->IsEditable() || !CanDrop(aDragEvent)) {
    
    if ((mEditor->IsReadonly() || mEditor->IsDisabled()) &&
        !IsFileControlTextBox()) {
      
      
      
      
      return aDragEvent->StopPropagation();
    }
    return NS_OK;
  }

  aDragEvent->StopPropagation();
  aDragEvent->PreventDefault();
  return mEditor->InsertFromDrop(aDragEvent);
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
  if (!sourceNode) {
    return true;
  }

  
  

  nsCOMPtr<nsIDOMDocument> domdoc = mEditor->GetDOMDocument();
  NS_ENSURE_TRUE(domdoc, false);

  nsCOMPtr<nsIDOMDocument> sourceDoc;
  nsresult rv = sourceNode->GetOwnerDocument(getter_AddRefs(sourceDoc));
  NS_ENSURE_SUCCESS(rv, false);

  
  if (domdoc != sourceDoc) {
    return true;
  }

  nsRefPtr<Selection> selection = mEditor->GetSelection();
  if (!selection) {
    return false;
  }

  
  if (selection->Collapsed()) {
    return true;
  }

  nsCOMPtr<nsIDOMNode> parent;
  rv = aEvent->GetRangeParent(getter_AddRefs(parent));
  if (NS_FAILED(rv) || !parent) {
    return false;
  }

  int32_t offset = 0;
  rv = aEvent->GetRangeOffset(&offset);
  NS_ENSURE_SUCCESS(rv, false);

  int32_t rangeCount;
  rv = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(rv, false);

  for (int32_t i = 0; i < rangeCount; i++) {
    nsRefPtr<nsRange> range = selection->GetRangeAt(i);
    if (!range) {
      
      continue;
    }

    bool inRange = true;
    range->IsPointInRange(parent, offset, &inRange);
    if (inRange) {
      
      return false;
    }
  }
  return true;
}

nsresult
nsEditorEventListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
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
  if (!mEditor->IsAcceptableInputEvent(aCompositionEvent)) {
    return;
  }

  mEditor->EndIMEComposition();
}

nsresult
nsEditorEventListener::Focus(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(aEvent, NS_OK);

  
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
      if (!SameCOMIdentity(element, target)) {
        return NS_OK;
      }
    }
  }

  mEditor->OnFocus(target);

  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_OK);
  nsCOMPtr<nsIContent> focusedContent = mEditor->GetFocusedContentForIME();
  IMEStateManager::OnFocusInEditor(ps->GetPresContext(), focusedContent,
                                   mEditor);

  return NS_OK;
}

nsresult
nsEditorEventListener::Blur(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(aEvent, NS_OK);

  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, NS_OK);

  nsCOMPtr<nsIDOMElement> element;
  fm->GetFocusedElement(getter_AddRefs(element));
  if (!element) {
    mEditor->FinalizeSelection();
  }
  return NS_OK;
}

void
nsEditorEventListener::SpellCheckIfNeeded()
{
  
  
  uint32_t currentFlags = 0;
  mEditor->GetFlags(&currentFlags);
  if(currentFlags & nsIPlaintextEditor::eEditorSkipSpellCheck) {
    currentFlags ^= nsIPlaintextEditor::eEditorSkipSpellCheck;
    mEditor->SetFlags(currentFlags);
  }
}

bool
nsEditorEventListener::IsFileControlTextBox()
{
  dom::Element* root = mEditor->GetRoot();
  if (!root || !root->ChromeOnlyAccess()) {
    return false;
  }
  nsIContent* parent = root->FindFirstNonChromeOnlyAccessContent();
  if (!parent || !parent->IsHTMLElement(nsGkAtoms::input)) {
    return false;
  }
  nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(parent);
  return formControl->GetType() == NS_FORM_INPUT_FILE;
}

bool
nsEditorEventListener::ShouldHandleNativeKeyBindings(nsIDOMKeyEvent* aKeyEvent)
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
