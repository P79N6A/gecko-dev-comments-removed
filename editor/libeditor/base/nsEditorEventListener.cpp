




#include "mozilla/Assertions.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/dom/Element.h"        
#include "nsAString.h"
#include "nsCaret.h"                    
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsEditorEventListener.h"
#include "nsEventListenerManager.h"     
#include "nsFocusManager.h"             
#include "nsGUIEvent.h"                 
#include "nsGkAtoms.h"                  
#include "nsIClipboard.h"               
#include "nsIContent.h"                 
#include "nsID.h"
#include "nsIDOMDOMStringList.h"        
#include "nsIDOMDataTransfer.h"         
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
#include "nsIMEStateManager.h"          
#include "nsINode.h"                    
#include "nsIPlaintextEditor.h"         
#include "nsIPresShell.h"               
#include "nsIPrivateTextEvent.h"        
#include "nsIPrivateTextRange.h"        
#include "nsISelection.h"               
#include "nsISelectionController.h"     
#include "nsISelectionPrivate.h"        
#include "nsITransferable.h"            
#include "nsLiteralString.h"            
#include "nsServiceManagerUtils.h"      
#include "nsString.h"                   
#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
#include "nsContentUtils.h"             
#include "nsIBidiKeyboard.h"            
#endif

class nsPresContext;

using namespace mozilla;

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

  nsCOMPtr<nsIDOMEventTarget> piTarget = mEditor->GetDOMEventTarget();
  NS_ENSURE_TRUE(piTarget, NS_ERROR_FAILURE);

  
  nsEventListenerManager* elmP = piTarget->GetListenerManager(true);
  NS_ENSURE_STATE(elmP);

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("keydown"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("keyup"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
#endif
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("keypress"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("dragenter"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("dragover"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("dragexit"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("drop"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  
  
  
  
  
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("mousedown"),
                               NS_EVENT_FLAG_CAPTURE);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("mouseup"),
                               NS_EVENT_FLAG_CAPTURE);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("click"),
                               NS_EVENT_FLAG_CAPTURE);


  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("blur"),
                               NS_EVENT_FLAG_CAPTURE);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("focus"),
                               NS_EVENT_FLAG_CAPTURE);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("text"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("compositionstart"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->AddEventListenerByType(this,
                               NS_LITERAL_STRING("compositionend"),
                               NS_EVENT_FLAG_BUBBLE |
                               NS_EVENT_FLAG_SYSTEM_EVENT);

  return NS_OK;
}

void
nsEditorEventListener::Disconnect()
{
  if (!mEditor) {
    return;
  }
  UninstallFromEditor();
  mEditor = nullptr;
}

void
nsEditorEventListener::UninstallFromEditor()
{
  nsCOMPtr<nsIDOMEventTarget> piTarget = mEditor->GetDOMEventTarget();
  if (!piTarget) {
    return;
  }

  nsEventListenerManager* elmP =
    piTarget->GetListenerManager(true);
  if (!elmP) {
    return;
  }

#ifdef HANDLE_NATIVE_TEXT_DIRECTION_SWITCH
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("keydown"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("keyup"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
#endif
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("keypress"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("dragenter"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("dragover"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("dragexit"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("drop"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("mousedown"),
                                  NS_EVENT_FLAG_CAPTURE);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("mouseup"),
                                  NS_EVENT_FLAG_CAPTURE);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("click"),
                                  NS_EVENT_FLAG_CAPTURE);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("blur"),
                                  NS_EVENT_FLAG_CAPTURE);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("focus"),
                                  NS_EVENT_FLAG_CAPTURE);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("text"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("compositionstart"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  elmP->RemoveEventListenerByType(this,
                                  NS_LITERAL_STRING("compositionend"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
}

already_AddRefed<nsIPresShell>
nsEditorEventListener::GetPresShell()
{
  NS_PRECONDITION(mEditor,
    "The caller must check whether this is connected to an editor");
  return mEditor->GetPresShell();
}





NS_IMPL_ISUPPORTS1(nsEditorEventListener, nsIDOMEventListener)





NS_IMETHODIMP
nsEditorEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NOT_AVAILABLE);

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

  
  nsEditor::HandlingTrustedAction operation(mEditor, aKeyEvent);

  
  
  
  
  

  bool defaultPrevented;
  aKeyEvent->GetDefaultPrevented(&defaultPrevented);
  if (defaultPrevented) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
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
      nsIMEStateManager::OnClickInEditor(presContext,
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

  uint16_t button = (uint16_t)-1;
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

  
  nsEditor::HandlingTrustedAction operation(mEditor, aTextEvent);

  return mEditor->UpdateIMEComposition(composedText, textRangeList);
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

      
      if (mCaret)
        mCaret->EraseCaret();
      
      
      mCaret->DrawAtPosition(parent, offset);
    }
  }
  else
  {
    if (!IsFileControlTextBox()) {
      
      
      aDragEvent->StopPropagation();
    }

    if (mCaret)
    {
      mCaret->EraseCaret();
    }
  }

  return NS_OK;
}

void
nsEditorEventListener::CleanupDragDropCaret()
{
  if (mCaret)
  {
    mCaret->EraseCaret();
    mCaret->SetCaretVisible(false);    

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell)
    {
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

  nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
  aEvent->GetDataTransfer(getter_AddRefs(dataTransfer));
  NS_ENSURE_TRUE(dataTransfer, false);

  nsCOMPtr<nsIDOMDOMStringList> types;
  dataTransfer->GetTypes(getter_AddRefs(types));
  NS_ENSURE_TRUE(types, false);

  
  
  bool typeSupported;
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

  NS_ENSURE_TRUE(typeSupported, false);

  
  
  
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
  return mEditor->BeginIMEComposition();
}

void
nsEditorEventListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
  MOZ_ASSERT(mEditor);
  if (!mEditor->IsAcceptableInputEvent(aCompositionEvent)) {
    return;
  }

  
  nsEditor::HandlingTrustedAction operation(mEditor, aCompositionEvent);

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
      selectionPrivate->SetAncestorLimiter(nullptr);
    }

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell) {
      nsRefPtr<nsCaret> caret = presShell->GetCaret();
      if (caret) {
        caret->SetIgnoreUserModify(true);
      }
    }

    selCon->SetCaretEnabled(false);

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
  if (root && root->IsInNativeAnonymousSubtree()) {
    nsIContent* parent = root->FindFirstNonNativeAnonymous();
    if (parent && parent->IsHTML(nsGkAtoms::input)) {
      nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(parent);
      MOZ_ASSERT(formControl);
      return formControl->GetType() == NS_FORM_INPUT_FILE;
    }
  }
  return false;
}

