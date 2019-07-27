




#include "nsPlaintextEditor.h"

#include "mozilla/Assertions.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsCaret.h"
#include "nsCharTraits.h"
#include "nsComponentManagerUtils.h"
#include "nsContentCID.h"
#include "nsCopySupport.h"
#include "nsDebug.h"
#include "nsDependentSubstring.h"
#include "nsEditRules.h"
#include "nsEditorUtils.h"  
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsIClipboard.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOMKeyEvent.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDocumentEncoder.h"
#include "nsIEditorIMESupport.h"
#include "nsNameSpaceManager.h"
#include "nsINode.h"
#include "nsIPresShell.h"
#include "nsISelectionController.h"
#include "nsISupportsPrimitives.h"
#include "nsITransferable.h"
#include "nsIWeakReferenceUtils.h"
#include "nsInternetCiter.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsStringFwd.h"
#include "nsSubstringTuple.h"
#include "nsTextEditRules.h"
#include "nsTextEditUtils.h"
#include "nsUnicharUtils.h"
#include "nsXPCOM.h"

class nsIOutputStream;
class nsISupports;
class nsISupportsArray;

using namespace mozilla;
using namespace mozilla::dom;

nsPlaintextEditor::nsPlaintextEditor()
: nsEditor()
, mRules(nullptr)
, mWrapToWindow(false)
, mWrapColumn(0)
, mMaxTextLength(-1)
, mInitTriggerCounter(0)
, mNewlineHandling(nsIPlaintextEditor::eNewlinesPasteToFirst)
#ifdef XP_WIN
, mCaretStyle(1)
#else
, mCaretStyle(0)
#endif
{
  
  
  GetDefaultEditorPrefs(mNewlineHandling, mCaretStyle);
}

nsPlaintextEditor::~nsPlaintextEditor()
{
  
  
  RemoveEventListeners();

  if (mRules)
    mRules->DetachEditor();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsPlaintextEditor)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsPlaintextEditor, nsEditor)
  if (tmp->mRules)
    tmp->mRules->DetachEditor();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRules)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsPlaintextEditor, nsEditor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRules)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsPlaintextEditor, nsEditor)
NS_IMPL_RELEASE_INHERITED(nsPlaintextEditor, nsEditor)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsPlaintextEditor)
  NS_INTERFACE_MAP_ENTRY(nsIPlaintextEditor)
  NS_INTERFACE_MAP_ENTRY(nsIEditorMailSupport)
NS_INTERFACE_MAP_END_INHERITING(nsEditor)


NS_IMETHODIMP nsPlaintextEditor::Init(nsIDOMDocument *aDoc, 
                                      nsIContent *aRoot,
                                      nsISelectionController *aSelCon,
                                      uint32_t aFlags,
                                      const nsAString& aInitialValue)
{
  NS_PRECONDITION(aDoc, "bad arg");
  NS_ENSURE_TRUE(aDoc, NS_ERROR_NULL_POINTER);
  
  nsresult res = NS_OK, rulesRes = NS_OK;
  if (mRules) {
    mRules->DetachEditor();
  }
  
  {
    
    nsAutoEditInitRulesTrigger rulesTrigger(this, rulesRes);
  
    
    res = nsEditor::Init(aDoc, aRoot, aSelCon, aFlags, aInitialValue);
  }

  NS_ENSURE_SUCCESS(rulesRes, rulesRes);

  
  
  if (mRules) {
    mRules->SetInitialValue(aInitialValue);
  }

  return res;
}

static int32_t sNewlineHandlingPref = -1,
               sCaretStylePref = -1;

static void
EditorPrefsChangedCallback(const char *aPrefName, void *)
{
  if (nsCRT::strcmp(aPrefName, "editor.singleLine.pasteNewlines") == 0) {
    sNewlineHandlingPref =
      Preferences::GetInt("editor.singleLine.pasteNewlines",
                          nsIPlaintextEditor::eNewlinesPasteToFirst);
  } else if (nsCRT::strcmp(aPrefName, "layout.selection.caret_style") == 0) {
    sCaretStylePref = Preferences::GetInt("layout.selection.caret_style",
#ifdef XP_WIN
                                                 1);
    if (sCaretStylePref == 0)
      sCaretStylePref = 1;
#else
                                                 0);
#endif
  }
}


void
nsPlaintextEditor::GetDefaultEditorPrefs(int32_t &aNewlineHandling,
                                         int32_t &aCaretStyle)
{
  if (sNewlineHandlingPref == -1) {
    Preferences::RegisterCallback(EditorPrefsChangedCallback,
                                  "editor.singleLine.pasteNewlines");
    EditorPrefsChangedCallback("editor.singleLine.pasteNewlines", nullptr);
    Preferences::RegisterCallback(EditorPrefsChangedCallback,
                                  "layout.selection.caret_style");
    EditorPrefsChangedCallback("layout.selection.caret_style", nullptr);
  }

  aNewlineHandling = sNewlineHandlingPref;
  aCaretStyle = sCaretStylePref;
}

void 
nsPlaintextEditor::BeginEditorInit()
{
  mInitTriggerCounter++;
}

nsresult 
nsPlaintextEditor::EndEditorInit()
{
  nsresult res = NS_OK;
  NS_PRECONDITION(mInitTriggerCounter > 0, "ended editor init before we began?");
  mInitTriggerCounter--;
  if (mInitTriggerCounter == 0)
  {
    res = InitRules();
    if (NS_SUCCEEDED(res)) {
      
      
      EnableUndo(false);
      EnableUndo(true);
    }
  }
  return res;
}

NS_IMETHODIMP
nsPlaintextEditor::SetDocumentCharacterSet(const nsACString& characterSet)
{
  nsresult rv = nsEditor::SetDocumentCharacterSet(characterSet);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocument> domdoc = GetDOMDocument();
  NS_ENSURE_TRUE(domdoc, NS_ERROR_NOT_INITIALIZED);

  if (UpdateMetaCharset(domdoc, characterSet)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNodeList> headList;
  rv = domdoc->GetElementsByTagName(NS_LITERAL_STRING("head"), getter_AddRefs(headList));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(headList, NS_OK);

  nsCOMPtr<nsIDOMNode> headNode;
  headList->Item(0, getter_AddRefs(headNode));
  NS_ENSURE_TRUE(headNode, NS_OK);

  
  nsCOMPtr<nsIDOMNode> resultNode;
  rv = CreateNode(NS_LITERAL_STRING("meta"), headNode, 0, getter_AddRefs(resultNode));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(resultNode, NS_OK);

  
  if (characterSet.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<dom::Element> metaElement = do_QueryInterface(resultNode);
  if (!metaElement) {
    return NS_OK;
  }

  
  metaElement->SetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv,
                       NS_LITERAL_STRING("Content-Type"), true);
  metaElement->SetAttr(kNameSpaceID_None, nsGkAtoms::content,
                       NS_LITERAL_STRING("text/html;charset=") +
                         NS_ConvertASCIItoUTF16(characterSet),
                       true);
  return NS_OK;
}

bool
nsPlaintextEditor::UpdateMetaCharset(nsIDOMDocument* aDocument,
                                     const nsACString& aCharacterSet)
{
  MOZ_ASSERT(aDocument);
  
  nsCOMPtr<nsIDOMNodeList> list;
  nsresult rv = aDocument->GetElementsByTagName(NS_LITERAL_STRING("meta"),
                                                getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(list, false);

  nsCOMPtr<nsINodeList> metaList = do_QueryInterface(list);

  uint32_t listLength = 0;
  metaList->GetLength(&listLength);

  for (uint32_t i = 0; i < listLength; ++i) {
    nsCOMPtr<nsIContent> metaNode = metaList->Item(i);
    MOZ_ASSERT(metaNode);

    if (!metaNode->IsElement()) {
      continue;
    }

    nsAutoString currentValue;
    metaNode->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, currentValue);

    if (!FindInReadable(NS_LITERAL_STRING("content-type"),
                        currentValue,
                        nsCaseInsensitiveStringComparator())) {
      continue;
    }

    metaNode->GetAttr(kNameSpaceID_None, nsGkAtoms::content, currentValue);

    NS_NAMED_LITERAL_STRING(charsetEquals, "charset=");
    nsAString::const_iterator originalStart, start, end;
    originalStart = currentValue.BeginReading(start);
    currentValue.EndReading(end);
    if (!FindInReadable(charsetEquals, start, end,
                        nsCaseInsensitiveStringComparator())) {
      continue;
    }

    
    nsCOMPtr<nsIDOMElement> metaElement = do_QueryInterface(metaNode);
    MOZ_ASSERT(metaElement);
    rv = nsEditor::SetAttribute(metaElement, NS_LITERAL_STRING("content"),
                                Substring(originalStart, start) +
                                  charsetEquals +
                                  NS_ConvertASCIItoUTF16(aCharacterSet));
    return NS_SUCCEEDED(rv);
  }
  return false;
}

NS_IMETHODIMP nsPlaintextEditor::InitRules()
{
  if (!mRules) {
    
    mRules = new nsTextEditRules();
  }
  return mRules->Init(this);
}


NS_IMETHODIMP
nsPlaintextEditor::GetIsDocumentEditable(bool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);

  nsCOMPtr<nsIDOMDocument> doc = GetDOMDocument();
  *aIsDocumentEditable = doc && IsModifiable();

  return NS_OK;
}

bool nsPlaintextEditor::IsModifiable()
{
  return !IsReadonly();
}

nsresult
nsPlaintextEditor::HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent)
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
      if (IsTabbable()) {
        return NS_OK; 
      }

      if (nativeKeyEvent->IsShift() || nativeKeyEvent->IsControl() ||
          nativeKeyEvent->IsAlt() || nativeKeyEvent->IsMeta() ||
          nativeKeyEvent->IsOS()) {
        return NS_OK;
      }

      
      aKeyEvent->PreventDefault();
      return TypedText(NS_LITERAL_STRING("\t"), eTypedText);
    }
    case nsIDOMKeyEvent::DOM_VK_RETURN:
      if (IsSingleLineEditor() || nativeKeyEvent->IsControl() ||
          nativeKeyEvent->IsAlt() || nativeKeyEvent->IsMeta() ||
          nativeKeyEvent->IsOS()) {
        return NS_OK;
      }
      aKeyEvent->PreventDefault();
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







NS_IMETHODIMP
nsPlaintextEditor::TypedText(const nsAString& aString, ETypingAction aAction)
{
  nsAutoPlaceHolderBatch batch(this, nsGkAtoms::TypingTxnName);

  switch (aAction) {
    case eTypedText:
      return InsertText(aString);
    case eTypedBreak:
      return InsertLineBreak();
    default:
      
      return NS_ERROR_FAILURE;
  }
}

already_AddRefed<Element>
nsPlaintextEditor::CreateBRImpl(nsCOMPtr<nsINode>* aInOutParent,
                                int32_t* aInOutOffset,
                                EDirection aSelect)
{
  nsCOMPtr<nsIDOMNode> parent(GetAsDOMNode(*aInOutParent));
  nsCOMPtr<nsIDOMNode> br;
  
  CreateBRImpl(address_of(parent), aInOutOffset, address_of(br), aSelect);
  *aInOutParent = do_QueryInterface(parent);
  nsCOMPtr<Element> ret(do_QueryInterface(br));
  return ret.forget();
}

nsresult
nsPlaintextEditor::CreateBRImpl(nsCOMPtr<nsIDOMNode>* aInOutParent,
                                int32_t* aInOutOffset,
                                nsCOMPtr<nsIDOMNode>* outBRNode,
                                EDirection aSelect)
{
  NS_ENSURE_TRUE(aInOutParent && *aInOutParent && aInOutOffset && outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nullptr;
  nsresult res;
  
  
  nsCOMPtr<nsIDOMNode> node = *aInOutParent;
  int32_t theOffset = *aInOutOffset;
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(node);
  NS_NAMED_LITERAL_STRING(brType, "br");
  nsCOMPtr<nsIDOMNode> brNode;
  if (nodeAsText)  
  {
    int32_t offset;
    uint32_t len;
    nodeAsText->GetLength(&len);
    nsCOMPtr<nsIDOMNode> tmp = GetNodeLocation(node, &offset);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    if (!theOffset)
    {
      
    }
    else if (theOffset == (int32_t)len)
    {
      
      offset++;
    }
    else
    {
      
      res = SplitNode(node, theOffset, getter_AddRefs(tmp));
      NS_ENSURE_SUCCESS(res, res);
      tmp = GetNodeLocation(node, &offset);
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
    int32_t offset;
    nsCOMPtr<nsIDOMNode> parent = GetNodeLocation(*outBRNode, &offset);

    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_STATE(selection);
    if (aSelect == eNext)
    {
      
      selection->SetInterlinePosition(true);
      res = selection->Collapse(parent, offset+1);
    }
    else if (aSelect == ePrevious)
    {
      
      selection->SetInterlinePosition(true);
      res = selection->Collapse(parent, offset);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsPlaintextEditor::CreateBR(nsIDOMNode *aNode, int32_t aOffset, nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect)
{
  nsCOMPtr<nsIDOMNode> parent = aNode;
  int32_t offset = aOffset;
  return CreateBRImpl(address_of(parent), &offset, outBRNode, aSelect);
}

nsresult
nsPlaintextEditor::InsertBR(nsCOMPtr<nsIDOMNode>* outBRNode)
{
  NS_ENSURE_TRUE(outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nullptr;

  
  nsAutoRules beginRulesSniffing(this, EditAction::insertText, nsIEditor::eNext);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  nsresult res;
  if (!selection->Collapsed()) {
    res = DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
    NS_ENSURE_SUCCESS(res, res);
  }

  nsCOMPtr<nsIDOMNode> selNode;
  int32_t selOffset;
  res = GetStartNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  res = CreateBR(selNode, selOffset, outBRNode);
  NS_ENSURE_SUCCESS(res, res);
    
  
  selNode = GetNodeLocation(*outBRNode, &selOffset);
  selection->SetInterlinePosition(true);
  return selection->Collapse(selNode, selOffset+1);
}

nsresult
nsPlaintextEditor::ExtendSelectionForDelete(Selection* aSelection,
                                            nsIEditor::EDirection *aAction)
{
  nsresult result = NS_OK;

  bool bCollapsed = aSelection->Collapsed();

  if (*aAction == eNextWord || *aAction == ePreviousWord
      || (*aAction == eNext && bCollapsed)
      || (*aAction == ePrevious && bCollapsed)
      || *aAction == eToBeginningOfLine || *aAction == eToEndOfLine)
  {
    nsCOMPtr<nsISelectionController> selCont;
    GetSelectionController(getter_AddRefs(selCont));
    NS_ENSURE_TRUE(selCont, NS_ERROR_NO_INTERFACE);

    switch (*aAction)
    {
      case eNextWord:
        result = selCont->WordExtendForDelete(true);
        
        
        *aAction = eNone;
        break;
      case ePreviousWord:
        result = selCont->WordExtendForDelete(false);
        *aAction = eNone;
        break;
      case eNext:
        result = selCont->CharacterExtendForDelete();
        
        break;
      case ePrevious: {
        
        
        
        
        nsCOMPtr<nsIDOMNode> node;
        int32_t offset;
        result = GetStartNodeAndOffset(aSelection, getter_AddRefs(node), &offset);
        NS_ENSURE_SUCCESS(result, result);
        NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

        if (IsTextNode(node)) {
          nsCOMPtr<nsIDOMCharacterData> charData = do_QueryInterface(node);
          if (charData) {
            nsAutoString data;
            result = charData->GetData(data);
            NS_ENSURE_SUCCESS(result, result);

            if (offset > 1 &&
                NS_IS_LOW_SURROGATE(data[offset - 1]) &&
                NS_IS_HIGH_SURROGATE(data[offset - 2])) {
              result = selCont->CharacterExtendForBackspace();
            }
          }
        }
        break;
      }
      case eToBeginningOfLine:
        selCont->IntraLineMove(true, false);          
        result = selCont->IntraLineMove(false, true); 
        *aAction = eNone;
        break;
      case eToEndOfLine:
        result = selCont->IntraLineMove(true, true);
        *aAction = eNext;
        break;
      default:       
        result = NS_OK;
        break;
    }
  }
  return result;
}

nsresult
nsPlaintextEditor::DeleteSelection(EDirection aAction,
                                   EStripWrappers aStripWrappers)
{
  MOZ_ASSERT(aStripWrappers == eStrip || aStripWrappers == eNoStrip);

  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsresult result;

  
  nsAutoPlaceHolderBatch batch(this, nsGkAtoms::DeleteTxnName);
  nsAutoRules beginRulesSniffing(this, EditAction::deleteSelection, aAction);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  
  
  
  
  if (!selection->Collapsed() &&
      (aAction == eNextWord || aAction == ePreviousWord ||
       aAction == eToBeginningOfLine || aAction == eToEndOfLine))
  {
    if (mCaretStyle == 1)
    {
      result = selection->CollapseToStart();
      NS_ENSURE_SUCCESS(result, result);
    }
    else
    { 
      aAction = eNone;
    }
  }

  nsTextRulesInfo ruleInfo(EditAction::deleteSelection);
  ruleInfo.collapsedAction = aAction;
  ruleInfo.stripWrappers = aStripWrappers;
  bool cancel, handled;
  result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(result, result);
  if (!cancel && !handled)
  {
    result = DeleteSelectionImpl(aAction, aStripWrappers);
  }
  if (!cancel)
  {
    
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }

  return result;
}

NS_IMETHODIMP nsPlaintextEditor::InsertText(const nsAString &aStringToInsert)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  EditAction opID = EditAction::insertText;
  if (mComposition) {
    opID = EditAction::insertIMEText;
  }
  nsAutoPlaceHolderBatch batch(this, nullptr); 
  nsAutoRules beginRulesSniffing(this, opID, nsIEditor::eNext);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsAutoString resultString;
  
  
  
  nsTextRulesInfo ruleInfo(opID);
  ruleInfo.inString = &aStringToInsert;
  ruleInfo.outString = &resultString;
  ruleInfo.maxLength = mMaxTextLength;

  bool cancel, handled;
  nsresult res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
  }
  if (!cancel)
  {
    
    res = mRules->DidDoAction(selection, &ruleInfo, res);
  }
  return res;
}

NS_IMETHODIMP nsPlaintextEditor::InsertLineBreak()
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertBreak, nsIEditor::eNext);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(EditAction::insertBreak);
  ruleInfo.maxLength = mMaxTextLength;
  bool cancel, handled;
  nsresult res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    NS_ENSURE_STATE(selection->GetRangeAt(0));
    nsCOMPtr<nsINode> selNode = selection->GetRangeAt(0)->GetStartParent();
    int32_t selOffset = selection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(selNode);

    
    if (!IsTextNode(selNode) && !CanContainTag(*selNode,
                                               *nsGkAtoms::textTagName)) {
      return NS_ERROR_FAILURE;
    }

    
    nsCOMPtr<nsIDocument> doc = GetDocument();
    NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

    
    nsAutoTxnsConserveSelection dontSpazMySelection(this);

    
    res = InsertTextImpl(NS_LITERAL_STRING("\n"), address_of(selNode),
                         &selOffset, doc);
    if (!selNode) res = NS_ERROR_NULL_POINTER; 
    if (NS_SUCCEEDED(res))
    {
      
      res = selection->Collapse(selNode, selOffset);

      if (NS_SUCCEEDED(res))
      {
        
        nsCOMPtr<nsIDOMNode> endNode;
        int32_t endOffset;
        res = GetEndNodeAndOffset(selection, getter_AddRefs(endNode), &endOffset);

        if (NS_SUCCEEDED(res) && endNode == GetAsDOMNode(selNode)
            && endOffset == selOffset) {
          
          
          
          
          selection->SetInterlinePosition(true);
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

nsresult
nsPlaintextEditor::BeginIMEComposition(WidgetCompositionEvent* aEvent)
{
  NS_ENSURE_TRUE(!mComposition, NS_OK);

  if (IsPasswordEditor()) {
    NS_ENSURE_TRUE(mRules, NS_ERROR_NULL_POINTER);
    
    nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

    nsTextEditRules *textEditRules =
      static_cast<nsTextEditRules*>(mRules.get());
    textEditRules->ResetIMETextPWBuf();
  }

  return nsEditor::BeginIMEComposition(aEvent);
}

nsresult
nsPlaintextEditor::UpdateIMEComposition(nsIDOMEvent* aDOMTextEvent)
{
  MOZ_ASSERT(aDOMTextEvent, "aDOMTextEvent must not be nullptr");

  WidgetCompositionEvent* compositionChangeEvent =
    aDOMTextEvent->GetInternalNSEvent()->AsCompositionEvent();
  NS_ENSURE_TRUE(compositionChangeEvent, NS_ERROR_INVALID_ARG);
  MOZ_ASSERT(compositionChangeEvent->message == NS_COMPOSITION_CHANGE,
             "The internal event should be NS_COMPOSITION_CHANGE");

  EnsureComposition(compositionChangeEvent);

  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  
  
  
  
  
  
  
  
  MOZ_ASSERT(!mPlaceHolderBatch,
    "UpdateIMEComposition() must be called without place holder batch");
  TextComposition::CompositionChangeEventHandlingMarker
    compositionChangeEventHandlingMarker(mComposition, compositionChangeEvent);

  NotifyEditorObservers(eNotifyEditorObserversOfBefore);

  nsRefPtr<nsCaret> caretP = ps->GetCaret();

  nsresult rv;
  {
    nsAutoPlaceHolderBatch batch(this, nsGkAtoms::IMETxnName);

    rv = InsertText(compositionChangeEvent->mData);

    if (caretP) {
      caretP->SetSelection(selection);
    }
  }

  
  
  
  
  if (IsIMEComposing()) {
    NotifyEditorObservers(eNotifyEditorObserversOfEnd);
  }

  return rv;
}

already_AddRefed<nsIContent>
nsPlaintextEditor::GetInputEventTargetContent()
{
  nsCOMPtr<nsIContent> target = do_QueryInterface(mEventTarget);
  return target.forget();
}

NS_IMETHODIMP
nsPlaintextEditor::GetDocumentIsEmpty(bool *aDocumentIsEmpty)
{
  NS_ENSURE_TRUE(aDocumentIsEmpty, NS_ERROR_NULL_POINTER);
  
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
  
  return mRules->DocumentIsEmpty(aDocumentIsEmpty);
}

NS_IMETHODIMP
nsPlaintextEditor::GetTextLength(int32_t *aCount)
{
  NS_ASSERTION(aCount, "null pointer");

  
  *aCount = 0;
  
  
  bool docEmpty;
  nsresult rv = GetDocumentIsEmpty(&docEmpty);
  NS_ENSURE_SUCCESS(rv, rv);
  if (docEmpty)
    return NS_OK;

  dom::Element *rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContentIterator> iter =
    do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t totalLength = 0;
  iter->Init(rootElement);
  for (; !iter->IsDone(); iter->Next()) {
    nsCOMPtr<nsIDOMNode> currentNode = do_QueryInterface(iter->GetCurrentNode());
    nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(currentNode);
    if (textNode && IsEditable(currentNode)) {
      uint32_t length;
      textNode->GetLength(&length);
      totalLength += length;
    }
  }

  *aCount = totalLength;
  return NS_OK;
}

NS_IMETHODIMP
nsPlaintextEditor::SetMaxTextLength(int32_t aMaxTextLength)
{
  mMaxTextLength = aMaxTextLength;
  return NS_OK;
}

NS_IMETHODIMP
nsPlaintextEditor::GetMaxTextLength(int32_t* aMaxTextLength)
{
  NS_ENSURE_TRUE(aMaxTextLength, NS_ERROR_INVALID_POINTER);
  *aMaxTextLength = mMaxTextLength;
  return NS_OK;
}




NS_IMETHODIMP 
nsPlaintextEditor::GetWrapWidth(int32_t *aWrapColumn)
{
  NS_ENSURE_TRUE( aWrapColumn, NS_ERROR_NULL_POINTER);

  *aWrapColumn = mWrapColumn;
  return NS_OK;
}





static void CutStyle(const char* stylename, nsString& styleValue)
{
  
  int32_t styleStart = styleValue.Find(stylename, true);
  if (styleStart >= 0)
  {
    int32_t styleEnd = styleValue.Find(";", false, styleStart);
    if (styleEnd > styleStart)
      styleValue.Cut(styleStart, styleEnd - styleStart + 1);
    else
      styleValue.Cut(styleStart, styleValue.Length() - styleStart);
  }
}




NS_IMETHODIMP 
nsPlaintextEditor::SetWrapWidth(int32_t aWrapColumn)
{
  SetWrapColumn(aWrapColumn);

  
  
  if (!IsPlaintextEditor())
    return NS_OK;

  
  
  dom::Element *rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER);

  
  nsAutoString styleValue;
  rootElement->GetAttr(kNameSpaceID_None, nsGkAtoms::style, styleValue);

  
  CutStyle("white-space", styleValue);
  CutStyle("width", styleValue);
  CutStyle("font-family", styleValue);

  
  
  if (!styleValue.IsEmpty())
  {
    styleValue.Trim("; \t", false, true);
    styleValue.AppendLiteral("; ");
  }

  
  
  
  if (IsWrapHackEnabled() && aWrapColumn >= 0)
    styleValue.AppendLiteral("font-family: -moz-fixed; ");

  
  
  
  
  if (IsMailEditor())
  {
    mWrapToWindow =
      Preferences::GetBool("mail.compose.wrap_to_window_width", mWrapToWindow);
  }

  
  if (aWrapColumn > 0 && !mWrapToWindow)        
  {
    styleValue.AppendLiteral("white-space: pre-wrap; width: ");
    styleValue.AppendInt(aWrapColumn);
    styleValue.AppendLiteral("ch;");
  }
  else if (mWrapToWindow || aWrapColumn == 0)
    styleValue.AppendLiteral("white-space: pre-wrap;");
  else
    styleValue.AppendLiteral("white-space: pre;");

  return rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::style, styleValue, true);
}

NS_IMETHODIMP 
nsPlaintextEditor::SetWrapColumn(int32_t aWrapColumn)
{
  mWrapColumn = aWrapColumn;
  return NS_OK;
}




NS_IMETHODIMP 
nsPlaintextEditor::GetNewlineHandling(int32_t *aNewlineHandling)
{
  NS_ENSURE_ARG_POINTER(aNewlineHandling);

  *aNewlineHandling = mNewlineHandling;
  return NS_OK;
}




NS_IMETHODIMP 
nsPlaintextEditor::SetNewlineHandling(int32_t aNewlineHandling)
{
  mNewlineHandling = aNewlineHandling;
  
  return NS_OK;
}

NS_IMETHODIMP 
nsPlaintextEditor::Undo(uint32_t aCount)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsAutoUpdateViewBatch beginViewBatching(this);

  ForceCompositionEnd();

  NotifyEditorObservers(eNotifyEditorObserversOfBefore);

  nsAutoRules beginRulesSniffing(this, EditAction::undo, nsIEditor::eNone);

  nsTextRulesInfo ruleInfo(EditAction::undo);
  nsRefPtr<Selection> selection = GetSelection();
  bool cancel, handled;
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  
  if (!cancel && NS_SUCCEEDED(result))
  {
    result = nsEditor::Undo(aCount);
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  } 

  NotifyEditorObservers(eNotifyEditorObserversOfEnd);
  return result;
}

NS_IMETHODIMP 
nsPlaintextEditor::Redo(uint32_t aCount)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsAutoUpdateViewBatch beginViewBatching(this);

  ForceCompositionEnd();

  NotifyEditorObservers(eNotifyEditorObserversOfBefore);

  nsAutoRules beginRulesSniffing(this, EditAction::redo, nsIEditor::eNone);

  nsTextRulesInfo ruleInfo(EditAction::redo);
  nsRefPtr<Selection> selection = GetSelection();
  bool cancel, handled;
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  
  if (!cancel && NS_SUCCEEDED(result))
  {
    result = nsEditor::Redo(aCount);
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  } 

  NotifyEditorObservers(eNotifyEditorObserversOfEnd);
  return result;
}

bool
nsPlaintextEditor::CanCutOrCopy(PasswordFieldAllowed aPasswordFieldAllowed)
{
  nsRefPtr<Selection> selection = GetSelection();
  if (!selection) {
    return false;
  }

  if (aPasswordFieldAllowed == ePasswordFieldNotAllowed &&
      IsPasswordEditor())
    return false;

  return !selection->Collapsed();
}

bool
nsPlaintextEditor::FireClipboardEvent(int32_t aType, int32_t aSelectionType)
{
  if (aType == NS_PASTE)
    ForceCompositionEnd();

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, false);

  nsRefPtr<Selection> selection = GetSelection();
  if (!selection) {
    return false;
  }

  if (!nsCopySupport::FireClipboardEvent(aType, aSelectionType, presShell, selection))
    return false;

  
  
  return !mDidPreDestroy;
}

NS_IMETHODIMP nsPlaintextEditor::Cut()
{
  if (FireClipboardEvent(NS_CUT, nsIClipboard::kGlobalClipboard))
    return DeleteSelection(eNone, eStrip);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::CanCut(bool *aCanCut)
{
  NS_ENSURE_ARG_POINTER(aCanCut);
  *aCanCut = IsModifiable() && CanCutOrCopy(ePasswordFieldNotAllowed);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::Copy()
{
  FireClipboardEvent(NS_COPY, nsIClipboard::kGlobalClipboard);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::CanCopy(bool *aCanCopy)
{
  NS_ENSURE_ARG_POINTER(aCanCopy);
  *aCanCopy = CanCutOrCopy(ePasswordFieldNotAllowed);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::CanDelete(bool *aCanDelete)
{
  NS_ENSURE_ARG_POINTER(aCanDelete);
  *aCanDelete = IsModifiable() && CanCutOrCopy(ePasswordFieldAllowed);
  return NS_OK;
}


NS_IMETHODIMP
nsPlaintextEditor::GetAndInitDocEncoder(const nsAString& aFormatType,
                                        uint32_t aFlags,
                                        const nsACString& aCharset,
                                        nsIDocumentEncoder** encoder)
{
  nsresult rv = NS_OK;

  nsAutoCString formatType(NS_DOC_ENCODER_CONTRACTID_BASE);
  LossyAppendUTF16toASCII(aFormatType, formatType);
  nsCOMPtr<nsIDocumentEncoder> docEncoder (do_CreateInstance(formatType.get(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryReferent(mDocWeak);
  NS_ASSERTION(domDoc, "Need a document");

  rv = docEncoder->Init(domDoc, aFormatType, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aCharset.IsEmpty() && !aCharset.EqualsLiteral("null")) {
    docEncoder->SetCharset(aCharset);
  }

  int32_t wc;
  (void) GetWrapWidth(&wc);
  if (wc >= 0)
    (void) docEncoder->SetWrapColumn(wc);

  
  
  
  if (aFlags & nsIDocumentEncoder::OutputSelectionOnly)
  {
    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_STATE(selection);
    rv = docEncoder->SetSelection(selection);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  else
  {
    dom::Element* rootElement = GetRoot();
    NS_ENSURE_TRUE(rootElement, NS_ERROR_FAILURE);
    if (!rootElement->IsHTMLElement(nsGkAtoms::body)) {
      rv = docEncoder->SetNativeContainerNode(rootElement);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  docEncoder.forget(encoder);
  return NS_OK;
}


NS_IMETHODIMP 
nsPlaintextEditor::OutputToString(const nsAString& aFormatType,
                                  uint32_t aFlags,
                                  nsAString& aOutputString)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsString resultString;
  nsTextRulesInfo ruleInfo(EditAction::outputText);
  ruleInfo.outString = &resultString;
  
  nsAutoString str(aFormatType);
  ruleInfo.outputFormat = &str;
  bool cancel, handled;
  nsresult rv = mRules->WillDoAction(nullptr, &ruleInfo, &cancel, &handled);
  if (cancel || NS_FAILED(rv)) { return rv; }
  if (handled)
  { 
    aOutputString.Assign(*(ruleInfo.outString));
    return rv;
  }

  nsAutoCString charsetStr;
  rv = GetDocumentCharacterSet(charsetStr);
  if(NS_FAILED(rv) || charsetStr.IsEmpty())
    charsetStr.AssignLiteral("ISO-8859-1");

  nsCOMPtr<nsIDocumentEncoder> encoder;
  rv = GetAndInitDocEncoder(aFormatType, aFlags, charsetStr, getter_AddRefs(encoder));
  NS_ENSURE_SUCCESS(rv, rv);
  return encoder->EncodeToString(aOutputString);
}

NS_IMETHODIMP
nsPlaintextEditor::OutputToStream(nsIOutputStream* aOutputStream,
                             const nsAString& aFormatType,
                             const nsACString& aCharset,
                             uint32_t aFlags)
{
  nsresult rv;

  
  
  
  if (aFormatType.EqualsLiteral("text/plain"))
  {
    bool docEmpty;
    rv = GetDocumentIsEmpty(&docEmpty);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (docEmpty)
       return NS_OK;    
  }

  nsCOMPtr<nsIDocumentEncoder> encoder;
  rv = GetAndInitDocEncoder(aFormatType, aFlags, aCharset,
                            getter_AddRefs(encoder));

  NS_ENSURE_SUCCESS(rv, rv);

  return encoder->EncodeToStream(aOutputStream);
}

NS_IMETHODIMP
nsPlaintextEditor::InsertTextWithQuotations(const nsAString &aStringToInsert)
{
  return InsertText(aStringToInsert);
}

NS_IMETHODIMP
nsPlaintextEditor::PasteAsQuotation(int32_t aSelectionType)
{
  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans;
  rv = PrepareTransferable(getter_AddRefs(trans));
  if (NS_SUCCEEDED(rv) && trans)
  {
    
    clipboard->GetData(trans, aSelectionType);

    
    
    
    nsCOMPtr<nsISupports> genericDataObj;
    uint32_t len;
    char* flav = nullptr;
    rv = trans->GetAnyTransferData(&flav, getter_AddRefs(genericDataObj),
                                   &len);
    if (NS_FAILED(rv) || !flav)
    {
      return rv;
    }

    if (0 == nsCRT::strcmp(flav, kUnicodeMime) ||
        0 == nsCRT::strcmp(flav, kMozTextInternal))
    {
      nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
      if (textDataObj && len > 0)
      {
        nsAutoString stuffToPaste;
        textDataObj->GetData ( stuffToPaste );
        nsAutoEditBatch beginBatching(this);
        rv = InsertAsQuotation(stuffToPaste, 0);
      }
    }
    free(flav);
  }

  return rv;
}

NS_IMETHODIMP
nsPlaintextEditor::InsertAsQuotation(const nsAString& aQuotedText,
                                     nsIDOMNode **aNodeInserted)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  nsString quotedStuff;
  nsresult rv = nsInternetCiter::GetCiteString(aQuotedText, quotedStuff);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!aQuotedText.IsEmpty() && (aQuotedText.Last() != char16_t('\n')))
    quotedStuff.Append(char16_t('\n'));

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertText, nsIEditor::eNext);

  
  nsTextRulesInfo ruleInfo(EditAction::insertElement);
  bool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel) return NS_OK; 
  if (!handled)
  {
    rv = InsertText(quotedStuff);

    
    if (aNodeInserted && NS_SUCCEEDED(rv))
    {
      *aNodeInserted = 0;
      
    }
  }
  return rv;
}

NS_IMETHODIMP
nsPlaintextEditor::PasteAsCitedQuotation(const nsAString& aCitation,
                                         int32_t aSelectionType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPlaintextEditor::InsertAsCitedQuotation(const nsAString& aQuotedText,
                                          const nsAString& aCitation,
                                          bool aInsertHTML,
                                          nsIDOMNode **aNodeInserted)
{
  return InsertAsQuotation(aQuotedText, aNodeInserted);
}

nsresult
nsPlaintextEditor::SharedOutputString(uint32_t aFlags,
                                      bool* aIsCollapsed,
                                      nsAString& aResult)
{
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);

  *aIsCollapsed = selection->Collapsed();

  if (!*aIsCollapsed)
    aFlags |= nsIDocumentEncoder::OutputSelectionOnly;
  

  return OutputToString(NS_LITERAL_STRING("text/plain"), aFlags, aResult);
}

NS_IMETHODIMP
nsPlaintextEditor::Rewrap(bool aRespectNewlines)
{
  int32_t wrapCol;
  nsresult rv = GetWrapWidth(&wrapCol);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  
  if (wrapCol <= 0)
    wrapCol = 72;

  nsAutoString current;
  bool isCollapsed;
  rv = SharedOutputString(nsIDocumentEncoder::OutputFormatted
                          | nsIDocumentEncoder::OutputLFLineBreak,
                          &isCollapsed, current);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString wrapped;
  uint32_t firstLineOffset = 0;   
  rv = nsInternetCiter::Rewrap(current, wrapCol, firstLineOffset, aRespectNewlines,
                     wrapped);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isCollapsed)    
    SelectAll();

  return InsertTextWithQuotations(wrapped);
}

NS_IMETHODIMP    
nsPlaintextEditor::StripCites()
{
  nsAutoString current;
  bool isCollapsed;
  nsresult rv = SharedOutputString(nsIDocumentEncoder::OutputFormatted,
                                   &isCollapsed, current);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString stripped;
  rv = nsInternetCiter::StripCites(current, stripped);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isCollapsed)    
  {
    rv = SelectAll();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return InsertText(stripped);
}

NS_IMETHODIMP
nsPlaintextEditor::GetEmbeddedObjects(nsISupportsArray** aNodeList)
{
  *aNodeList = 0;
  return NS_OK;
}




NS_IMETHODIMP
nsPlaintextEditor::StartOperation(EditAction opID,
                                  nsIEditor::EDirection aDirection)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsEditor::StartOperation(opID, aDirection);  
  if (mRules) return mRules->BeforeEdit(mAction, mDirection);
  return NS_OK;
}




NS_IMETHODIMP
nsPlaintextEditor::EndOperation()
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  nsresult res = NS_OK;
  if (mRules) res = mRules->AfterEdit(mAction, mDirection);
  nsEditor::EndOperation();  
  return res;
}  


nsresult
nsPlaintextEditor::SelectEntireDocument(Selection* aSelection)
{
  if (!aSelection || !mRules) { return NS_ERROR_NULL_POINTER; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  bool bDocIsEmpty;
  if (NS_SUCCEEDED(mRules->DocumentIsEmpty(&bDocIsEmpty)) && bDocIsEmpty)
  {
    
    nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(GetRoot());
    NS_ENSURE_TRUE(rootElement, NS_ERROR_FAILURE);

    
    return aSelection->Collapse(rootElement, 0);
  }

  SelectionBatcher selectionBatcher(aSelection);
  nsresult rv = nsEditor::SelectEntireDocument(aSelection);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int32_t selOffset;
  nsCOMPtr<nsIDOMNode> selNode;
  rv = GetEndNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> childNode = GetChildAt(selNode, selOffset - 1);

  if (childNode && nsTextEditUtils::IsMozBR(childNode)) {
    int32_t parentOffset;
    nsCOMPtr<nsIDOMNode> parentNode = GetNodeLocation(childNode, &parentOffset);

    return aSelection->Extend(parentNode, parentOffset);
  }

  return NS_OK;
}

already_AddRefed<mozilla::dom::EventTarget>
nsPlaintextEditor::GetDOMEventTarget()
{
  nsCOMPtr<mozilla::dom::EventTarget> copy = mEventTarget;
  return copy.forget();
}


nsresult
nsPlaintextEditor::SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                            const nsAString & aAttribute,
                                            const nsAString & aValue,
                                            bool aSuppressTransaction)
{
  return nsEditor::SetAttribute(aElement, aAttribute, aValue);
}

nsresult
nsPlaintextEditor::RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                               const nsAString & aAttribute,
                                               bool aSuppressTransaction)
{
  return nsEditor::RemoveAttribute(aElement, aAttribute);
}
